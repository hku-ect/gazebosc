#include "osccreate_actor.h"

zmsg_t *
osccreate_actor_handler( sphactor_event_t *ev, void* args )
{
    static zosc_t *oscmsg = NULL;
    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(osccreate_capabilities));
    }
    else
    if ( streq(ev->type, "API"))
    {
        char *cmd = zmsg_popstr(ev->msg);
        assert(cmd);
        if ( streq(cmd, "SET OSC"))
        {
            char *oscs = zmsg_popstr(ev->msg);
            assert(oscs);
            // destroy old message
            if (oscmsg)
                zosc_destroy(&oscmsg);
            assert(oscmsg == NULL);
            // create new message from string
            oscmsg = zosc_fromstring(oscs);
            assert(oscmsg);
            zstr_free(&oscs);
        }
        if ( streq(cmd, "OSC SEND") && oscmsg != NULL)
        {
            zmsg_t *msg = zmsg_new();
            zframe_t *frame = zframe_new(zosc_data(oscmsg), zosc_size(oscmsg));
            zmsg_append(msg, &frame);

            zmsg_destroy(&ev->msg);
            // publish new msg
            sphactor_actor_send(ev->actor, msg);
        }
        zstr_free(&cmd);
    }
    if ( streq(ev->type, "SOCK") && oscmsg != NULL)
    {
        zmsg_t *msg = zmsg_new();
        zframe_t *frame = zframe_new(zosc_data(oscmsg), zosc_size(oscmsg));
        zmsg_append(msg, &frame);

        zmsg_destroy(&ev->msg);
        // publish new msg
        return msg;
    }
    zmsg_destroy(&ev->msg);
    return NULL;
}
