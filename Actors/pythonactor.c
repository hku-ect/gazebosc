#include "pythonactor.h"
#include "pyzmsg.h"

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

static zosc_t *
s_py_zosc(PyObject *pAddress, PyObject *pData)
{
    assert( PyUnicode_Check(pAddress) );
    assert( PyList_Check(pData) );
    PyObject *stringbytes = PyUnicode_AsASCIIString(pAddress);
    zosc_t *ret = zosc_new( PyBytes_AsString( stringbytes) );
    // iterate
    for ( Py_ssize_t i=0;i<PyList_Size(pData);++i )
    {
        PyObject *item = PyList_GetItem(pData, i);
        assert(item);
        // determine type
        if ( PyLong_Check(item) )
        {
            long v = PyLong_AsLong(item);
            zosc_append(ret, "h", v);
        }
        else
            zsys_warning("unsupported python type");
    }
    return ret;
}

int python_init()
{
    Py_UnbufferedStdioFlag = 1;
    //  add internal wrapper to available modules
    int rc = PyImport_AppendInittab("sph", PyInit_PyZmsg);
    Py_Initialize();
    assert(rc == 0);
    //  add some paths for importing python files
    rc = PyRun_SimpleString(
                    "import sys\n"
                    "import os\n"
                    //"print(os.getcwd())\n"
                    "sys.path.append(os.getcwd())\n"
                    //"sys.path.append('/home/arnaud/src/czmq/bindings/python')\n"
                    //"print(\"Python {0} initialized. Paths: {1}\".format(sys.version, sys.path))\n"
                       );
    assert(rc == 0);
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

    return rc;
}

const char * pythonactorcapabilities =
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
}

void
pythonactor_destroy(pythonactor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        pythonactor_t *self = *self_p;
        if (self->main_filename )
            zstr_free(&self->main_filename);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

zmsg_t *
pythonactor_init(pythonactor_t *self, sphactor_event_t *ev)
{
    sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(pythonactorcapabilities));

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

        if ( strlen(filename) )
        {
            if(self->main_filename)
                zstr_free(&self->main_filename);
            self->main_filename = filename;

            char *filebasename = s_basename(filename);
            char *pyname = s_remove_ext(filebasename);

            //  Acquire the GIL
            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure();

            //  import our python file
            PyObject *pClass, *pModule, *pName;
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
            pModule = PyImport_Import( pName );
            if ( pModule == NULL )
            {
                if ( PyErr_Occurred() )
                    PyErr_Print();
                zsys_error("error importing %s", filename);
                PyGILState_Release(gstate);
                zstr_free(&pyname);
                zstr_free(&filebasename);
                return NULL;
            }

            //  get the class with the same name as the filename
            pClass = PyObject_GetAttrString(pModule, pyname );
            if (pClass == NULL )
            {
                if (PyErr_Occurred())
                    PyErr_Print();
                zsys_error("pClass is NULL");
                PyGILState_Release(gstate);
                zstr_free(&pyname);
                zstr_free(&filebasename);
                return NULL;
            }

            // create an instance of the class
            self->pyinstance = PyObject_CallObject((PyObject *) pClass, NULL);
            if (self->pyinstance == NULL )
            {
                if (PyErr_Occurred())
                    PyErr_Print();
                zsys_error("pClassInstance is NULL");
                PyGILState_Release(gstate);
                zstr_free(&pyname);
                zstr_free(&filebasename);
                return NULL;
            }

            //  this is our last Python API call before threads jump in,
            //  thus we need to release the GIL
            // Release the GIL again as we are ready with Python
            zsys_info("Successfully loaded %s", filename);
            PyGILState_Release(gstate);
            zstr_free(&pyname);
            zstr_free(&filebasename);
            return NULL;
        }
        zstr_free(&filename);
    }

    if ( ev->msg ) zmsg_destroy(&ev->msg);
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
    PyObject *pReturn = PyObject_CallMethod(self->pyinstance, "handleMsg", "Osss", evmsg, ev->type, ev->name, ev->uuid);
    Py_XINCREF(pReturn);  // increase refcount to prevent destroy
    if (!pReturn)
    {
        PyErr_Print();
        Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
        zsys_error("pythonactor: error calling handleMsg");
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
                char buf[size];// = new char[size];
                memcpy(buf, PyBytes_AsString(pReturn), size);
                zmsg_addmem(ret, buf, size);
            }
            else
            {
                zsys_warning("PyBytes has zero size");
            }
            Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
            // Release the GIL again as we are ready with Python
            PyGILState_Release(gstate);
            // already destroyed: zmsg_destroy(&ev->msg);
            return ret;
        }
        else if ( PyTuple_Check(pReturn) )
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
            Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
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
                Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
                // Release the GIL again as we are ready with Python
                PyGILState_Release(gstate);
                zmsg_destroy(&ev->msg);
                return ret;
            }
        }
        Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
    }
    // Release the GIL again as we are ready with Python
    PyGILState_Release(gstate); // TODO: didn't we already do this?

    return NULL;
}

zmsg_t *
pythonactor_custom_socket(pythonactor_t *self, sphactor_event_t *ev)
{
    return NULL;
}

zmsg_t *
pythonactor_stop(pythonactor_t *self, sphactor_event_t *ev)
{
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
    if ( streq(ev->type, "INIT") )
    {
        return pythonactor_init(self, ev);
    }
    else if ( streq(ev->type, "TIME") )
    {
        return pythonactor_timer(self, ev);
    }
    else if ( streq(ev->type, "API") )
    {
        return pythonactor_api(self, ev);
    }
    else if ( streq(ev->type, "SOCK") )
    {
        return pythonactor_socket(self, ev);
    }
    else if ( streq(ev->type, "FDSOCK") )
    {
        zframe_t *frame = zmsg_pop(ev->msg);
        assert(frame);
        zmsg_destroy(&ev->msg);

        if (zframe_size(frame) == sizeof( void *) )
        {
            void *p = *( (void **)zframe_data(frame));
            zsock_t *sock = (zsock_t *)zsock_resolve(p);
            if ( sock )
            {
                zmsg_t *msg = zmsg_recv(sock);
                assert(msg);
                ev->msg = msg;
            }
            else
            {
                zsys_error("we received a FDSOCK event but we didn't get a socket pointer");
            }
        }
        zframe_destroy(&frame);

        return pythonactor_custom_socket(self, ev);
    }
    else if ( streq(ev->type, "STOP") )
    {
        return pythonactor_stop(self, ev);
    }
    else if ( streq(ev->type, "DESTROY") )
    {
        pythonactor_destroy(&self);
        return NULL;
    }
    else
        zsys_error("Unhandled sphactor event: %s", ev->type);
}

