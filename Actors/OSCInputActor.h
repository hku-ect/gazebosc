//
// Created by Aaron Oostdijk on 08/12/2020.
//

#ifndef GAZEBOSC_OSCINPUTACTOR_H
#define GAZEBOSC_OSCINPUTACTOR_H

#include "libsphactor.h"
#include <string>

struct OSCInput {
    std::string port = "6200";
    zsock_t* dgramr = NULL;
    zmsg_t * handleMsg( sphactor_event_t *ev );
};

#endif //GAZEBOSC_OSCINPUTACTOR_H
