#include "pythonactor.h"
#include "pyzmsg.h"

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
        "outputs\n"
        "    output\n"
        //TODO: Perhaps add NatNet output type so we can filter the data multiple times...
        "        type = \"OSC\"\n";

// this is needed because we have to conform to returning a void * thus we need to wrap
// the pythonactor_new method, unless some know a better method?
void *
pythonactor_construct(void *args)
{
    return (void *)pythonactor_new(args);
}

pythonactor_t *
pythonactor_new(void *args)
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
            strcpy(self->main_filename, filename);
            //  Acquire the GIL
            PyGILState_STATE gstate;
            gstate = PyGILState_Ensure();

            //  import our python file
            PyObject *pClass, *pModule, *pName;
            pName = PyUnicode_DecodeFSDefault( filename );
            if ( pName == NULL )
            {
                if ( PyErr_Occurred() )
                    PyErr_Print();
                zsys_error("error loading %s", filename);
                PyGILState_Release(gstate);
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
                return NULL;
            }

            //  get the class with the same name as the filename
            pClass = PyObject_GetAttrString(pModule, filename );
            if (pClass == NULL )
            {
                if (PyErr_Occurred())
                    PyErr_Print();
                zsys_error("pClass is NULL");
                PyGILState_Release(gstate);
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
                return NULL;
            }

            //  this is our last Python API call before threads jump in,
            //  thus we need to release the GIL
            // Release the GIL again as we are ready with Python
            zsys_info("Successfully loaded %s", filename);
            PyGILState_Release(gstate);
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
            zmsg_t *ret = zmsg_new();
            Py_ssize_t size = PyBytes_Size(pReturn);
            char buf[size];// = new char[size];
            memcpy(buf, PyBytes_AsString(pReturn), size);
            zmsg_addmem(ret, buf, size);
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
    // if there are things left publish them
    if ( ev->msg && zmsg_size(ev->msg) > 0 )
    {
        return ev->msg;
    }
    else
    {
        zmsg_destroy(&ev->msg);
    }
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

