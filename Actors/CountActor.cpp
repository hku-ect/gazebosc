#include "CountActor.h"

const char * countCapabilities = "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n"
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t *
Count::handleInit( sphactor_event_t *ev ) {
    sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(countCapabilities));
    return Sphactor::handleInit(ev);
}

zmsg_t *
Count::handleSocket( sphactor_event_t *ev ) {
    this->msgCount++;
    zosc_t * msg = zosc_create("/report", "si",
                               "counter", (int32_t)this->msgCount);

    sphactor_actor_set_custom_report_data( (sphactor_actor_t*)ev->actor, msg );

    // forward event to pub
    return ev->msg;
}
