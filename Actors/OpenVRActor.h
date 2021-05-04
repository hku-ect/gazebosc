#ifndef GAZEBOSC_OPENVRACTOR_H
#define GAZEBOSC_OPENVRACTOR_H

#include "libsphactor.hpp"
#include <string>
#include "../ext/openvr/headers/openvr.h"
#include "DeviceList.hpp"

class OpenVR : public Sphactor {
private:
    bool sendTrackers = TRUE;
    bool sendDevices = TRUE;

    vr::IVRSystem* vrSystem;
    DeviceList devices;

public:
    OpenVR() : Sphactor() {

    }

    zmsg_t * handleInit( sphactor_event_t *ev );
    zmsg_t * handleStop( sphactor_event_t *ev );
    zmsg_t * handleCustomSocket( sphactor_event_t *ev );
    zmsg_t * handleAPI( sphactor_event_t *ev );
    zmsg_t * handleTimer( sphactor_event_t* ev );
};

#endif //GAZEBOSC_OPENVRACTOR_H
