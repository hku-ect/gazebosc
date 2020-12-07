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
                                "    data\n"
                                "        name = \"receive port\"\n"
                                "        type = \"int\"\n"
                                "        value = \"6200\"\n"
                                "        min = \"1\"\n"
                                "        max = \"10000\"\n"
                                "        api_call = \"SET PORT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t * NatNet::handleMsg( sphactor_event_t * ev ) {
    if ( streq(ev->type, "INIT") ) {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(natnetCapabilities));
    }
    /*
     * Application will hang if done here when closing application entirely...
     * Even if you have destroyed client actors in between... very odd...
     * */
    else if ( streq(ev->type, "DESTROY") ) {
        zsys_info("Handling Destroy");
        //TODO: Fix zsys_shutdown crash when destroying dgrams along the way...
        if ( this->dgrams != NULL ) {
            //zsock_destroy(&this->dgrams);
            this->dgrams = NULL;
        }
        if ( this->dgramr != NULL ) {
            //zsock_destroy(&this->dgramr);
            this->dgramr = NULL;
        }

        return ev->msg;
    }
    else if ( streq(ev->type, "SOCK") ) {
        //TODO: ?
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET PORT") ) {
                if ( this->dgramr != NULL ) {
                    //TODO: poller remove in new form

                    zsock_destroy(&this->dgramr);
                }

                char * port = zmsg_popstr(ev->msg);
                std::string url = "udp://*.";
                url += port;
                this->dgramr = zsock_new_dgram(url.c_str());

                //TODO: poller add in new format

                zsys_info("SET PORT: %s", url.c_str());

                zstr_free(&port);
            }
            else if ( streq(cmd, "SET HOST") ) {
                if ( this->dgrams != NULL ) {
                    //TODO: poller remove in new form

                    zsock_destroy(&this->dgrams);
                }

                char * host_addr = zmsg_popstr(ev->msg);

                std::string url = "udp://";
                url += host_addr;
                url += ":1511";
                this->dgrams = zsock_new_dgram(url.c_str());

                if ( this->dgrams != NULL ) {
                    //TODO: poller add in new format
                }
                else {
                    zsys_info("Could not connect to host: %s", url.c_str());
                }

                zsys_info("SET HOST: %s", host_addr);

                zstr_free(&host_addr);
            }

            zstr_free(&cmd);
        }

        zmsg_destroy(&ev->msg);

        return NULL;
    }

    return ev->msg;
}
