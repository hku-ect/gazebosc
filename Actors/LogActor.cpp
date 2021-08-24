#include "libsphactor.h"

const char * logCapabilities = "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n";

zmsg_t * LogActor( sphactor_event_t *ev, void* args ) {
    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(logCapabilities));
    }
    else
    if ( streq(ev->type, "SOCK")) {
        if ( ev->msg == NULL ) return NULL;

        byte *msgBuffer;
        zframe_t* frame = NULL;

        do {
            if ( frame != NULL ) {
                zframe_destroy(&frame);
            }
            frame = zmsg_pop(ev->msg);
            if ( frame ) {
                //parse zosc_t msg
                msgBuffer = zframe_data(frame);
                size_t len = zframe_size(frame);

                char* msg;
                zosc_t * oscMsg = zosc_frommem( (char*)msgBuffer, len);
                if ( oscMsg ) {
                    const char* address = zosc_address(oscMsg);
                    printf("address: %s", address);

                    char type = '0';
                    const void *data = zosc_first(oscMsg, &type);
                    bool name = false;
                    while( data ) {
                        switch(type) {
                            case 's': {
                                char* value;
                                int rc = zosc_pop_string(oscMsg, &value);
                                if( rc == 0)
                                {
                                    printf(", %s", value);
                                    zstr_free(&value);
                                }
                            } break;
                            case 'c': {
                                char value;
                                zosc_pop_char(oscMsg, &value);
                                printf(", %c", value);
                            } break;
                            case 'i': {
                                int32_t value;
                                zosc_pop_int32(oscMsg, &value);
                                printf(", %i", value);
                            } break;
                            case 'h': {
                                int64_t value;
                                zosc_pop_int64(oscMsg, &value);
                                printf(", %lli", value);
                            } break;
                            case 'f': {
                                float value;
                                zosc_pop_float(oscMsg, &value);
                                printf(", %f", value);
                            } break;
                            case 'd': {
                                double value;
                                zosc_pop_double(oscMsg, &value);
                                printf(", %f", value);
                            } break;
                            case 'F': {
                                printf(", FALSE");
                            } break;
                            case 'T': {
                                printf(", TRUE");
                            } break;
                        }

                        data = zosc_next(oscMsg, &type);
                    }
                    printf("\n");
                }

                zosc_retr( oscMsg, "s", &msg );
                zsys_info("%s: %s", msgBuffer, msg);
                zstr_free(&msg);
                //zosc_destroy(&oscMsg); // TODO: leaks we need to fix zosc_frommem
            }
        } while (frame != NULL );

        zframe_destroy(&frame);
        zmsg_destroy(&ev->msg);

        return NULL;
    }

    return ev->msg;
}
