#ifndef CLIENTACTOR_H
#define CLIENTACTOR_H

#include "libsphactor.h"

struct Client {

    zmsg_t * handleMsg( sphactor_event_t * ev );

    Client() {

    }
};

#endif // CLIENTACTOR_H
