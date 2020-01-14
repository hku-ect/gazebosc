//
//  UtilityActors.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "DefaultActors.h"
#include <sstream>

///
/// CountActor
///
CountActor::CountActor(const char* uuid) : GActor(   "Count",
                             { {"OSC", ActorSlotOSC} },    //Input slots
                             { {"OSC", ActorSlotOSC} }, uuid )// Output slots
{
    
}

zmsg_t *CountActor::ActorMessage(sphactor_event_t *ev)
{
    count += 1;
    
    //send whatever we received to our output
    return ev->msg;
}

void CountActor::Render(float deltaTime) {
    ImGui::Text("Count: %i", count);
}

// CountActor


///
/// LogActor
///

LogActor::LogActor(const char* uuid) : GActor(   "Log",
                             { {"OSC", ActorSlotOSC} },    //Input slot
                             { }, uuid )// Output slotss
{
    
}

zmsg_t *LogActor::ActorMessage(sphactor_event_t *ev)
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
            printf("OSCLog: %s \n", msgBuffer);
            
            // parse individual arguments
            int count = lo_message_get_argc(lo);
            
            char *types = lo_message_get_types(lo);
            lo_arg **argv = lo_message_get_argv(lo);
            lo_timetag time;
            for ( int i = 0; i < count; ++i ) {
                switch(types[i]) {
                    case 's':
                        printf(" String: %s \n", &argv[i]->S);
                        break;
                    case 'i':
                        printf(" Int: %i \n", argv[i]->i);
                        break;
                    case 'f':
                        printf(" Float: %f \n", argv[i]->f);
                        break;
                    case 't':
                        time = argv[i]->t;
                        // fraction is a measure of 1/2^32nd
                        printf(" Timestamp: %f \n", ( time.sec - startTime.sec ) + time.frac * ONE_HALF_TO_32 );
                        break;
                    default:
                        printf(" Unhandled type: %c \n", types[i]);
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

// LogActor


///
/// PulseActor
///

//Most basic form of an actor that performs its own (threaded) behaviour
PulseActor::PulseActor(const char* uuid) : GActor(   "Pulse",
                                {  },    //Input slot
                                { { "OSC", ActorSlotOSC } }, uuid )// Output slotss
{
    msgBuffer = new char[1024];
    address = new char[32];
    rate = 1;
    
    sprintf(address, "/pulse" );
}

PulseActor::~PulseActor() {
    delete[] msgBuffer;
    delete[] address;
}

void PulseActor::CreateActor() {
    GActor::CreateActor();
    SetRate(rate);
}

void PulseActor::Render(float deltaTime) {
    ImGui::SetNextItemWidth(100);
    if ( ImGui::InputInt( "Rate", &rate ) ) {
        SetRate(rate);
    }
    ImGui::SetNextItemWidth(100);
    ImGui::InputText( "Address", address, 32 );
}

zmsg_t *PulseActor::ActorCallback() {
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

void PulseActor::SerializeActorData( zconfig_t *section ) {
    zconfig_t *zRate = zconfig_new("rate", section);
    zconfig_set_value(zRate, "%i", rate);
    
    zconfig_t *zAddress = zconfig_new("address", section);
    zconfig_set_value(zAddress, "%s", address);
    
    GActor::SerializeActorData(section);
}

void PulseActor::DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it ) {
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
    GActor::DeserializeActorData(args, it);
}

// PulseActor


///
/// ManualPulse
///

//Most basic form of an actor that performs its own (threaded) behaviour
ManualPulse::ManualPulse(const char* uuid) : GActor(   "ManualPulse",
                                {  },    //Input slot
                                { { "OSC", ActorSlotOSC } }, uuid )// Output slotss
{
    msgBuffer = new char[1024];
    address = new char[32];
    delay = 1;
    timer = 0;
    send = false;
    
    sprintf(address, "/pulse" );
}

ManualPulse::~ManualPulse() {
    delete[] msgBuffer;
    delete[] address;
}

void ManualPulse::CreateActor() {
    GActor::CreateActor();
    SetRate(60);
}

void ManualPulse::Render(float deltaTime) {
    ImGui::SetNextItemWidth(100);
    ImGui::InputFloat( "Delay", &delay );
    ImGui::SetNextItemWidth(100);
    ImGui::InputText( "Address", address, 32 );
    if ( ImGui::Button( "Send" ) ) {
        send = true;
    }
}

zmsg_t *ManualPulse::ActorCallback() {
    if ( !send ) return NULL;
    
    timer += .016f;
    if ( timer >= delay ) {
        timer = 0;
        lo_message lo = lo_message_new();
        size_t len = sizeof(msgBuffer);
        lo_message_serialise(lo, address, msgBuffer, &len);
        lo_message_free(lo);
        
        zmsg_t *msg =zmsg_new();
        zframe_t *frame = zframe_new(msgBuffer, len);
        zmsg_append(msg, &frame);
        
        //TODO: figure out if this needs to be destroyed here...
        zframe_destroy(&frame);
        
        send = false;
        
        return msg;
    }
    
    return NULL;
}

void ManualPulse::SerializeActorData( zconfig_t *section ) {
    zconfig_t *zDelay = zconfig_new("rate", section);
    zconfig_set_value(zDelay, "%f", delay);
    
    zconfig_t *zAddress = zconfig_new("address", section);
    zconfig_set_value(zAddress, "%s", address);
    
    GActor::SerializeActorData(section);
}

void ManualPulse::DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it ) {
    //pop args from front
    char* strDelay = *it;
    it++;
    char* strAddress = *it;
    it++;
    
    delay = atof(strDelay);
    sprintf(address, "%s", strAddress);
    
    free(strDelay);
    free(strAddress);
    
    //send remaining args (probably just xpos/ypos) to base
    GActor::DeserializeActorData(args, it);
}

// ManualPulse
