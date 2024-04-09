#ifndef PYTHONACTOR_H
#define PYTHONACTOR_H

#include "libsphactor.h"
#include "Python.h"
#include <structmember.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _pythonactor_t
{
    char     *main_filename;  // the full path to the main python source file
    PyObject *pymodule;       // the imported pythonfile as a module
    PyObject *pyinstance;     // our python instance
    PyObject *_exitexc;       // ref to the SystemExit exception
    int      fd;              // filedescriptor for file change events
    int      wd;              // watch descriptor of the file change events
};

typedef struct _pythonactor_t pythonactor_t;

int python_init();
void python_add_path(const char *path);
void python_remove_path(const char *path);

PyObject *python_call_file_func(const char *file, const char *func, const char *format, ...);

void * pythonactor_new_helper(void *args);
pythonactor_t * pythonactor_new();
void pythonactor_destroy(pythonactor_t **self_p);

zmsg_t *pythonactor_init(pythonactor_t *self, sphactor_event_t *ev);
zmsg_t *pythonactor_timer(pythonactor_t *self, sphactor_event_t *ev);
zmsg_t *pythonactor_api(pythonactor_t *self, sphactor_event_t *ev);
zmsg_t *pythonactor_socket(pythonactor_t *self, sphactor_event_t *ev);
zmsg_t *pythonactor_custom_socket(pythonactor_t *self, sphactor_event_t *ev);
zmsg_t *pythonactor_stop(pythonactor_t *self, sphactor_event_t *ev);

zmsg_t * pythonactor_handler(sphactor_event_t *ev, void *args);
zmsg_t * pythonactor_handle_msg(pythonactor_t *self, sphactor_event_t *ev);

#ifdef __cplusplus
} // end extern
#endif

#endif // PYTHONACTOR_H
