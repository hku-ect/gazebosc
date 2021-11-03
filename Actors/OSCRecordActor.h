//
// Created by Aaron Oostdijk on 28/10/2021.
//

#ifndef GAZEBOSC_OSCRECORDACTOR_H
#define GAZEBOSC_OSCRECORDACTOR_H

#include "libsphactor.hpp"
#include <string>

struct time_bytes {
public:
    unsigned int timeCode;
    unsigned int bytes;
};

class OSCRecord : public Sphactor {
private:

public:
    static const char *capabilities;

    const char* fileName = nullptr;
    zfile_t * file = nullptr;
    int offset = 0;
    bool playing = false;
    bool loop = false;
    bool blockDuringPlay = false;

    unsigned int startTimeCode = 0;
    unsigned int startRecordTimeCode = 0;
    unsigned int read_offset = 0;
    time_bytes * current_tc = nullptr;

    OSCRecord() : Sphactor() {

    }

    void handleEOF( sphactor_actor_t * actor );
    void setReport( sphactor_actor_t * actor );

    zmsg_t * handleTimer( sphactor_event_t *ev );
    zmsg_t * handleSocket( sphactor_event_t *ev );
    zmsg_t * handleAPI( sphactor_event_t *ev );

    // Or perhaps a fixed buffer, that writes when something is bigger than it can store?
};

#endif //GAZEBOSC_OSCRECORDACTOR_H
