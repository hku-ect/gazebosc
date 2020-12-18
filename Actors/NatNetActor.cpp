#include "NatNetActor.h"
#include <string>

const char * natnetCapabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"motive ip\"\n"
                                "        type = \"string\"\n"
                                "        value = \"192.168.10.30\"\n"
                                "        api_call = \"SET HOST\"\n"
                                "        api_value = \"s\"\n"           // optional picture format used in zsock_send
                                "    data\n"
                                "        name = \"sendTimeout\"\n"
                                "        type = \"int\"\n"
                                "        value = \"60\"\n"
                                "        api_call = \"SET TIMEOUT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
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
                                "        name = \"velocities\"\n"
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
                                "outputs\n"
                                "    output\n"
                                //TODO: Perhaps add NatNet output type so we can filter the data multiple times...
                                "        type = \"OSC\"\n";

zmsg_t * NatNet::handleMsg( sphactor_event_t * ev ) {
    if ( streq(ev->type, "INIT") ) {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(natnetCapabilities));

        //receive socket on port DATA_PORT
        //Test multicast group to receive data
        //TODO: Discover network interfaces, and have user select which to bind (drop-down)?
        std::string url = "udp://" + MULTICAST_ADDRESS + ":" + PORT_DATA_STR;
        DataSocket = zsock_new(ZMQ_DGRAM);
        //zsock_connect(DataSocket, "%s", url.c_str());
        zsock_bind(DataSocket, "%s", url.c_str());
        assert( DataSocket );
        dataFD = zsock_fd(DataSocket);
        int rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, DataSocket );
        assert(rc == 0);

        // receive socket on CMD_PORT
        url = "udp://*:"+ PORT_COMMAND_STR;
        CommandSocket = zsock_new_dgram(url.c_str());
        assert( CommandSocket );
        cmdFD = zsock_fd(CommandSocket);
        rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, CommandSocket );
        assert(rc == 0);
    }
    else if ( streq( ev->type, "DESTROY")) {
        delete this;
        zmsg_destroy(&ev->msg);
        return NULL;
    }
    else if ( streq(ev->type, "DESTROY") ) {
        if ( CommandSocket != NULL ) {
            sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, DataSocket);
            zsock_destroy(&CommandSocket);
            CommandSocket = NULL;
            cmdFD = -1;
        }
        if ( DataSocket != NULL ) {
            sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, DataSocket);
            zsock_destroy(&DataSocket);
            DataSocket = NULL;
            dataFD = -1;
        }

        return ev->msg;
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET HOST") ) {
                char * host_addr = zmsg_popstr(ev->msg);
                host = host_addr;
                zsys_info("SET HOST: %s", host_addr);
                zstr_free(&host_addr);

                // Say hello to our new host
                // send initial ping command
                sPacket PacketOut;
                PacketOut.iMessage = NAT_PING;
                PacketOut.nDataBytes = 0;
                int nTries = 3;
                while (nTries--)
                {
                    //int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
                    std::string url = host+":"+PORT_COMMAND_STR;
                    zstr_sendm(CommandSocket, url.c_str());
                    int rc = zsock_send(CommandSocket,  "b", (char*)&PacketOut, 4 + PacketOut.nDataBytes);
                    //if(rc != SOCKET_ERROR) {
                        zsys_info("Sent ping to %s", url.c_str());
                    //    break;
                    //}
                }
            }
            else if ( streq(cmd, "SET MARKERS") ) {
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
    else if ( streq(ev->type, "FDSOCK" ) )
    {
        zsys_info("GOT FDSOCK");

        // Get the socket...
        assert(ev->msg);
        zframe_t *frame = zmsg_pop(ev->msg);
        if (zframe_size(frame) == sizeof( void *) )
        {
            void *p = *(void **)zframe_data(frame);
            zsock_t* sock = (zsock_t*) zsock_resolve( p );
            if ( sock )
            {
                SOCKET sockFD = zsock_fd(sock);

                if ( sockFD == cmdFD ) {
                    zsys_info("CMD SOCKET");
                    // Parse command
                    int addr_len;
                    int nDataBytesReceived;
                    char str[256];
                    sPacket PacketIn;
                    addr_len = sizeof(struct sockaddr);

                    //int rc = zmq_recv(CommandSocket, (char *) &PacketIn, sizeof(sPacket), 0);
                    zmsg_t* zmsg = zmsg_recv(CommandSocket);
                    if ( zmsg ) {
                        zframe_t *zframe = zmsg_pop(zmsg);
                        if ( zframe ) {
                            HandleCommand((sPacket *) zframe_data(zframe));
                            zframe_destroy(&zframe);
                        }
                        zmsg_destroy(&zmsg);
                    }
                }
                else if ( sockFD == dataFD ) {
                    zsys_info("DATA SOCKET");

                    // Parse packet
                    zmsg_t* zmsg = zmsg_recv(DataSocket);
                    if ( zmsg ) {
                        zframe_t *zframe = zmsg_pop(zmsg);
                        if (zframe) {
                            Unpack((char *) zframe_data(zframe));
                            zframe_destroy(&zframe);
                        }
                        zmsg_destroy(&zmsg);
                    }

                    zmsg_destroy(&ev->msg);
                    return NULL;
                }
            }
        }
        else
            zsys_error("args is not a zsock instance");

        zmsg_destroy(&ev->msg);
        zframe_destroy(&frame);

        return NULL;
    }
    else if ( streq( ev->type, "TIME")) {

        //set to false if number of skels/rb's needs updating
        bool rigidbodiesReady = true;
        bool skeletonsReady = true;

        // if there is a difference do natnet.sendRequestDescription(); to get up to date rigidbodie descriptions and thus names
        if (rigidbody_descs.size() != rigidbodies.size())
        {
            if (sentRequest <= 0) {
                sendRequestDescription();
                sentRequest = 60; //1 second
            }
            rigidbodiesReady = false;
        }

        //get & check skeletons size
        if (skeleton_descs.size() != skeletons.size())
        {
            if (sentRequest <= 0) {
                sendRequestDescription();
                sentRequest = 60; //1 second
            }
            skeletonsReady = false;
        }

        if (sentRequest > 0) sentRequest--;

        zmsg_t* oscMsg = zmsg_new();

        //markers
        for (int i = 0; i < markers.size(); i++)
        {
            zosc_t * osc = zosc_create("/marker", "ifff", i, markers[i][0], markers[i][1], markers[i][2]);
            zmsg_add(oscMsg, zosc_pack(osc));
            //TODO: clean up osc* ?
        }

        //rigidbodies
        addRigidbodies(oscMsg);

        //skeletons
        //TODO: implement skeletons (requires zosc_append)
        addSkeletons(oscMsg);

        //TODO: add vivetrackers data
        if ( zmsg_content_size(oscMsg) != 0 ){
            //zsys_info("NatNet: sending data");
            zmsg_destroy(&ev->msg);
            return oscMsg;
        }
        else {
            //zsys_info("NatNet: nothing to send");
            zmsg_destroy(&oscMsg);
        }
    }

    zmsg_destroy(&ev->msg);
    return NULL;
}

void NatNet::addRigidbodies(zmsg_t *zmsg)
{
    for (int i = 0; i < rigidbodies.size(); i++)
    {
        const RigidBody &RB = rigidbodies[i];

        // Decompose to get the different elements
        Vec3 position = RB.position;
        Vec4 rotation = RB.rotation;

        //we're going to fetch or create this
        //TODO: re-implement rigidbody histories for velocity data
        /*
        RigidBodyHistory *rb;

        //Get or create rigidbodyhistory
        bool found = false;
        for( int r = 0; r < rbHistory.size(); ++r )
        {
            if ( rbHistory[r].rigidBodyId == rbd[i].id )
            {
                rb = &rbHistory[r];
                found = true;
            }
        }

        if ( !found )
        {
            rb = new RigidBodyHistory( rbd[i].id, position, rotation );
            rbHistory.push_back(*rb);
        }

        Vec3 velocity;
        Vec3 angularVelocity;

        if ( rb->firstRun == TRUE )
        {
            rb->currentDataPoint = 0;
            rb->firstRun = FALSE;
        }
        else
        {
            if ( rb->currentDataPoint < 2 * SMOOTHING + 1 )
            {
                rb->velocities[rb->currentDataPoint] = ( position - rb->previousPosition ) * invFPS;

                ofVec3f diff = ( rb->previousOrientation * rotation.inverse() ).getEuler();
                rb->angularVelocities[rb->currentDataPoint] = ( diff * invFPS );

                rb->currentDataPoint++;
            }
            else
            {
                int count = 0;
                int maxDist = SMOOTHING;
                ofVec3f totalVelocity;
                ofVec3f totalAngularVelocity;
                //calculate smoothed velocity
                for( int x = 0; x < SMOOTHING * 2 + 1; ++x )
                {
                    //calculate integer distance from "center"
                    //above - maxDist = influence of data point
                    int dist = abs( x - SMOOTHING );
                    int infl = ( maxDist - dist ) + 1;

                    //add all
                    totalVelocity += rb->velocities[x] * infl;
                    totalAngularVelocity += rb->angularVelocities[x] * infl;
                    //count "influences"
                    count += infl;
                }

                //divide by total data point influences
                velocity = totalVelocity / count;
                angularVelocity = totalAngularVelocity / count;

                for( int x = 0; x < rb->currentDataPoint - 1; ++x )
                {
                    rb->velocities[x] = rb->velocities[x+1];
                    rb->angularVelocities[x] = rb->angularVelocities[x+1];
                }
                rb->velocities[rb->currentDataPoint-1] = ( position - rb->previousPosition ) * invFPS;

                ofVec3f diff = ( rb->previousOrientation * rotation.inverse() ).getEuler();
                rb->angularVelocities[rb->currentDataPoint-1] = ( diff * invFPS );
            }

            rb->previousPosition = position;
            rb->previousOrientation = rotation;
        }
        */

        //TODO: support hierarchy mode
        zosc_t * oscMsg = zosc_create("/rigidbody", "isfffffffi",
                                        RB.id,
                                        rigidbody_descs[i].name.c_str(),
                                        position[0],
                                        position[1],
                                        position[2],
                                        rotation[0],
                                        rotation[1],
                                        rotation[2],
                                        rotation[3],
                                        ( RB.isActive() ? 1 : 0 )
                                        );

        //TODO: Velocity add
        /*
        if ( c->getModeFlags() & ClientFlag_Velocity )
        {
            //velocity over SMOOTHING * 2 + 1 frames
            m.addFloatArg(velocity.x * 1000);
            m.addFloatArg(velocity.y * 1000);
            m.addFloatArg(velocity.z * 1000);
            //angular velocity (euler), also smoothed
            m.addFloatArg(angularVelocity.x * 1000);
            m.addFloatArg(angularVelocity.y * 1000);
            m.addFloatArg(angularVelocity.z * 1000);
        }
        */

        zmsg_add(zmsg, zosc_pack(oscMsg));
    }
}

void NatNet::addSkeletons(zmsg_t *zmsg)
{
    for (int j = 0; j < skeletons.size(); j++)
    {
        const Skeleton &S = skeletons[j];
        std::vector<RigidBodyDescription> rbd = skeleton_descs[j].joints;

        //TODO: Hierarchy mode
        /*if ( c->getHierarchy())
        {
            for (int i = 0; i < S.joints.size(); i++)
            {
                const ofxNatNet::RigidBody &RB = S.joints[i];

                ofxOscMessage m;
                m.setAddress("/skeleton/" + ofToString(sd[j].name) + "/" +
                             ofToString(ofToString(rbd[i].name)));

                // Get the matirx
                ofMatrix4x4 matrix = RB.matrix;

                // Decompose to get the different elements
                ofVec3f position;
                ofQuaternion rotation;
                ofVec3f scale;
                ofQuaternion so;
                matrix.decompose(position, rotation, scale, so);
                m.addStringArg(ofToString(rbd[i].name));
                m.addFloatArg(position.x);
                m.addFloatArg(position.y);
                m.addFloatArg(position.z);
                m.addFloatArg(rotation.x());
                m.addFloatArg(rotation.y());
                m.addFloatArg(rotation.z());
                m.addFloatArg(rotation.w());
                //needed for skeleton retargeting
                if ( c->getModeFlags() & ClientFlag_FullSkeleton )
                {
                    m.addIntArg(rbd[i].parent_id);
                    m.addFloatArg(rbd[i].offset.x);
                    m.addFloatArg(rbd[i].offset.y);
                    m.addFloatArg(rbd[i].offset.z);
                    //TODO: Figure out if this is needed. It's obvious for rigidbodies,
                    //          but skeletons don't have an "isActive" as a totality
                    //m.addBoolArg(RB.isActive());
                }

                bundle->addMessage(m);
            }
        }
        else
        */
        {
            //TODO: Support append for zosc messages
            /*
            ofxOscMessage m;
            m.setAddress("/skeleton");
            m.addStringArg(ofToString(sd[j].name));
            m.addIntArg(S.id);

            for (int i = 0; i < S.joints.size(); i++)
            {
                const ofxNatNet::RigidBody &RB = S.joints[i];

                // Get the matirx
                ofMatrix4x4 matrix = RB.matrix;

                // Decompose to get the different elements
                ofVec3f position;
                ofQuaternion rotation;
                ofVec3f scale;
                ofQuaternion so;
                matrix.decompose(position, rotation, scale, so);
                m.addStringArg(ofToString(rbd[i].name));
                m.addFloatArg(position.x);
                m.addFloatArg(position.y);
                m.addFloatArg(position.z);
                m.addFloatArg(rotation.x());
                m.addFloatArg(rotation.y());
                m.addFloatArg(rotation.z());
                m.addFloatArg(rotation.w());
                //needed for skeleton retargeting
                if ( c->getModeFlags() & ClientFlag_FullSkeleton )
                {
                    m.addIntArg(rbd[i].parent_id);
                    m.addFloatArg(rbd[i].offset.x);
                    m.addFloatArg(rbd[i].offset.y);
                    m.addFloatArg(rbd[i].offset.z);
                }
            }

            bundle->addMessage(m);
            */
        }
    }
}

void NatNet::fixRanges( Vec3 *euler )
{
    if ((*euler)[0] < -180.0f)
        (*euler)[0] += 360.0f;
    else if ((*euler)[0] > 180.0f)
        (*euler)[0] -= 360.0f;

    if ((*euler)[1] < -180.0f)
        (*euler)[1] += 360.0f;
    else if ((*euler)[1] > 180.0f)
        (*euler)[1] -= 360.0f;

    if ((*euler)[2] < -180.0f)
        (*euler)[2] += 360.0f;
    else if ((*euler)[2] > 180.0f)
        (*euler)[2] -= 360.0f;
}

// NatNet implementation
bool NatNet::IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address) {
    return false;
}

bool DecodeTimecode(unsigned int inTimecode, unsigned int inTimecodeSubframe, int* hour, int* minute, int* second, int* frame, int* subframe)
{
    bool bValid = true;

    *hour = (inTimecode>>24)&255;
    *minute = (inTimecode>>16)&255;
    *second = (inTimecode>>8)&255;
    *frame = inTimecode&255;
    *subframe = inTimecodeSubframe;

    return bValid;
}

bool TimecodeStringify(unsigned int inTimecode, unsigned int inTimecodeSubframe, char *Buffer, int BufferSize)
{
    bool bValid;
    int hour, minute, second, frame, subframe;
    bValid = DecodeTimecode(inTimecode, inTimecodeSubframe, &hour, &minute, &second, &frame, &subframe);

    zsys_sprintf(Buffer,BufferSize,"%2d:%2d:%2d:%2d.%d",hour, minute, second, frame, subframe);
    for(unsigned int i=0; i<strlen(Buffer); i++)
        if(Buffer[i]==' ')
            Buffer[i]='0';

    return bValid;
}

char* NatNet::unpackMarkerSet(char* ptr, std::vector<Marker>& ref_markers)
{
    int nMarkers = 0;
    memcpy(&nMarkers, ptr, 4);
    ptr += 4;

    ref_markers.resize(nMarkers);

    for (int j = 0; j < nMarkers; j++)
    {
        Marker p;
        memcpy(&p[0], ptr, 4);
        ptr += 4;
        memcpy(&p[1], ptr, 4);
        ptr += 4;
        memcpy(&p[2], ptr, 4);
        ptr += 4;

        //TODO: Matrix
        //p = transform.preMult(p);

        ref_markers[j] = p;
    }

    return ptr;
}

char* NatNet::unpackRigidBodies(char* ptr, std::vector<RigidBody>& ref_rigidbodies)
{
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];
    //it was --> int major = NatNetVersion[0];
    //it was --> int minor = NatNetVersion[1];

    //TODO: Matrix, ofQuaternion?
    //ofQuaternion rot = transform.getRotate();

    int nRigidBodies = 0;
    memcpy(&nRigidBodies, ptr, 4);
    ptr += 4;

    ref_rigidbodies.resize(nRigidBodies);

    for (int j = 0; j < nRigidBodies; j++)
    {
        RigidBody& RB = ref_rigidbodies[j];

        Vec3 pp;
        Vec4 q;

        int ID = 0;
        memcpy(&ID, ptr, 4);
        ptr += 4;

        memcpy(&pp[0], ptr, 4);
        ptr += 4;

        memcpy(&pp[1], ptr, 4);
        ptr += 4;

        memcpy(&pp[2], ptr, 4);
        ptr += 4;

        memcpy(&q[0], ptr, 4);
        ptr += 4;

        memcpy(&q[1], ptr, 4);
        ptr += 4;

        memcpy(&q[2], ptr, 4);
        ptr += 4;

        memcpy(&q[3], ptr, 4);
        ptr += 4;

        RB.id = ID;
        RB.position = pp;
        RB.rotation = q;

        //TODO: Matrix, what does preMult do? Is it even necessary?
        //pp = transform.preMult(pp);
        //
        //ofMatrix4x4 mat;
        //mat.setTranslation(pp);
        //mat.setRotate(q * rot);
        //RB.matrix = mat;

        // associated marker positions
        int nRigidMarkers = 0;
        memcpy(&nRigidMarkers, ptr, 4);
        ptr += 4;

        int nBytes = nRigidMarkers * 3 * sizeof(float);
        float* markerData = (float*)malloc(nBytes);
        memcpy(markerData, ptr, nBytes);
        ptr += nBytes;

        if (major >= 2)
        {
            // associated marker IDs
            nBytes = nRigidMarkers * sizeof(int);
            ptr += nBytes;

            // associated marker sizes
            nBytes = nRigidMarkers * sizeof(float);
            ptr += nBytes;
        }

        RB.markers.resize(nRigidMarkers);

        for (int k = 0; k < nRigidMarkers; k++)
        {
            float x = markerData[k * 3];
            float y = markerData[k * 3 + 1];
            float z = markerData[k * 3 + 2];

            Vec3 pp(x, y, z);
            //TODO: Matrix?
            //pp = transform.preMult(pp);
            RB.markers[k] = pp;
        }

        if (markerData) free(markerData);

        if (major >= 2)
        {
            // Mean marker error
            float fError = 0.0f;
            memcpy(&fError, ptr, 4);
            ptr += 4;

            RB.mean_marker_error = fError;
            RB._active = RB.mean_marker_error > 0;
        } else {
            RB.mean_marker_error = 0;
        }

        // 2.6 and later
        if (((major == 2) && (minor >= 6)) || (major > 2) || (major == 0))
        {
            // params
            short params = 0; memcpy(&params, ptr, 2); ptr += 2;
            bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
        }

    }  // next rigid body

    return ptr;
}

void NatNet::Unpack( char * pData ) {
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];

    //TODO: Matrix?
    //Vec4 rotation = transform.getRotate();

    char *ptr = pData;

    //zsys_info("Begin Packet\n-------\n");

    // message ID
    int MessageID = 0;
    memcpy(&MessageID, ptr, 2); ptr += 2;
    //zsys_info("Message ID : %d\n", MessageID);

    // size
    int nBytes = 0;
    memcpy(&nBytes, ptr, 2); ptr += 2;
    //zsys_info("Byte count : %d\n", nBytes);

    if(MessageID == 7)      // FRAME OF MOCAP DATA packet
    {
        // temporary data containers
        std::vector<std::vector<Marker> > tmp_markers_set;
        std::vector<Skeleton> tmp_skeletons;
        std::vector<Marker> tmp_markers;
        std::vector<Marker> tmp_filtered_markers;
        std::vector<RigidBody> tmp_rigidbodies;

        // frame number
        int frameNumber = 0; memcpy(&frameNumber, ptr, 4); ptr += 4;
        //zsys_info("Frame # : %d\n", frameNumber);

        // number of data sets (markersets, rigidbodies, etc)
        int nMarkerSets = 0; memcpy(&nMarkerSets, ptr, 4); ptr += 4;
        //zsys_info("Marker Set Count : %d\n", nMarkerSets);

        tmp_markers_set.resize(nMarkerSets);

        for (int i=0; i < nMarkerSets; i++)
        {
            // Markerset name
            char szName[256];
            strcpy(szName, ptr);
            int nDataBytes = (int) strlen(szName) + 1;
            ptr += nDataBytes;
            //zsys_info("Model Name: %s\n", szName);

            ptr = unpackMarkerSet(ptr, tmp_markers_set[i]);
        }

        // unidentified markers
        ptr = unpackMarkerSet(ptr, tmp_markers);

        // rigid bodies
        ptr = unpackRigidBodies(ptr, tmp_rigidbodies);

        // skeletons (version 2.1 and later)
        if( ((major == 2)&&(minor>0)) || (major>2))
        {
            int nSkeletons = 0;
            memcpy(&nSkeletons, ptr, 4); ptr += 4;
            //zsys_info("Skeleton Count : %d\n", nSkeletons);

            tmp_skeletons.resize(nSkeletons);

            for (int j=0; j < nSkeletons; j++)
            {
                // skeleton id
                int skeletonID = 0;
                memcpy(&skeletonID, ptr, 4); ptr += 4;

                tmp_skeletons[j].id = skeletonID;

                ptr = unpackRigidBodies(ptr, tmp_skeletons[j].joints);
            } // next skeleton
        }

        // labeled markers (version 2.3 and later)
        if( ((major == 2)&&(minor>=3)) || (major>2))
        {
            int nLabeledMarkers = 0;
            memcpy(&nLabeledMarkers, ptr, 4); ptr += 4;
            //zsys_info("Labeled Marker Count : %d\n", nLabeledMarkers);
            for (int j=0; j < nLabeledMarkers; j++)
            {
                // id
                int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                // x
                float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
                // y
                float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
                // z
                float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
                // size
                float size = 0.0f; memcpy(&size, ptr, 4); ptr += 4;

                // 2.6 and later
                if( ((major == 2)&&(minor >= 6)) || (major > 2) || (major == 0) )
                {
                    // marker params
                    short params = 0; memcpy(&params, ptr, 2); ptr += 2;
                    bool bOccluded = params & 0x01;     // marker was not visible (occluded) in this frame
                    bool bPCSolved = params & 0x02;     // position provided by point cloud solve
                    bool bModelSolved = params & 0x04;  // position provided by model solve
                }

                //zsys_info("ID  : %d\n", ID);
                //zsys_info("pos : [%3.2f,%3.2f,%3.2f]\n", x,y,z);
                //zsys_info("size: [%3.2f]\n", size);

                Vec3 pp(x, y, z);
                //TODO: Matrix?
                //pp = transform.preMult(pp);
                tmp_markers.push_back(pp);
            }
        }

        // Force Plate data (version 2.9 and later)
        if (((major == 2) && (minor >= 9)) || (major > 2))
        {
            int nForcePlates;
            memcpy(&nForcePlates, ptr, 4); ptr += 4;
            for (int iForcePlate = 0; iForcePlate < nForcePlates; iForcePlate++)
            {
                // ID
                int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                //zsys_info("Force Plate : %d\n", ID);

                // Channel Count
                int nChannels = 0; memcpy(&nChannels, ptr, 4); ptr += 4;

                // Channel Data
                for (int i = 0; i < nChannels; i++)
                {
                    //zsys_info(" Channel %d : ", i);
                    int nFrames = 0; memcpy(&nFrames, ptr, 4); ptr += 4;
                    for (int j = 0; j < nFrames; j++)
                    {
                        float val = 0.0f;  memcpy(&val, ptr, 4); ptr += 4;
                        //zsys_info("%3.2f   ", val);
                    }
                    //zsys_info("\n");
                }
            }
        }

        // latency
        float latency = 0.0f; memcpy(&latency, ptr, 4);	ptr += 4;
        //zsys_info("latency : %3.3f\n", latency);
        this->latency = latency;

        // timecode
        unsigned int timecode = 0; 	memcpy(&timecode, ptr, 4);	ptr += 4;
        unsigned int timecodeSub = 0; memcpy(&timecodeSub, ptr, 4); ptr += 4;
        char szTimecode[128] = "";
        TimecodeStringify(timecode, timecodeSub, szTimecode, 128);

        // timestamp
        double timestamp = 0.0f;
        // 2.7 and later - increased from single to double precision
        if( ((major == 2)&&(minor>=7)) || (major>2))
        {
            memcpy(&timestamp, ptr, 8); ptr += 8;
        }
        else
        {
            float fTemp = 0.0f;
            memcpy(&fTemp, ptr, 4); ptr += 4;
            timestamp = (double)fTemp;
        }

        // frame params
        short params = 0;  memcpy(&params, ptr, 2); ptr += 2;
        bool bIsRecording = params & 0x01;                  // 0x01 Motive is recording
        bool bTrackedModelsChanged = params & 0x02;         // 0x02 Actively tracked model list has changed


        // end of data tag
        int eod = 0; memcpy(&eod, ptr, 4); ptr += 4;
        //zsys_info("End Packet\n-------------\n");

        // filter markers
        // TODO: re-implement marker filtering (?)
        /*
        if (duplicated_point_removal_distance > 0)
        {
            std::map<int, RigidBody>::iterator it =
                    this->rigidbodies.begin();
            while (it != this->rigidbodies.end())
            {
                RigidBody& RB = it->second;

                for (int i = 0; i < RB.markers.size(); i++)
                {
                    Vec3& v = RB.markers[i];
                    std::vector<Marker>::iterator it = remove_if(
                            tmp_filtered_markers.begin(), tmp_filtered_markers.end(),
                            remove_dups(v, duplicated_point_removal_distance));
                    tmp_filtered_markers.erase(it, tmp_filtered_markers.end());
                }

                it++;
            }
        }
         */

        //Copy data to instance...
        this->latency = latency;
        this->frame_number = frameNumber;
        this->markers_set = tmp_markers_set;
        this->markers = tmp_markers;
        this->filtered_markers = tmp_filtered_markers;
        this->rigidbodies_arr = tmp_rigidbodies;
        this->skeletons_arr = tmp_skeletons;

        // fill the rigidbodies map
        {
            for (int i = 0; i < tmp_rigidbodies.size(); i++) {
                RigidBody &RB = tmp_rigidbodies[i];
                RigidBody &tRB = this->rigidbodies[RB.id];
                tRB = RB;
            }
        }
        {
            for (int i = 0; i < tmp_skeletons.size(); i++) {
                Skeleton &S = tmp_skeletons[i];
                Skeleton &tS = this->skeletons[S.id];
                tS = S;
            }
        }
    }
    else if(MessageID == 5) // Data Descriptions
    {
        std::vector<RigidBodyDescription> tmp_rigidbody_descs;
        std::vector<SkeletonDescription> tmp_skeleton_descs;
        std::vector<MarkerSetDescription> tmp_markerset_descs;

        // number of datasets
        int nDatasets = 0; memcpy(&nDatasets, ptr, 4); ptr += 4;
        //zsys_info("Dataset Count : %d\n", nDatasets);

        for(int i=0; i < nDatasets; i++)
        {
            //zsys_info("Dataset %d\n", i);

            int type = 0; memcpy(&type, ptr, 4); ptr += 4;
            //zsys_info("Type : %d\n", i, type);

            if(type == 0)   // markerset
            {
                MarkerSetDescription description;

                // name
                char szName[256];
                strcpy(szName, ptr);
                int nDataBytes = (int) strlen(szName) + 1;
                ptr += nDataBytes;
                //zsys_info("Markerset Name: %s\n", szName);
                description.name = szName;

                // marker data
                int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;
                //zsys_info("Marker Count : %d\n", nMarkers);

                for(int j=0; j < nMarkers; j++)
                {
                    char szName[256];
                    strcpy(szName, ptr);
                    int nDataBytes = (int) strlen(szName) + 1;
                    ptr += nDataBytes;
                    //zsys_info("Marker Name: %s\n", szName);
                    description.marker_names.push_back(szName);
                }
                tmp_markerset_descs.push_back(description);
            }
            else if(type ==1)   // rigid body
            {
                RigidBodyDescription description;

                if(major >= 2)
                {
                    // name
                    char szName[MAX_NAMELENGTH];
                    strcpy(szName, ptr);
                    ptr += strlen(ptr) + 1;
                    //zsys_info("Name: %s\n", szName);
                    description.name = szName;
                }

                int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                //zsys_info("ID : %d\n", ID);
                description.id = ID;

                int parentID = 0; memcpy(&parentID, ptr, 4); ptr +=4;
                //zsys_info("Parent ID : %d\n", parentID);
                description.parent_id = parentID;

                float xoffset = 0; memcpy(&xoffset, ptr, 4); ptr +=4;
                //zsys_info("X Offset : %3.2f\n", xoffset);

                float yoffset = 0; memcpy(&yoffset, ptr, 4); ptr +=4;
                //zsys_info("Y Offset : %3.2f\n", yoffset);

                float zoffset = 0; memcpy(&zoffset, ptr, 4); ptr +=4;
                //zsys_info("Z Offset : %3.2f\n", zoffset);

                description.offset[0] = xoffset;
                description.offset[1] = xoffset;
                description.offset[2] = xoffset;

                tmp_rigidbody_descs.push_back(description);
            }
            else if(type ==2)   // skeleton
            {
                SkeletonDescription description;

                char szName[MAX_NAMELENGTH];
                strcpy(szName, ptr);
                ptr += strlen(ptr) + 1;
                //zsys_info("Name: %s\n", szName);
                description.name = szName;

                int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                //zsys_info("ID : %d\n", ID);
                description.id = ID;

                int nRigidBodies = 0; memcpy(&nRigidBodies, ptr, 4); ptr +=4;
                //zsys_info("RigidBody (Bone) Count : %d\n", nRigidBodies);
                description.joints.resize(nRigidBodies);

                for(int i=0; i< nRigidBodies; i++)
                {
                    if(major >= 2)
                    {
                        // RB name
                        char szName[MAX_NAMELENGTH];
                        strcpy(szName, ptr);
                        ptr += strlen(ptr) + 1;
                        //zsys_info("Rigid Body Name: %s\n", szName);
                        description.joints[i].name = szName;
                    }

                    int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                    //zsys_info("RigidBody ID : %d\n", ID);
                    description.joints[i].id = ID;

                    int parentID = 0; memcpy(&parentID, ptr, 4); ptr +=4;
                    //zsys_info("Parent ID : %d\n", parentID);
                    description.joints[i].parent_id = parentID;

                    float xoffset = 0; memcpy(&xoffset, ptr, 4); ptr +=4;
                    //zsys_info("X Offset : %3.2f\n", xoffset);

                    float yoffset = 0; memcpy(&yoffset, ptr, 4); ptr +=4;
                    //zsys_info("Y Offset : %3.2f\n", yoffset);

                    float zoffset = 0; memcpy(&zoffset, ptr, 4); ptr +=4;
                    //zsys_info("Z Offset : %3.2f\n", zoffset);

                    description.joints[i].offset[0] = xoffset;
                    description.joints[i].offset[0] = yoffset;
                    description.joints[i].offset[0] = zoffset;
                }
                tmp_skeleton_descs.push_back(description);
            }

        }   // next dataset

        //zsys_info("End Packet\n-------------\n");

        //Store data
        this->markerset_descs = tmp_markerset_descs;
        this->rigidbody_descs = tmp_rigidbody_descs;
        this->skeleton_descs = tmp_skeleton_descs;
    }
    else
    {
        zsys_info("Unrecognized Packet Type.\n");
    }
}

int NatNet::GetLocalIPAddresses(unsigned long Addresses[], int nMax) {
    return 0;
}

void NatNet::sendRequestDescription() {
    sPacket packet;
    packet.iMessage = NAT_REQUEST_MODELDEF;
    packet.nDataBytes = 0;

    std::string url = host+":"+PORT_COMMAND_STR;
    zstr_sendm(CommandSocket, url.c_str());
    int rc = zsock_send(CommandSocket,  "b", (char*)&packet, 4 + packet.nDataBytes);
    if(rc != 0)
    {
        zsys_info("Socket error sending command: NAT_REQUEST_MODELDEF");
    }
}

int NatNet::SendCommand(char* szCommand) {
    // reset global result
    gCommandResponse = -1;

    // format command packet
    sPacket commandPacket;
    strcpy(commandPacket.Data.szData, szCommand);
    commandPacket.iMessage = NAT_REQUEST;
    commandPacket.nDataBytes = (int)strlen(commandPacket.Data.szData) + 1;

    //TODO: Check if replacement works...
    //int iRet = sendto(CommandSocket, (char *)&commandPacket, 4 + commandPacket.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
    std::string url = host+":"+PORT_COMMAND_STR;
    zstr_sendm(CommandSocket, url.c_str());
    int rc = zsock_send(CommandSocket,  "b", (char*)&commandPacket, 4 + commandPacket.nDataBytes);
    if(rc != 0)
    {
        zsys_info("Socket error sending command: %s", szCommand);
    }

    return gCommandResponse;
}

void NatNet::HandleCommand( sPacket *PacketIn ) {
    zsys_info("Message ID: %i", PacketIn->iMessage);
    // handle command
    switch (PacketIn->iMessage)
    {
        case NAT_MODELDEF:
            Unpack((char*)&PacketIn);
            break;
        case NAT_FRAMEOFDATA:
            Unpack((char*)&PacketIn);
            break;
        case NAT_PINGRESPONSE:
            for(int i=0; i<4; i++)
            {
                NatNetVersion[i] = (int)PacketIn->Data.Sender.NatNetVersion[i];
                ServerVersion[i] = (int)PacketIn->Data.Sender.Version[i];
            }
            break;
        case NAT_RESPONSE:
            gCommandResponseSize = PacketIn->nDataBytes;
            if(gCommandResponseSize==4)
                memcpy(&gCommandResponse, &PacketIn->Data.lData[0], gCommandResponseSize);
            else
            {
                memcpy(&gCommandResponseString[0], &PacketIn->Data.cData[0], gCommandResponseSize);
                zsys_info("Response : %s", gCommandResponseString);
                gCommandResponse = 0;   // ok
            }
            break;
        case NAT_UNRECOGNIZED_REQUEST:
            zsys_info("[Client] received 'unrecognized request'\n");
            gCommandResponseSize = 0;
            gCommandResponse = 1;       // err
            break;
        case NAT_MESSAGESTRING:
            zsys_info("[Client] Received message: %s\n", PacketIn->Data.szData);
            break;
    }
}