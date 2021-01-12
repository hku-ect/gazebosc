#ifndef CLIENTACTOR_H
#define CLIENTACTOR_H

#include "libsphactor.hpp"
#include <string>

class Client : public Sphactor {
public:
    zsock_t* dgrams = NULL;

    std::string name = "";
    std::string host = "";
    std::string port = "";

    zmsg_t *handleInit(sphactor_event_t *ev);

    zmsg_t *handleAPI(sphactor_event_t *ev);

    zmsg_t *handleSocket(sphactor_event_t *ev);

    zmsg_t *handleStop(sphactor_event_t *ev);

    Client() : Sphactor() {

    }

    /* This is never called */
    ~Client() {
        if ( this->dgrams != NULL ) {
            zsys_info("Destroying dgrams");
            zsock_destroy(&this->dgrams);
            this->dgrams = NULL;
        }
    }
};

#endif // CLIENTACTOR_H
