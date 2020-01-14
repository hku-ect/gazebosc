//
//  MidiActor.cpp
//  gazebosc
//
//  Created by aaronvark on 22/10/2019.
//

#include "DefaultActors.h"

MidiActor::MidiActor( const char* uuid ) : GActor( "Midi",
                                { },    //no input slots
                                {
                                    {"OSC", ActorSlotOSC}  // Output slots
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
}
    
MidiActor::~MidiActor() {
    
    if ( midiin != NULL )
    {
        midiin->closePort();
        delete midiin;
    }
    
    delete[] address;
    delete[] msgBuffer;
    delete[] activePort;
}

void MidiActor::CreateActor() {
    GActor::CreateActor();
    SetRate(60);
}

void  MidiActor::Connect() {
    if ( midiin->isPortOpen()) {
        midiin->closePort();
    }
    
    if ( nPorts > midiPort && midiPort != -1 ) {
        midiin->openPort(midiPort);
        sprintf(activePort, "%s", portNames.at(midiPort).c_str());
        printf("Connected to %s \n", activePort);
    }
    else {
        activePort = nullptr;
    }
}

zmsg_t * MidiActor::ActorCallback()
{
    static double stamp;
    static int nBytes;
    static std::vector<unsigned char> message;
    
    if ( midiPort != -1 && midiin != NULL && midiin->isPortOpen() ) {
        zmsg_t *msg = nullptr;
        
        do {
            stamp = midiin->getMessage( &message );
            nBytes = message.size();
            
            if ( nBytes > 0 ) {
                //TODO: this assumes the message is always 3 bytes...
                if ( nBytes != 3 ) {
                    printf("NBYTES NOT 3: %i \n", nBytes );
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

void MidiActor::Render(float deltaTime) {
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
    
    GActor::Render(deltaTime);
}

void MidiActor::SerializeActorData( zconfig_t *section ) {
    zconfig_t *device = zconfig_new("device", section);
    if ( midiPort != -1 ) {
        zconfig_set_value(device, "%s", portNames.at(midiPort).c_str());
    }
    else {
        zconfig_set_value(device, "(null)");
    }
    GActor::SerializeActorData(section);
}

void MidiActor::DeserializeActorData( ImVector<char*> *args, ImVector<char*>::iterator it ) {
    //pop args from front
    char* strPort = *it;
    it++;
    
    // Find strPort in portNames, connect & update UI
    int i = 0;
    bool found = false;
    for( auto it = portNames.begin(); it != portNames.end(); it++) {
        if ( streq( it->c_str(), strPort ) ) {
            midiPort = i;
            found = true;
            current_item = it->c_str();
            Connect();
            break;
        }
        i++;
    }
    
    if ( !found ) {
        // TODO: Display error messages...
        printf("Device not found: %s \n", strPort);
    }
    
    free(strPort);
    
    //send remaining args (probably just xpos/ypos) to base
    GActor::DeserializeActorData(args, it);
}
