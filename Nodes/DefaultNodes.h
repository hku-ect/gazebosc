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
#include "GNode.h"


struct MidiNode : GNode
{
    using GNode::GNode;
    
    int                         midiPort = -1;
    unsigned int                nPorts = 0;
    RtMidiIn                    *midiin = nullptr;
    std::vector<std::string>    portNames;
    char                        *activePort;
    char                        *msgBuffer;
    char                        *address;
    const char*                 current_item = "Select device...";
    
    explicit MidiNode( const char* uuid );
    virtual ~MidiNode();
    
    
    
    // Node specific functions
    void Connect();
    
    // UI overrides
    void Render(float deltaTime);
    
    // Thread overrides
    void CreateActor();
    zmsg_t *ActorCallback();
    
    // Serialization overrides
    void SerializeNodeData( zconfig_t *section );
    void DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it );
};


struct CountNode : GNode
{
    using GNode::GNode;

    int count = 0;
    
    explicit CountNode(const char* uuid);
    virtual void Render(float deltaTime);
    virtual zmsg_t *ActorMessage(sphactor_event_t *ev);
};


struct LogNode : GNode
{
    explicit LogNode(const char* uuid);
    
    virtual zmsg_t *ActorMessage(sphactor_event_t *ev);
};


struct RelayNode : GNode
{
    explicit RelayNode(const char* uuid) : GNode(   "Relay",
                                 { {"OSC", NodeSlotOSC} },    //Input slot
                                 { {"OSC", NodeSlotOSC} }, uuid )// Output slotss
    {
       
    }
};


//Most basic form of node that performs its own (threaded) behaviour
struct PulseNode : GNode
{
    using GNode::GNode;
    
    char* msgBuffer;
    char* address;
    int rate;
    
    explicit PulseNode(const char* uuid);
    virtual ~PulseNode();
    
    void Render(float deltaTime);
    
    void CreateActor();
    zmsg_t *ActorCallback();
    
    virtual void SerializeNodeData( zconfig_t *section );
    void DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it );
};


struct ClientNode : GNode
{
    using GNode::GNode;
    
    char *ipAddress;
    int port;
    bool isDirty = true;
    bool debug = false;
    lo_address address = nullptr;
    byte *msgBuffer;
    zframe_t* frame;
    
    explicit ClientNode(const char* uuid);
    virtual ~ClientNode();
    
    void Render(float deltaTime);
    
    zmsg_t *ActorMessage( sphactor_event_t *ev );
    
    virtual void SerializeNodeData( zconfig_t *section );
    
    void DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it );
};


struct OSCListenerNode : GNode
{
    using GNode::GNode;
    
    int port;
    bool isDirty = true;
    //lo_server server = NULL;
    float timer = 0;
    SOCKET udpSock = -1;
    
    explicit OSCListenerNode(const char* uuid);
    virtual ~OSCListenerNode();
    
    void CreateActor();
    
    void StopAndDestroyServer( const sphactor_node_t* node );
    void StartServer( const sphactor_node_t* node );
    
    void ActorInit(const sphactor_node_t *node);
    void ActorStop(const sphactor_node_t *node);
    
    static zmsg_t * MessageReceived(void * socket) {
        
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
                        lo_message_add_timetag(msg, ts);

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
                // Got single message
                //  Just throw the byte array to our output...
                zmsg_append(zmsg, &frame);
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
