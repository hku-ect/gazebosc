#ifdef PYTHON3_FOUND
#include "PythonNode.h"
#include <iostream>
#include <lo/lo_cpp.h>
#include "pyzmsg.h"

void
python_init()
{
    //  add internal wrapper to available modules
    PyImport_AppendInittab("sph", PyInit_PyZmsg);
    Py_Initialize();

    //  add some paths for importing python files
    PyRun_SimpleString(
                    "import sys\n"
                    "import os\n"
                    "print(os.getcwd())\n"
                    "sys.path.append(os.getcwd())\n"
                    "sys.path.append('/home/arnaud/src/czmq/bindings/python')\n"
                    "print(\"Python {0} initialized. Paths: {1}\".format(sys.version, sys.path))\n");

    //  import the internal wrapper module
    PyObject *imp = PyImport_ImportModule("sph");
    assert(imp);
    //  this is our last Python API call before threads jump in,
    //  thus we need to release the GIL
    PyEval_SaveThread();
}

int
PythonNode::UpdatePythonFile()
{
    //  Acquire the GIL
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    //  import our python file
    PyObject *pClass, *pModule, *pName;
    pName = PyUnicode_DecodeFSDefault( filename.c_str() );
    if ( pName == NULL )
    {
        if ( PyErr_Occurred() )
            PyErr_Print();
        zsys_error("error loading %s", filename.c_str());
        return 1;
    }

    //  we loaded the file now import it
    pModule = PyImport_Import( pName );
    if ( pModule == NULL )
    {
        if ( PyErr_Occurred() )
            PyErr_Print();
        zsys_error("error importing %s", filename.c_str());

        return 2;
    }

    //  get the class with the same name as the filename
    pClass = PyObject_GetAttrString(pModule, "tester" );
    if (pClass == NULL )
    {
        if (PyErr_Occurred())
            PyErr_Print();
        zsys_error("pClass is NULL");
        return 3;
    }

    // create an instance of the class
    pClassInstance = PyObject_CallObject((PyObject *) pClass, NULL);
    if (pClassInstance == NULL )
    {
        if (PyErr_Occurred())
            PyErr_Print();
        zsys_error("pClassInstance is NULL");
        return 4;
    }

    //  this is our last Python API call before threads jump in,
    //  thus we need to release the GIL
    // Release the GIL again as we are ready with Python
    PyGILState_Release(gstate);
    return 0;
}
void
PythonNode::ActorInit( const sphactor_node_t *node )
{
    this->UpdatePythonFile();
}

zmsg_t *
py_class_actor(sphactor_event_t *ev, void *args)
{
    zsys_info("Hello from py_class_actor wrapper: type=%s", ev->type);
    //  This is not compatible with subinterpreters!
    //  https://docs.python.org/3/c-api/init.html#c.PyGILState_Ensure
    //  Acquire the GIL
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // pass the msg to a python callable member
    PyObject *pActorClass = (PyObject *)args;
    assert(pActorClass);
    // build evmsg for member argument
    PyZmsgObject *evmsg = (PyZmsgObject *)PyObject_CallObject((PyObject *)&PyZmsgType, NULL);
    assert(evmsg);
    if (ev->msg)
    {
        zmsg_destroy(&evmsg->msg);
        evmsg->msg = ev->msg;
    }
    // call member 'handleMsg' with event arguments
    PyObject *pReturn = PyObject_CallMethod(pActorClass, "handleMsg", "Osss", evmsg, ev->type, ev->name, ev->uuid);
    Py_XINCREF(pReturn);  // increase refcount to prevent destroy
    if (!pReturn)
    {
        PyErr_Print();
        Py_XDECREF(pReturn);  // decrease refcount to trigger destroy

    }
    else if (pReturn != Py_None)
    {
        if (PyBytes_Check(pReturn))
        {
            // handle python bytes
            zmsg_t *ret = zmsg_new();
            Py_ssize_t size = PyBytes_Size(pReturn);
            char *buf = new char[size];
            memcpy(buf, PyBytes_AsString(pReturn), size);
            zmsg_addmem(ret, buf, size);
            Py_XDECREF(pReturn);  // decrease refcount to trigger destroy
            // Release the GIL again as we are ready with Python
            PyGILState_Release(gstate);
            zmsg_destroy(&ev->msg);
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
    PyGILState_Release(gstate);
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
PythonNode::ActorMessage(sphactor_event_t *ev)
{
    std::cout << "Python Node: name=" << ev->name << " type=" << ev->type << " uuid=" << ev->uuid << "\n";

    return py_class_actor(ev, this->pClassInstance);
    /*
    lo_message oscmsg = lo_message_new();
    lo_message_add_string(oscmsg, "Hello World");

    byte* buf = new byte[2048];
    size_t len = sizeof(buf);

    lo_message_serialise(oscmsg, "hello", buf, &len);
    lo_message_free(oscmsg);

    zmsg_t *returnMsg = zmsg_new();
    zmsg_pushmem(returnMsg, buf, len);
    return returnMsg;
    */
}

#endif
