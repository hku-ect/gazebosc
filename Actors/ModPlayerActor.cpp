#include "ModPlayerActor.h"
static SDL_AudioSpec fmt;

// callback audio
static void mixaudio(void *userdata, Uint8 *stream, int len)
{
    static_cast<ModPlayerActor *>(userdata)->mixAudio(stream, len);
}

const char * ModPlayerActor::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"modfile\"\n"
        "        type = \"filename\"\n"
        "        valid_files = \".mod\"\n"
        "        value = \"\"\n"
        "        api_call = \"SET MOD\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"playback\"\n"
        "        type = \"mediacontrol\"\n"
        "    data\n"
        "        name = \"start_position\"\n"
        "        type = \"int\"\n"
        "        value = \"0\"\n"
        "        min = \"0\"\n"
        "        max = \"127\"\n"
        "        step = \"1\"\n"
        "        api_call = \"SET START\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"end_position\"\n"
        "        type = \"int\"\n"
        "        value = \"127\"\n"
        "        min = \"0\"\n"
        "        max = \"127\"\n"
        "        step = \"1\"\n"
        "        api_call = \"SET END\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"row_delay\"\n"
        "        type = \"int\"\n"
        "        value = \"0\"\n"
        "        min = \"0\"\n"
        "        max = \"63\"\n"
        "        step = \"1\"\n"
        "        api_call = \"SET ROWDELAY\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
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
        /* TODO: This might need tweaking. The idea is we calculate the time
         * it takes to process one row, this time is used for the actor timeout so
         * we have events on every row. See handle_timer function
         * The formula is not easy as it's not very clear how tempo and speed is related
         * to the row time. In openMPT it's documented somewhat:
         * https://wiki.openmpt.org/Manual:_Song_Properties#Tempo_Mode
         * I've used this but it doens't work as expected. The queued buffer slowly
         * decreases so we're not submitting data fast enough?
         * Perhaps our calculation of the buffer is incorrect or the calculation
         * of the time on each row is incorrect.
         */

        // calculate the required buffer size for a single row
        // fileSize = (bitsPerSample * samplesPerSecond * channels * duration) / 8;
        // see MPT documentation about timing, that's where the 2500ms comes from
        int duration = (2500/this->modctx.bpm)*this->modctx.song.speed;
        unsigned int buffersize = (16*48000*2*duration)/1000/8;
#ifdef _MSC_VER
        msample *stream = (msample *)_malloca( buffersize * sizeof(msample) );
        hxcmod_fillbuffer(&modctx, stream, buffersize / 4, &trackbuf_state1);
        int rc = SDL_QueueAudio(audiodev, stream, buffersize);
        if (rc < 0 )
            zsys_error("SDL_QueueAudio failed: %s\n", SDL_GetError());
        _freea(stream);
#else
        msample stream[buffersize];
        hxcmod_fillbuffer(&modctx, (msample*)stream, buffersize / 4, &trackbuf_state1);
        int rc = SDL_QueueAudio(audiodev, stream, buffersize);
        if (rc < 0 )
            zsys_error("SDL_QueueAudio failed: %s\n", SDL_GetError());
#endif
        return SDL_GetQueuedAudioSize(audiodev);
    }
    return -1;
}

const char *HEXCHAR = "0123456789ABCDEF";
#define MOD(a,b) ((((a)%(b))+(b))%(b))

zmsg_t *
ModPlayerActor::getPatternEventMsg()
{
    tracker_state *state = this->trackbuf_state1.track_state_buf;
    if (state->cur_pattern_pos == prev_row )
        return NULL;

    if (state->cur_pattern_pos - prev_row > 1 ) zsys_warning("OMG we're skipping rows %i to %i", prev_row, state->cur_pattern_pos);

    note *cur_note = modctx.patterndata[state->cur_pattern] + (state->cur_pattern_pos * 4);

    // acquire values, see http://www.aes.id.au/modformat.html for details
    muint sample = (cur_note->sampperiod & 0xF0) | (cur_note->sampeffect >> 4);
    muint period = ((cur_note->sampperiod & 0xF) << 8) | cur_note->period;
    muint effect = ((cur_note->sampeffect & 0xF) << 8) | cur_note->effect;
    muchar effect_op = cur_note->sampeffect & 0xF;
    muchar effect_param = cur_note->effect;
    char effect_param_hex[3];
    sprintf(effect_param_hex, "%02X", effect_param);
    //muchar effect_param_l = effect_param & 0x0F;
    //muchar effect_param_h = effect_param >> 4;

    zosc_t *oscm = zosc_create("/patternevent", "iiiiics",
                                      state->cur_pattern_table_pos, // song pos
                                      state->cur_pattern,           // pattern nr
                                      state->cur_pattern_pos,       // pattern row
                                      period,   // note
                                      sample,
                                      HEXCHAR[effect_op],
                                      effect_param_hex
                                     );
    assert(oscm);
    //append rest of the tracks
    for (int i=1;i<state->number_of_tracks;i++)
    {
        cur_note+=1; // increment to next channel
        // acquire our values
        sample = (cur_note->sampperiod & 0xF0) | (cur_note->sampeffect >> 4);
        period = ((cur_note->sampperiod & 0xF) << 8) | cur_note->period;
        effect = ((cur_note->sampeffect & 0xF) << 8) | cur_note->effect;
        effect_op = cur_note->sampeffect & 0xF;
        effect_param = cur_note->effect;
        sprintf(effect_param_hex, "%02X", effect_param);

        zosc_append(oscm, "iics",
                        period,   // note
                        sample,
                        HEXCHAR[effect_op],
                        effect_param_hex
                    );
    }
    zmsg_t *ret = zmsg_new();
    zframe_t *data = zosc_packx(&oscm);
    zmsg_append(ret, &data);
    prev_row = state->cur_pattern_pos;
    return ret;
}


zmsg_t *
ModPlayerActor::handleInit( sphactor_event_t *event )
{
    //init capabilities
    sphactor_actor_set_timeout( (sphactor_actor_t*)event->actor, (2500/144)*31);
    // init hxcmod player
    hxcmod_init(&this->modctx);
    memset(this->orig_patterntable, 0, 128);

    if ( event->msg ) zmsg_destroy(&event->msg);
    return nullptr;
}

zmsg_t *
ModPlayerActor::handleAPI(sphactor_event_t *event)
{
    char *cmd = zmsg_popstr(event->msg);
    if ( streq(cmd, "SET MOD") )
    {
        char *file = zmsg_popstr(event->msg);
        if (strlen(file) )
        {
            if ( this->modctx.mod_loaded == 1 )
            {
                hxcmod_unload( &this->modctx );
                // free modfile
                free( this->modfile );
                memset(this->orig_patterntable, 0, 128);
                if (audiodev != -1)
                    SDL_ClearQueuedAudio(audiodev);
            }

            FILE *f;
            int filesize = 0;
            f = fopen(file,"rb");
            if ( f == NULL)
                return NULL;
            fseek(f,0,SEEK_END);
            filesize = ftell(f);
            fseek(f,0,SEEK_SET);
            this->modfile = (unsigned char *)malloc(filesize);
            fread(this->modfile,filesize,1,f);
            hxcmod_setcfg(&this->modctx, SAMPLERATE, 1, 1);
            hxcmod_load(&this->modctx,(void*)this->modfile,filesize);
            fclose(f);

            if ( this->modctx.mod_loaded == 1 )
            {
                memcpy(orig_patterntable, modctx.song.patterntable, 128); // backup the patterntable
                zosc_t *msg = zosc_create("/loaded", "sssisisi",
                                          "title", this->modctx.song.title,
                                          "length", int(this->modctx.song.length),
                                          "speed", int(this->modctx.song.speed),
                                          "bpm", int(this->modctx.bpm));
                sphactor_actor_set_custom_report_data((sphactor_actor_t*)event->actor, msg);
                sphactor_actor_set_timeout( (sphactor_actor_t*)event->actor, (2500/this->modctx.bpm)*this->modctx.song.speed);

                if (audiodev == -1 ) // init audio
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
            else
            {
                hxcmod_unload(&this->modctx);
                free(this->modfile);
                zosc_t *msg = zosc_create("/report", "ss", "error loading file:", file);
                sphactor_actor_set_custom_report_data((sphactor_actor_t*)event->actor, msg);
            }
        }
        zstr_free(&file);
    }
    else if ( streq(cmd, "PLAY") )
    {
        if ( this->modctx.mod_loaded == 1)
            this->playing = true;
    }
    else if ( streq(cmd, "PAUSE") )
        this->playing = false;
    else if ( streq(cmd, "BACK") )
    {
        this->modctx.tablepos = 0;
        this->modctx.patternpos = 0;
        if (audiodev != -1)
            SDL_ClearQueuedAudio(audiodev);
    }
    else if ( streq(cmd, "SET START") )
    {
        zframe_t *f = zmsg_pop(event->msg);
        assert(f);
        char *astart = (char *)zframe_data(f);
        start_pos = atoi(astart);
        assert(start_pos < 129);
        if (start_pos > end_pos )
        {
            zsys_error("End position %i is before start position %i", end_pos, start_pos);
        }
        else if (modctx.mod_loaded )
        {
            memset(modctx.song.patterntable, 0, 128);
            memcpy(modctx.song.patterntable, orig_patterntable+start_pos,end_pos-start_pos+1);
            this->modctx.tablepos = 0;
            this->modctx.song.length = end_pos - start_pos + 1;
        }
        zframe_destroy(&f);
    }
    else if ( streq(cmd, "SET END") )
    {
        zframe_t *f = zmsg_pop(event->msg);
        assert(f);
        char *aend = (char *)zframe_data(f);
        end_pos = atoi(aend);
        assert(end_pos < 129);
        if ( end_pos < start_pos)
        {
            zsys_error("End position %i is before start position %i", end_pos, start_pos);
        }
        else if (modctx.mod_loaded )
        {
            memset(modctx.song.patterntable, 0, 128);
            memcpy(modctx.song.patterntable, orig_patterntable+start_pos,end_pos-start_pos+1);
            this->modctx.tablepos = 0;
            this->modctx.patternpos = 0;
            this->modctx.song.length = end_pos - start_pos + 1;
        }
        zframe_destroy(&f);
    }
    else if ( streq(cmd, "SET ROWDELAY") )
    {
        zframe_t *f = zmsg_pop(event->msg);
        assert(f);
        char *rowd = (char *)zframe_data(f);
        this->rowdelay = atoi(rowd);
        assert(rowdelay < 64);
        zframe_destroy(&f);
    }

    if ( cmd )
        zstr_free(&cmd);

    zmsg_destroy(&event->msg);
    return NULL;
}

zmsg_t *
ModPlayerActor::handleTimer(sphactor_event_t *event)
{
    if ( this->modctx.mod_loaded == 1 && this->playing )
    {
        queueAudio();
        SDL_PauseAudioDevice(audiodev, 0);
        zosc_t *msg = zosc_create("/loaded", "sssisisisisi",
                                  "title", this->modctx.song.title,
                                  "length", int(this->modctx.song.length),
                                  "speed", int(this->modctx.song.speed),
                                  "bpm", int(this->modctx.bpm),
                                  "position", this->modctx.tablepos + start_pos,
                                  "pattern", this->modctx.song.patterntable[this->modctx.tablepos] );
        sphactor_actor_set_custom_report_data((sphactor_actor_t*)event->actor, msg);
        // determine next timeout based on speed and bpm of the song! 2500ms/bpm*speed
        int duration = (2500/this->modctx.bpm)*this->modctx.song.speed;
        unsigned int buffersize = (16*48000*2*duration)/1000/8 *2;
        unsigned int qs = SDL_GetQueuedAudioSize(audiodev);
        float fillpct = qs/float(buffersize);
        if (fillpct < 1.0f && fillpct > 0.1f)
        {
            zsys_warning("buffersize: %i, qs: %i, fill %f", buffersize, qs, fillpct);
            duration = fillpct * duration;
        }
        else if (fillpct > 2.0f )
        {
            zsys_error("Audio queue is too large, this should not happen");
        }
        sphactor_actor_set_timeout( (sphactor_actor_t*)event->actor, duration);
        // we save messages in a circular buffer to provide row delaying
        delayed_msgs[delayed_msgs_idx] = getPatternEventMsg();
        int sendidx = MOD(delayed_msgs_idx - rowdelay, rowdelay+1);
        zmsg_t *ret = delayed_msgs[sendidx];
        delayed_msgs[sendidx] = NULL;
        delayed_msgs_idx = sendidx;
        return ret;
    }
    if ( event->msg ) zmsg_destroy(&event->msg);
    return nullptr;
}

zmsg_t *
ModPlayerActor::handleStop(sphactor_event_t *event)
{
    if ( this->modctx.mod_loaded == 1 )
        free( this->modfile ); // free modfile

    hxcmod_unload(&this->modctx);
    if (audiodev != -1 )
        free( trackbuf_state1.track_state_buf ); // free tracker state buf

    SDL_CloseAudio();
    if ( event->msg ) zmsg_destroy(&event->msg);
    return NULL;
}
