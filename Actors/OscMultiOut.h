#ifndef OSCMULTIOUT_H
#define OSCMULTIOUT_H
#include "libsphactor.hpp"

class OSCMultiOut : public Sphactor
{
public:
    OSCMultiOut() : Sphactor() {}
    static const char *capabilities;
    zsock_t* dgrams = NULL;
    zlist_t* hosts = nullptr;

    zmsg_t *handleInit(sphactor_event_t *ev);

    zmsg_t *handleAPI(sphactor_event_t *ev);

    zmsg_t *handleSocket(sphactor_event_t *ev);

    zmsg_t *handleStop(sphactor_event_t *ev);

};

#endif // OSCMULTIOUT_H
