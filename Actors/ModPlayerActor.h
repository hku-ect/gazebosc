#ifndef MODPLAYERACTOR_H
#define MODPLAYERACTOR_H

#include "libsphactor.hpp"
#include "SDL_audio.h"

extern "C" {
#include "hxcmod.h"
}

#define SAMPLERATE 48000
#define NBSTEREO16BITSAMPLES 16384

struct ModPlayerActor : Sphactor
{
    static const char *capabilities;

    ModPlayerActor() {}

    zmsg_t *
    handleInit(sphactor_event_t *ev);

    zmsg_t *
    handleAPI(sphactor_event_t *ev);

    zmsg_t *
    handleTimer(sphactor_event_t *ev);

    zmsg_t *
    handleStop(sphactor_event_t *ev);

    int queueAudio();
    void mixAudio(Uint8 *stream, int len);
    zmsg_t *getPatternEventMsg();

    uint32_t *buffer_dat;
    modcontext modctx;
    muchar orig_patterntable[128];
    int start_pos;
    int end_pos;
    int rowdelay;
    int prev_row = 0;
    zmsg_t *delayed_msgs[64] = { NULL };
    int delayed_msgs_idx = 0;
    unsigned char * modfile;
    tracker_buffer_state trackbuf_state1;
    SDL_AudioDeviceID audiodev = -1;
    bool playing = false;
};

#endif // MODPLAYERACTOR_H
