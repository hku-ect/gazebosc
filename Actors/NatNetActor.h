#ifndef NATNETACTOR_H
#define NATNETACTOR_H

#include "libsphactor.h"
#include <string>

struct NatNet {
    // receive / send datagrams
    // TODO: implement NatNet?
    zsock_t* dgramr = NULL;
    zsock_t* dgrams = NULL;

    std::string host = "";

    zmsg_t * handleMsg( sphactor_event_t *ev );

    NatNet() {

    }
};

#endif