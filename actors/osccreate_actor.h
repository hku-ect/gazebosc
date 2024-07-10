#ifndef OSCCREATE_ACTOR_H
#define OSCCREATE_ACTOR_H

#include "libsphactor.h"

#ifdef __cplusplus
extern "C" {
#endif

static const
char * osccreate_capabilities =
    "capabilities\n"
    "    data\n"
    "        name = \"oscstring\"\n"
    "        type = \"string\"\n"
    "        help = \"String representing the OSC message: i=int h=int4 f=float d=double s=string \"\n"
    "        value = \"/example si hello 42\"\n"
    "        api_call = \"SET OSC\"\n"
    "        api_value = \"s\"\n"           // optional picture format used in zsock_send
    "    data\n"
    "        name = \"send OSC\"\n"
    "        type = \"trigger\"\n"
    "        help = \"Send the OSC message\"\n"
    "        api_call = \"OSC SEND\"\n"
    "outputs\n"
    "    output\n"
    "        type = \"OSC\"\n";


//  Simple actor that sends an osc message
zmsg_t *
osccreate_actor_handler (sphactor_event_t *event, void *args);

#ifdef __cplusplus
}
#endif

#endif // OSCCREATE_ACTOR_H
