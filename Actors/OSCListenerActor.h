#ifndef OSCLISTENERACTOR_H
#define OSCLISTENERACTOR_H

#include "libsphactor.h"
#include <czmq.h>

struct OSCListener {
    int port;
    SOCKET udpSock = -1;

    static void * ConstructOSCListener(void* args) {
        return new OSCListener(1234);
    }

    OSCListener(int port) {
        this->port = port;
    }

    ~OSCListener() {

    }

    void StopAndDestroyServer( const sphactor_actor_t* actor );
    void StartServer( const sphactor_actor_t* actor );

    static zmsg_t * MessageReceived(void * socket) {
        zsys_info("message received");

        int result = 0;
        char* peer = new char[INET_ADDRSTRLEN];
        zframe_t * frame;
        SOCKET s = *(int*)socket;

        frame = zsys_udp_recv(s, peer, INET_ADDRSTRLEN);

        delete[] peer;

        if ( frame ) {
            zmsg_t *zmsg = zmsg_new();

            //TODO: Maybe move this somewhere else?
            // get byte array for frame
            byte * data = zframe_data(frame);
            size_t size = zframe_size(frame);

            //TODO: why is this even here?
            //ssize_t len = lo_validate_string(data, size);

            if (!strcmp((const char *) data, "#bundle")) {
                // Got bundle
                // TODO:  Parse separate messages into frame
            }
            else {
                zsys_info("GOT SOMETHING");
                // Got single message
                //  Just throw the byte array to our output...

                //Deserialize into zosc_msg??
                zosc_t * msg = zosc_frommem((char*)data, size);
                assert(msg);
                zsys_info("msg format: %s", zosc_format(msg));

                if (msg == NULL) {
                    zsys_info("OSC ERROR: Invalid message received");
                }
                else {
                    zframe_t *frame = zframe_new(data, size);
                    zmsg_append(zmsg, &frame);

                    zframe_destroy(&frame);
                    zosc_destroy(&msg);
                }
            }

            zframe_destroy(&frame);

            return zmsg;
        }

        return NULL;
    }
};


#endif // OSCLISTENERACTOR_H
