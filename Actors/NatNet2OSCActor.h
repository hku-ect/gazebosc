#ifndef GAZEBOSC_NATNET2OSCACTOR_H
#define GAZEBOSC_NATNET2OSCACTOR_H

struct NatNet2OSC {
    bool sendMarkers = false;
    bool sendRigidbodies = false;
    bool sendSkeletons = false;
    bool sendSkeletonDefinitions = false;
    bool sendVelocities = false;
    bool sendHierarchy = true;

    zmsg_t *handleMsg( sphactor_event_t *ev );

    NatNet2OSC() {
    }

    ~NatNet2OSC() {
    }
};

#endif //GAZEBOSC_NATNET2OSCACTOR_H
