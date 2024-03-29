#ifndef OSCOUTPUTACTOR_H
#define OSCOUTPUTACTOR_H

#include "libsphactor.hpp"
#include <string>

class OSCOutput : public Sphactor {
public:
    static const char *capabilities;
    zsock_t* dgrams = NULL;

    std::string name = "";
    std::string host = "";
    std::string port = "";

    zmsg_t *handleInit(sphactor_event_t *ev);

    zmsg_t *handleAPI(sphactor_event_t *ev);

    zmsg_t *handleSocket(sphactor_event_t *ev);

    zmsg_t *handleStop(sphactor_event_t *ev);

    OSCOutput() : Sphactor() {

    }

    /* This is never called */
    ~OSCOutput() {
        if ( this->dgrams != NULL ) {
            zsys_info("Destroying dgrams");
            zsock_destroy(&this->dgrams);
            this->dgrams = NULL;
        }
    }
};

#endif // OSCOUTPUTACTOR_H
