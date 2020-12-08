#include "NatNetActor.h"
#include <string>

const char * natnetCapabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"motive ip\"\n"
                                "        type = \"string\"\n"
                                "        value = \"192.168.0.1\"\n"
                                "        api_call = \"SET HOST\"\n"
                                "        api_value = \"s\"\n"           // optional picture format used in zsock_send
                                //"    data\n"
                                //"        name = \"receive port\"\n"
                                //"        type = \"int\"\n"
                                //"        value = \"6200\"\n"
                                //"        min = \"1\"\n"
                                //"        max = \"10000\"\n"
                                //"        api_call = \"SET PORT\"\n"
                                //"        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t * NatNet::handleMsg( sphactor_event_t * ev ) {
    if ( streq(ev->type, "INIT") ) {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(natnetCapabilities));

        //TODO: Check receive port in NatNet specifications
        //receive socket on port 1511
        this->dgramr = zsock_new_dgram("udp://*:1511");
        dgram_fd = zsock_fd(this->dgramr);

        //send socket on any
        this->dgrams = zsock_new_dgram("udp://*:*");
        assert( this->dgramr && this->dgrams );
        int rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, this->dgramr );
        assert(rc == 0);
    }
    /*
     * Application will hang if done here when closing application entirely...
     * Even if you have destroyed client actors in between... very odd...
     * */
    else if ( streq(ev->type, "DESTROY") ) {
        //TODO: Fix zsys_shutdown crash when destroying dgrams along the way...
        if ( this->dgrams != NULL ) {
            zsock_destroy(&this->dgrams);
            this->dgrams = NULL;
        }
        if ( this->dgramr != NULL ) {
            sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, this->dgramr);
            zsock_destroy(&this->dgramr);
            this->dgramr = NULL;
        }

        return ev->msg;
    }
    else if ( streq(ev->type, "SOCK") ) {
        //TODO: ?
        zsys_info("GOT SOCK");
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET HOST") ) {
                char * host_addr = zmsg_popstr(ev->msg);
                this->host = host_addr;
                std::string url = "udp://" + this->host + "1510";
                zsys_info("SET HOST: %s", url.c_str());
                zstr_free(&host_addr);
            }

            zstr_free(&cmd);
        }

        zmsg_destroy(&ev->msg);

        return NULL;
    }
    else if ( streq(ev->type, "FDSOCK" ) )
    {
        zsys_info("GOT FDSOCK");

        //TODO: Read data from our socket...
        assert(ev->msg);
        zframe_t *frame = zmsg_pop(ev->msg);
        if (zframe_size(frame) == sizeof( void *) )
        {
            void *p = *(void **)zframe_data(frame);
            zsock_t* sock = (zsock_t*) zsock_resolve( p );
            if ( sock )
            {
                byte* msg;
                size_t len;

                //zsock_recv(sock, "b", &msg, &len );

                zmsg_t * zmsg = zmsg_recv(sock);
                assert(zmsg);

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
