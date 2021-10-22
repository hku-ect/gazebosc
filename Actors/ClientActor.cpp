#include "ClientActor.h"
#include <string>
#include <time.h>

const char *
Client::capabilities =          "capabilities\n"
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
                                "        max = \"65534\"\n"
                                "        api_call = \"SET PORT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n";

zmsg_t* Client::handleInit( sphactor_event_t * ev ) {
    this->dgrams = zsock_new_dgram("udp://*:*");
    return Sphactor::handleInit(ev);
}

zmsg_t* Client::handleStop( sphactor_event_t * ev ) {
    if ( this->dgrams != NULL ) {
        zsock_destroy(&this->dgrams);
        this->dgrams = NULL;
    }

    return Sphactor::handleStop(ev);
}

zmsg_t* Client::handleSocket( sphactor_event_t * ev ) {
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
        }
    } while (frame != NULL );

    return Sphactor::handleSocket(ev);
}

zmsg_t* Client::handleAPI( sphactor_event_t * ev ) {
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

    return Sphactor::handleAPI(ev);
}
