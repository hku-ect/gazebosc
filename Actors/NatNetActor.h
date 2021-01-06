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
    int dataFD = -1;

    // CommandSocket;
    zsock_t* CommandSocket = NULL;
    int cmdFD = -1;

    // Actor Settings
    // ServerAddress
    std::string host = "";

    // NatNet vars
    static int* NatNetVersion;// = {0,0,0,0};
    static int* ServerVersion;// = {0,0,0,0};

    int gCommandResponse = 0;
    int gCommandResponseSize = 0;
    unsigned char gCommandResponseString[260]; //MAX_PATH = 260?
    int gCommandResponseCode = 0;

    int frame_number;
    float latency;
    float timeout;
    int sentRequest;
    float duplicated_point_removal_distance = 0;

    std::vector<std::vector<Marker> > markers_set;
    std::vector<Marker> filtered_markers;
    std::vector<Marker> markers;

    std::map<int, RigidBody> rigidbodies;
    std::map<int, Skeleton> skeletons;

    static std::vector<RigidBodyDescription> rigidbody_descs;
    static std::vector<SkeletonDescription> skeleton_descs;
    static std::vector<MarkerSetDescription> markerset_descs;

    zmsg_t * handleMsg( sphactor_event_t *ev );

    //NatNet parse functions
    int SendCommand(char* szCOmmand);
    void HandleCommand(sPacket *PacketIn);

    void Unpack( char * pData );
    char* unpackRigidBodies(char* ptr, std::vector<RigidBody>& ref_rigidbodies);
    char* unpackMarkerSet(char* ptr, std::vector<Marker>& ref_markers);
    void sendRequestDescription();

    //TODO: necessary?
    bool IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address);
    int GetLocalIPAddresses(unsigned long Addresses[], int nMax);

    NatNet() {
        NatNetVersion = new int[]{0,0,0,0};
        ServerVersion = new int[]{0,0,0,0};
    }
};

#endif