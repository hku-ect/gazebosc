#ifndef GAZEBOSC_NATNETDATATYPES_H
#define GAZEBOSC_NATNETDATATYPES_H

#include <string>
#include <vector>

// Custom data structs
struct Vec3 {
    float p[3] = {0,0,0};
    float& operator [](int id) {
        return p[id];
    }

    Vec3()= default;

    Vec3(float x, float y, float z) {
        p[0] = x;
        p[1] = y;
        p[2] = z;
    }
};

struct Vec4 {
    float p[4] = {0,0,0,0};
    float& operator [](int id) {
        return p[id];
    }
};

typedef Vec3 Marker;

struct RigidBody
{
    int id;

    //TODO: matrix...?
    //ofMatrix4x4 matrix;
    Vec3 position;
    Vec4 rotation;

    std::vector<Marker> markers;

    float mean_marker_error;

    inline bool isActive() const { return _active; }
    //TODO: matrix...?
    //const ofMatrix4x4& getMatrix() const { return matrix; }

    bool _active;
};

struct Skeleton
{
    int id;
    std::vector<RigidBody> joints;
};

struct RigidBodyDescription
{
    std::string name;
    int id;
    int parent_id;
    Vec3 offset;
    std::vector<std::string> marker_names;
};

struct SkeletonDescription
{
    std::string name;
    int id;
    std::vector<RigidBodyDescription> joints;
};

struct MarkerSetDescription
{
    std::string name;
    std::vector<std::string> marker_names;
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

//#define MULTICAST_ADDRESS		"239.255.42.99"     // IANA, local network
#define PORT_COMMAND            1510
#define PORT_DATA  			    1511

static std::string MULTICAST_ADDRESS = "239.255.42.99";
static std::string PORT_COMMAND_STR = "1510";
static std::string PORT_DATA_STR = "1511";

#endif //GAZEBOSC_NATNETDATATYPES_H
