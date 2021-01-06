#include "ClientActor.h"
#include <string>

const char * clientCapabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"name\"\n"
                                "        type = \"string\"\n"
                                "        value = \"default\"\n"
                                "        api_call = \"SET NAME\"\n"
                                "        api_value = \"s\"\n"
                                "    data\n"
                                "        name = \"ip\"\n"
                                "        type = \"string\"\n"
                                "        value = \"192.168.0.1\"\n"
                                "        api_call = \"SET HOST\"\n"
                                "        api_value = \"s\"\n"           // optional picture format used in zsock_send
                                "    data\n"
                                "        name = \"port\"\n"
                                "        type = \"int\"\n"
                                "        value = \"6200\"\n"
                                "        min = \"1\"\n"
                                "        max = \"10000\"\n"
                                "        api_call = \"SET PORT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n";

zmsg_t* Client::handleMsg( sphactor_event_t * ev ) {
    if ( streq(ev->type, "INIT") ) {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(clientCapabilities));
        this->dgrams = zsock_new_dgram("udp://*:*");
    }
    else if ( streq(ev->type, "DESTROY") ) {
        zsys_info("Handling Destroy");
        if ( this->dgrams != NULL ) {
            zsock_destroy(&this->dgrams);
            this->dgrams = NULL;
        }

        delete this;
        zmsg_destroy(&ev->msg);
        return NULL;
    }
    else if ( streq(ev->type, "SOCK") ) {
        if ( ev->msg == NULL ) return NULL;
        if ( this->dgrams == NULL ) return ev->msg;

        byte *msgBuffer;
        zframe_t* frame;

        do {
        frame = zmsg_pop(ev->msg);
            if ( frame ) {
                msgBuffer = zframe_data(frame);
                size_t len = zframe_size(frame);

                std::string url = this->host+":"+this->port;
                zstr_sendm(dgrams, url.c_str());
                int rc = zsock_send(dgrams,  "b", msgBuffer, len);
                if ( rc != 0 ) {
                    zsys_info("Error sending zosc message to: %s, %i", url.c_str(), rc);
                }
                else {
                    zsys_info( "Sent message to %s", url.c_str());
                }
            }
        } while (frame != NULL );

        zmsg_destroy(&ev->msg);

        return NULL;
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET NAME") ) {
                char * name = zmsg_popstr(ev->msg);
                this->name = name;
                zstr_free(&name);
            }
            else if ( streq(cmd, "SET PORT") ) {
                char * port = zmsg_popstr(ev->msg);
                this->port = port;
                std::string url = ("udp://" + this->host + ":" + this->port);
                zsys_info("SET PORT: %s", url.c_str());
                zstr_free(&port);
            }
            else if ( streq(cmd, "SET HOST") ) {
                char * host_addr = zmsg_popstr(ev->msg);
                this->host = host_addr;
                std::string url = ("udp://" + this->host + ":" + this->port);
                zsys_info("SET HOST: %s", url.c_str());
                zstr_free(&host_addr);
            }

            zstr_free(&cmd);
        }

        zmsg_destroy(&ev->msg);

        return NULL;
    }

    return ev->msg;
}
