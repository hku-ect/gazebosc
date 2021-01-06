#include "libsphactor.h"
#include "NatNet2OSCActor.h"

//TODO: Add filter options
const char * natNet2OSCCapabilities = "capabilities\n"
                                "    data\n"
                                "        name = \"markers\"\n"
                                "        type = \"bool\"\n"
                                "        value = \"False\"\n"
                                "        api_call = \"SET MARKERS\"\n"
                                "        api_value = \"s\"\n"
                                "    data\n"
                                "        name = \"rigidbodies\"\n"
                                "        type = \"bool\"\n"
                                "        value = \"False\"\n"
                                "        api_call = \"SET RIGIDBODIES\"\n"
                                "        api_value = \"s\"\n"
                                "    data\n"
                                "        name = \"skeletons\"\n"
                                "        type = \"bool\"\n"
                                "        value = \"False\"\n"
                                "        api_call = \"SET SKELETONS\"\n"
                                "        api_value = \"s\"\n"
                                "    data\n"
                                "        name = \"rbVelocities\"\n"
                                "        type = \"bool\"\n"
                                "        value = \"False\"\n"
                                "        api_call = \"SET VELOCITIES\"\n"
                                "        api_value = \"s\"\n"
                                "    data\n"
                                "        name = \"hierarchy\"\n"
                                "        type = \"bool\"\n"
                                "        value = \"True\"\n"
                                "        api_call = \"SET HIERARCHY\"\n"
                                "        api_value = \"s\"\n"
                                "    data\n"
                                "        name = \"sendSkeletonDefinitions\"\n"
                                "        type = \"bool\"\n"
                                "        value = \"False\"\n"
                                "        api_call = \"SET SKELETONDEF\"\n"
                                "        api_value = \"s\"\n"
                                "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n" //TODO: NatNet input?
                                "outputs\n"
                                "    output\n"
                                "        type = \"OSC\"\n";

zmsg_t *
NatNet2OSC::handleMsg( sphactor_event_t *ev ) {
    if ( streq(ev->type, "INIT")) {
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(natNet2OSCCapabilities));
    }
    else if ( streq( ev->type, "DESTROY")) {
        delete this;
        zmsg_destroy(&ev->msg);
        return NULL;
    }
    else if ( streq(ev->type, "SOCK")) {
        //TODO: unpack msg and parse into OSC
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET MARKERS") ) {
                char * value = zmsg_popstr(ev->msg);
                sendMarkers = streq( value, "True");
                //zsys_info("Got: %s, set to %s", value, sendMarkers ? "True" : "False");
            }
            else if ( streq(cmd, "SET RIGIDBODIES") ) {
                char * value = zmsg_popstr(ev->msg);
                sendRigidbodies = streq( value, "True");
                //zsys_info("Got: %s, set to %s", value, sendRigidbodies ? "True" : "False");
            }
            else if ( streq(cmd, "SET SKELETONS") ) {
                char * value = zmsg_popstr(ev->msg);
                sendSkeletons = streq( value, "True");
                //zsys_info("Got: %s, set to %s", value, sendSkeletons ? "True" : "False");
            }
            else if ( streq(cmd, "SET VELOCITIES") ) {
                char * value = zmsg_popstr(ev->msg);
                sendVelocities = streq( value, "True");
                //zsys_info("Got: %s, set to %s", value, sendVelocities ? "True" : "False");
            }
            else if ( streq(cmd, "SET HIERARCHY") ) {
                char * value = zmsg_popstr(ev->msg);
                sendHierarchy = streq( value, "True");
                //zsys_info("Got: %s, set to %s", value, sendHierarchy ? "True" : "False");
            }
            else if ( streq(cmd, "SET SKELETONDEF") ) {
                char * value = zmsg_popstr(ev->msg);
                sendSkeletonDefinitions = streq( value, "True");
                //zsys_info("Got: %s, set to %s", value, sendSkeletonDefinitions ? "True" : "False");
            }

            zstr_free(&cmd);
        }

        zmsg_destroy(&ev->msg);

        return NULL;
    }

    return ev->msg;
}