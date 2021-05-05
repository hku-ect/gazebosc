#include "OpenVRActor.h"
#include <time.h>

const char * openVRCapabilities =
        "capabilities\n"
        "    data\n"
        "        name = \"timeout\"\n"
        "        type = \"int\"\n"
        "        value = \"16\"\n"
        "        min = \"8\"\n"
        "        max = \"1000\"\n"
        "        step = \"1\"\n"
        "        api_call = \"SET TIMEOUT\"\n"
        "        api_value = \"i\"\n"           // optional picture format used in zsock_send
        "    data\n"
        "        name = \"trackers\"\n"
        "        type = \"bool\"\n"
        "        value = \"True\"\n"
        "        api_call = \"SET TRACKERS\"\n"
        "        api_value = \"s\"\n"
        "    data\n"
        "        name = \"devices\"\n"
        "        type = \"bool\"\n"
        "        value = \"True\"\n"
        "        api_call = \"SET DEVICES\"\n"
        "        api_value = \"s\"\n"
        "outputs\n"
        "    output\n"
        "        type = \"OSC\"\n";

zmsg_t * OpenVR::handleInit( sphactor_event_t *ev )
{
    vr::EVRInitError initError = vr::VRInitError_None;
    vrSystem = vr::VR_Init(&initError, vr::EVRApplicationType::VRApplication_Other);

    if (initError != vr::VRInitError_None) {
        vrSystem = nullptr;
        zsys_info("Could not initialize OpenVR");
    }

    // Initialize report timestamp
    zosc_t* msg = zosc_create("/report", "sh",
        "lastActive", (int64_t)0);

    sphactor_actor_set_custom_report_data((sphactor_actor_t*)ev->actor, msg);

    sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(openVRCapabilities));
    return Sphactor::handleInit(ev);
}

zmsg_t * OpenVR::handleStop( sphactor_event_t *ev ) {
    vr::VR_Shutdown();
    return Sphactor::handleStop(ev);
}

zmsg_t * OpenVR::handleAPI( sphactor_event_t *ev )
{
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        if ( streq(cmd, "SET TRACKERS") ) {
            char * value = zmsg_popstr(ev->msg);
            sendTrackers = streq( value, "True");
        }
        else if  ( streq(cmd, "SET DEVICES") ) {
            char * value = zmsg_popstr(ev->msg);
            sendDevices = streq( value, "True");
        }

        zstr_free(&cmd);
    }
    return Sphactor::handleAPI(ev);
}

zmsg_t* OpenVR::handleTimer(sphactor_event_t* ev)
{
    if (vrSystem != nullptr && (sendTrackers || sendDevices)) {
        devices.update(vrSystem);
        
        zmsg_destroy(&ev->msg);
        zmsg_t* msg = zmsg_new();

        //parse and build OSC Message
        if ( sendTrackers ) {
            std::vector<Device *> *trackers = devices.getTrackers();
            for (int i = 0; i < trackers->size(); ++i) {
                Device *device = trackers->at(i);

                if (device->bConnected) {
                    // add oscmsg as frame to zmsg
                    std::string address = "/vr_trackers/" + device->serialNumber;
                    std::string serial = device->serialNumber;
                    zosc_t *oscMsg = zosc_create(address.c_str(), "sfffffffffffff",
                                                 serial.c_str(),
                                                 device->position.x, device->position.y, device->position.z,
                                                 device->quaternion.x, device->quaternion.y, device->quaternion.z,
                                                 device->quaternion.w,
                                                 device->linearVelocity.x, device->linearVelocity.y,
                                                 device->linearVelocity.z,
                                                 device->angularVelocity.x, device->angularVelocity.y,
                                                 device->angularVelocity.z
                    );

                    zframe_t *frame = zosc_packx(&oscMsg);
                    zmsg_append(msg, &frame);
                }
            }
        }

        if ( sendDevices ) {
            std::vector<Device *> *trackedDevices = devices.getDevices();
            for (int i = 0; i < trackedDevices->size(); ++i) {
                Device *device = trackedDevices->at(i);

                if (device->bConnected) {
                    // add oscmsg as frame to zmsg
                    std::string address = "/vr_devices/" + device->serialNumber;
                    std::string serial = device->serialNumber;
                    zosc_t *oscMsg = zosc_create(address.c_str(), "sfffffffffffff",
                                                 serial.c_str(),
                                                 device->position.x, device->position.y, device->position.z,
                                                 device->quaternion.x, device->quaternion.y, device->quaternion.z,
                                                 device->quaternion.w,
                                                 device->linearVelocity.x, device->linearVelocity.y,
                                                 device->linearVelocity.z,
                                                 device->angularVelocity.x, device->angularVelocity.y,
                                                 device->angularVelocity.z
                    );

                    zframe_t *frame = zosc_packx(&oscMsg);
                    zmsg_append(msg, &frame);
                }
            }
        }

        return msg;
    }

    return Sphactor::handleTimer(ev);
}

zmsg_t * OpenVR::handleCustomSocket( sphactor_event_t *ev )
{
    return nullptr;
}