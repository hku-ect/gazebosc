//
// Created by Aaron Oostdijk on 28/10/2021.
//

#include "OSCRecordActor.h"

const char * OSCRecord::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"fileName\"\n"
        "        type = \"filename\"\n"
        "        value = \"\"\n"
        "        api_call = \"SET FILE\"\n"
        "        api_value = \"s\"\n"           // optional picture format used in zsock_send
        "        options = \"rw\"\n"
        "    data\n"
        "        name = \"Record\"\n"
        "        type = \"trigger\"\n"
        "        api_call = \"START_RECORD\"\n"
        "    data\n"
        "        name = \"Stop\"\n"
        "        type = \"trigger\"\n"
        "        api_call = \"STOP_RECORD\"\n"
        "        value = \"\"\n"
        "    data\n"
        "        name = \"Play\"\n"
        "        type = \"trigger\"\n"
        "        api_call = \"PLAY_RECORDING\"\n"
        "        value = \"\"\n"
        "    data\n"
        "        name = \"loop\"\n"
        "        type = \"bool\"\n"
        "        api_call = \"SET LOOPING\"\n"
        "        value = \"False\"\n"
        "        api_value = \"s\"\n"
        "    data\n"
        "        name = \"blockDuringPlay\"\n"
        "        type = \"bool\"\n"
        "        api_call = \"SET BLOCKING\"\n"
        "        value = \"False\"\n"
        "        api_value = \"s\"\n"
        "inputs\n"
        "    input\n"
        "        type = \"OSC\"\n"
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";


void OSCRecord::handleEOF(sphactor_actor_t* actor) {
    if ( loop ) {
        // reset playback values
        read_offset = 0;
        startTimeCode = zclock_mono();

        // calculate timeout for next read
        zchunk_t * chunk = zfile_read(file, sizeof(time_bytes), read_offset);
        read_offset += sizeof(time_bytes);

        current_tc = (time_bytes*)zchunk_data(chunk);
        startRecordTimeCode = current_tc->timeCode;

        // handle this first message immediately
        sphactor_actor_set_timeout(actor, 1);
    }
    else {
        playing = false;
        zfile_close(file);
        file = nullptr;
        sphactor_actor_set_timeout(actor, -1);
    }
}

void OSCRecord::setReport(sphactor_actor_t* actor) {
    // Build report
    std::string time_display;
    time_display += zsys_sprintf("%i", read_offset);
    time_display += " / ";
    time_display += zsys_sprintf("%i", zfile_cursize(file));
    zosc_t * msg = zosc_create("/report", "ss", "Playing", time_display.c_str());

    sphactor_actor_set_custom_report_data(actor, msg);
}

zmsg_t *
OSCRecord::handleTimer( sphactor_event_t *ev ) {
    zmsg_destroy(&ev->msg);

    // Read data from current message
    zchunk_t * dataChunk = zfile_read(file, current_tc->bytes, read_offset);
    read_offset += current_tc->bytes;

    // Create return message, add data (we assume it's there)
    zmsg_t * retMsg = zmsg_new();
    zframe_t * frame = zchunk_packx(&dataChunk);
    zmsg_append( retMsg, &frame);

    // Read next timeCode chunk
    zchunk_t * chunk = zfile_read(file, sizeof(time_bytes), read_offset);
    read_offset += sizeof(time_bytes);

    if ( chunk == nullptr || zfile_eof(file) ) {
        handleEOF((sphactor_actor_t*)ev->actor);
        sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, nullptr);
        return retMsg;
    }
    else {
        current_tc = (time_bytes*)zchunk_data(chunk);

        //calculate next timeout
        unsigned int cur_delta = zclock_mono() - startTimeCode;
        int next_timeout = ( current_tc->timeCode - startRecordTimeCode ) - cur_delta;
        while( next_timeout <= 0 ) {
            // read bytes from current message
            zchunk_t * dataChunk = zfile_read(file, current_tc->bytes, read_offset);
            read_offset += current_tc->bytes;

            // append to msg as frame
            zframe_t * frame = zchunk_packx(&dataChunk);
            zmsg_append( retMsg, &frame);

            // read next timeCode chunk
            zchunk_t * chunk = zfile_read(file, sizeof(time_bytes), read_offset);
            read_offset += sizeof(time_bytes);

            if ( chunk == nullptr || zfile_eof(file) ) {
                handleEOF((sphactor_actor_t*)ev->actor);
                sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, nullptr);
                return retMsg;
            }
            else
            {
                current_tc = (time_bytes*)zchunk_data(chunk);

                //calculate next timeout
                cur_delta = zclock_mono() - startTimeCode;
                next_timeout = ( current_tc->timeCode - startRecordTimeCode ) - cur_delta;
            }
        }
        sphactor_actor_set_timeout((sphactor_actor_t*)ev->actor, next_timeout);
        setReport((sphactor_actor_t*)ev->actor);
        return retMsg;
    }

    return nullptr;
}

zmsg_t *
OSCRecord::handleAPI( sphactor_event_t *ev )
{
    //pop msg for command
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        if ( streq(cmd, "START_RECORD") ) {
            zsys_info("TODO: start recording if valid file & not recording");

            // if ! recording

            // if file does not exist
            // TODO: OR overwrite (for now, always overwrite)
            if ( file == nullptr ) {
                if ( !zfile_exists(fileName) ) {
                    file = zfile_new(NULL, fileName);
                    zfile_output(file);
                    offset = 0;
                    zsys_info("file created");

                    // Build report
                    zosc_t * msg = zosc_create("/report", "ss", "Recording", "...");
                    sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);
                }
                else {
                    // TODO: how do we deal with an existing file?
                    zsys_info("file exists");
                }
            }
            else {
                zsys_info("already recording");
            }
        }
        else if ( streq(cmd, "STOP_RECORD") ) {
            if ( file ) {
                zsys_info("closing file");
                zfile_close(file);

                sphactor_actor_set_timeout((sphactor_actor_t*)ev->actor, -1);
                sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, nullptr);

                file = nullptr;
                playing = false;
            }
        }
        else if ( streq(cmd, "PLAY_RECORDING") ) {
            if ( file == nullptr && !playing ) {
                if ( zfile_exists(fileName) ) {
                    file = zfile_new(NULL, fileName);
                    zfile_input(file);
                    playing = true;
                    startTimeCode = zclock_mono();
                    read_offset = 0;

                    // read the next message and store its future timecode
                    zchunk_t * chunk = zfile_read(file, sizeof(time_bytes), read_offset);
                    read_offset += sizeof(time_bytes);

                    if ( chunk == nullptr || zfile_eof(file) ) {
                        playing = false;
                        zfile_close(file);
                        file = nullptr;
                    }
                    else {
                        // calculate timeout for next read
                        current_tc = (time_bytes*)zchunk_data(chunk);
                        startRecordTimeCode = current_tc->timeCode;

                        sphactor_actor_set_timeout((sphactor_actor_t*)ev->actor, 1);
                    }
                }
                else {
                    zsys_info("Invalid file path/name");
                }
            }
        }
        else if ( streq(cmd, "SET FILE") ) {
            fileName = zmsg_popstr(ev->msg);
            zsys_info("GOT FILE: %s", fileName);
        }
        else if ( streq(cmd, "SET LOOPING") ) {
            loop = streq( zmsg_popstr(ev->msg), "True" );
        }
        else if ( streq(cmd, "SET BLOCKING") ) {
            blockDuringPlay = streq( zmsg_popstr(ev->msg), "True" );
        }
    }

    zmsg_destroy(&ev->msg);
    return nullptr;
}

zmsg_t *
OSCRecord::handleSocket( sphactor_event_t *ev )
{
    if ( file && !playing ) {
        zframe_t * frame = zmsg_pop(ev->msg);
        const char * retChar = "\n";

        zmsg_t * passthrough = zmsg_new();


        zchunk_t *endLineChunk = zchunk_new(retChar, 1);

        while ( frame ) {
            zchunk_t *chunk = zchunk_new(zframe_data(frame), zframe_size(frame));

            time_bytes * tc = new time_bytes();
            tc->timeCode = zclock_mono();
            tc->bytes = zchunk_size(chunk);
            zchunk_t *headerChunk = zchunk_new(tc, sizeof(time_bytes));

            int rc = zfile_write(file, headerChunk, offset);
            offset += zchunk_size(headerChunk);
            rc = zfile_write(file, chunk, offset);
            offset += zchunk_size(chunk);

            if ( rc != 0 ) {
                zsys_info("error writing");
            }

            free(tc);
            zchunk_destroy(&chunk);
            zchunk_destroy(&headerChunk);

            zmsg_append( passthrough, &frame );

            frame = zmsg_pop(ev->msg);
        }

        zchunk_destroy(&endLineChunk);

        return passthrough;
    }

    // Passthrough if no file
    if ( !playing || !blockDuringPlay ) {
        return ev->msg;
    }
    else {
        zmsg_destroy(&ev->msg);
        return nullptr;
    }
}