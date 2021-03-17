#include "LiveLinkActor.h"
#include <string>

const char* liveLinkCapabilities = "inputs\n"
"    input\n"
"        type = \"OSC\"\n"
"outputs\n"
"    output\n"
"        type = \"OSC\"\n";

static const char* const names[] = { 
    "eyeBlink_R",                       //1
    "eyeLookDown_R",                   //2
    "eyeLookIn_R",                     //3
    "eyeLookOut_R",                    //4
    "eyeLookUp_R",                     //5
    "eyeSquint_R",                      //6
    "eyeWide_R",                        //7
    "eyeBlink_L",                       //8
    "eyeLookDown_L",                   //9
    "eyeLookIn_L",                     //10
    "eyeLookOut_L",                    //11
    "eyeLookUp_L",                     //12
    "eyeSquint_L",                      //13
    "eyeWide_L",                        //14
    "jawForward",                       //15
    "jawRight",                         //16
    "jawLeft",                          //17
    "jawOpen",                          //18
    "mouthClose",                       //19
    "mouthFunnel",                      //20
    "mouthPucker",                      //21
    "mouthRight",                       //22
    "mouthLeft",                        //23
    "mouthSmile_R",                     //24
    "mouthSmile_L",                     //25
    "mouthFrown_R",                     //26
    "mouthFrown_L",                     //27
    "mouthDimple_R",                   //28
    "mouthDimple_L",                   //29
    "mouthStretch_R",                   //30
    "mouthStretch_L",                   //31
    "mouthRollLower",                   //32
    "mouthRollUpper",                   //33
    "mouthShrugLower",                  //34
    "mouthShrugUpper",                  //35
    "mouthPress_R",                     //36
    "mouthPress_L",                     //37
    "mouthLowerDown_R",                 //38
    "mouthLowerDown_L",                 //39
    "mouthUpperUp_R",                   //40
    "mouthUpperUp_L",                   //41
    "browDown_R",                       //42
    "browDown_L",                       //43
    "browInnerUp",                      //44
    "browOuterUp_R",                    //45
    "browOuterUp_L",                    //46
    "cheekPuff",                        //47
    "cheekSquint_R",                    //48
    "cheekSquint_L",                    //49
    "noseSneer_R",                      //50
    "noseSneer_L",                      //51
    "tongueOut",                        //52
    "HeadYaw",                          //53
    "HeadPitch",                        //54
    "HeadRoll",                         //55
    "LeftEyeYaw",                       //56
    "LeftEyePitch",                     //57
    "LeftEyeRoll",                      //58
    "RightEyeYaw",                      //59
    "RightEyePitch",                    //60
    "RightEyeRoll",                     //61 
};

zmsg_t*
LiveLink::handleInit(sphactor_event_t* ev) {
    values = new float[128];
    name = new char[64];

    sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(liveLinkCapabilities));
    return Sphactor::handleInit(ev);
}

zmsg_t*
LiveLink::handleSocket(sphactor_event_t* ev) {

    //Data Format:
    // Structure data (0-44)
    // Subject Name (byte 45)
    // TimeCode (45 + len(name) + 1, no idea what the format is)
    // BlendShapeCount (might not be included)
    //  Above 2 13 bytes total...
    // TArray<float> (45 + len(name) + 1 + 13, 61 * 4 bytes)


    zframe_t* frame = zmsg_pop(ev->msg);

    byte* bytes = zframe_data(frame);
    size_t len = zframe_size(frame);
    
    //zsys_info("byteCount: %i", len);

    //name location
    int ptr = 45;

    // Device Name
    memcpy(name, bytes + ptr, 64);
    //zsys_info("Name: %s", name);
    ptr += strlen(name) + 1;

    // Time Code Values (format: 00:00:00:00.000 )
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //hours?
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //minutes?
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //seconds?
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //milliseconds?
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //ticks?
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???
    //zsys_info("(%i): %hhu", ptr, *(bytes + ptr));
    ptr += 1;   //???

    //expecting 61 floating-point values...
    int fI = 0;
    bool gotValues = false;
    for (int i = ptr; i < len - 4; i += 4) {
        gotValues = true;
        memcpy(values + fI, bytes + i, 4);
        fI++;
    }

    if (gotValues) {
        //build OSC message
        std::string address = "/livelinkface/";
        address.append(name);
        zosc_t* osc = zosc_create(address.c_str(), "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
            values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8], values[9],
            values[10], values[11], values[12], values[13], values[14], values[15], values[16], values[17], values[18], values[19],
            values[20], values[21], values[22], values[23], values[24], values[25], values[26], values[27], values[28], values[29],
            values[30], values[31], values[32], values[33], values[34], values[35], values[36], values[37], values[38], values[39],
            values[40], values[41], values[42], values[43], values[44], values[45], values[46], values[47], values[48], values[49],
            values[50], values[51], values[52], values[53], values[54], values[55], values[56], values[57], values[58], values[59],
            values[60]
            );

        zmsg_t* msg = zmsg_new();
        zframe_t* msgFrame = zosc_packx(&osc);
        zmsg_append(msg, &msgFrame);

        zframe_destroy(&msgFrame);
        zmsg_destroy(&ev->msg);

        return msg;
    }

    // clean up
    zframe_destroy(&frame);

    return nullptr;
}