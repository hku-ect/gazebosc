#ifndef CLIENTACTOR_H
#define CLIENTACTOR_H

#include "libsphactor.h"
#include <string>

struct Client {
    zsock_t* dgrams = NULL;

    std::string name = "";
    std::string host = "";
    std::string port = "";

    zmsg_t * handleMsg( sphactor_event_t * ev );

    Client() {

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
