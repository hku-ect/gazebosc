#ifndef GAZEBOSC_NATNETDATATYPES_H
#define GAZEBOSC_NATNETDATATYPES_H

#include <string>
#include <vector>
#include "../ext/glm/glm/glm.hpp"
#include "../ext/glm/glm/ext.hpp"
#include "../ext/glm/glm/gtc/quaternion.hpp"

// Custom data structs

typedef glm::vec3 Marker;

struct RigidBody
{
    int id;

    //TODO: matrix...?
    //ofMatrix4x4 matrix;
    glm::vec3 position;
    glm::vec4 rotation;

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
    glm::vec3 offset;
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

struct RigidBodyHistory {
public:
    int rigidBodyId;
    glm::vec3 previousPosition;
    glm::quat previousOrientation;

    std::vector<glm::vec3> velocities;
    std::vector<glm::vec3> angularVelocities;

    bool firstRun = TRUE;
    int currentDataPoint = 0;
    int framesInactive = 0;

    RigidBodyHistory( int id, glm::vec3 p, glm::quat r )
    {
        previousPosition = p;
        previousOrientation = r;
        rigidBodyId = id;

        currentDataPoint = 0;
        firstRun = TRUE;
        framesInactive = 0;
    }
};

struct remove_dups
{
    glm::vec3 v;
    float dist;

    remove_dups(const glm::vec3& v, float dist)
            : v(v)
            , dist(dist)
    {
    }

    bool operator()(const glm::vec3& t) { return glm::distance(v, t) <= dist; }
};

// Natnet settings

#define SMOOTHING                   2

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
