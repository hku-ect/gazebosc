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
    char     *main_filename;  // the main python source file
    PyObject *pyinstance;     // our python instance
};

typedef struct _pythonactor_t pythonactor_t;

int python_init();

void * pythonactor_construct(void *args);
pythonactor_t * pythonactor_new(void *args);
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
