//
// Created by Aaron Oostdijk on 28/10/2021.
//

#ifndef GAZEBOSC_RECORDACTOR_H
#define GAZEBOSC_RECORDACTOR_H

#include "libsphactor.hpp"
#include <string>

struct time_bytes {
public:
    unsigned int timeCode;
    unsigned int bytes;
};

class Record : public Sphactor {
private:

public:
    static const char *capabilities;

    // State variables
    zfile_t * file = nullptr;
    bool playing = false;
    unsigned int startTimeCode = 0;
    unsigned int startRecordTimeCode = 0;
    unsigned int read_offset = 0;
    unsigned int write_offset = 0;
    time_bytes * current_tc = nullptr;

    // Controls
    const char* fileName = nullptr;
    bool loop = false;
    bool blockDuringPlay = false;
    bool overwrite = false;

    Record() : Sphactor() {

    }

    bool isAbsolutePath(const char* path) {
#if WIN32
        return (path[1] == ':' && ( path[2] == '\\' || path[2] == '/' ));
#else
        return path[0] == '/';
#endif
    }

    void handleEOF( sphactor_actor_t * actor );
    void setReport( sphactor_actor_t * actor );

    zmsg_t * handleTimer( sphactor_event_t *ev );
    zmsg_t * handleSocket( sphactor_event_t *ev );
    zmsg_t * handleAPI( sphactor_event_t *ev );

    // Or perhaps a fixed buffer, that writes when something is bigger than it can store?
};

#endif //GAZEBOSC_RECORDACTOR_H
