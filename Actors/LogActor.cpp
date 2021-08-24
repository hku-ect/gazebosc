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

        zframe_t* frame = NULL;

        do {
            frame = zmsg_pop(ev->msg);
            if ( frame ) {
                char* msg;
                // parse zosc_t msg
                zosc_t * oscMsg = zosc_fromframe(frame);
                if ( oscMsg )
                    zosc_print(oscMsg);

                zosc_destroy(&oscMsg);
            }
        } while (frame != NULL );

        zframe_destroy(&frame);
        zmsg_destroy(&ev->msg);

        return NULL;
    }

    return ev->msg;
}
