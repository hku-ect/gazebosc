#ifndef PYZMSG_H
#define PYZMSG_H
#include <Python.h>
#include <structmember.h>
#include <czmq.h>

extern "C" {
typedef struct {
    PyObject_HEAD
    zmsg_t *msg;
} PyZmsgObject;

static void
PyZmsg_dealloc(PyZmsgObject *self)
{
    zmsg_destroy(&self->msg);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
PyZmsg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyZmsgObject *self;
    self = (PyZmsgObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->msg = zmsg_new();
        zsys_info("init zmsg");
        if (self->msg == NULL) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject *) self;
}

static int
PyZmsg_init(PyZmsgObject *self, PyObject *args, PyObject *kwds)
{
    //  not using any constructor arguments
    return 0;
}

static PyObject *
PyZmsg_size(PyZmsgObject *self, PyObject *Py_UNUSED(ignored) )
{
    return PyLong_FromSize_t(zmsg_size(self->msg));
}

static PyObject *
PyZmsg_content_size(PyZmsgObject *self, PyObject *Py_UNUSED(ignored) )
{
    return PyLong_FromSize_t(zmsg_content_size(self->msg));
}

static PyObject *
PyZmsg_addstr(PyZmsgObject *self, PyObject *string, void *closure )
{
    PyObject* pStrObj = PyUnicode_AsUTF8String(string);
    char* zStr = PyBytes_AsString(pStrObj);
    char* zStrDup = strdup(zStr);
    Py_DECREF(pStrObj);
    zsys_info("%s", zStrDup);
    return PyLong_FromLong(zmsg_addstr(self->msg, zStrDup));
}

static PyObject *
PyZmsg_popstr(PyZmsgObject *self, PyObject *Py_UNUSED(ignored) )
{
    char* msg = zmsg_popstr(self->msg);
    // todo: do we need to cleanup?
    if ( msg ) return PyUnicode_FromString(msg);
    //PyErr_SetString(PyExc_TypeError,
    //                        "No more strings in zmsg");
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef PyZmsg_methods[] = {
    {"size", (PyCFunction) PyZmsg_size, METH_NOARGS,
     "Return size of message, i.e. number of frames (0 or more)."
    },
    {"content_size", (PyCFunction) PyZmsg_content_size, METH_NOARGS,
     "Return total size of all frames in message."
    },
    {"addstr", (PyCFunction) PyZmsg_addstr, METH_O,
     "Push string as new frame to end of message. Returns 0 on success, -1 on error."
    },
    {"popstr", (PyCFunction) PyZmsg_popstr, METH_NOARGS,
     "Pop frame off front of message, return as fresh string. If there were no more frames in the message, returns NULL."
    },
    {NULL}  /* Sentinel */
};

/*static PyTypeObject PyZmsgType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "internal.Zmsg",
    .tp_doc = "Internal Sphactor Zmsg",
    .tp_basicsize = sizeof(PyZmsgObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyZmsg_new,
    //.tp_init = (initproc) PyZmsg_init,
    .tp_dealloc = (destructor) PyZmsg_dealloc,
    //.tp_members = Custom_members,
    .tp_methods = PyZmsg_methods,
};*/

static PyTypeObject PyZmsgType = {
    PyObject_HEAD_INIT(NULL)
    "internal.Zmsg",           // tp_name
    sizeof(PyZmsgObject),      // tp_basicsize
    0,                         // tp_itemsize
    (destructor) PyZmsg_dealloc,
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare //tp_as_async
    0,                         // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    0,                         // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,        // tp_flags
    "Internal Sphactor Zmsg",  // tp_doc
    0,		                     // tp_traverse
    0,		                     // tp_clear
    0,		                     // tp_richcompare
    0,		                     // tp_weaklistoffset
    0,		                     // tp_iter
    0,		                     // tp_iternext
    PyZmsg_methods,            // tp_methods
    0,                         // tp_members
    0,                         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    0,                         // tp_init
    0,                         // tp_alloc
    PyZmsg_new,          // tp_new
  };

static PyModuleDef pyzmsgmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "internalwrap",
    .m_doc = "Internal wrapper to zmsg type.",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_PyZmsg(void)
{
    PyObject *m;
    if (PyType_Ready(&PyZmsgType) < 0)
        return NULL;

    m = PyModule_Create(&pyzmsgmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyZmsgType);
    if (PyModule_AddObject(m, "PyZmsg", (PyObject *) &PyZmsgType) < 0) {
        Py_DECREF(&PyZmsgType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
} // end extern
#endif // ZMSG_H
