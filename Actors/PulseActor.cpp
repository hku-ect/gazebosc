#include "libsphactor.h"
#include "PulseActor.h"

const char * pulseCapabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"timeout\"\n"
                                "        type = \"int\"\n"
                                "        value = \"60\"\n"
                                "        min = \"1\"\n"
                                "        max = \"10000\"\n"
                                "        step = \"1\"\n"
                                "        api_call = \"SET TIMEOUT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "    data\n"
                                "        name = \"someFloat\"\n"
                                "        type = \"float\"\n"
                                "        value = \"1.0\"\n"
                                "        min = \"0\"\n"
                                "        max = \"10\"\n"
                                "        step = \"0\"\n"
                                "    data\n"
                                "        name = \"someText\"\n"
                                "        type = \"string\"\n"
                                "        value = \"Hello world!\"\n"
                                "        max = \"64\"\n"
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t * Pulse::handleMsg( sphactor_event_t *ev ) {
    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(pulseCapabilities));
        return ev->msg;
    }
    else
    if ( streq(ev->type, "TIME")) {
        zosc_t * osc = zosc_create("/pulse", "s", "PULSE");

        zmsg_t *msg = zmsg_new();
        zframe_t *frame = zframe_new(zosc_data(osc), zosc_size(osc));
        zmsg_append(msg, &frame);

        zframe_destroy(&frame);

        return msg;
    }
    else return nullptr;
}
