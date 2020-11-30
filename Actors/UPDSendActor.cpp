#include "libsphactor.h"
#include "UDPSendActor.h"

zmsg_t* UDPSendActor( sphactor_event_t *ev, void* args ) {
    //handle various actor messages to interact with instance
    UDPSend* self = (UDPSend*) args;
    assert(self);

    if ( streq(ev->type, "INIT")) {
        self->CreateSocket(ev->actor);
    }
    else if ( streq(ev->type, "STOP")) {
        self->DestroySocket(ev->actor);
    }
    else if ( streq(ev->type, "DESTROY")) {
        zsys_info("TODO: DESTROY?");
    }
    else if ( streq(ev->type, "SOCK") ) {
        // TODO: update our address info if changed?

        // Parse individual frames into osc messages
        do {
            self->frame = zmsg_pop(ev->msg);
            if ( self->frame ) {

                //TODO: for now (until BUNDLE support)
                // Forward as-is, this should already be an OSC message...
                //zsys_info("Sending");
                int rc = zsys_udp_send(self->udpSock, self->frame, (sockaddr_in*)self->bind_to->ai_addr, INET_ADDRSTRLEN );
                assert(rc==0);

                //TODO: Add to bundle (NOT SUPPORTED?)
                //if ( bundle == NULL ) {
                //    lo_timetag now;
                //    lo_timetag_now(&now);
                //    bundle = lo_bundle_new(now);
                //}
                //lo_bundle_add_message(bundle, (char*) self->msgBuffer, lo);

                zframe_destroy(&self->frame);
            }
        }
        while ( self->frame != NULL );

        //TODO: Send bundle
        //TODO: Free bundle recursively (all nested messages as well)

        //clean up msg, return null
        zmsg_destroy(&ev->msg);
        return nullptr;
    }
    return ev->msg;
}


void UDPSend::CreateSocket(const sphactor_actor_t* actor) {
    char* buf = new char[32];
    sprintf( buf, "%i", port);

    udpSock = zsys_udp_new(false);

    bind_to = NULL;
    struct addrinfo hint;
    memset (&hint, 0, sizeof(struct addrinfo));
    hint.ai_flags = AI_NUMERICHOST;
#if !defined (CZMQ_HAVE_ANDROID) && !defined (CZMQ_HAVE_FREEBSD)
    hint.ai_flags |= AI_V4MAPPED;
#endif
    hint.ai_socktype = SOCK_DGRAM;
    hint.ai_protocol = IPPROTO_UDP;
    hint.ai_family = zsys_ipv6 () ? AF_INET6 : AF_INET;

    int rc = getaddrinfo ("127.0.0.1", buf, &hint, &bind_to);
    assert ( rc == 0 );

    if ( bind (udpSock, bind_to->ai_addr, bind_to->ai_addrlen) ) {
        //TODO: Figure out what this if passing even means...
    }

    delete[] buf;
}

void UDPSend::DestroySocket(const sphactor_actor_t* actor) {
    zsys_udp_close(udpSock);
    freeaddrinfo (bind_to);
}
