//
// Created by Aaron Oostdijk on 13/10/2021.
//

#ifndef GAZEBOSC_MIDI2OSCACTOR_H
#define GAZEBOSC_MIDI2OSCACTOR_H

#include "libsphactor.hpp"
#include "../ext/rtmidi/RtMidi.h"

class Midi2OSC : public Sphactor {
private:
    RtMidiIn  *midiin = 0;
    int activePort = 0;

public:
    Midi2OSC() : Sphactor() {

    }

    zmsg_t * handleInit( sphactor_event_t *ev );
    zmsg_t * handleStop( sphactor_event_t *ev );
    zmsg_t * handleCustomSocket( sphactor_event_t *ev );
    zmsg_t * handleAPI( sphactor_event_t *ev );
};

#endif //GAZEBOSC_MIDI2OSCACTOR_H
