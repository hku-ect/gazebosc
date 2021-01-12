#ifndef GAZEBOSC_OSCINPUTACTOR_H
#define GAZEBOSC_OSCINPUTACTOR_H

#include "libsphactor.hpp"
#include <string>

class OSCInput : public Sphactor {
private:
    std::string port = "6200";
    zsock_t* dgramr = NULL;

public:
    OSCInput() : Sphactor() {

    }

    zmsg_t * handleInit( sphactor_event_t *ev );
    zmsg_t * handleStop( sphactor_event_t *ev );
    zmsg_t * handleCustomSocket( sphactor_event_t *ev );
    zmsg_t * handleAPI( sphactor_event_t *ev );
};

#endif //GAZEBOSC_OSCINPUTACTOR_H
