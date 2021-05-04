#ifndef GAZEBOSC_NATNET2OSCACTOR_H
#define GAZEBOSC_NATNET2OSCACTOR_H

#include "libsphactor.hpp"
#include "NatNetDataTypes.h"
#include <map>

class NatNet2OSC : public Sphactor
{
public:
    bool sendMarkers = false;
    bool sendRigidbodies = false;
    bool sendSkeletons = false;
    bool sendSkeletonDefinitions = false;
    bool sendVelocities = false;
    bool sendHierarchy = true;

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

    std::vector<RigidBodyHistory> rbHistory;

    zmsg_t *handleInit( sphactor_event_t *ev );
    zmsg_t *handleSocket( sphactor_event_t *ev );
    zmsg_t *handleAPI( sphactor_event_t *ev );

    //NatNet parse functions
    void Unpack( char ** pData );
    char* unpackRigidBodies(char* ptr, std::vector<RigidBody>& ref_rigidbodies);
    char* unpackMarkerSet(char* ptr, std::vector<Marker>& ref_markers);
    void sendRequestDescription();

    //OSC Sending Functions
    void addRigidbodies(zmsg_t *zmsg);
    void addSkeletons(zmsg_t *zmsg);
    void fixRanges( glm::vec3 *euler );

    NatNet2OSC() {
    }

    ~NatNet2OSC() {
    }
};

#endif //GAZEBOSC_NATNET2OSCACTOR_H
