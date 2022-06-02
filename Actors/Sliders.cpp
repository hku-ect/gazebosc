#include "Sliders.h"

const char *IntSlider::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"address\"\n"
        "        type = \"string\"\n"
        "        help = \"OSC address string\"\n"
        "        value = \"/slider\"\n"
        "        api_call = \"SET NAME\"\n"
        "        api_value = \"s\"\n"
        "    data\n"
        "        name = \"slider\"\n"
        "        type = \"slider\"\n"
        "        help = \"Value slider from min to max\"\n"
        "        value = \"\"\n"
        "        min = \"0\"\n"
        "        max = \"255\"\n"
        "        step = \"0\"\n"
        "        api_call = \"OSCSEND\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

zmsg_t *
IntSlider::handleAPI(sphactor_event_t *ev)
{
    char *cmd = zmsg_popstr(ev->msg);
    if ( streq(cmd, "OSCSEND") )
    {
        char *data = zmsg_popstr(ev->msg);
        char *addr = strdup(sphactor_actor_name( (sphactor_actor_t*)ev->actor ));

        zosc_t *retosc =  zosc_create(addr, "i", atoi(data)  );
        assert(retosc);
        zmsg_t *ret = zmsg_new();
        zmsg_add(ret, (zosc_packx(&retosc)));
        int rc = sphactor_actor_send((sphactor_actor_t *)ev->actor, ret);
        assert(rc == 0);
    }
    return NULL;
}

#include "Sliders.h"

const char *FloatSlider::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"address\"\n"
        "        type = \"string\"\n"
        "        help = \"OSC address string\"\n"
        "        value = \"/slider\"\n"
        "        api_call = \"SET NAME\"\n"
        "        api_value = \"s\"\n"
        "    data\n"
        "        name = \"slider\"\n"
        "        type = \"slider\"\n"
        "        help = \"Value slider from min to max\"\n"
        "        value = \"\"\n"
        "        min = \"0.0\"\n"
        "        max = \"1.0\"\n"
        "        step = \"0\"\n"
        "        api_call = \"OSCSEND\"\n"
        "        api_value = \"f\"\n"           // optional picture format used in zsock_send
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

zmsg_t *
FloatSlider::handleAPI(sphactor_event_t *ev)
{
    char *cmd = zmsg_popstr(ev->msg);
    if ( streq(cmd, "OSCSEND") )
    {
        char *data = zmsg_popstr(ev->msg);
        char *addr = strdup(sphactor_actor_name( (sphactor_actor_t*)ev->actor ));

        zosc_t *retosc =  zosc_create(addr, "f", atof(data)  );
        assert(retosc);
        zmsg_t *ret = zmsg_new();
        zmsg_add(ret, (zosc_packx(&retosc)));
        int rc = sphactor_actor_send((sphactor_actor_t *)ev->actor, ret);
        assert(rc == 0);
    }
    return NULL;
}
