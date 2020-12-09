#ifndef GAZEBOSC_NATNETDATATYPES_H
#define GAZEBOSC_NATNETDATATYPES_H

#include <string>
#include <vector>

// Custom data structs
struct MarkerDefinition {
    float x, y, z;
};

struct RigidbodyDefinition {
    std::string name;
    int id;
    float x, y, z;
    float qx, qy, qz, qw;
    int parentID;
    int xOffset, yOffset, zOffset;
};

struct SkeletonDefinition {
    std::string name;
    int id;
    std::vector<RigidbodyDefinition> rigidBodies;
};

// Natnet settings

#define MAX_NAMELENGTH              256

// NATNET message ids
#define NAT_PING                    0
#define NAT_PINGRESPONSE            1
#define NAT_REQUEST                 2
#define NAT_RESPONSE                3
#define NAT_REQUEST_MODELDEF        4
#define NAT_MODELDEF                5
#define NAT_REQUEST_FRAMEOFDATA     6
#define NAT_FRAMEOFDATA             7
#define NAT_MESSAGESTRING           8
#define NAT_UNRECOGNIZED_REQUEST    100
#define UNDEFINED                   999999.9999

#define MAX_PACKETSIZE				100000	// max size of packet (actual packet size is dynamic)

// sender
typedef struct
{
    char szName[MAX_NAMELENGTH];            // sending app's name
    unsigned char Version[4];               // sending app's version [major.minor.build.revision]
    unsigned char NatNetVersion[4];         // sending app's NatNet version [major.minor.build.revision]

} sSender;

typedef struct
{
    unsigned short iMessage;                // message ID (e.g. NAT_FRAMEOFDATA)
    unsigned short nDataBytes;              // Num bytes in payload
    union
    {
        unsigned char  cData[MAX_PACKETSIZE];
        char           szData[MAX_PACKETSIZE];
        unsigned long  lData[MAX_PACKETSIZE/4];
        float          fData[MAX_PACKETSIZE/4];
        sSender        Sender;
    } Data;                                 // Payload

} sPacket;

#define MULTICAST_ADDRESS		"239.255.42.99"     // IANA, local network
#define PORT_COMMAND            1510
#define PORT_DATA  			    1511

static std::string DATA_PORT = "1150";
static std::string CMD_PORT = "1151";

#endif //GAZEBOSC_NATNETDATATYPES_H
