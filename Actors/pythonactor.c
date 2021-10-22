#include "pythonactor.h"
#include "pyzmsg.h"
#include <wchar.h>
#ifdef __UTYPE_OSX
#include "CoreFoundation/CoreFoundation.h"
#elif defined(__WINDOWS__)
#include <windows.h>
#endif
#ifdef __UTYPE_LINUX
#include <sys/inotify.h>

zmsg_t *
s_pythonactor_set_file(pythonactor_t *self, const char *filename);

static void
s_handle_inotify_events(pythonactor_t *self, sphactor_actor_t *actorinst, int fd, int *wd)
{
   /* Some systems cannot read integer variables if they are not
      properly aligned. On other systems, incorrect alignment may
      decrease performance. Hence, the buffer used for reading from
      the inotify file descriptor should have the same alignment as
      struct inotify_event. */

   char buf[4096]
       __attribute__ ((aligned(__alignof__(struct inotify_event))));
   const struct inotify_event *event;
   ssize_t len;

   // TODO: We might need to loop on read until we have no events left?
   /* If the nonblocking read() found no events to read, then
      it returns -1 with errno set to EAGAIN. In that case,
      we exit the loop. */

   len = read(fd, buf, sizeof(buf));
   if (len == -1 && errno != EAGAIN) {
       zsys_error("Inotify read failure");
   }

   if (len <= 0)
       return;

   /* Loop over all events in the buffer. */

   for (char *ptr = buf; ptr < buf + len;
           ptr += sizeof(struct inotify_event) + event->len) {

       event = (const struct inotify_event *) ptr;

       if ( event->mask & IN_DELETE_SELF || event->mask & IN_DELETE )
       {
           zsys_warning("Watched file %s is deleted", self->main_filename);
           s_pythonactor_set_file(self, "");
           return;
       }
       /*
       if (event->mask & IN_MODIFY)
           printf("IN_MODIFY: ");
       if (event->mask & IN_CLOSE_WRITE)
           printf("IN_CLOSE_WRITE: ");
       if (event->mask & IN_MOVE_SELF)
           printf("IN_MOVE_SELF: ");
        */
       s_pythonactor_set_file(self, self->main_filename);
   }
   fflush(stdout);
}

static void
s_destroy_inotify(pythonactor_t *self, sphactor_actor_t *actorinst)
{
    if (self->wd != -1)
    {
        // removing the watch from the watch list.
        inotify_rm_watch( self->fd, self->wd );
    }
    if (self->fd != -1)
    {
        sphactor_actor_poller_remove(actorinst, (void *)&self->fd);
        // closing the INOTIFY instance
        close( self->fd );
    }
}

static void
s_init_inotify(pythonactor_t *self, const char *filename, sphactor_actor_t *actorinst)
{
    assert(self);
    assert(filename);
    assert(actorinst);
    if (self->fd != -1)
        s_destroy_inotify(self, actorinst);

    self->fd = inotify_init1(0);
    //zsys_info("Inotify fd is %i", self->fd);
    /*checking for error*/
    if (self->fd == -1)
    {
        zsys_error("inotify_init error");
        return;
    }
    // add file watcher
    self->wd = inotify_add_watch( self->fd, filename, IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_CLOSE_WRITE);
    /*checking for error*/
    if (self->wd == -1)
    {
        zsys_error("inotify_add_watch error %i", errno);
        s_destroy_inotify(self, actorinst);
        return;
    }

    sphactor_actor_poller_add(actorinst, (void *)&self->fd);

    return;    
}
#endif

// https://stackoverflow.com/questions/2736753/how-to-remove-extension-from-file-name
static char *
s_remove_ext(const char* myStr) {
    char *retStr;
    char *lastExt;
    if (myStr == NULL) return NULL;
    if ((retStr = malloc (strlen (myStr) + 1)) == NULL) return NULL;
    strcpy (retStr, myStr);
    lastExt = strrchr (retStr, '.');
    if (lastExt != NULL)
        *lastExt = '\0';
    return retStr;
}

static char *
s_basename(char const *path)
{
    char *s = strrchr(path, '/');
    if (!s)
        return strdup(path);
    else
        return strdup(s + 1);
}

static bool
s_dir_exists(const wchar_t *pypath)
{
    // bleeh we need to convert to char as czmq only supports char paths
    char path[PATH_MAX];
    wcstombs(path, pypath, PATH_MAX);
    zfile_t *dir = zfile_new(NULL, path);
    assert(dir);
    bool ret = zfile_is_directory(dir);
    zfile_destroy(&dir);
    return ret;
}

static const char *pythonactorcapabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"pyfile\"\n"
        "        type = \"filename\"\n"
        "        value = \"\"\n"
        "        api_call = \"SET FILE\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "inputs\n"
        "    input\n"
        "        type = \"OSC\"\n"
        "outputs\n"
        "    output\n"
        //TODO: Perhaps add NatNet output type so we can filter the data multiple times...
        "        type = \"OSC\"\n";

zmsg_t *
s_pythonactor_set_file(pythonactor_t *self, const char *filename)
{
    char *filebasename = s_basename(filename);
    char *pyname = s_remove_ext(filebasename);

    // Load or reload the module
    // first check if we need to reload
    //  Acquire the GIL
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    if (self->pymodule && self->main_filename && streq(filename, self->main_filename) )
    {
        PyObject *pyreloaded = PyImport_ReloadModule(self->pymodule);
        if (pyreloaded == NULL)
        {
            if ( PyErr_Occurred() )
                PyErr_Print();
            zsys_error("Error reloading module %s", filename);
            PyGILState_Release(gstate);
            zstr_free(&pyname);
            zstr_free(&filebasename);
            return NULL;
        }
        Py_CLEAR(self->pymodule);
        self->pymodule = pyreloaded;
    }
    else // load python file (module) normally
    {
        //  import our python file
        PyObject *pName;
        pName = PyUnicode_DecodeFSDefault( pyname );
        if ( pName == NULL )
        {
            if ( PyErr_Occurred() )
                PyErr_Print();
            zsys_error("error loading %s", pyname);
            PyGILState_Release(gstate);
            zstr_free(&pyname);
            zstr_free(&filebasename);
            return NULL;
        }

        //  we loaded the file now import it
        Py_CLEAR(self->pymodule);
        self->pymodule = PyImport_Import( pName );
        if ( self->pymodule == NULL )
        {
            if ( PyErr_Occurred() )
                PyErr_Print();
            zsys_error("error importing %s", filename);
            Py_DECREF(pName);
            PyGILState_Release(gstate);
            zstr_free(&pyname);
            zstr_free(&filebasename);
            return NULL;
        }
        Py_DECREF(pName);
        // to be sure we have the updated code we also reload the module (workaround for loading updated file in new actor)
        PyObject *pyreloaded = PyImport_ReloadModule(self->pymodule);
        if (pyreloaded == NULL)
        {
            if ( PyErr_Occurred() )
                PyErr_Print();
            zsys_error("Error reloading module on fresh load %s", filename);
            PyGILState_Release(gstate);
            zstr_free(&pyname);
            zstr_free(&filebasename);
            return NULL;
        }
        Py_CLEAR(self->pymodule);
        self->pymodule = pyreloaded;
    }
    // load the class
    //  get the class with the same name as the filename
    PyObject *pClass = PyObject_GetAttrString(self->pymodule, pyname );
    if (pClass == NULL )
    {
        if (PyErr_Occurred())
            PyErr_Print();
        zsys_error("pClass is NULL");
        Py_CLEAR(self->pymodule);
        PyGILState_Release(gstate);
        zstr_free(&pyname);
        zstr_free(&filebasename);
        return NULL;
    }

    // instanciate the class and optionally destroy the old instance
    // create an instance of the class
    Py_CLEAR(self->pyinstance);
    self->pyinstance = PyObject_CallObject((PyObject *) pClass, NULL);
    if (self->pyinstance == NULL )
    {
        if (PyErr_Occurred())
            PyErr_Print();
        zsys_error("pClass instance is NULL");
        Py_DECREF(pClass);
        Py_DECREF(self->pymodule);
        self->pymodule = NULL;
        PyGILState_Release(gstate);
        zstr_free(&pyname);
        zstr_free(&filebasename);
        return NULL;
    }
    zsys_info("Successfully (re)loaded %s", filename);
    Py_DECREF(pClass);
    PyGILState_Release(gstate);
    zstr_free(&pyname);
    zstr_free(&filebasename);
    self->main_filename = strdup(filename);
    return NULL;
}

static zosc_t *
s_py_zosc(PyObject *pAddress, PyObject *pData)
{
    assert( PyUnicode_Check(pAddress) );
    assert( PyList_Check(pData) );
    PyObject *stringbytes = PyUnicode_AsASCIIString(pAddress);
    zosc_t *ret = zosc_new( PyBytes_AsString( stringbytes) );
    Py_DECREF(stringbytes);

    // iterate
    for ( Py_ssize_t i=0;i<PyList_Size(pData);++i )
    {
        PyObject *item = PyList_GetItem(pData, i);
        assert(item);
        // determine type, first check if boolean otherwise it will be an int
        if (PyBool_Check(item))
        {
            long b = PyLong_AsLong(item);
            if (b)
                zosc_append(ret, "T");
            else
                zosc_append(ret, "F");
        }
        else if ( PyLong_Check(item) )
        {
            long v = PyLong_AsLong(item);
            zosc_append(ret, "h", v);
        }
        else if (PyFloat_Check(item))
        {
            double v = PyFloat_AsDouble(item);
            zosc_append(ret, "d", v);
        }
        else if (PyUnicode_Check(item))
        {
            PyObject* ascii = PyUnicode_AsASCIIString(item);
            assert(ascii);
            char* s = PyBytes_AsString(ascii);
            zosc_append(ret, "s", s);
            Py_DECREF(ascii);
        }
        else if (PyBytes_Check(item)) // this can be used to force 32bit int, ie: struct.pack("I", 32)
        {
            Py_ssize_t bytesize = PyBytes_Size(item);
            if ( bytesize == 4 ) // 32bit int
            {
                char * buffer = PyBytes_AsString(item);
                uint32_t* vp = (uint32_t *)(buffer);
                zosc_append(ret, "i", *vp);
            }
            else
                zsys_warning("I don't know what to do with these bytes, please help: https://github.com/hku-ect/gazebosc");
        }
        else {
            // not a supported native python type try ctypes
            PyTypeObject* type = Py_TYPE(item);
            const char * typename = _PyType_Name(type);
            if (streq(typename, "c_int"))
            {
                zsys_warning("Unsupported ctypes.c_int until we find a way to access the value :S");
            }
            else
                zsys_warning("unsupported python type");

            Py_DECREF(type);
        }

    }

    Py_DECREF(pAddress);
    Py_DECREF(pData);
    return ret;
}

int python_init()
{
    Py_UnbufferedStdioFlag = 1;
    //  add internal wrapper to available modules
    int rc = PyImport_AppendInittab("sph", PyInit_PyZmsg);
    // TODO set the python home to our bundled python or system installed
    // We need to check whether the PYTHONHOME env var is provided,
    //   * if so let python handle it, we do nothing
    //   * else check if we have a bundled python dir and set pythonhome to it
    //   * otherwise do nothing hoping a system installed python is found?
    //   ref: https://github.com/blender/blender/blob/594f47ecd2d5367ca936cf6fc6ec8168c2b360d0/source/blender/python/generic/py_capi_utils.c#L894
    char *homepath = getenv("PYTHONHOME");
    if ( homepath == NULL ) // PYTHONHOME not set so check for bundled python
    {
        wchar_t pypath[PATH_MAX];
#ifdef __UTYPE_OSX
        char path[PATH_MAX];
        CFURLRef res = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
        CFURLGetFileSystemRepresentation(res, TRUE, (UInt8 *)path, PATH_MAX);
        CFRelease(res);
        // append python to our path
        swprintf(pypath, PATH_MAX, L"%hs/python", path);
#elif defined(__WINDOWS__)
        wchar_t path[PATH_MAX];
        GetModuleFileNameW(NULL, path, PATH_MAX);
        // remove program name by finding the last delimiter
        wchar_t *s = wcsrchr(path, L'\\');
        if (s) *s = 0;
        swprintf(pypath, PATH_MAX, L"%ls\\python", path);
#else   // linux, we could check for __UTYPE_LINUX
        size_t pathsize = PATH_MAX;
        char path[PATH_MAX];
        long result = readlink("/proc/self/exe", path, pathsize);
        if (result == -1)
        {
            fprintf(stderr, "Error occurred calling readlink: %s\n", strerror(errno));
        }
        assert(result > -1);
        if (result > 0)
        {
            path[result] = 0; /* add NULL */
        }
        // remove program name by finding the last /
        char *s = strrchr(path, '/');
        if (s) *s = 0;
        // append python to our path
        swprintf(pypath, PATH_MAX, L"%hs/python", path);
#endif
        if ( s_dir_exists(pypath) )
        {
            Py_SetPythonHome(pypath);
        }
        else
            zsys_warning("No python dir found in '%s' so hoping for a system installed", path);
    }
    //Py_IgnoreEnvironmentFlag = true;
    //Py_NoUserSiteDirectory = true;
    Py_Initialize();
    assert(rc == 0);
    //  add some paths for importing python files
    rc = PyRun_SimpleString(
                    "import sys\n"
                    "import os\n"
                    //"print(os.getcwd())\n"
                    "sys.path.append(os.getcwd())\n"
                    //"sys.path.append('/home/arnaud/src/czmq/bindings/python')\n"
                    "print(\"Python {0} initialized. Paths: {1}\".format(sys.version, sys.path))\n"
                       );
    //  import the internal wrapper module
    PyObject *imp = PyImport_ImportModule("sph");
    assert(imp);
    //  this is our last Python API call before threads jump in,
    //  thus we need to release the GIL
    PyEval_SaveThread();

    if ( rc != 0 )
    {
        PyErr_Print();
    }

    sphactor_register("Python", &pythonactor_handler, zconfig_str_load(pythonactorcapabilities), &pythonactor_new_helper, NULL); // https://stackoverflow.com/questions/65957511/typedef-for-a-registering-a-constructor-function-in-c
    return rc;
}

// this is needed because we have to conform to returning a void * thus we need to wrap
// the pythonactor_new method, unless some know a better method?
void *
pythonactor_new_helper(void *args)
{
    return (void *)pythonactor_new();
}

pythonactor_t *
pythonactor_new()
{
    pythonactor_t *self = (pythonactor_t *) zmalloc (sizeof (pythonactor_t));
    assert (self);

    self->pyinstance = NULL;
    self->main_filename = NULL;
    self->fd = -1;
    self->wd = -1;

    return self;
}

void
pythonactor_destroy(pythonactor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pythonactor_t *self = *self_p;
        if (self->main_filename )
            zstr_free(&self->main_filename);
        // free the pyinstance
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        Py_CLEAR(self->pyinstance);
        Py_CLEAR(self->pymodule);
        PyGILState_Release(gstate);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

zmsg_t *
pythonactor_init(pythonactor_t *self, sphactor_event_t *ev)
{
    // if file loaded execute init!
    if ( ev->msg ) zmsg_destroy(&ev->msg);
    return NULL;
}

zmsg_t *
pythonactor_timer(pythonactor_t *self, sphactor_event_t *ev)
{
    return NULL;
}

zmsg_t *
pythonactor_api(pythonactor_t *self, sphactor_event_t *ev)
{
    char *cmd = zmsg_popstr(ev->msg);
    if ( streq(cmd, "SET FILE") )
    {
        char *filename = zmsg_popstr(ev->msg);
        // dispose the event msg we don't use it further
        zmsg_destroy(&ev->msg);

        // check the filename
        if ( strlen(filename) < 1)
        {
            zsys_error("Empty filename received", filename);
            zstr_free(&filename);
            zstr_free(&cmd);
            return NULL;
        }

#ifdef __UTYPE_LINUX
        if ( self->main_filename == NULL || ! streq(filename, self->main_filename) )
            s_init_inotify(self, filename, (sphactor_actor_t *)ev->actor);
#endif
        s_pythonactor_set_file(self, filename);


        return NULL;

    }

    if ( ev->msg ) zmsg_destroy(&ev->msg);
    zstr_free(&cmd);
    return NULL;
}

zmsg_t *
pythonactor_socket(pythonactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    assert(self->pyinstance);
    //zsys_info("Hello from py_class_actor wrapper: type=%s", ev->type);
    //  This is not compatible with subinterpreters!
    //  https://docs.python.org/3/c-api/init.html#c.PyGILState_Ensure
    //  Acquire the GIL
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // pass the msg to our pyinstance
    // build evmsg for member argument
    PyZmsgObject *evmsg = (PyZmsgObject *)PyObject_CallObject((PyObject *)&PyZmsgType, NULL);
    assert(evmsg);
    // dispose the event message as we already copied it to a python object
    if (ev->msg)
    {
        zmsg_destroy(&evmsg->msg);
        evmsg->msg = ev->msg;
    }
    // call member 'handleMsg' with event arguments
    PyObject *pReturn = PyObject_CallMethod(self->pyinstance, "handleSocket", "Osss", evmsg, ev->type, ev->name, ev->uuid);
    Py_XINCREF(pReturn);  // increase refcount to prevent destroy
    if (pReturn == NULL)
    {
        PyErr_Print();
        zsys_error("pythonactor: error calling handleSocket");
    }
    else if (pReturn != Py_None)
    {
        if (PyBytes_Check(pReturn))
        {
            // handle python bytes
            zmsg_t *ret = NULL;
            Py_ssize_t size = PyBytes_Size(pReturn);
            if ( size > 0 )
            {
                ret = zmsg_new();
#ifdef _MSC_VER
                char *buf = (char *)_malloca( size );
                memcpy(buf, PyBytes_AsString(pReturn), size);
                zmsg_addmem(ret, buf, size);
                _freea(buf);
#else
                char buf[size];// = new char[size];
                memcpy(buf, PyBytes_AsString(pReturn), size);
                zmsg_addmem(ret, buf, size);
#endif
            }
            else
            {
                zsys_warning("zsock_resolvePyBytes has zero size");
            }
            Py_DECREF(pReturn);  // decrease refcount to trigger destroy
            // Release the GIL again as we are ready with Python
            PyGILState_Release(gstate);
            // already destroyed: zmsg_destroy(&ev->msg);
            return ret;
        }
        else if ( PyTuple_Check(pReturn) ) // we expect a tuple in the format ( address, [data])
        {
            zmsg_t *ret = NULL; // our return
            // convert the tuple to an osc message
            Py_ssize_t tuple_len = PyTuple_Size(pReturn);
            assert(tuple_len > 0 );
            // first item must be the address string
            PyObject *pAddress = PyTuple_GetItem(pReturn, 0);
            assert(pAddress);
            if ( ! PyUnicode_Check(pAddress) )
            {
                zsys_error("first item in the tuple is not a string, first item should be the address string");
            }
            else
            {
                PyObject *pData = PyTuple_GetItem(pReturn, 1);
                assert(pData);
                if ( PyList_Check(pData) )
                {
                    zosc_t *retosc = s_py_zosc(pAddress, pData);
                    assert(retosc);
                    ret = zmsg_new();
                    assert(ret);
                    zframe_t *data = zosc_packx(&retosc);
                    assert(data);
                    zmsg_append(ret, &data);
                }
            }
            Py_DECREF(pReturn);  // decrease refcount to trigger destroy
            // Release the GIL again as we are ready with Python
            PyGILState_Release(gstate);
            // already destroyed: zmsg_destroy(&ev->msg);
            return ret;
        }
        else // we expect a zmsg type
        {
            // check whether we received our own message
            PyZmsgObject *c = (PyZmsgObject *)pReturn;
            assert(c->msg);
            if (c->msg == ev->msg)
            {
                zsys_info("we received our own message");
            }
            if ( zmsg_size(c->msg) > 0 )
            {
                //  It would be nicer if we could just return the zmsg in
                //  the python object. However how do we then prevent destruction
                //  by the garbage controller. For now just duplicate.
                zmsg_t *ret = zmsg_dup( c->msg );
                Py_DECREF(pReturn);  // decrease refcount to trigger destroy
                // Release the GIL again as we are ready with Python
                PyGILState_Release(gstate);
                zmsg_destroy(&ev->msg);
                return ret;
            }
        }
    }
    Py_DECREF(pReturn);  // decrease refcount to trigger destroy
    // Release the GIL again as we are ready with Python
    PyGILState_Release(gstate); // TODO: didn't we already do this?

    return NULL;
}

zmsg_t *
pythonactor_custom_socket(pythonactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    assert(ev->msg);
    // Get the socket...
    zframe_t *frame = zmsg_pop(ev->msg);
    if (zframe_size(frame) == sizeof( void *) )
    {
        void *p = *(void **)zframe_data(frame);
#ifdef __UTYPE_LINUX
        assert(p == (void *)&self->fd);
        s_handle_inotify_events(self, (sphactor_actor_t *)ev->actor, self->fd, &self->wd);
#endif
    }

    zframe_destroy(&frame);
    zmsg_destroy(&ev->msg);
    return NULL;
}

zmsg_t *
pythonactor_stop(pythonactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    if (self->pyinstance)
    {
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        PyObject *pReturn = PyObject_CallMethod(self->pyinstance, "handleStop", "Osss", Py_None, ev->type, ev->name, ev->uuid);
        Py_XINCREF(pReturn);  // increase refcount to prevent destroy
        if (!pReturn)
        {
            PyErr_Print();
            zsys_error("pythonactor: error calling handleStop method");
        }
        Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
        PyGILState_Release(gstate);
    }
    // remove the watched file
#ifdef __UTYPE_LINUX
    s_destroy_inotify(self, (sphactor_actor_t *)ev->actor);
#endif
    return NULL;
}

zmsg_t *
pythonactor_handler(sphactor_event_t *ev, void *args)
{
    assert(args);
    pythonactor_t *self = (pythonactor_t *)args;
    return pythonactor_handle_msg(self, ev);
}

zmsg_t *
pythonactor_handle_msg(pythonactor_t *self, sphactor_event_t *ev)
{
    assert(self);
    assert(ev);
    // these calls don't require a python instance
    if (streq(ev->type, "API"))
    {
        return pythonactor_api(self, ev);
    }
    else if (streq(ev->type, "DESTROY"))
    {
        pythonactor_destroy(&self);
        return NULL;
    }
    else if (streq(ev->type, "INIT"))
    {
        return pythonactor_init(self, ev);
    }
    else if (self->pyinstance == NULL)
    {
        zsys_warning("No valid python file has been loaded (yet)");
        return NULL;
    }
    // these calls require a pyinstance! (loaded python file)
    else if ( streq(ev->type, "TIME") )
    {
        return pythonactor_timer(self, ev);
    }
    else if ( streq(ev->type, "SOCK") )
    {
        return pythonactor_socket(self, ev);
    }
    else if ( streq(ev->type, "FDSOCK") )
    {
        return pythonactor_custom_socket(self, ev);
    }
    else if ( streq(ev->type, "STOP") )
    {
        return pythonactor_stop(self, ev);
    }
    else
        zsys_error("Unhandled sphactor event: %s", ev->type);

    return NULL;
}

