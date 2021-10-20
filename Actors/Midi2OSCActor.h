//
// Created by Aaron Oostdijk on 13/10/2021.
//

#ifndef GAZEBOSC_MIDI2OSCACTOR_H
#define GAZEBOSC_MIDI2OSCACTOR_H

#include "libsphactor.hpp"
#include "../ext/rtmidi/RtMidi.h"

// MIDI status bytes
enum MidiStatus {
    MIDI_UNKNOWN            = 0x00,

    // channel voice messages
    MIDI_NOTE_OFF           = 0x80,
    MIDI_NOTE_ON            = 0x90,
    MIDI_CONTROL_CHANGE     = 0xB0,
    MIDI_PROGRAM_CHANGE     = 0xC0,
    MIDI_PITCH_BEND         = 0xE0,
    MIDI_AFTERTOUCH         = 0xD0, // aka channel pressure
    MIDI_POLY_AFTERTOUCH    = 0xA0, // aka key pressure

    // system messages
    MIDI_SYSEX              = 0xF0,
    MIDI_TIME_CODE          = 0xF1,
    MIDI_SONG_POS_POINTER   = 0xF2,
    MIDI_SONG_SELECT        = 0xF3,
    MIDI_TUNE_REQUEST       = 0xF6,
    MIDI_SYSEX_END          = 0xF7,
    MIDI_TIME_CLOCK         = 0xF8,
    MIDI_START              = 0xFA,
    MIDI_CONTINUE           = 0xFB,
    MIDI_STOP               = 0xFC,
    MIDI_ACTIVE_SENSING     = 0xFE,
    MIDI_SYSTEM_RESET       = 0xFF
};

// number range defines
// because it's sometimes hard to remember these  ...
#define MIDI_MIN_BEND       0
#define MIDI_MAX_BEND       16383

class Midi2OSC : public Sphactor {
private:
    RtMidiIn  *midiin = 0;
    int activePort = 0;
    bool sendRaw = false;

    std::vector<std::string> portList;
    std::vector<unsigned char> messageBuffer;

public:
    static const char *capabilities;

    Midi2OSC() : Sphactor() {

    }

    zmsg_t * handleInit( sphactor_event_t *ev );
    zmsg_t * handleStop( sphactor_event_t *ev );
    zmsg_t* handleTimer(sphactor_event_t *ev);
    zmsg_t * handleAPI( sphactor_event_t *ev );

    std::string getStatusString(MidiStatus status) {
        switch (status) {
            case MIDI_NOTE_OFF:
                return "Note_Off";
            case MIDI_NOTE_ON:
                return "Note_On";
            case MIDI_CONTROL_CHANGE:
                return "Control_Change";
            case MIDI_PROGRAM_CHANGE:
                return "Program_Change";
            case MIDI_PITCH_BEND:
                return "Pitch_Bend";
            case MIDI_AFTERTOUCH:
                return "Aftertouch";
            case MIDI_POLY_AFTERTOUCH:
                return "Poly_Aftertouch";
            case MIDI_SYSEX:
                return "Sysex";
            case MIDI_TIME_CODE:
                return "Time_Code";
            case MIDI_SONG_POS_POINTER:
                return "Song_Pos";
            case MIDI_SONG_SELECT:
                return "Song_Select";
            case MIDI_TUNE_REQUEST:
                return "Tune_Request";
            case MIDI_SYSEX_END:
                return "Sysex_End";
            case MIDI_TIME_CLOCK:
                return "Time_Clock";
            case MIDI_START:
                return "Start";
            case MIDI_CONTINUE:
                return "Continue";
            case MIDI_STOP:
                return "Stop";
            case MIDI_ACTIVE_SENSING:
                return "Active_Sensing";
            case MIDI_SYSTEM_RESET:
                return "System_Reset";
            default:
                return "Unknown";
        }
    }
};

#endif //GAZEBOSC_MIDI2OSCACTOR_H
