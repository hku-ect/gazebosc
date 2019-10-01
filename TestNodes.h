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
    
    virtual zmsg_t *Update(sphactor_event_t *ev, void*args)
    {
        static double stamp;
        static int nBytes, i;
        static std::vector<unsigned char> message;
        
        stamp = midiin->getMessage( &message );
        nBytes = message.size();
        
        if ( nBytes > 0 ) {
            for ( i=0; i<nBytes; i++ )
                printf( "Byte %i = %i, stamp = %f \n", i, (int)message[i], stamp );
            //TODO: Create zmsg_t with byte data
            
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

    virtual zmsg_t *Update(sphactor_event_t *ev, void*args)
    {
        //TODO: Print whatever OSC we received...
        
        
        count += 1;
        zframe_t *f = zframe_new(&count, sizeof(int));
        zmsg_t* ret = zmsg_new();
        zmsg_add(ret, f);
        return ret;
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
