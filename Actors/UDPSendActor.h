#ifndef UDPSENDACTOR_H
#define UDPSENDACTOR_H

#include "libsphactor.h"
#include <czmq.h>

struct UDPSend {
    byte *msgBuffer;
    zframe_t* frame;

    int port;
    struct addrinfo *bind_to;
    SOCKET udpSock;

    static void * ConstructUDPSend(void* args) {
        return new UDPSend();
    }

    ~UDPSend() {

    }

    void CreateSocket(const sphactor_actor_t* actor);
    void DestroySocket(const sphactor_actor_t* actor);
};

#endif // UDPSENDACTOR_H
