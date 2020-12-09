#ifndef NATNETACTOR_H
#define NATNETACTOR_H

#include "libsphactor.h"
#include <string>
#include <vector>
#include <map>
#include "NatNetDataTypes.h"

struct NatNet {
    // DataSocket;
    zsock_t* DataSocket = NULL;
    SOCKET dataFD = -1;

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

    int frame_number;
    float latency;
    float timeout;

    std::vector<std::vector<Marker> > markers_set;
    std::vector<Marker> filtered_markers;
    std::vector<Marker> markers;

    std::map<int, RigidBody> rigidbodies;
    std::vector<RigidBody> rigidbodies_arr;

    std::map<int, Skeleton> skeletons;
    std::vector<Skeleton> skeletons_arr;

    std::vector<RigidBodyDescription> rigidbody_descs;
    std::vector<SkeletonDescription> skeleton_descs;
    std::vector<MarkerSetDescription> markerset_descs;

    zmsg_t * handleMsg( sphactor_event_t *ev );

    //NatNet parse functions
    void Unpack( char * pData );
    int SendCommand(char* szCOmmand);
    void HandleCommand(sPacket *PacketIn);

    // ofxNatNet borrowed functions
    char* unpackRigidBodies(char* ptr, std::vector<RigidBody>& ref_rigidbodies);
    char* unpackMarkerSet(char* ptr, std::vector<Marker>& ref_markers);


    //OSC Sending Functions
    void addRigidbodies(zmsg_t *zmsg);
    void addSkeletons(zmsg_t *zmsg);
    void fixRanges( Vec3 *euler );

    //TODO: necessary?
    bool IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address);
    int GetLocalIPAddresses(unsigned long Addresses[], int nMax);



    NatNet() {

    }
};

#endif