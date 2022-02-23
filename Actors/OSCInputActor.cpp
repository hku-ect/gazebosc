#include "OSCInputActor.h"
#include <time.h>

const char * OSCInput::capabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"port\"\n"
        "        type = \"int\"\n"
        "        help = \"The port number to listen on. Only use unique port numbers, don't use two actors with the same port number\"\n"
        "        value = \"6200\"\n"
        "        min = \"1\"\n"
        "        max = \"10000\"\n"
        "        api_call = \"SET PORT\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

zmsg_t * OSCInput::handleInit( sphactor_event_t *ev )
{
    return Sphactor::handleInit(ev);
}

zmsg_t * OSCInput::handleStop( sphactor_event_t *ev ) {
    if (this->dgramr) {
        sphactor_actor_poller_remove((sphactor_actor_t *) ev->actor, this->dgramr);
        zsock_destroy(&this->dgramr);
    }

    return Sphactor::handleStop(ev);
}

zmsg_t * OSCInput::handleAPI( sphactor_event_t *ev )
{
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        if ( streq(cmd, "SET PORT") ) {
            char * port = zmsg_popstr(ev->msg);
            this->port = port;
            if ( this->dgramr ) {
                sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, this->dgramr);
                zsock_destroy(&this->dgramr);
            }

            std::string url = "udp://*:" + this->port;
            this->dgramr = zsock_new_dgram(url.c_str());
            if ( this->dgramr ) {
                sphactor_actor_poller_add((sphactor_actor_t *) ev->actor, this->dgramr);
                zsys_info("Listening on url: %s", url.c_str());
            }
            else {
                zsys_info("Error creating listener for url: %s", url.c_str());
            }

            zsys_info("SET PORT: %s", url.c_str());
            zstr_free(&port);
        }

        zstr_free(&cmd);
    }
    return Sphactor::handleAPI(ev);
}

zmsg_t * OSCInput::handleCustomSocket( sphactor_event_t *ev )
{
    assert(ev->msg);
    zmsg_t *retmsg = NULL;
    zframe_t *frame = zmsg_pop(ev->msg);
    if (zframe_size(frame) == sizeof( void *) )
    {
        void *p = *(void **)zframe_data(frame);
        if ( zsock_is( p ) && p == this->dgramr )
        {
            // get our message
            retmsg = zmsg_recv(this->dgramr);
            // first message is the source address, pop it
            char *senderaddr = zmsg_popstr(retmsg);
            zstr_free(&senderaddr);
        }
    }
    return retmsg;
}
