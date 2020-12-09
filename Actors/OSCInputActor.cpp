#include "OSCInputActor.h"

const char * oscInputCapabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"port\"\n"
        "        type = \"int\"\n"
        "        value = \"6200\"\n"
        "        min = \"1\"\n"
        "        max = \"10000\"\n"
        "        api_call = \"SET PORT\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

zmsg_t * OSCInput::handleMsg( sphactor_event_t *ev )
{
    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(oscInputCapabilities));
    }
    else if ( streq( ev->type, "DESTROY")) {
        if ( this->dgramr ) {
            sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, this->dgramr);
            zsock_destroy(&this->dgramr);
        }

        delete this;
        zmsg_destroy(&ev->msg);
        return NULL;
    }
    else if ( streq( ev->type, "API")) {
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

        zmsg_destroy(&ev->msg);

        return NULL;
    }
    else if ( streq( ev->type, "FDSOCK")) {
        zsys_info("GOT FDSOCK");

        assert(ev->msg);
        zframe_t *frame = zmsg_pop(ev->msg);
        if (zframe_size(frame) == sizeof( void *) )
        {
            void *p = *(void **)zframe_data(frame);
            zsock_t* sock = (zsock_t*) zsock_resolve( p );
            if ( sock )
            {
                zmsg_t * zmsg = zmsg_recv(sock);
                assert(zmsg);

                // Is this always the case?
                //  -> works when sending OSC message from Unity, so I assume it is
                char* source = zmsg_popstr(zmsg);
                zstr_free(&source);

                zframe_t * oscFrame = zmsg_pop(zmsg);
                assert(oscFrame);

                //repackage and return as new message
                zmsg_t* oscMsg = zmsg_new();
                zmsg_add(oscMsg, oscFrame);
                return oscMsg;
            }
        }
        else
            zsys_error("args is not a zsock instance");

        zmsg_destroy(&ev->msg);
        zframe_destroy(&frame);

        return NULL;
    }

    return ev->msg;
}