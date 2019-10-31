//
//  ClientNode.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "DefaultNodes.h"

UDPSendNode::UDPSendNode(const char* uuid) : GNode(   "UDPSend",
                                { { "OSC", NodeSlotOSC } },    //Input slot
                                { }, uuid )// Output slotss
{
    ipAddress = new char[64];
    port = 1234;
}

UDPSendNode::~UDPSendNode() {
    delete[] ipAddress;
    
    if ( address != NULL ) {
        lo_address_free(address);
        address = NULL;
    }
}

void UDPSendNode::Render(float deltaTime) {
    ImGui::SetNextItemWidth(150);
    if ( ImGui::InputText("IP Address", ipAddress, 64) ) {
        isDirty = true;
    }
    ImGui::SetNextItemWidth(100);
    if ( ImGui::InputInt( "Port", &port ) ) {
        isDirty = true;
    }
}

zmsg_t *UDPSendNode::ActorMessage( sphactor_event_t *ev )
{
    // If port/ip information changed, update our target address
    if ( isDirty ) {
        if ( address != nullptr ) {
            lo_address_free(address);
        }
        
        char* buf = new char[32];
        sprintf( buf, "%i", port);
        address = lo_address_new(ipAddress, buf);
        delete[] buf;
        
        isDirty = false;
    }
    
    lo_bundle bundle = NULL;
    
    // Parse individual frames into messages
    do {
        frame = zmsg_pop(ev->msg);
        if ( frame ) {
            // get byte array for frame
            msgBuffer = zframe_data(frame);
            size_t len = zframe_size(frame);
            
            // convert to osc message
            int result;
            lo_message lo = lo_message_deserialise(msgBuffer, len, &result);
            //lo_timetag time = lo_message_get_timestamp(lo);
            //if ( time.sec == 0 ) {
            //    lo_timetag_now(&time);
            //    lo_message_add_timetag(lo, time );
            //}
            assert( result == 0 );
            
            // add to bundle
            if ( bundle == NULL ) {
                lo_timetag now;
                lo_timetag_now(&now);
                bundle = lo_bundle_new(now);
            }
            lo_bundle_add_message(bundle, (char*) msgBuffer, lo);
            
            zframe_destroy(&frame);
        }
    }
    while ( frame != NULL );
    
    //send as bundle
    lo_send_bundle(address, bundle);
    //free bundle recursively (all nested messages as well)
    lo_bundle_free_recursive(bundle);
    
    //clean up msg, return null
    zmsg_destroy(&ev->msg);
    return nullptr;
}

void UDPSendNode::SerializeNodeData( zconfig_t *section ) {
    zconfig_t *zIP = zconfig_new("ipAddress", section);
    zconfig_set_value(zIP, "%s", ipAddress);
    
    zconfig_t *zPort = zconfig_new("port", section);
    zconfig_set_value(zPort, "%i", port);
    
    GNode::SerializeNodeData(section);
}

void UDPSendNode::DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it ) {
    //pop args from front
    char* strIp = *it;
    it++;
    char* strPort = *it;
    it++;
    
    sprintf(ipAddress, "%s", strIp);
    port = atoi(strPort);
    
    isDirty = true;
    
    free(strIp);
    free(strPort);
    
    //send remaining args (probably just xpos/ypos) to base
    GNode::DeserializeNodeData(args, it);
}
