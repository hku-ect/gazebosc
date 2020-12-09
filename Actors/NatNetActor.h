#ifndef NATNETACTOR_H
#define NATNETACTOR_H

#include "libsphactor.h"
#include <string>
#include <vector>
#include "NatNetDataTypes.h"

struct NatNet {
    // HostAddr = udp://*:* ?
    // DataSocket;
    zsock_t* DataSocket = NULL;
    SOCKET dataFD = -1;
    char  szData[20000];    //data receive buffer

    // CommandSocket;
    zsock_t* CommandSocket = NULL;
    SOCKET cmdFD = -1;
    // ServerAddress;
    std::string host = "";

    int NatNetVersion[4] = {0,0,0,0};
    int ServerVersion[4] = {0,0,0,0};

    int gCommandResponse = 0;
    int gCommandResponseSize = 0;
    unsigned char gCommandResponseString[260]; //MAX_PATH = 260?
    int gCommandResponseCode = 0;

    //IDEA: definition vectors
    std::vector<MarkerDefinition> markers;
    std::vector<RigidbodyDefinition> rigidbodies;
    std::vector<SkeletonDefinition> skeletons;

    zmsg_t * handleMsg( sphactor_event_t *ev );

    //Natnet parse functions
    //TODO: necessary?
    bool IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address);

    void Unpack( char * pData );

    //TODO: necessary?
    int GetLocalIPAddresses(unsigned long Addresses[], int nMax);

    //TODO: copy/paste, edit using our dgrams
    int SendCommand(char* szCOmmand);
    void HandleCommand(sPacket *PacketIn);

    NatNet() {

    }
};

#endif