//
// Created by Aaron Oostdijk on 13/10/2021.
//

#include "Midi2OSCActor.h"

const char * midi2OSCCapabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"deviceIndex\"\n"
        "        type = \"string\"\n"
        "        value = \"0\"\n"
        "        min = \"0\"\n"
        "        max = \"10\"\n"
        "        api_call = \"SET DEVICEINDEX\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"activePort\"\n"
        "        type = \"string\"\n"
        "        value = \"0\"\n"
        "        min = \"0\"\n"
        "        max = \"10\"\n"
        "        api_call = \"SET PORT\"\n"
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

    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    zsys_info("There are %i MIDI input sources available.", nPorts);
    std::string portName;
    for ( unsigned int i=0; i<nPorts; i++ ) {
        try {
            portName = midiin->getPortName(i);
        }
        catch ( RtMidiError &error ) {
            zsys_info("RTMIDI ERROR: %s", error.getMessage().c_str());
            break;
        }
        zsys_info("Input port %i: %s", i+1, portName.c_str());
    }

    //TODO: Get devices, store in list, alter max-capability for deviceIndex

    sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(midi2OSCCapabilities));
    return Sphactor::handleInit(ev);
}

zmsg_t * Midi2OSC::handleStop( sphactor_event_t *ev ) {
    return Sphactor::handleStop(ev);
}

zmsg_t * Midi2OSC::handleAPI( sphactor_event_t *ev )
{
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        //TODO: Handle deviceIndex capability change and "listen" to that device (remove poller, add poller)

        if ( streq(cmd, "SET PORT") ) {
            zsys_info("SET PORT");
            int newPort = 0;
            if ( activePort != newPort ) {
                if ( midiin->isPortOpen()) {
                    midiin->closePort();
                }

                activePort = newPort;

                midiin->openPort(activePort);
            }

        }


        zstr_free(&cmd);
    }
    return Sphactor::handleAPI(ev);
}

zmsg_t * Midi2OSC::handleCustomSocket( sphactor_event_t *ev )
{
    assert(ev->msg);
    zmsg_t *retmsg = NULL;


    zframe_t *frame = zmsg_pop(ev->msg);
    if (zframe_size(frame) == sizeof( void *) )
    {
        // TODO: Handle incoming midi, convert to OSC & return
        void *p = *(void **)zframe_data(frame);
        /*
        if ( zsock_is( p ) && p == this->dgramr )
        {

        }
        */
    }
    zframe_destroy(&frame);


    return retmsg;
}