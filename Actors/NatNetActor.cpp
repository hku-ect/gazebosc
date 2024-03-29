#include "NatNetActor.h"
#include <string>
#include <algorithm>
#include <time.h>

//static variable definitions
int* NatNet::NatNetVersion = new int[4]{0,0,0,0};
int* NatNet::ServerVersion = new int[4]{0,0,0,0};
std::vector<RigidBodyDescription> NatNet::rigidbody_descs;
std::vector<SkeletonDescription> NatNet::skeleton_descs;
std::vector<MarkerSetDescription> NatNet::markerset_descs;
std::mutex NatNet::desc_mutex;

const char * NatNet::capabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"motive_ip\"\n"
                                "        type = \"string\"\n"
                                "        help = \"Ipaddress of the host running Motive\"\n"
                                "        value = \"192.168.10.30\"\n"
                                "        api_call = \"SET HOST\"\n"
                                "        api_value = \"s\"\n"           // optional picture format used in zsock_send
                                "    data\n"
                                "        name = \"network_interface\"\n"
                                "        type = \"int\"\n"
                                "        help = \"Select the interface connected to the motive host\"\n"
                                "        value = \"0\"\n"
                                "        api_call = \"SET INTERFACE\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "    data\n"
                                "        name = \"sendTimeout\"\n"
                                "        type = \"int\"\n"
                                "        help = \"Timeout when waiting for a Motive reply\"\n"
                                "        value = \"60\"\n"
                                "        api_call = \"SET TIMEOUT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "        min = 1\n"
                                "    data\n"
                                "        name = \"Reset\"\n"
                                "        type = \"trigger\"\n"
                                "        help = \"Re-retrieve the motive definitions\"\n"
                                "        api_call = \"RESET\"\n"
                                "outputs\n"
                                "    output\n"
                                //TODO: Perhaps add NatNet output type so we can filter the data multiple times...
                                "        type = \"NatNet\"\n";

zmsg_t * NatNet::handleInit( sphactor_event_t * ev )
{
    // generate a list of availbale interfaces
    ziflist_t * ifList = ziflist_new();
    const char* cur = ziflist_first(ifList);
    while( cur != nullptr ) {
        ifNames.push_back(cur);
        ifAddresses.push_back(ziflist_address(ifList));

        cur = ziflist_next(ifList);
    }
    ziflist_destroy(&ifList);

    //build report
    SetReport(ev->actor);

    if ( ifNames.size() == 0 ) {
        zsys_info("Error: no available interfaces");
        return nullptr;
    }

    // initially we use the first interface
    activeInterface = ifNames[0];

    // Setup the receive socket on port DATA_PORT
    std::string url = "udp://" + activeInterface + ";" + MULTICAST_ADDRESS + ":" + PORT_DATA_STR;
    DataSocket = zsock_new(ZMQ_DGRAM);
    assert( DataSocket );
    //zsock_connect(DataSocket, "%s", url.c_str());
    int rc = zsock_bind(DataSocket, "%s", url.c_str());
    assert( rc != -1 );
    rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, DataSocket );
    assert(rc == 0);


    url = "udp://*:*";
    CommandSocket = zsock_new_dgram(url.c_str());
    assert( CommandSocket );
    rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, CommandSocket );
    assert(rc == 0);

    return NULL;
}

zmsg_t * NatNet::handleStop( sphactor_event_t * ev )
{
    if ( CommandSocket != NULL ) {
        sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, CommandSocket);
        zsock_destroy(&CommandSocket);
        CommandSocket = NULL;
    }
    if ( DataSocket != NULL ) {
        sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, DataSocket);
        zsock_destroy(&DataSocket);
        DataSocket = NULL;
    }

    return NULL;
}

zmsg_t * NatNet::handleTimer( sphactor_event_t * ev )
{
    zmsg_destroy(&ev->msg);

    if ( lastData != nullptr ) {
        zmsg_t * returnMsg = lastData;
        lastData = nullptr;
        return returnMsg;
    }

    return nullptr;
}

zmsg_t * NatNet::handleAPI( sphactor_event_t * ev )
{
    //pop msg for command
    char * cmd = zmsg_popstr(ev->msg);
    if (cmd) {
        if ( streq(cmd, "RESET") ) {
            this->rigidbodies.clear();
            this->skeletons.clear();
            this->rigidbodiesReady = false;
            this->skeletonsReady = false;
            this->sentRequest = 0;

            // re-send ping as well to force response packet
            SendPing();
        }
        if ( streq(cmd, "SET HOST") ) {
            char *host_addr = zmsg_popstr(ev->msg);
            host = host_addr;
            zsys_info("SET HOST: %s", host_addr);
            zstr_free(&host_addr);

            if ( CommandSocket != nullptr ) {
                SendPing();
            }
        }
        else if ( streq(cmd, "SET INTERFACE") ) {
            char* ifChar = zmsg_popstr(ev->msg);
            int ifIndex = std::stoi(ifChar);
            zstr_free(&ifChar);

            // zsys_info("SET INTERFACE: %i", ifIndex);
            if ( ifIndex < ifNames.size() ) {
                activeInterface = ifNames[ifIndex];

                if (DataSocket != NULL) {
                    sphactor_actor_poller_remove((sphactor_actor_t *) ev->actor, DataSocket);
                    zsock_destroy(&DataSocket);
                }

                std::string url = "udp://" + activeInterface + ";" + MULTICAST_ADDRESS + ":" + PORT_DATA_STR;
                DataSocket = zsock_new(ZMQ_DGRAM);
                //zsock_connect(DataSocket, "%s", url.c_str());
                zsock_bind(DataSocket, "%s", url.c_str());
                assert(DataSocket);
                int rc = sphactor_actor_poller_add((sphactor_actor_t *) ev->actor, DataSocket);
                assert(rc == 0);
            }
            else {
                zsys_info("ERROR: Invalid interface number");
            }
        }

        zstr_free(&cmd);
    }

    return NULL;
}

void NatNet::SendPing() {
    // Say hello to our new host
    // send initial ping command
    sPacket PacketOut;
    PacketOut.iMessage = NAT_PING;
    PacketOut.nDataBytes = 0;
    int nTries = 3;
    while (nTries--) {
        //int iRet = sendto(CommandSocket, (char *)&PacketOut, 4 + PacketOut.nDataBytes, 0, (sockaddr *)&HostAddr, sizeof(HostAddr));
        std::string url = host + ":" + PORT_COMMAND_STR;
        zstr_sendm(CommandSocket, url.c_str());
        int rc = zsock_send(CommandSocket, "b", (char *) &PacketOut, 4 + PacketOut.nDataBytes);
        if (rc != -1) {
            zsys_info("Sent ping to %s", url.c_str());
        }
    }
}

zmsg_t * NatNet::handleCustomSocket( sphactor_event_t * ev )
{
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];
    bool validVersion = !(major == 0 && minor == 0 );
    //zsys_info("GOT FDSOCK");

    // Get the socket...
    assert(ev->msg);
    zframe_t *frame = zmsg_pop(ev->msg);
    if (zframe_size(frame) == sizeof( void *) )
    {
        void *p = *(void **)zframe_data(frame);
        if ( zsock_is( p ) )
        {
            zsock_t* which = (zsock_t*)p;

            if ( which == CommandSocket ) {
                // zsys_info("CMD SOCKET");
                // Parse command
                int addr_len;
                int nDataBytesReceived;
                char str[256];
                sPacket PacketIn;
                addr_len = sizeof(struct sockaddr);

                //int rc = zmq_recv(CommandSocket, (char *) &PacketIn, sizeof(sPacket), 0);
                zmsg_t* zmsg = zmsg_recv(CommandSocket);
                if ( zmsg ) {
                    // pop the source address off
                    char* zStr = zmsg_popstr(zmsg);
                    if ( zStr ) {
                        //zsys_info("zStr: %s", zStr );
                        zstr_free(&zStr);
                    }

                    zframe_t *zframe = zmsg_pop(zmsg);
                    if ( zframe ) {
                        sPacket *packet = (sPacket*) zframe_data(zframe);
                        if ( packet->iMessage == 7 && validVersion ) {
                            // zsys_info("Got frame of data");
                            size_t len = zframe_size(zframe);
                            Unpack((char **) &packet);

                            //zsys_info("rigidbodies after handled frame: %i, %i", rigidbodies.size(), rigidbody_descs.size());

                            // if there is a difference do natnet.questDescription(); to get up to date rigidbody descriptions and thus names
                            if (rigidbody_descs.size() != rigidbodies.size() || !rigidbodiesReady)
                            {
                                if (sentRequest <= 0) {
                                    sendRequestDescription();
                                    sentRequest = 60; //1 second
                                }
                                rigidbodiesReady = false;
                            }

                            //get & check skeletons size
                            if (skeleton_descs.size() != skeletons.size() || !skeletonsReady)
                            {
                                if (sentRequest <= 0) {
                                    sendRequestDescription();
                                    sentRequest = 60; //1 second
                                }
                                skeletonsReady = false;
                            }

                            if (sentRequest > 0) sentRequest--;

                            // re-append the zframe that was popped
                            // only send if definitions are updated
                            if ( skeletonsReady && rigidbodiesReady ) {
                                SetReport(ev->actor);

                                if ( lastData != nullptr ) {
                                    zmsg_destroy(&lastData);
                                }
                                lastData = zmsg_new();
                                zmsg_addmem(lastData, (byte*)packet, len);

                                zframe_destroy(&zframe);
                                return nullptr;
                            }
                        }
                        else {
                            HandleCommand(packet);
                        }
                        zframe_destroy(&zframe);
                    }
                    zmsg_destroy(&zmsg);
                }
            }
            else if ( which == DataSocket ) {
                //zsys_info("DATA SOCKET");
                zmsg_destroy(&ev->msg);

                //TODO: Send data packet to connected natnet2osc clients
                //          this will allow for multiple filters
                //Parse packet
                zmsg_t* zmsg = zmsg_recv(DataSocket);

                // We're still unpacking the data here because it might contain definitions we need to store
                // TODO: Find a way to optimize getting definitions
                if ( zmsg ) {
                    // pop the source address off
                    char* zStr = zmsg_popstr(zmsg);
                    if ( zStr ) {
                        //zsys_info("zStr: %s", zStr );
                        zstr_free(&zStr);
                    }

                    // parse the remaining bytes as mocap data message
                    zframe_t *zframe = zmsg_pop(zmsg);
                    if (zframe && validVersion) {
                        byte *data = zframe_data(zframe);
                        size_t len = zframe_size(zframe);
                        Unpack((char **) &data);

                        //zsys_info("rigidbodies after handled frame: %i, %i", rigidbodies.size(), rigidbody_descs.size());

                        // if there is a difference do natnet.questDescription(); to get up to date rigidbody descriptions and thus names
                        if (rigidbody_descs.size() != rigidbodies.size() || !rigidbodiesReady)
                        {
                            if (sentRequest <= 0) {
                                sendRequestDescription();
                                sentRequest = 60; //1 second
                            }
                            rigidbodiesReady = false;
                        }

                        //get & check skeletons size
                        if (skeleton_descs.size() != skeletons.size() || !skeletonsReady)
                        {
                            if (sentRequest <= 0) {
                                sendRequestDescription();
                                sentRequest = 60; //1 second
                            }
                            skeletonsReady = false;
                        }

                        if (sentRequest > 0) sentRequest--;

                        // re-append the zframe that was popped
                        // only send if definitions are updated
                        if ( skeletonsReady && rigidbodiesReady ) {
                            SetReport(ev->actor);

                            if ( lastData != nullptr ) {
                                zmsg_destroy(&lastData);
                            }
                            lastData = zmsg_new();
                            zmsg_addmem(lastData, data, len);

                            zframe_destroy(&zframe);
                            return nullptr;
                        }
                    }
                    // nothing valid or not sent on, so clean up
                    zmsg_destroy(&zmsg);
                    return NULL;
                }

                return NULL;
            }
        }
    }
    else
        zsys_error("received pointer is not a zsock instance (%s:%d)", __FILE__, __LINE__);

    zframe_destroy(&frame);

    return NULL;
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

// NatNet implementation
bool NatNet::IPAddress_StringToAddr(char *szNameOrAddress, struct in_addr *Address) {
    return false;
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

        //TODO: Matrix, only used for scale previously
        //p = transform.preMult(p);

        ref_markers[j] = p;
    }

    return ptr;
}

char* NatNet::unpackRigidBodies(char* ptr, std::vector<RigidBody>& ref_rigidbodies)
{
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];

    //TODO: Matrix, only used for scale previously
    //ofQuaternion rot = transform.getRotate();

    int nRigidBodies = 0;
    memcpy(&nRigidBodies, ptr, 4);
    ptr += 4;

    ref_rigidbodies.resize(nRigidBodies);

    for (int j = 0; j < nRigidBodies; j++)
    {
        RigidBody& RB = ref_rigidbodies[j];

        glm::vec3 pp;
        glm::vec4 q;

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

        //TODO: These associated markers are no longer part of the 3.1 SDK example
        //          -> Check if we can/need to remove this...

        if ( major < 3 ) {
            // associated marker positions
            int nRigidMarkers = 0;
            memcpy(&nRigidMarkers, ptr, 4);
            ptr += 4;

            int nBytes = nRigidMarkers * 3 * sizeof(float);
            float *markerData = (float *) malloc(nBytes);
            memcpy(markerData, ptr, nBytes);
            ptr += nBytes;

            if (major >= 2) {
                // associated marker IDs
                nBytes = nRigidMarkers * sizeof(int);
                ptr += nBytes;

                // associated marker sizes
                nBytes = nRigidMarkers * sizeof(float);
                ptr += nBytes;
            }

            RB.markers.resize(nRigidMarkers);

            for (int k = 0; k < nRigidMarkers; k++) {
                float x = markerData[k * 3];
                float y = markerData[k * 3 + 1];
                float z = markerData[k * 3 + 2];

                glm::vec3 pp(x, y, z);
                //TODO: Matrix, only used for scale previously
                //pp = transform.preMult(pp);
                RB.markers[k] = pp;
            }

            if (markerData) free(markerData);
        }
        // TODO: 3.1 SDK Contains everything after this again

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

void NatNet::Unpack( char ** pData ) {
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];

    //TODO: Matrix, only used for scale previously
    //Vec4 rotation = transform.getRotate();

    char *ptr = *pData;

    sPacket* packet = (sPacket*)*pData;

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
                    // 3.0 additions
                    if ((major >= 3) || (major == 0))
                    {
                        bool bHasModel = (params & 0x08) != 0;     // marker has an associated asset in the data stream
                        bool bUnlabeled = (params & 0x10) != 0;    // marker is 'unlabeled', but has a point cloud ID
                        bool bActiveMarker = (params & 0x20) != 0; // marker is an actively labeled LED marker
                    }
                }

                // NatNet version 3.0 and later
                float residual = 0.0f;
                if ((major >= 3) || (major == 0))
                {
                    // Marker residual
                    memcpy(&residual, ptr, 4); ptr += 4;
                }

                //zsys_info("ID  : %d\n", ID);
                //zsys_info("pos : [%3.2f,%3.2f,%3.2f]\n", x,y,z);
                //zsys_info("size: [%3.2f]\n", size);

                glm::vec3 pp(x, y, z);
                //TODO: Matrix, only used for scale previously
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

        // TODO: Device data (NatNet version 3.0 and later)
        if (((major == 2) && (minor >= 11)) || (major > 2))
        {
            int nDevices;
            memcpy(&nDevices, ptr, 4); ptr += 4;
            for (int iDevice = 0; iDevice < nDevices; iDevice++)
            {
                // ID
                int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                //printf("Device : %d\n", ID);

                // Channel Count
                int nChannels = 0; memcpy(&nChannels, ptr, 4); ptr += 4;

                // Channel Data
                for (int i = 0; i < nChannels; i++)
                {
                    //printf(" Channel %d : ", i);
                    int nFrames = 0; memcpy(&nFrames, ptr, 4); ptr += 4;
                    for (int j = 0; j < nFrames; j++)
                    {
                        float val = 0.0f;  memcpy(&val, ptr, 4); ptr += 4;
                        //printf("%3.2f   ", val);
                    }
                    //printf("\n");
                }
            }
        }

        // latency, removed in 3.0
        if (major < 3)
        {
            float latency = 0.0f; memcpy(&latency, ptr, 4);	ptr += 4;
            //zsys_info("latency : %3.3f\n", latency);
            this->latency = latency;
        }

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

        // high res timestamps (version 3.0 and later)
        if ((major >= 3) || (major == 0))
        {
            uint64_t cameraMidExposureTimestamp = 0;
            memcpy(&cameraMidExposureTimestamp, ptr, 8); ptr += 8;
            //printf("Mid-exposure timestamp : %" PRIu64"\n", cameraMidExposureTimestamp);

            uint64_t cameraDataReceivedTimestamp = 0;
            memcpy(&cameraDataReceivedTimestamp, ptr, 8); ptr += 8;
            //printf("Camera data received timestamp : %" PRIu64"\n", cameraDataReceivedTimestamp);

            uint64_t transmitTimestamp = 0;
            memcpy(&transmitTimestamp, ptr, 8); ptr += 8;
            //printf("Transmit timestamp : %" PRIu64"\n", transmitTimestamp);
        }

        // frame params
        short params = 0;  memcpy(&params, ptr, 2); ptr += 2;
        bool bIsRecording = params & 0x01;                  // 0x01 Motive is recording
        bool bTrackedModelsChanged = params & 0x02;         // 0x02 Actively tracked model list has changed


        // end of data tag
        int eod = 0; memcpy(&eod, ptr, 4); ptr += 4;
        //zsys_info("End Packet\n-------------\n");

        // filter markers
        // TODO: Test re-implemented marker filtering (replaced remove_dups)
        if (duplicated_point_removal_distance > 0)
        {
            std::map<int, RigidBody>::iterator it =
                    this->rigidbodies.begin();
            while (it != this->rigidbodies.end())
            {
                RigidBody& RB = it->second;

                for (int i = 0; i < RB.markers.size(); i++)
                {
                    glm::vec3& v = RB.markers[i];
                    std::vector<Marker>::iterator it = std::remove_if(
                            tmp_filtered_markers.begin(), tmp_filtered_markers.end(),
                            remove_dups(v, duplicated_point_removal_distance));
                    tmp_filtered_markers.erase(it, tmp_filtered_markers.end());
                }

                it++;
            }
        }

        //Copy data to instance...
        this->latency = latency;
        this->frame_number = frameNumber;
        this->markers_set = tmp_markers_set;
        this->markers = tmp_markers;
        this->filtered_markers = tmp_filtered_markers;

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
                description.offset[1] = yoffset;
                description.offset[2] = zoffset;

                // TODO: Per-marker data (NatNet 3.0 and later)
                if (major >= 3)
                {
                    int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;

                    // Marker positions
                    nBytes = nMarkers * 3 * sizeof(float);
                    float* markerPositions = (float*)malloc(nBytes);
                    memcpy(markerPositions, ptr, nBytes);
                    ptr += nBytes;

                    // Marker required active labels
                    nBytes = nMarkers * sizeof(int);
                    int* markerRequiredLabels = (int*)malloc(nBytes);
                    memcpy(markerRequiredLabels, ptr, nBytes);
                    ptr += nBytes;

                    for (int markerIdx = 0; markerIdx < nMarkers; ++markerIdx)
                    {
                        float* markerPosition = markerPositions + markerIdx * 3;
                        const int markerRequiredLabel = markerRequiredLabels[markerIdx];

                        //printf("\tMarker #%d:\n", markerIdx);
                        //printf("\t\tPosition: %.2f, %.2f, %.2f\n", markerPosition[0], markerPosition[1], markerPosition[2]);

                        if (markerRequiredLabel != 0)
                        {
                            //printf("\t\tRequired active label: %d\n", markerRequiredLabel);
                        }
                    }

                    free(markerPositions);
                    free(markerRequiredLabels);
                }

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
                    description.joints[i].offset[1] = yoffset;
                    description.joints[i].offset[2] = zoffset;

                    if ( major >= 3 ) {
                        // Extra # of markers attached to each rigidbody (always 0 for skeletons, but still 4 bytes)
                        ptr += 4;
                    }
                }
                tmp_skeleton_descs.push_back(description);
            }
            else if ( major >= 3 ) {
                // Force Plate
                if ( type == 3 ) {
                    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                    // printf("ID : %d\n", ID);

                    // Serial Number
                    char strSerialNo[128];
                    strcpy(strSerialNo, ptr);
                    ptr += strlen(ptr) + 1;
                    // printf("Serial Number : %s\n", strSerialNo);

                    // Dimensions
                    float fWidth = 0; memcpy(&fWidth, ptr, 4); ptr += 4;
                    // printf("Width : %3.2f\n", fWidth);

                    float fLength = 0; memcpy(&fLength, ptr, 4); ptr += 4;
                    // printf("Length : %3.2f\n", fLength);

                    // Origin
                    float fOriginX = 0; memcpy(&fOriginX, ptr, 4); ptr += 4;
                    float fOriginY = 0; memcpy(&fOriginY, ptr, 4); ptr += 4;
                    float fOriginZ = 0; memcpy(&fOriginZ, ptr, 4); ptr += 4;
                    // printf("Origin : %3.2f,  %3.2f,  %3.2f\n", fOriginX, fOriginY, fOriginZ);

                    // Calibration Matrix
                    const int kCalMatX = 12;
                    const int kCalMatY = 12;
                    float fCalMat[kCalMatX][kCalMatY];
                    // printf("Cal Matrix\n");
                    for (int calMatX = 0; calMatX < kCalMatX; ++calMatX)
                    {
                        printf("  ");
                        for (int calMatY = 0; calMatY < kCalMatY; ++calMatY)
                        {
                            memcpy(&fCalMat[calMatX][calMatY], ptr, 4); ptr += 4;
                            printf("%3.3e ", fCalMat[calMatX][calMatY]);
                        }
                        printf("\n");
                    }

                    // Corners
                    const int kCornerX = 4;
                    const int kCornerY = 3;
                    float fCorners[kCornerX][kCornerY] = { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} };
                    // printf("Corners\n");
                    for (int cornerX = 0; cornerX < kCornerX; ++cornerX)
                    {
                        printf("  ");
                        for (int cornerY = 0; cornerY < kCornerY; ++cornerY)
                        {
                            memcpy(&fCorners[cornerX][cornerY], ptr, 4); ptr += 4;
                            // printf("%3.3e ", fCorners[cornerX][cornerY]);
                        }
                        printf("\n");
                    }

                    // Plate Type
                    int iPlateType = 0; memcpy(&iPlateType, ptr, 4); ptr += 4;
                    // printf("Plate Type : %d\n", iPlateType);

                    // Channel Data Type
                    int iChannelDataType = 0; memcpy(&iChannelDataType, ptr, 4); ptr += 4;
                    // printf("Channel Data Type : %d\n", iChannelDataType);

                    // Number of Channels
                    int nChannels = 0; memcpy(&nChannels, ptr, 4); ptr += 4;
                    // printf("  Number of Channels : %d\n", nChannels);

                    for (int chNum = 0; chNum < nChannels; ++chNum)
                    {
                        char szName[MAX_NAMELENGTH];
                        strcpy(szName, ptr);
                        int nDataBytes = (int)strlen(szName) + 1;
                        ptr += nDataBytes;
                        // printf("    Channel Name %d: %s\n",chNum,  szName);
                    }
                }
                // Device
                else if ( type == 4 ) {
                    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                    //printf("ID : %d\n", ID);

                    // Name
                    char strName[128];
                    strcpy(strName, ptr);
                    ptr += strlen(ptr) + 1;
                    //printf("Device Name :       %s\n", strName);

                    // Serial Number
                    char strSerialNo[128];
                    strcpy(strSerialNo, ptr);
                    ptr += strlen(ptr) + 1;
                    //printf("Serial Number :     %s\n", strSerialNo);

                    int iDeviceType = 0; memcpy(&iDeviceType, ptr, 4); ptr += 4;
                    //printf("Device Type :        %d\n", iDeviceType);

                    int iChannelDataType = 0; memcpy(&iChannelDataType, ptr, 4); ptr += 4;
                    //printf("Channel Data Type : %d\n", iChannelDataType);

                    int nChannels = 0; memcpy(&nChannels, ptr, 4); ptr += 4;
                    printf("Number of Channels : %d\n", nChannels);
                    char szChannelName[MAX_NAMELENGTH];

                    for (int chNum = 0; chNum < nChannels; ++chNum) {
                        strcpy(szChannelName, ptr);
                        ptr += strlen(ptr) + 1;
                        printf("  Channel Name %d:     %s\n", chNum, szChannelName);
                    }
                }
                // Camera
                if (type == 5) {
                    // Name
                    char szName[MAX_NAMELENGTH];
                    strcpy(szName, ptr);
                    ptr += strlen(ptr) + 1;
                    //MakeAlnum(szName, MAX_NAMELENGTH);
                    //printf("Camera Name  : %s\n", szName);

                    // Pos
                    float cameraPosition[3];
                    memcpy(cameraPosition+0, ptr, 4); ptr += 4;
                    memcpy(cameraPosition+1, ptr, 4); ptr += 4;
                    memcpy(cameraPosition+2, ptr, 4); ptr += 4;
                    //printf("  Position   : %3.2f, %3.2f, %3.2f\n",
                    //       cameraPosition[0], cameraPosition[1],
                    //       cameraPosition[2]);

                    // Ori
                    float cameraOriQuat[4]; // x, y, z, w
                    memcpy(cameraOriQuat + 0, ptr, 4); ptr += 4;
                    memcpy(cameraOriQuat + 1, ptr, 4); ptr += 4;
                    memcpy(cameraOriQuat + 2, ptr, 4); ptr += 4;
                    memcpy(cameraOriQuat + 3, ptr, 4); ptr += 4;
                    //printf("  Orientation: %3.2f, %3.2f, %3.2f, %3.2f\n",
                    //       cameraOriQuat[0], cameraOriQuat[1],
                    //       cameraOriQuat[2], cameraOriQuat[3] );
                }
            }

        }   // next dataset

        //zsys_info("End Packet\n-------------\n");

        //Store data
        // Lock while doing this (loops to read them must finish and can't start @ NatNet2OSCActor)
        {
            const std::lock_guard<std::mutex> lock(desc_mutex);
            this->markerset_descs = tmp_markerset_descs;
            this->rigidbody_descs = tmp_rigidbody_descs;
            this->skeleton_descs = tmp_skeleton_descs;
        }
    }
    else
    {
        zsys_info("Unrecognized Packet Type: %i\n", MessageID);
    }
}

int NatNet::GetLocalIPAddresses(unsigned long Addresses[], int nMax) {
    return 0;
}

void NatNet::sendRequestDescription() {
    zsys_info("requesting description");
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
            Unpack((char**)&PacketIn);
            rigidbodiesReady = true;
            skeletonsReady = true;
            break;
        case NAT_FRAMEOFDATA:
            if ( !( NatNetVersion[0] == 0 && NatNetVersion[1] == 0 ) ) {
                Unpack((char **) &PacketIn);
            }
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

void NatNet::SetReport(const sphactor_actor_t* actor)
{
    //build report
    zosc_t * msg = zosc_create("/report", "si", "Network Interfaces", ifNames.size());
    for( int i = 0; i < ifNames.size(); ++i ) {
        zosc_append(msg, "ss", (std::to_string(i)).c_str(), (ifNames[i] + " ("+ifAddresses[i]+")").c_str());
    }

    sphactor_actor_set_custom_report_data((sphactor_actor_t *)actor, msg);
}
