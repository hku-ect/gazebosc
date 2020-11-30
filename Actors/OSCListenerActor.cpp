#include "OSCListenerActor.h"

zmsg_t* OSCListenerActor( sphactor_event_t *ev, void* args ) {

    OSCListener *self = (OSCListener*)args;
    assert(self);

    if ( streq(ev->type, "INIT")) {
        //zsys_info("INIT");
        self->StartServer(ev->actor);
    }
    else if ( streq(ev->type, "STOP")) {
        //zsys_info("STOP");
        self->StopAndDestroyServer(ev->actor);
    }
    else if ( streq(ev->type, "DESTROY")) {
        zsys_info("TODO: DESTROY?");
    }

    return ev->msg;
}

void OSCListener::StartServer( const sphactor_actor_t* actor ) {
    char* buf = new char[32];
    sprintf( buf, "%i", port);

    udpSock = zsys_udp_new(false);

    struct addrinfo *bind_to = NULL;
    struct addrinfo hint;
    memset (&hint, 0, sizeof(struct addrinfo));
    hint.ai_flags = AI_NUMERICHOST;
#if !defined (CZMQ_HAVE_ANDROID) && !defined (CZMQ_HAVE_FREEBSD)
    hint.ai_flags |= AI_V4MAPPED;
#endif
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;
    hint.ai_family = zsys_ipv6 () ? AF_INET6 : AF_INET;

    int rc = getaddrinfo ("0.0.0.0", buf, &hint, &bind_to);
    assert ( rc == 0 );

    rc = bind (udpSock, bind_to->ai_addr, bind_to->ai_addrlen);
    assert( rc == 0 );

    zsys_info("Bind result: %i", rc);

    delete[] buf;
    freeaddrinfo (bind_to);

    sphactor_actor_poller_add((sphactor_actor_t*)actor, &udpSock, MessageReceived);
}


void OSCListener::StopAndDestroyServer( const sphactor_actor_t* actor ) {
    sphactor_actor_poller_remove((sphactor_actor_t*)actor, (void*)&udpSock);
    zsys_udp_close(udpSock);
}
