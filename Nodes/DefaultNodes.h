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
    virtual void Render(float deltaTime);
    
    // Thread overrides
    virtual zmsg_t *ActorCallback();
    
    // Serialization overrides
    virtual void SerializeNodeData( zconfig_t *section );
    virtual void DeserializeNodeData( ImVector<char*> *args, ImVector<char*>::iterator it );
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
    byte *msgBuffer;
    lo_server_thread server = NULL;
    zframe_t* frame;
    zmsg_t* toSend;
    float timer = 0;
    
    explicit OSCListenerNode(const char* uuid);
    virtual ~OSCListenerNode();
    
    void StopAndDestroyServer();
    void StartServer();
    
    static int MessageReceived( const char *path, const char *types, lo_arg **argv, int argc, lo_message lo_msg, void *user_data ) {
        OSCListenerNode *self = (OSCListenerNode*)user_data;
        
        // Parse msg into a zmgs_t
        zmsg_t *zmsg = zmsg_new();
        
        size_t len = sizeof(msgBuffer);
        lo_message_serialise(lo_msg, path, self->msgBuffer, &len);
        
        zframe_t *frame = zframe_new(self->msgBuffer, len);
        zmsg_append(zmsg, &frame);
        
        // Tell our actor to send the msg
        sphactor_send_msg(self->actor, zmsg);
        
        return 0;
    }
    
    void Render(float deltaTime);
};

#endif /* DefaultNodes_h */
