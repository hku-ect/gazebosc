#ifndef COUNTACTOR_H
#define COUNTACTOR_H
#include "libsphactor.h"

struct Count {
    int msgCount;

    zmsg_t *handleMsg( sphactor_event_t *ev );

    Count() {
        msgCount = 0;
    }
};

#endif // COUNTACTOR_H
