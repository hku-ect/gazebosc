#ifndef MODPLAYERACTOR_H
#define MODPLAYERACTOR_H

#include "libsphactor.h"
#include "SDL2/SDL_audio.h"

extern "C" {
#include "hxcmod.h"
}

#define SAMPLERATE 48000
#define NBSTEREO16BITSAMPLES 16384

class ModPlayerActor
{
public:
    ModPlayerActor() {};

    zmsg_t *
    handleMsg( sphactor_event_t *event );

    int queueAudio();
    void mixAudio(Uint8 *stream, int len);

    uint32_t *buffer_dat;
    modcontext modctx;
    unsigned char * modfile;
    tracker_buffer_state trackbuf_state1;
    SDL_AudioDeviceID audiodev = -1;
    bool playing = false;
};

#endif // MODPLAYERACTOR_H
