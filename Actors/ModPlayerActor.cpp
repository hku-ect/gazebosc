#include "ModPlayerActor.h"
static SDL_AudioSpec fmt;

// callback audio
static void mixaudio(void *userdata, Uint8 *stream, int len)
{
    static_cast<ModPlayerActor *>(userdata)->mixAudio(stream, len);
}

const char * modplayercapabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"modfile\"\n"
        "        type = \"string\"\n"
        "        value = \"/home/arnaud/go.mod\"\n"
        "        api_call = \"SET MOD\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"playTrigger\"\n"
        "        type = \"bool\"\n"
        "        value = \"False\"\n"
        "        api_call = \"SET PLAY\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "outputs\n"
        "    output\n"
        //TODO: Perhaps add NatNet output type so we can filter the data multiple times...
        "        type = \"OSC\"\n";

void
ModPlayerActor::mixAudio(Uint8 *stream, int len)
{
    trackbuf_state1.nb_of_state = 0;
    hxcmod_fillbuffer(&modctx, (msample*)stream, len / 4, &trackbuf_state1);
}

int
ModPlayerActor::queueAudio()
{
    if ( this->modctx.mod_loaded == 1 )
    {
        trackbuf_state1.nb_of_state = 0;
        /* TODO: This doesn't work correctly yet. The idea is we calculate the time
         * it takes to process one row, this time is used for the actor timeout so
         * we have events on every row.
         * The formula is not easy as it's not very clear how tempo and speed is related
         * to the row time. In openMPT it's documented somewhat:
         * https://wiki.openmpt.org/Manual:_Song_Properties#Tempo_Mode
         * I've used this but it doens't work as expected. The queued buffer slowly
         * decreases so we're not submitting data fast enough?
         * Perhaps our calculation of the buffer is incorrect or the calculation
         * of the time on each row is incorrect.
         */

        // calculate the required buffer size
        // fileSize = (bitsPerSample * samplesPerSecond * channels * duration) / 8;
        // we double the buffer size to prevent underrun
        // see MPT documentation about timing, that's where the 2500ms comes from
        int duration = (2500/this->modctx.bpm)*this->modctx.song.speed;
        unsigned int buffersize = (16*48000*2*duration)/1000/8;
        unsigned int maxbuffer = buffersize * 4;
        // always try to fill the buffer but not more
        unsigned int qs = SDL_GetQueuedAudioSize(audiodev);
        if (qs <= 0)
        {
            zsys_warning("Buffer underrun (or just starting)!");
            // fill the buffer to the max
            buffersize = maxbuffer;
        }
        //zsys_info("bs %i, qs %i, mb %i",buffersize,qs, maxbuffer);
        else if ( qs > maxbuffer )
        {
            zsys_warning("Buffer overrun!");
            buffersize = 4;
        }
        else if ( qs < maxbuffer )
        {
            unsigned int fillsize = maxbuffer - qs;
            if ( buffersize > fillsize )
                buffersize = fillsize;
        }
        zsys_info("fill len %i, qs %i", buffersize, qs );
        msample stream[buffersize];
        hxcmod_fillbuffer(&modctx, (msample*)stream, buffersize / 4, &trackbuf_state1);
        SDL_QueueAudio(audiodev, stream, buffersize);
    }
}

zmsg_t *
ModPlayerActor::handleMsg( sphactor_event_t *event )
{
    if ( streq(event->type, "INIT") )
    {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)event->actor, zconfig_str_load(modplayercapabilities));
        sphactor_actor_set_timeout( (sphactor_actor_t*)event->actor, (2500/144)*31);
    }
    else if ( streq(event->type, "API") )
    {
        char *cmd = zmsg_popstr(event->msg);
        if ( streq(cmd, "SET MOD") )
        {
            char *file = zmsg_popstr(event->msg);
            if (strlen(file) )
            {
                FILE *f;
                int filesize = 0;
                f = fopen(file,"rb");
                if ( f == NULL)
                    return NULL;
                fseek(f,0,SEEK_END);
                filesize = ftell(f);
                fseek(f,0,SEEK_SET);
                unsigned char *mod = (unsigned char *)malloc(filesize);
                fread(mod,filesize,1,f);
                hxcmod_init(&this->modctx);
                hxcmod_setcfg(&this->modctx, SAMPLERATE, 0, 0);
                hxcmod_load(&this->modctx,(void*)mod,filesize);

                fclose(f);
                zosc_t *msg = zosc_create("/loaded", "sssisisi",
                                          "title", this->modctx.song.title,
                                          "length", int(this->modctx.song.length),
                                          "speed", int(this->modctx.song.speed),
                                          "bpm", int(this->modctx.bpm));
                sphactor_actor_set_custom_report_data((sphactor_actor_t*)event->actor, msg);
                sphactor_actor_set_timeout( (sphactor_actor_t*)event->actor, (2500/this->modctx.bpm)*this->modctx.song.speed);
            }
            else
            {
                zosc_t *msg = zosc_create("/report", "ss", "error loading file:", file);
                sphactor_actor_set_custom_report_data((sphactor_actor_t*)event->actor, msg);
            }
            zstr_free(&file);

            if (audiodev == -1 )
            {

                fmt.freq = 48000;
                fmt.format = AUDIO_S16;
                fmt.channels = 2;
                fmt.samples = NBSTEREO16BITSAMPLES/4;
                fmt.callback = NULL;
                fmt.userdata = NULL;

                memset(&this->trackbuf_state1,0,sizeof(tracker_buffer_state));
                trackbuf_state1.nb_max_of_state = 100;
                trackbuf_state1.track_state_buf = (tracker_state *)malloc(sizeof(tracker_state) * trackbuf_state1.nb_max_of_state);
                memset(trackbuf_state1.track_state_buf,0,sizeof(tracker_state) * trackbuf_state1.nb_max_of_state);
                trackbuf_state1.sample_step = ( NBSTEREO16BITSAMPLES ) / trackbuf_state1.nb_max_of_state;
                audiodev = SDL_OpenAudioDevice(NULL, 0, &fmt, NULL, 0);
                if ( audiodev < 1 )
                {
                    zsys_error("Cannot open Audio: %s\n", SDL_GetError());
                }
            }
        }
        else if ( streq(cmd, "SET PLAY") )
        {
            char *val = zmsg_popstr(event->msg);
            if ( streq( val, "True" ) )
            {
                this->playing = true;
            }
            else
                this->playing = false;
        }
        zmsg_destroy(&event->msg);
        return NULL;
    }
    else if ( streq( event->type, "TIME"))
    {
        if ( this->modctx.mod_loaded == 1 && this->playing )
        {
            queueAudio();
            SDL_PauseAudioDevice(audiodev, 0);
            zosc_t *msg = zosc_create("/loaded", "sssisisi",
                                      "title", this->modctx.song.title,
                                      "length", int(this->modctx.song.length),
                                      "speed", int(this->modctx.song.speed),
                                      "bpm", int(this->modctx.bpm));
            sphactor_actor_set_custom_report_data((sphactor_actor_t*)event->actor, msg);
            // determine next timeout based on speed and bpm of the song! 2500ms/bpm*speed
            sphactor_actor_set_timeout( (sphactor_actor_t*)event->actor, (2500/this->modctx.bpm)*this->modctx.song.speed);
        }
    }
    else if ( streq( event->type, "STOP"))
    {
        SDL_CloseAudio();
        zmsg_destroy(&event->msg);
        return NULL;
    }
    return nullptr;
}
