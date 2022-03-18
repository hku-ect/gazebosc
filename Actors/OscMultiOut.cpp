#include "OscMultiOut.h"
#include <string>
#include <time.h>

const char *
OSCMultiOut::capabilities =          "capabilities\n"
                                "    data\n"
                                "        name = \"host list\"\n"
                                "        type = \"list\"\n"
                                "        help = \"List of hosts\"\n"
                                "        name\n"
                                "            type = \"string\"\n"
                                "            help = \"Just a name for your convenience\"\n"
                                "        ip\n"
                                "            type = \"ipaddress\"\n"
                                "            help = \"The ipaddress of the host to send to\"\n"
                                "        port\n"
                                "            type = \"int\"\n"
                                "            help = \"The port number of the host to send to\"\n"
                                "            value = \"6200\"\n"
                                "            min = \"1\"\n"
                                "            max = \"65534\"\n"
                                "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n";

zmsg_t* OSCMultiOut::handleInit( sphactor_event_t * ev ) {
    this->dgrams = zsock_new_dgram("udp://*:*");
    return Sphactor::handleInit(ev);
}

zmsg_t* OSCMultiOut::handleStop( sphactor_event_t * ev ) {
    if ( this->dgrams != NULL ) {
        zsock_destroy(&this->dgrams);
        this->dgrams = NULL;
    }

    return Sphactor::handleStop(ev);
}

zmsg_t* OSCMultiOut::handleSocket( sphactor_event_t * ev ) {
    if ( ev->msg == NULL ) return NULL;
    if ( this->dgrams == NULL ) return ev->msg;

    byte *msgBuffer;
    zframe_t* frame;

    do {
        frame = zmsg_pop(ev->msg);
        if ( frame ) {
            msgBuffer = zframe_data(frame);
            size_t len = zframe_size(frame);

            char *host = (char *)zlist_first(this->hosts);
            while ( host )
            {
                char *url = strchr(host, ',');
                assert(url);
                zstr_sendm(dgrams, url);
                int rc = zsock_send(dgrams,  "b", msgBuffer, len);
                if ( rc != 0 ) {
                    zsys_info("Error sending zosc message to: %s, %i", url, rc);
                }
                host = (char *)zlist_next(this->hosts);
            }
        }
    } while (frame != NULL );

    return Sphactor::handleSocket(ev);
}

zmsg_t* OSCMultiOut::handleAPI( sphactor_event_t * ev ) {
    //pop msg for command
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        if ( streq(cmd, "SET HOSTS") ) {
            // clear the current list
            if (this->hosts)
                zlist_destroy(&this->hosts);
            this->hosts = zlist_new();
            zlist_autofree(this->hosts);

            // fill with new data
            char *name;
            while ( name = zmsg_popstr(ev->msg) )
            {
                char *ip = zmsg_popstr(ev->msg);
                assert(ip);
                char *port = zmsg_popstr(ev->msg);
                assert(port);
                char host[PATH_MAX];
                snprintf(host, PATH_MAX, "%s,%s:%s");
                zlist_append(this->hosts, (void *)&host[0]);
                zstr_free(&name);
                zstr_free(&ip);
                zstr_free(&port);
            }
        }
        zstr_free(&cmd);
    }

    return Sphactor::handleAPI(ev);
}
