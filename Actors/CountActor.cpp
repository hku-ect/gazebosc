#include "libsphactor.h"
#include "CountActor.h"

const char * countCapabilities = "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n"
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t * CountActor( sphactor_event_t *ev, void* args ) {
    Count* self = (Count*) args;

    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(countCapabilities));
    }
    if ( streq(ev->type, "SOCK")) {
        self->msgCount++;
        zosc_t * msg = zosc_create("/report", "si", "Message Count", self->msgCount );
        sphactor_actor_set_custom_report_data( (sphactor_actor_t*)ev->actor, msg );
    }

    return ev->msg;
}
