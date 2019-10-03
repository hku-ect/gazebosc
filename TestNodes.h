//
//  TestNodes.h
//  gazebosc
//
//  Created by Call +31646001602 on 26/09/2019.
//

#ifndef TestNodes_h
#define TestNodes_h

#include <cstdlib>
#include "RtMidi.h"
#include <imgui.h>
#include <czmq.h>
#include <lo/lo_cpp.h>

struct MidiNode : GNode
{
    using GNode::GNode;
    
    int midiPort = 0;
    unsigned int nPorts = 0;
    RtMidiIn *midiin = nullptr;
    std::vector<std::string> portNames;
    const char *activePort;
    
    explicit MidiNode() : GNode( "MidiNode",
                                { },    //no input slots
                                {
                                    {"MidiOSC", NodeSlotOSC}  // Output slots
                                } )
    {
        //TODO: Setup thread-timing on the sphactor side
        
        //get midi
        try {
          midiin = new RtMidiIn();
        } catch (RtMidiError &error) {
          error.printMessage();
        }
        
        if ( midiin ) {
            //find connected devices
            nPorts = midiin->getPortCount();
            std::string portName;
            printf("Number of ports: %i \n", nPorts );
            
            portNames.clear();
            for ( unsigned int i=0; i<nPorts; i++ ) {
              try {
                portName = midiin->getPortName(i);
                portNames.push_back(portName);
              }
              catch ( RtMidiError &error ) {
                error.printMessage();
              }
                printf( "  Input Port #%i: %s \n", i+1, portName.c_str() );
            }
            
            //open selected port
            Connect();
        }
    }
    
    //This function attempts to open the port midiPort
    // the idea is that this can be called from the UI with a button after this data changes...
    //  TODO: decide if this automatically should happen when this value changes...
    void Connect() {
        if ( midiin->isPortOpen()) {
            midiin->closePort();
        }
        
        if ( nPorts > midiPort ) {
            midiin->openPort(midiPort);
            activePort = portNames.at(midiPort).c_str();
            printf("Connected to %s \n", activePort);
        }
        else {
            activePort = nullptr;
        }
    }
    
    virtual ~MidiNode() {
        
        if ( midiin )
        {
            midiin->closePort();
            delete midiin;
        }
    }
    
    virtual zmsg_t *HandleMessage(sphactor_event_t *ev, void*args)
    {
        static char* msgBuffer = new char[1024];
        static double stamp;
        static int nBytes, i;
        static std::vector<unsigned char> message;
        
        if ( midiin && midiin->isPortOpen() ) {
            zmsg_t *msg = nullptr;
            
            do {
                stamp = midiin->getMessage( &message );
                nBytes = message.size();
                
                if ( nBytes > 0 ) {
                    //manually read the three bytes
                    byte b1 = message[0];
                    int channel = b1 & 0x0F;
                    int index = (int)message[1];
                    int value = (int)message[2];
                    
                    char *address = new char[64];
                    sprintf(address, "/%s/%i/event", portNames[midiPort].c_str(), channel);
                    
                    lo_message lo = lo_message_new();
                    lo_message_add_int32(lo, index);
                    lo_message_add_int32(lo, value);
                    size_t len = sizeof(msgBuffer);
                    lo_message_serialise(lo, address, msgBuffer, &len);
                    lo_message_free(lo);
                    
                    if ( msg == NULL ) {
                        msg = zmsg_new();
                    }
                    
                    zframe_t *frame = zframe_new(msgBuffer, len);
                    zmsg_append(msg, &frame);
                }
            } while ( nBytes > 0 );

            return msg;
        }
        
        return nullptr;
    }
    
    virtual void RenderUI() {
        ImGui::SetNextItemWidth(200);
        
        static const char* current_item = "Select device...";
        if ( ImGui::BeginCombo("", current_item) ) {
            for (int n = 0; n < portNames.size(); n++)
            {
                bool is_selected = (midiPort == n);
                if (ImGui::Selectable(portNames.at(n).c_str(), is_selected)) {
                    current_item = portNames.at(n).c_str();
                    midiPort = n;
                    Connect();
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndCombo();
        }
        
        GNode::RenderUI();
    }
};

struct CountNode : GNode
{
    using GNode::GNode;

    int count = 0;
    
    explicit CountNode() : GNode(   "Count",
                                 { {"OSC", NodeSlotOSC} },    //Input slots
                                 { {"OSC", NodeSlotOSC} } )// Output slots
    {
        
    }

    virtual zmsg_t *HandleMessage(sphactor_event_t *ev, void*args)
    {
        if ( ev->msg != NULL ) {
            count += 1;
        }
        
        //send whatever we received to our output
        return ev->msg;
    }
    
    virtual void RenderUI() {
        ImGui::Text("Count: %i", count);
    }
};

struct LogNode : GNode
{
    explicit LogNode() : GNode(   "Log",
                                 { {"OSC", NodeSlotOSC} },    //Input slot
                                 { } )// Output slotss
    {
        
    }
    
    virtual zmsg_t *HandleMessage(sphactor_event_t *ev, void*args)
    {
        static byte *msgBuffer = new byte[1024];
        static zframe_t* frame;
        if ( ev->msg != NULL ) {
            printf("Log: \n");
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
                    printf(" %s \n", msgBuffer);
                    
                    // parse individual arguments
                    int count = lo_message_get_argc(lo);
                    char *types = lo_message_get_types(lo);
                    lo_arg **argv = lo_message_get_argv(lo);
                    for ( int i = 0; i < count; ++i ) {
                        switch(types[i]) {
                            case 'i':
                                printf("  Int: %i \n", argv[i]->i);
                                break;
                            case 'f':
                                printf("  Float: %f \n", argv[i]->f);
                                break;
                            default:
                                printf("  Unhandled type: %c \n", types[i]);
                                break;
                        }
                    }
                    
                    //free message
                    lo_message_free(lo);
                }
            } while ( frame != NULL );
        }
        
        //TODO: Clean up the message
        
        return NULL;
    }
};

//Most basic form of node that performs its own (threaded) behaviour
struct PulseNode : GNode
{
    using GNode::GNode;
    
    int millis = 16;    //60 fps
    
    virtual zmsg_t *Timer() {
        return nullptr;
    }
};

#endif /* TestNodes_h */
