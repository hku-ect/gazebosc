//
//  OSCListener.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "DefaultActors.h"

OSCListenerActor::OSCListenerActor(const char* uuid) : GActor(   "OSCListener",
                                {  },                               // Input slot
                                { { "OSC", ActorSlotOSC } }, uuid )  // Output slots
{
    port = 1234;
    isDirty = false;
    //server = NULL;
}

OSCListenerActor::~OSCListenerActor() {
    //if ( server != NULL ) {;
    //    lo_server_free(server);
    //}
}

void OSCListenerActor::CreateActor() {
    GActor::CreateActor();
    SetRate(1);
}

void OSCListenerActor::ActorInit( const sphactor_actor_t *actor ) {
    StartServer(actor);
}

void OSCListenerActor::ActorStop( const sphactor_actor_t *actor ) {
    StopAndDestroyServer(actor);
}

void OSCListenerActor::StopAndDestroyServer( const sphactor_actor_t *actor ) {
    sphactor_actor_poller_remove((sphactor_actor_t*)actor, (void*)&udpSock);
    zsys_udp_close(udpSock);
}

void OSCListenerActor::StartServer( const sphactor_actor_t *actor ) {
    
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
    
    if ( bind (udpSock, bind_to->ai_addr, bind_to->ai_addrlen) ) {
        //TODO: Figure out what this if passing even means...
    }
    
    delete[] buf;
    freeaddrinfo (bind_to);
    
    sphactor_actor_poller_add((sphactor_actor_t*)actor, &udpSock, MessageReceived);
}

void OSCListenerActor::Render(float deltaTime) {
    ImGui::SetNextItemWidth(100);
    if ( ImGui::InputInt( "Port", &port ) ) {
        isDirty = true;
        timer = 0;
    }
    
    // Short delay before responding to edits
    timer += deltaTime;
    if ( isDirty && timer > 1 ) {
        CreateActor();
        isDirty = false;
    }
}
