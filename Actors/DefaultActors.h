//
//  DefaultNodes.h
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#ifndef DefaultNodes_h
#define DefaultNodes_h

#include <cstdlib>
#include "RtMidi.h"
#include <imgui.h>
#include <czmq.h>
#include <lo/lo_cpp.h>
#include "GActor.h"
#include <sstream>


struct MidiActor : GActor
{
    using GActor::GActor;
    
    int                         midiPort = -1;
    unsigned int                nPorts = 0;
    RtMidiIn                    *midiin = nullptr;
    std::vector<std::string>    portNames;
    char                        *activePort;
    char                        *msgBuffer;
    char                        *address;
    const char*                 current_item = "Select device...";
    
    explicit MidiActor( const char* uuid );
    virtual ~MidiActor();
    
    // Node specific functions
    void Connect();
    
    // UI overrides
    void Render(float deltaTime);
    
    // Thread overrides
    void CreateActor();
    zmsg_t *ActorCallback();
    
    // Serialization overrides
    void SerializeActorData( zconfig_t *section );
    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it );
};


struct CountActor : GActor
{
    using GActor::GActor;

    int count = 0;
    
    explicit CountActor(const char* uuid);
    virtual void Render(float deltaTime);
    virtual zmsg_t *ActorMessage(sphactor_event_t *ev);
};


struct LogActor : GActor
{
    explicit LogActor(const char* uuid);
    
    virtual zmsg_t *ActorMessage(sphactor_event_t *ev);
};


struct RelayActor : GActor
{
    explicit RelayActor(const char* uuid) : GActor(   "Relay",
                                 { {"OSC", ActorSlotOSC} },    //Input slot
                                 { {"OSC", ActorSlotOSC} }, uuid )// Output slotss
    {
       
    }
};


//Most basic form of node that performs its own (threaded) behaviour
struct PulseActor : GActor
{
    using GActor::GActor;
    
    char* msgBuffer;
    char* address;
    int rate;
    
    explicit PulseActor(const char* uuid);
    virtual ~PulseActor();
    
    void Render(float deltaTime);
    
    void CreateActor();
    zmsg_t *ActorCallback();
    
    virtual void SerializeActorData( zconfig_t *section );
    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it );
};



//Sends a message when you click in the UI
struct ManualPulse : GActor
{
    using GActor::GActor;
    
    char* msgBuffer;
    char* address;
    float delay;
    float timer;
    bool send;
    
    explicit ManualPulse(const char* uuid);
    virtual ~ManualPulse();
    
    void Render(float deltaTime);
    
    void CreateActor();
    zmsg_t *ActorCallback();
    
    virtual void SerializeActorData( zconfig_t *section );
    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it );
};



struct UDPSendActor : GActor
{
    using GActor::GActor;
    
    char *ipAddress;
    int port;
    bool isDirty = true;
    bool debug = false;
    lo_address address = nullptr;
    byte *msgBuffer;
    zframe_t* frame;
    
    explicit UDPSendActor(const char* uuid);
    virtual ~UDPSendActor();
    
    void Render(float deltaTime);
    
    zmsg_t *ActorMessage( sphactor_event_t *ev );
    
    virtual void SerializeActorData( zconfig_t *section );
    
    void DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it );
};


struct OSCListenerActor : GActor
{
    using GActor::GActor;
    
    int port;
    bool isDirty = true;
    //lo_server server = NULL;
    float timer = 0;
    SOCKET udpSock = -1;
    
    explicit OSCListenerActor(const char* uuid);
    virtual ~OSCListenerActor();
    
    void CreateActor();
    
    void StopAndDestroyServer( const sphactor_actor_t* actor );
    void StartServer( const sphactor_actor_t* actor );
    
    void ActorInit(const sphactor_actor_t *actor);
    void ActorStop(const sphactor_actor_t *actor);
    
    static zmsg_t * MessageReceived(void * socket) {
        int result = 0;
        char* peer = new char[INET_ADDRSTRLEN];
        zframe_t * frame;
        SOCKET s = *(int*)socket;
        
        frame = zsys_udp_recv(s, peer, INET_ADDRSTRLEN);
        
        delete[] peer;
        
        if ( frame ) {
            zmsg_t *zmsg = zmsg_new();
            
            //TODO: Maybe move this somewhere else?
            // get byte array for frame
            byte * data = zframe_data(frame);
            size_t size = zframe_size(frame);
            ssize_t len = lo_validate_string(data, size);
            
            if (!strcmp((const char *) data, "#bundle")) {
                // Got bundle
                //  Parse separate messages into frames
                char *pos;
                int remain;
                uint32_t elem_len;
                lo_timetag ts, now;
                
                pos = (char*) data + len;
                remain = size - len;

                lo_timetag_now(&now);
                ts.sec = lo_otoh32(*((uint32_t *) pos));
                pos += 4;
                ts.frac = lo_otoh32(*((uint32_t *) pos));
                pos += 4;
                remain -= 8;
                
                while (remain >= 4) {
                    lo_message msg;
                    elem_len = lo_otoh32(*((uint32_t *) pos));
                    pos += 4;
                    remain -= 4;

                    if (!strcmp(pos, "#bundle")) {
                        //TODO: Figure out what needs to happen here...
                        // This is probably if you receive nested bundles
                        zsys_info("Unhandled case...");
                    } else {
                        int result;
                        msg = lo_message_deserialise(pos, elem_len, &result);
                        
                        // set timetag from bundle
                        // FIXME: this balloons the messages, find a better way
                        //lo_message_add_timetag(msg, ts);

                        // bump the reference count so that it isn't
                        // automatically released
                        lo_message_incref(msg);

                        // Send
                        
                        byte* buf = new byte[2048];
                        size_t len = sizeof(buf);
                        lo_message_serialise(msg, pos, buf, &len);
                        
                        zframe_t *frame = zframe_new(buf, len);
                        zmsg_append(zmsg, &frame);
                        
                        zframe_destroy(&frame);
                        lo_message_free(msg);
                        delete[] buf;
                    }

                    pos += elem_len;
                    remain -= elem_len;
                }
            }
            else {
                zsys_info("GOT SOMETHING");
                // Got single message
                //  Just throw the byte array to our output...
                lo_message msg = lo_message_deserialise(data, size, &result);
                if (NULL == msg) {
                    zsys_info("OSC ERROR: Invalid message received");
                }
                else {
                    zframe_t *frame = zframe_new(data, size);
                    zmsg_append(zmsg, &frame);
                    
                    zframe_destroy(&frame);
                    lo_message_free(msg);
                }
            }
            
            zframe_destroy(&frame);
            
            return zmsg;
        }
        
        return NULL;
    }
    
    static ssize_t lo_validate_string(void *data, ssize_t size)
    {
        ssize_t i = 0, len = 0;
        char *pos = (char*) data;

        if (size < 0) {
            return -LO_ESIZE;       // invalid size
        }
        for (i = 0; i < size; ++i) {
            if (pos[i] == '\0') {
                len = 4 * (i / 4 + 1);
                break;
            }
        }
        if (0 == len) {
            return -LO_ETERM;       // string not terminated
        }
        if (len > size) {
            return -LO_ESIZE;       // would overflow buffer
        }
        for (; i < len; ++i) {
            if (pos[i] != '\0') {
                return -LO_EPAD;    // non-zero char found in pad area
            }
        }
        return len;
    }
    
    void Render(float deltaTime);
};

#endif /* DefaultNodes_h */
