#include "ClientActor.h"

zmsg_t* Client::handleMsg( sphactor_event_t * ev ) {

    if ( streq(ev->type, "INIT") ) {
        //TODO: init capabilities

        //TODO: Create default dgram socket?
    }
    else if ( streq(ev->type, "INIT") ) {
        //TODO: send msg over our dgram socket, if it exists
    }
    return NULL;
}
