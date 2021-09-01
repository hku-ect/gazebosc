#ifndef NATNETACTOR_H
#define NATNETACTOR_H

#include "libsphactor.hpp"
#include <string>
#include <vector>
#include <map>
#include "NatNetDataTypes.h"

class NatNet : public Sphactor {
public:
    // DataSocket;
    zsock_t* DataSocket = NULL;

    // CommandSocket;
    zsock_t* CommandSocket = NULL;

    zmsg_t * lastData = nullptr;

    // Actor Settings
    // ServerAddress
    std::string host = "";

    std::vector<std::string> ifNames;
    std::vector<std::string> ifAddresses;
    std::string activeInterface = "";

    // NatNet vars
    static int* NatNetVersion;
    static int* ServerVersion;
    bool rigidbodiesReady = true;
    bool skeletonsReady = true;

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

    zmsg_t* handleInit(sphactor_event_t *ev);
    zmsg_t* handleTimer(sphactor_event_t *ev);
    zmsg_t* handleAPI(sphactor_event_t *ev);
    //zmsg_t* handleSocket(sphactor_event_t *ev)
    zmsg_t* handleCustomSocket(sphactor_event_t *ev);
    zmsg_t* handleStop(sphactor_event_t *ev);

    //NatNet parse functions
    int SendCommand(char* szCOmmand);
    void HandleCommand(sPacket *PacketIn);

    void Unpack( char ** pData );
    char* unpackRigidBodies(char* ptr, std::vector<RigidBody>& ref_rigidbodies);
    char* unpackMarkerSet(char* ptr, std::vector<Marker>& ref_markers);
    void sendRequestDescription();

    //TODO: necessary?
    bool IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address);
    int GetLocalIPAddresses(unsigned long Addresses[], int nMax);

    NatNet() {

    }
};

#endif
