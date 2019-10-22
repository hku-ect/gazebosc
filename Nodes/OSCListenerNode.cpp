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
    msgBuffer = new byte[1024];
    
    port = 1234;
    StartServer();
    SetRate(1);
}

OSCListenerNode::~OSCListenerNode() {
    StopAndDestroyServer();
    
    delete[] msgBuffer;
}

void OSCListenerNode::StopAndDestroyServer() {
    if ( server != NULL ) {
        lo_server serv = lo_server_thread_get_server(server);
        if ( serv != NULL ) {
            lo_server_thread_stop(server);
        }
        lo_server_thread_free(server);
        server = NULL;
    }
}

void OSCListenerNode::StartServer() {
    StopAndDestroyServer();
    
    char* buf = new char[32];
    sprintf( buf, "%i", port);
    server = lo_server_thread_new(buf, NULL);
    delete[] buf;
    
    lo_server_thread_start(server);
    lo_server_thread_add_method(server, NULL, NULL, MessageReceived, this);
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
       StartServer();
       isDirty = false;
    }
}
