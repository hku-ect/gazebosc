//
// Created by Aaron Oostdijk on 13/10/2021.
//

#include "Midi2OSCActor.h"

const char * Midi2OSC::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"activePort\"\n"
        "        type = \"int\"\n"
        "        help = \"The port number of the MIDI receiver\"\n"
        "        value = \"0\"\n"
        "        min = \"0\"\n"
        "        max = \"10\"\n"
        "        api_call = \"SET PORT\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"timeout\"\n"
        "        type = \"int\"\n"
        "        help = \"Time to wait before getting new MIDI messages\"\n"
        "        value = \"16\"\n"
        "        min = \"1\"\n"
        "        max = \"10000\"\n"
        "        api_call = \"SET TIMEOUT\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"sendRaw\"\n"
        "        type = \"bool\"\n"
        "        help = \"Send the raw MIDI data through OSC or convert it to an interpreted OSC message\"\n"
        "        value = \"False\"\n"
        "        api_call = \"SET RAW\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

zmsg_t * Midi2OSC::handleInit( sphactor_event_t *ev )
{
    // RtMidiIn constructor
    try {
        midiin = new RtMidiIn();
    }
    catch ( RtMidiError &error ) {
        zsys_info("RTMIDI ERROR: %s", error.getMessage().c_str());
    }

    // Check input count
    unsigned int nPorts = midiin->getPortCount();
    zsys_info("There are %i MIDI input sources available.", nPorts);

    // Get ports, store in list
    std::string portName;
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            portName = midiin->getPortName(i);
            portList.push_back(portName);
        }
        catch ( RtMidiError &error ) {
            zsys_info("RTMIDI ERROR: %s", error.getMessage().c_str());
            break;
        }
    }

    if ( nPorts > 0 )
        midiin->openPort(0);

    return Sphactor::handleInit(ev);
}

zmsg_t * Midi2OSC::handleTimer( sphactor_event_t *ev ) {

    if ( midiin->isPortOpen() ) {
        zmsg_t * retMsg = zmsg_new();
        while ( midiin->getMessage(&messageBuffer) > 0. )
        {
            // message values
            int status = 0, channel = 0, pitch = 0, velocity = 0, control = 0, value = 0;

            // parse (taken from ofxMidi)
            if(messageBuffer[0] >= MIDI_SYSEX) {
                status = (MidiStatus) (messageBuffer[0] & 0xFF);
                channel = 0;
            } else {
                status = (MidiStatus) (messageBuffer[0] & 0xF0);
                channel = (int) (messageBuffer[0] & 0x0F)+1;
            }
            switch(status) {
                case MIDI_NOTE_ON:
                case MIDI_NOTE_OFF:
                    pitch = (int) messageBuffer[1];
                    velocity = (int) messageBuffer[2];
                    break;
                case MIDI_CONTROL_CHANGE:
                    control = (int) messageBuffer[1];
                    value = (int) messageBuffer[2];
                    break;
                case MIDI_PROGRAM_CHANGE:
                case MIDI_AFTERTOUCH:
                    value = (int) messageBuffer[1];
                    break;
                case MIDI_PITCH_BEND:
                    value = (int) (messageBuffer[2] << 7) + (int) messageBuffer[1]; // msb + lsb
                    break;
                case MIDI_POLY_AFTERTOUCH:
                    pitch = (int) messageBuffer[1];
                    value = (int) messageBuffer[2];
                    break;
                case MIDI_SONG_POS_POINTER:
                    value = (int) (messageBuffer[2] << 7) + (int) messageBuffer[1]; // msb + lsb
                    break;
                default:
                    break;
            }

            // zsys_info( "%i, %i, %i, %i, %i, %i", status, channel, pitch, velocity, control, value );

            std::string address;
            zosc_t * oscMsg = nullptr;

            // Put into OSC Message & return, taken from NatNet2OSCBridge
            if ( sendRaw )
            {
                address = "/" + portList[activePort];
                oscMsg = zosc_create(address.c_str(), "ccc", messageBuffer[0], messageBuffer[1], messageBuffer[2]);
            }
            else if (status < MIDI_SYSEX) {
                address = "/" + portList[activePort] + "/" + std::to_string(channel) + "/" +
                          getStatusString((MidiStatus) status);
                switch (status) {
                    case MIDI_NOTE_OFF:
                    case MIDI_NOTE_ON:
                        address.append("/" + std::to_string(pitch));
                        oscMsg = zosc_create(address.c_str(), "i", velocity);
                        break;
                    case MIDI_CONTROL_CHANGE:
                        address.append("/" + std::to_string(control));
                        oscMsg = zosc_create(address.c_str(), "i", value);
                        break;
                    case MIDI_PROGRAM_CHANGE:
                    case MIDI_AFTERTOUCH: // aka channel pressure
                        oscMsg = zosc_create(address.c_str(), "i", value);
                        break;
                    case MIDI_PITCH_BEND:
                        oscMsg = zosc_create(address.c_str(), "i", value);
                        break;
                    case MIDI_POLY_AFTERTOUCH: // aka key pressure
                        address.append("/" + std::to_string(pitch));
                        oscMsg = zosc_create(address.c_str(), "i", value);
                        break;
                    case MIDI_SONG_POS_POINTER:
                        oscMsg = zosc_create(address.c_str(), "i", value);
                        break;
                    default:
                        zsys_info("UNHANDLED MIDI STATUS: %i", status);
                        break;
                }
            }

            if ( oscMsg != nullptr ) {
                zmsg_add(retMsg, zosc_packx(&oscMsg));
            }
        }
        if ( zmsg_size(retMsg) )
            return retMsg;
    }

    zmsg_destroy(&ev->msg);
    return nullptr;
}

zmsg_t * Midi2OSC::handleStop( sphactor_event_t *ev ) {
    // Clean up RtMidi instance
    if ( midiin->isPortOpen() ) {
        midiin->closePort();
    }
    delete midiin;

    return Sphactor::handleStop(ev);
}

zmsg_t * Midi2OSC::handleAPI( sphactor_event_t *ev )
{
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        //TODO: Handle deviceIndex capability change and "listen" to that device (remove poller, add poller)

        if ( streq(cmd, "SET PORT") ) {
            zsys_info("SET PORT");
            char* ifChar = zmsg_popstr(ev->msg);
            int newPort = std::stoi(ifChar);
            zstr_free(&ifChar);

            if ( newPort < portList.size() ) {
                if ( activePort != newPort ) {
                    if ( midiin->isPortOpen()) {
                        midiin->closePort();
                    }
                    activePort = newPort;
                    midiin->openPort(activePort);
                }
            }
            else {
                zsys_info( "INVALID PORT NUMBER: %i", newPort);
            }
        }
        else if ( streq(cmd, "SET RAW") ) {
            char * value = zmsg_popstr(ev->msg);
            sendRaw = streq( value, "True");
        }

        zstr_free(&cmd);
    }
    return Sphactor::handleAPI(ev);
}
