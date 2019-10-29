//
//  UtilityNodes.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "DefaultNodes.h"

///
/// CountNode
///
CountNode::CountNode(const char* uuid) : GNode(   "Count",
                             { {"OSC", NodeSlotOSC} },    //Input slots
                             { {"OSC", NodeSlotOSC} }, uuid )// Output slots
{
    
}

zmsg_t *CountNode::ActorMessage(sphactor_event_t *ev)
{
    count += 1;
    
    //send whatever we received to our output
    return ev->msg;
}

void CountNode::Render(float deltaTime) {
    ImGui::Text("Count: %i", count);
}

// CountNode


///
/// LogNode
///

LogNode::LogNode(const char* uuid) : GNode(   "Log",
                             { {"OSC", NodeSlotOSC} },    //Input slot
                             { }, uuid )// Output slotss
{
    
}

zmsg_t *LogNode::ActorMessage(sphactor_event_t *ev)
{
    static double ONE_HALF_TO_32 = .000000000232831;
    static lo_timetag* timePointer = NULL;
    static lo_timetag startTime;
    
    if ( timePointer == NULL ) {
        lo_timetag_now(&startTime);
        timePointer = &startTime;
    }
    
    byte *msgBuffer;
    zframe_t* frame;
    
    zsys_info("Log: ");
    do {
        frame = zmsg_pop(ev->msg);
        if ( frame ) {
            // get byte array for frame
            msgBuffer = zframe_data(frame);
            size_t len = zframe_size(frame);
            
            // convert to osc message
            int result;
            lo_message lo = lo_message_deserialise(msgBuffer, len, &result);
            assert( result == 0 );
            
            // first part of bytes is the osc address
            zsys_info(" %s", msgBuffer);
            
            // parse individual arguments
            int count = lo_message_get_argc(lo);
            char *types = lo_message_get_types(lo);
            lo_arg **argv = lo_message_get_argv(lo);
            lo_timetag time;
            for ( int i = 0; i < count; ++i ) {
                switch(types[i]) {
                    case 'i':
                        zsys_info("  Int: %i ", argv[i]->i);
                        break;
                    case 'f':
                        zsys_info("  Float: %f ", argv[i]->f);
                        break;
                    case 't':
                        time = argv[i]->t;
                        // fraction is a measure of 1/2^32nd
                        zsys_info("  Timestamp: %f", ( time.sec - startTime.sec ) + time.frac * ONE_HALF_TO_32 );
                        break;
                    default:
                        zsys_info("  Unhandled type: %c ", types[i]);
                        break;
                }
            }
            
            //free message
            lo_message_free(lo);
            zframe_destroy(&frame);
        }
    } while ( frame != NULL );
    
    zmsg_destroy(&ev->msg);
    
    return NULL;
}

// LogNode::


///
/// PulseNode
///

//Most basic form of node that performs its own (threaded) behaviour
PulseNode::PulseNode(const char* uuid) : GNode(   "Pulse",
                                {  },    //Input slot
                                { { "OSC", NodeSlotOSC } }, uuid )// Output slotss
{
    msgBuffer = new char[1024];
    address = new char[32];
    rate = 1;
    
    sprintf(address, "/pulse" );
}

PulseNode::~PulseNode() {
    delete[] msgBuffer;
    delete[] address;
}

void PulseNode::CreateActor() {
    GNode::CreateActor();
    SetRate(rate);
}

void PulseNode::Render(float deltaTime) {
    ImGui::SetNextItemWidth(100);
    if ( ImGui::InputInt( "Rate", &rate ) ) {
        SetRate(rate);
    }
    ImGui::SetNextItemWidth(100);
    ImGui::InputText( "Address", address, 32 );
}

zmsg_t *PulseNode::ActorCallback() {
    lo_message lo = lo_message_new();
    size_t len = sizeof(msgBuffer);
    lo_message_serialise(lo, address, msgBuffer, &len);
    lo_message_free(lo);
    
    zmsg_t *msg =zmsg_new();
    zframe_t *frame = zframe_new(msgBuffer, len);
    zmsg_append(msg, &frame);
    
    //TODO: figure out if this needs to be destroyed here...
    zframe_destroy(&frame);
    
    return msg;
}

void PulseNode::SerializeNodeData( zconfig_t *section ) {
    zconfig_t *zRate = zconfig_new("rate", section);
    zconfig_set_value(zRate, "%i", rate);
    
    zconfig_t *zAddress = zconfig_new("address", section);
    zconfig_set_value(zAddress, "%s", address);
    
    GNode::SerializeNodeData(section);
}

void PulseNode::DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it ) {
    //pop args from front
    char* strRate = *it;
    it++;
    char* strAddress = *it;
    it++;
    
    rate = atoi(strRate);
    sprintf(address, "%s", strAddress);
    
    SetRate(rate);
    
    free(strRate);
    free(strAddress);
    
    //send remaining args (probably just xpos/ypos) to base
    GNode::DeserializeNodeData(args, it);
}

// PulseNode
