#ifndef NATNETACTOR_H
#define NATNETACTOR_H

#include "libsphactor.h"

struct NatNet {
    // receive / send datagrams
    // TODO: implement ofxNatNet?
    zsock_t* dgramr = NULL;
    zsock_t* dgrams = NULL;

    zmsg_t * handleMsg( sphactor_event_t *ev );

    NatNet() {

    }
};

#endif // NATNETACTOR_H
