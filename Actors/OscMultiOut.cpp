#include "OscMultiOut.h"
#include <string>
#include <time.h>

static void
s_replace_char(char *str, char from, char to)
{
    ssize_t length = strlen(str);
    for (int i = 0; i<length;i++)
    {
        if (str[i] == from )
        {
            str[i] = to;
        }
    }
}

const char *
OSCMultiOut::capabilities =          "capabilities\n"
                                "    data\n"
                                "        name = \"host_list\"\n"
                                "        type = \"list\"\n"
                                "        help = \"List of hosts with port (host:port)\"\n"
                                "        value = \"127.0.0.1:1234\"\n"
                                "        api_call = \"SET HOSTS\"\n"
                                "        api_value = \"s\"\n"
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
    if (this->hosts)
        zlist_destroy(&this->hosts);

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
                //char *url = strchr(host, ',');
                //assert(url);
                zstr_sendm(dgrams, host);
                int rc = zsock_send(dgrams,  "b", msgBuffer, len);
                if ( rc != 0 ) {
                    zsys_info("Error sending zosc message to: %s, %i", host, rc);
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
            char *hosts = zmsg_popstr(ev->msg);
            while ( hosts != NULL )
            {
                ssize_t length = strlen(hosts);
                // replace ,  with 0x0. Multiline use , as seperator
                s_replace_char(hosts, ',', (char)0x0);
                char *host = hosts;

                // Iterate through the characters in the array
                for (ssize_t i = 0; i < length; i++) {
                    // Check if the current character is a 0 terminator
                    if (hosts[i] == (char)0x0 )
                    {
                        if (strlen(host) > 6) // just a validation as the host must contain a : and port
                            zlist_append(this->hosts, host); //copies the string!
                        host = hosts+i+1;
                    }
                    else if (i == length - 1)
                    {
                        if (strlen(host) > 6)
                            zlist_append(this->hosts, host); //copies the string!
                    }
                }
                zstr_free(&hosts);
                hosts = zmsg_popstr(ev->msg);
            }
        }
        zstr_free(&cmd);
    }

    return Sphactor::handleAPI(ev);
}
