//
//  TestNodes.h
//  gazebosc
//
//  Created by aaronvark on 26/09/2019.
//

#ifndef TestNodes_h
#define TestNodes_h

#include <cstdlib>
#include "RtMidi.h"
#include <imgui.h>
#include <czmq.h>
#include <lo/lo_cpp.h>
#include "GNode.h"

struct MidiNode : GNode
{
    using GNode::GNode;
    
    int midiPort = 0;
    unsigned int nPorts = 0;
    RtMidiIn *midiin = nullptr;
    std::vector<std::string> portNames;
    char *activePort;
    char *msgBuffer;
    char *address;
    const char* current_item = "Select device...";
    
    explicit MidiNode( const char* uuid ) : GNode( "Midi",
                                { },    //no input slots
                                {
                                    {"OSC", NodeSlotOSC}  // Output slots
                                }, uuid )
    {
        msgBuffer = new char[1024];
        address = new char[64];
        activePort = new char[128];
        
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
        }
        
        SetRate(60);
    }
    
    virtual ~MidiNode() {
        
        if ( midiin )
        {
            midiin->closePort();
            delete midiin;
        }
        
        delete[] address;
        delete[] msgBuffer;
        delete[] activePort;
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
            sprintf(activePort, "%s", portNames.at(midiPort).c_str());
            printf("Connected to %s \n", activePort);
        }
        else {
            activePort = nullptr;
        }
    }
    
    zmsg_t *Update()
    {
        static double stamp;
        static int nBytes;
        static std::vector<unsigned char> message;
        
        if ( midiin && midiin->isPortOpen() ) {
            zmsg_t *msg = nullptr;
            
            do {
                stamp = midiin->getMessage( &message );
                nBytes = message.size();
                
                if ( nBytes > 0 ) {
                    //TODO: this assumes the message is always 3 bytes...
                    if ( nBytes != 3 ) {
                        printf("NBYTES NOT 3: %i", nBytes );
                    }
                    //manually read three bytes
                    byte b1 = message[0];
                    int channel = b1 & 0x0F;
                    int index = (int)message[1];
                    int value = (int)message[2];
                    
                    sprintf(address, "/midi/%s/%i/event", portNames[midiPort].c_str(), channel);
                    
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
    
    void RenderUI() {
        ImGui::SetNextItemWidth(200);
        
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
    
    virtual void Serialize( zconfig_t *section ) {
        zconfig_t *device = zconfig_new("device", section);
        zconfig_set_value(device, "%s", activePort);
        
        GNode::Serialize(section);
    }
    
    void HandleArgs( ImVector<char*> *args, ImVector<char*>::iterator it ) {
        //pop args from front
        char* strPort = *it;
        it++;
        
        //find activePort in portNames, and assign index to midiPort
        //TODO: TEST THIS!
        int i = 0;
        bool found = false;
        for( auto it = portNames.begin(); it != portNames.end(); it++) {
            if ( streq( it->c_str(), activePort ) ) {
                midiPort = i;
                found = true;
                Connect();
                break;
            }
            i++;
        }
        
        if ( !found ) {
            zsys_error("Device not found: %s", activePort);
        }
        
        free(strPort);
        
        //send remaining args (probably just xpos/ypos) to base
        GNode::HandleArgs(args, it);
    }
};

struct CountNode : GNode
{
    using GNode::GNode;

    int count = 0;
    
    explicit CountNode(const char* uuid) : GNode(   "Count",
                                 { {"OSC", NodeSlotOSC} },    //Input slots
                                 { {"OSC", NodeSlotOSC} }, uuid )// Output slots
    {
        
    }

    zmsg_t *HandleMessage(sphactor_event_t *ev)
    {
        count += 1;
        
        //send whatever we received to our output
        return ev->msg;
    }
    
    void RenderUI() {
        ImGui::Text("Count: %i", count);
    }
};

struct LogNode : GNode
{
    explicit LogNode(const char* uuid) : GNode(   "Log",
                                 { {"OSC", NodeSlotOSC} },    //Input slot
                                 { }, uuid )// Output slotss
    {
       
    }
    
    zmsg_t *HandleMessage(sphactor_event_t *ev)
    {
        static zframe_t* frame;
        
        byte *msgBuffer;// = new byte[1024];
        
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
                for ( int i = 0; i < count; ++i ) {
                    switch(types[i]) {
                        case 'i':
                            zsys_info("  Int: %i ", argv[i]->i);
                            break;
                        case 'f':
                            zsys_info("  Float: %f ", argv[i]->f);
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
        
        //delete[] msgBuffer;
        
        zmsg_destroy(&ev->msg);
        
        return NULL;
    }
};


//Most basic form of node that performs its own (threaded) behaviour
struct PulseNode : GNode
{
    using GNode::GNode;
    
    char* msgBuffer;
    char* address;
    int rate;
    
    explicit PulseNode(const char* uuid) : GNode(   "Pulse",
                                    {  },    //Input slot
                                    { { "OSC", NodeSlotOSC } }, uuid )// Output slotss
    {
        msgBuffer = new char[1024];
        address = new char[32];
        rate = 1;
        
        sprintf(address, "/pulse" );
        
        SetRate(rate);
    }
    
    virtual ~PulseNode() {
        delete[] msgBuffer;
        delete[] address;
    }
    
    void RenderUI() {
        ImGui::SetNextItemWidth(100);
        if ( ImGui::InputInt( "Rate", &rate ) ) {
            SetRate(rate);
        }
        ImGui::SetNextItemWidth(100);
        ImGui::InputText( "Address", address, 32 );
    }
    
    zmsg_t *Update() {
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
    
    virtual void Serialize( zconfig_t *section ) {
        zconfig_t *zRate = zconfig_new("rate", section);
        zconfig_set_value(zRate, "%i", rate);
        
        zconfig_t *zAddress = zconfig_new("address", section);
        zconfig_set_value(zAddress, "%s", address);
        
        GNode::Serialize(section);
    }
    
    void HandleArgs( ImVector<char*> *args, ImVector<char*>::iterator it ) {
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
        GNode::HandleArgs(args, it);
    }
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
    
    explicit ClientNode(const char* uuid) : GNode(   "Client",
                                    { { "OSC", NodeSlotOSC } },    //Input slot
                                    { }, uuid )// Output slotss
    {
        ipAddress = new char[64];
        port = 1234;
    }
    
    virtual ~ClientNode() {
        delete[] ipAddress;
        
        if ( address != NULL ) {
            lo_address_free(address);
            address = NULL;
        }
    }
    
    void RenderUI() {
        ImGui::SetNextItemWidth(150);
        if ( ImGui::InputText("IP Address", ipAddress, 64) ) {
            isDirty = true;
        }
        ImGui::SetNextItemWidth(100);
        if ( ImGui::InputInt( "Port", &port ) ) {
            isDirty = true;
        }
        ImGui::SetNextItemWidth(100);
        if ( ImGui::Checkbox("Debug", &debug) ) {
            
        }
    }
    
    zmsg_t *HandleMessage( sphactor_event_t *ev )
    {
        //if port/ip information changed, update our target address
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
        
        lo_bundle bundle = nullptr;
        
        //parse individual frames into messages
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
                
                // add to bundle
                if ( bundle == nullptr ) {
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
    
    virtual void Serialize( zconfig_t *section ) {
        zconfig_t *zIP = zconfig_new("ipAddress", section);
        zconfig_set_value(zIP, "%s", ipAddress);
        
        zconfig_t *zPort = zconfig_new("port", section);
        zconfig_set_value(zPort, "%i", port);
        
        GNode::Serialize(section);
    }
    
    void HandleArgs( ImVector<char*> *args, ImVector<char*>::iterator it ) {
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
        GNode::HandleArgs(args, it);
    }
};

#endif /* TestNodes_h */
