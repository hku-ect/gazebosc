#ifndef PROCESSACTOR_H
#define PROCESSACTOR_H

#include "libsphactor.hpp"

class ProcessActor : public Sphactor
{
public:
    static const char *capabilities;

    ProcessActor() {}
    ~ProcessActor() {}

    zproc_t *proc = NULL;
    bool keepalive = false;

    // handle initialisation
    zmsg_t * handleInit(sphactor_event_t *ev);

    // handle timed events
    zmsg_t * handleTimer(sphactor_event_t *ev);

    zmsg_t * handleStop(sphactor_event_t *ev);

    zmsg_t * handleAPI(sphactor_event_t *ev);

    zmsg_t * handleCustomSocket(sphactor_event_t *ev);

    // handle receiving messages
    zmsg_t * handleSocket(sphactor_event_t *ev) { if ( ev->msg ) zmsg_destroy(&ev->msg); return nullptr; }

};

#endif // PROCESSACTOR_H
