//
//  OSCListener.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "DefaultNodes.h"

OSCListenerNode::OSCListenerNode(const char* uuid) : GNode(   "OSCListener",
                                {  },    //Input slot
                                { { "OSC", NodeSlotOSC } }, uuid )// Output slotss
{
    port = 1234;
    isDirty = false;
    //server = NULL;
}

OSCListenerNode::~OSCListenerNode() {
    //if ( server != NULL ) {;
    //    lo_server_free(server);
    //}
}

void OSCListenerNode::CreateActor() {
    GNode::CreateActor();
    SetRate(1);
}

void OSCListenerNode::ActorInit( const sphactor_node_t *node ) {
    StartServer(node);
}

void OSCListenerNode::ActorStop( const sphactor_node_t *node ) {
    StopAndDestroyServer(node);
}

void OSCListenerNode::StopAndDestroyServer( const sphactor_node_t *node ) {
    sphactor_node_poller_remove((sphactor_node_t*)node, (void*)&udpSock);
    zsys_udp_close(udpSock);
}

void OSCListenerNode::StartServer( const sphactor_node_t *node ) {
    
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
    
    int rc = getaddrinfo (NULL, buf, &hint, &bind_to);
    assert ( rc == 0 );
    
    if ( bind (udpSock, bind_to->ai_addr, bind_to->ai_addrlen) ) {
        //TODO: Figure out what this if passing even means...
    }
    
    delete[] buf;
    freeaddrinfo (bind_to);
    
    sphactor_node_poller_add((sphactor_node_t*)node, &udpSock, MessageReceived);
}

void OSCListenerNode::Render(float deltaTime) {
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
