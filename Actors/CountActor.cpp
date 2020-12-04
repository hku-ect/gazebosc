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
    else if ( streq(ev->type, "SOCK")) {
        self->msgCount++;
        zsys_info("SOCK EVENT %i", self->msgCount);
        zosc_t * msg = zosc_create("/report", "ssscsishsf",
                                                "Some Text", "12345678",
                                                "Some Char", 'q',     //TODO: FIX
                                                "Small Int", (int32_t)self->msgCount,
                                                "Big Int", (int64_t)self->msgCount,
                                                "A float!", (float)0.23454 );

        sphactor_actor_set_custom_report_data( (sphactor_actor_t*)ev->actor, msg );
    }

    return ev->msg;
}
