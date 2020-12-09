#include "libsphactor.h"
#include "CountActor.h"

const char * countCapabilities = "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n"
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t *
Count::handleMsg( sphactor_event_t *ev ) {

    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(countCapabilities));
    }
    else if ( streq( ev->type, "DESTROY")) {
        delete this;
        zmsg_destroy(&ev->msg);
        return NULL;
    }
    else if ( streq(ev->type, "SOCK")) {
        this->msgCount++;
        zosc_t * msg = zosc_create("/report", "ssscsishsf",
                                                "Some Text", "12345678",
                                                "Some Char", 'q',
                                                "Small Int", (int32_t)this->msgCount,
                                                "Big Int", (int64_t)this->msgCount,
                                                "A float!", (float)0.23454 );

        sphactor_actor_set_custom_report_data( (sphactor_actor_t*)ev->actor, msg );
    }

    return ev->msg;
}
