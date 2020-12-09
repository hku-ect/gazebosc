#include "NatNetActor.h"
#include <string>

const char * natnetCapabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"motive ip\"\n"
                                "        type = \"string\"\n"
                                "        value = \"192.168.0.1\"\n"
                                "        api_call = \"SET HOST\"\n"
                                "        api_value = \"s\"\n"           // optional picture format used in zsock_send
                                "outputs\n"
                                "    output\n"
                                //TODO: Perhaps add NatNet output type so we can filter the data multiple times...
                                "        type = \"OSC\"\n";

zmsg_t * NatNet::handleMsg( sphactor_event_t * ev ) {
    if ( streq(ev->type, "INIT") ) {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(natnetCapabilities));

        //TODO: Check receive port in NatNet specifications
        //receive socket on port DATA_PORT
        std::string url = "udp://*:"+DATA_PORT;
        this->DataSocket = zsock_new_dgram(url.c_str());
        assert( this->DataSocket );
        this->dataFD = zsock_fd(this->DataSocket);
        int rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, this->DataSocket );
        assert(rc == 0);

        // receive socket on CMD_PORT
        url = "udp://*:"+CMD_PORT;
        this->CommandSocket = zsock_new_dgram(url.c_str());
        assert( this->CommandSocket );
        this->cmdFD = zsock_fd(this->CommandSocket);
        rc = sphactor_actor_poller_add((sphactor_actor_t*)ev->actor, this->CommandSocket );
        assert(rc == 0);
    }
    /*
     * Application will hang if done here when closing application entirely...
     * Even if you have destroyed client actors in between... very odd...
     * */
    else if ( streq(ev->type, "DESTROY") ) {
        if ( this->CommandSocket != NULL ) {
            sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, this->DataSocket);
            zsock_destroy(&this->CommandSocket);
            this->CommandSocket = NULL;
            this->cmdFD = -1;
        }
        if ( this->DataSocket != NULL ) {
            sphactor_actor_poller_remove((sphactor_actor_t*)ev->actor, this->DataSocket);
            zsock_destroy(&this->DataSocket);
            this->DataSocket = NULL;
            this->dataFD = -1;
        }

        return ev->msg;
    }
    else if ( streq(ev->type, "SOCK") ) {
        //TODO: ?
        zsys_info("GOT SOCK");
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET HOST") ) {
                char * host_addr = zmsg_popstr(ev->msg);
                this->host = host_addr;
                std::string url = "udp://" + this->host + DATA_PORT;
                zsys_info("SET HOST: %s", url.c_str());
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
                    std::string url = this->host+":"+CMD_PORT;
                    zstr_sendm(this->CommandSocket, url.c_str());
                    int rc = zsock_send(this->CommandSocket,  "b", (char*)&PacketOut, 4 + PacketOut.nDataBytes);
                    if(rc != SOCKET_ERROR)
                        break;
                }
            }

            zstr_free(&cmd);
        }

        zmsg_destroy(&ev->msg);

        return NULL;
    }
    else if ( streq(ev->type, "FDSOCK" ) )
    {
        zsys_info("GOT FDSOCK");

        //TODO: Read data from our socket...
        assert(ev->msg);
        zframe_t *frame = zmsg_pop(ev->msg);
        if (zframe_size(frame) == sizeof( void *) )
        {
            void *p = *(void **)zframe_data(frame);
            zsock_t* sock = (zsock_t*) zsock_resolve( p );
            if ( sock )
            {
                // Used to determine which socket it is
                SOCKET id = zsock_fd(sock);

                if ( id == cmdFD ) {
                    zsys_info("CMD SOCKET");
                    //TODO: Parse command
                    int addr_len;
                    int nDataBytesReceived;
                    char str[256];
                    sPacket PacketIn;
                    addr_len = sizeof(struct sockaddr);

                    int rc = zmq_recv(CommandSocket, (char *) &PacketIn, sizeof(sPacket),
                                                  0);
                    if ( rc == 0 ) {
                        HandleCommand(&PacketIn);
                    }
                }
                else if ( id == dataFD ) {
                    zsys_info("DATA SOCKET");
                    //TODO: Parse packet, can we use zmq_recv?
                    int nDataBytesReceived = zmq_recv(DataSocket, &szData, sizeof(szData), 0);
                    Unpack(szData);
                }

                // TESTCODE
                byte* msg;
                size_t len;

                //TODO: Implement natnet read from stream to data buffer
                // -> this will probably require some kind of while read...
                zmsg_t * zmsg = zmsg_recv(sock);
                assert(zmsg);

                char* source = zmsg_popstr(zmsg);
                zstr_free(&source);

                zframe_t * oscFrame = zmsg_pop(zmsg);
                assert(oscFrame);

                //TODO: Parse data buffer pointer to natnet implementation

                //TODO: package resulting data into osc messages and return as new message
                // -> each osc message should be a zframe of the zmsg
                zmsg_t* oscMsg = zmsg_new();
                zmsg_add(oscMsg, oscFrame);
                return oscMsg;
            }
        }
        else
            zsys_error("args is not a zsock instance");

        zmsg_destroy(&ev->msg);
        zframe_destroy(&frame);

        return NULL;
    }

    return ev->msg;
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

void NatNet::Unpack( char * pData ) {
    int major = NatNetVersion[0];
    int minor = NatNetVersion[1];

    char *ptr = pData;

    zsys_info("Begin Packet\n-------\n");

    // message ID
    int MessageID = 0;
    memcpy(&MessageID, ptr, 2); ptr += 2;
    zsys_info("Message ID : %d\n", MessageID);

    // size
    int nBytes = 0;
    memcpy(&nBytes, ptr, 2); ptr += 2;
    zsys_info("Byte count : %d\n", nBytes);

    if(MessageID == 7)      // FRAME OF MOCAP DATA packet
    {
        // frame number
        int frameNumber = 0; memcpy(&frameNumber, ptr, 4); ptr += 4;
        zsys_info("Frame # : %d\n", frameNumber);

        // number of data sets (markersets, rigidbodies, etc)
        int nMarkerSets = 0; memcpy(&nMarkerSets, ptr, 4); ptr += 4;
        zsys_info("Marker Set Count : %d\n", nMarkerSets);

        for (int i=0; i < nMarkerSets; i++)
        {
            // Markerset name
            char szName[256];
            strcpy(szName, ptr);
            int nDataBytes = (int) strlen(szName) + 1;
            ptr += nDataBytes;
            zsys_info("Model Name: %s\n", szName);

            // marker data
            int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;
            zsys_info("Marker Count : %d\n", nMarkers);

            for(int j=0; j < nMarkers; j++)
            {
                float x = 0; memcpy(&x, ptr, 4); ptr += 4;
                float y = 0; memcpy(&y, ptr, 4); ptr += 4;
                float z = 0; memcpy(&z, ptr, 4); ptr += 4;
                zsys_info("\tMarker %d : [x=%3.2f,y=%3.2f,z=%3.2f]\n",j,x,y,z);
            }
        }

        // unidentified markers
        int nOtherMarkers = 0; memcpy(&nOtherMarkers, ptr, 4); ptr += 4;
        zsys_info("Unidentified Marker Count : %d\n", nOtherMarkers);
        for(int j=0; j < nOtherMarkers; j++)
        {
            float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
            float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
            float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
            zsys_info("\tMarker %d : pos = [%3.2f,%3.2f,%3.2f]\n",j,x,y,z);
        }

        // rigid bodies
        int nRigidBodies = 0;
        memcpy(&nRigidBodies, ptr, 4); ptr += 4;
        zsys_info("Rigid Body Count : %d\n", nRigidBodies);
        for (int j=0; j < nRigidBodies; j++)
        {
            // rigid body pos/ori
            int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
            float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
            float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
            float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
            float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
            float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
            float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
            float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
            zsys_info("ID : %d\n", ID);
            zsys_info("pos: [%3.2f,%3.2f,%3.2f]\n", x,y,z);
            zsys_info("ori: [%3.2f,%3.2f,%3.2f,%3.2f]\n", qx,qy,qz,qw);

            // associated marker positions
            int nRigidMarkers = 0;  memcpy(&nRigidMarkers, ptr, 4); ptr += 4;
            zsys_info("Marker Count: %d\n", nRigidMarkers);
            int nBytes = nRigidMarkers*3*sizeof(float);
            float* markerData = (float*)malloc(nBytes);
            memcpy(markerData, ptr, nBytes);
            ptr += nBytes;

            if(major >= 2)
            {
                // associated marker IDs
                nBytes = nRigidMarkers*sizeof(int);
                int* markerIDs = (int*)malloc(nBytes);
                memcpy(markerIDs, ptr, nBytes);
                ptr += nBytes;

                // associated marker sizes
                nBytes = nRigidMarkers*sizeof(float);
                float* markerSizes = (float*)malloc(nBytes);
                memcpy(markerSizes, ptr, nBytes);
                ptr += nBytes;

                for(int k=0; k < nRigidMarkers; k++)
                {
                    zsys_info("\tMarker %d: id=%d\tsize=%3.1f\tpos=[%3.2f,%3.2f,%3.2f]\n", k, markerIDs[k], markerSizes[k], markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                }

                if(markerIDs)
                    free(markerIDs);
                if(markerSizes)
                    free(markerSizes);

            }
            else
            {
                for(int k=0; k < nRigidMarkers; k++)
                {
                    zsys_info("\tMarker %d: pos = [%3.2f,%3.2f,%3.2f]\n", k, markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                }
            }
            if(markerData)
                free(markerData);

            if(major >= 2)
            {
                // Mean marker error
                float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
                zsys_info("Mean marker error: %3.2f\n", fError);
            }

            // 2.6 and later
            if( ((major == 2)&&(minor >= 6)) || (major > 2) || (major == 0) )
            {
                // params
                short params = 0; memcpy(&params, ptr, 2); ptr += 2;
                bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
            }

        } // next rigid body


        // skeletons (version 2.1 and later)
        if( ((major == 2)&&(minor>0)) || (major>2))
        {
            int nSkeletons = 0;
            memcpy(&nSkeletons, ptr, 4); ptr += 4;
            zsys_info("Skeleton Count : %d\n", nSkeletons);
            for (int j=0; j < nSkeletons; j++)
            {
                // skeleton id
                int skeletonID = 0;
                memcpy(&skeletonID, ptr, 4); ptr += 4;
                // # of rigid bodies (bones) in skeleton
                int nRigidBodies = 0;
                memcpy(&nRigidBodies, ptr, 4); ptr += 4;
                zsys_info("Rigid Body Count : %d\n", nRigidBodies);
                for (int j=0; j < nRigidBodies; j++)
                {
                    // rigid body pos/ori
                    int ID = 0; memcpy(&ID, ptr, 4); ptr += 4;
                    float x = 0.0f; memcpy(&x, ptr, 4); ptr += 4;
                    float y = 0.0f; memcpy(&y, ptr, 4); ptr += 4;
                    float z = 0.0f; memcpy(&z, ptr, 4); ptr += 4;
                    float qx = 0; memcpy(&qx, ptr, 4); ptr += 4;
                    float qy = 0; memcpy(&qy, ptr, 4); ptr += 4;
                    float qz = 0; memcpy(&qz, ptr, 4); ptr += 4;
                    float qw = 0; memcpy(&qw, ptr, 4); ptr += 4;
                    zsys_info("ID : %d\n", ID);
                    zsys_info("pos: [%3.2f,%3.2f,%3.2f]\n", x,y,z);
                    zsys_info("ori: [%3.2f,%3.2f,%3.2f,%3.2f]\n", qx,qy,qz,qw);

                    // associated marker positions
                    int nRigidMarkers = 0;  memcpy(&nRigidMarkers, ptr, 4); ptr += 4;
                    zsys_info("Marker Count: %d\n", nRigidMarkers);
                    int nBytes = nRigidMarkers*3*sizeof(float);
                    float* markerData = (float*)malloc(nBytes);
                    memcpy(markerData, ptr, nBytes);
                    ptr += nBytes;

                    // associated marker IDs
                    nBytes = nRigidMarkers*sizeof(int);
                    int* markerIDs = (int*)malloc(nBytes);
                    memcpy(markerIDs, ptr, nBytes);
                    ptr += nBytes;

                    // associated marker sizes
                    nBytes = nRigidMarkers*sizeof(float);
                    float* markerSizes = (float*)malloc(nBytes);
                    memcpy(markerSizes, ptr, nBytes);
                    ptr += nBytes;

                    for(int k=0; k < nRigidMarkers; k++)
                    {
                        zsys_info("\tMarker %d: id=%d\tsize=%3.1f\tpos=[%3.2f,%3.2f,%3.2f]\n", k, markerIDs[k], markerSizes[k], markerData[k*3], markerData[k*3+1],markerData[k*3+2]);
                    }

                    // Mean marker error (2.0 and later)
                    if(major >= 2)
                    {
                        float fError = 0.0f; memcpy(&fError, ptr, 4); ptr += 4;
                        zsys_info("Mean marker error: %3.2f\n", fError);
                    }

                    // Tracking flags (2.6 and later)
                    if( ((major == 2)&&(minor >= 6)) || (major > 2) || (major == 0) )
                    {
                        // params
                        short params = 0; memcpy(&params, ptr, 2); ptr += 2;
                        bool bTrackingValid = params & 0x01; // 0x01 : rigid body was successfully tracked in this frame
                    }

                    // release resources
                    if(markerIDs)
                        free(markerIDs);
                    if(markerSizes)
                        free(markerSizes);
                    if(markerData)
                        free(markerData);

                } // next rigid body

            } // next skeleton
        }

        // labeled markers (version 2.3 and later)
        if( ((major == 2)&&(minor>=3)) || (major>2))
        {
            int nLabeledMarkers = 0;
            memcpy(&nLabeledMarkers, ptr, 4); ptr += 4;
            zsys_info("Labeled Marker Count : %d\n", nLabeledMarkers);
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

                zsys_info("ID  : %d\n", ID);
                zsys_info("pos : [%3.2f,%3.2f,%3.2f]\n", x,y,z);
                zsys_info("size: [%3.2f]\n", size);
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
                zsys_info("Force Plate : %d\n", ID);

                // Channel Count
                int nChannels = 0; memcpy(&nChannels, ptr, 4); ptr += 4;

                // Channel Data
                for (int i = 0; i < nChannels; i++)
                {
                    zsys_info(" Channel %d : ", i);
                    int nFrames = 0; memcpy(&nFrames, ptr, 4); ptr += 4;
                    for (int j = 0; j < nFrames; j++)
                    {
                        float val = 0.0f;  memcpy(&val, ptr, 4); ptr += 4;
                        zsys_info("%3.2f   ", val);
                    }
                    zsys_info("\n");
                }
            }
        }

        // latency
        float latency = 0.0f; memcpy(&latency, ptr, 4);	ptr += 4;
        zsys_info("latency : %3.3f\n", latency);

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
        zsys_info("End Packet\n-------------\n");

    }
    else if(MessageID == 5) // Data Descriptions
    {
        // number of datasets
        int nDatasets = 0; memcpy(&nDatasets, ptr, 4); ptr += 4;
        zsys_info("Dataset Count : %d\n", nDatasets);

        for(int i=0; i < nDatasets; i++)
        {
            zsys_info("Dataset %d\n", i);

            int type = 0; memcpy(&type, ptr, 4); ptr += 4;
            zsys_info("Type : %d\n", i, type);

            if(type == 0)   // markerset
            {
                // name
                char szName[256];
                strcpy(szName, ptr);
                int nDataBytes = (int) strlen(szName) + 1;
                ptr += nDataBytes;
                zsys_info("Markerset Name: %s\n", szName);

                // marker data
                int nMarkers = 0; memcpy(&nMarkers, ptr, 4); ptr += 4;
                zsys_info("Marker Count : %d\n", nMarkers);

                for(int j=0; j < nMarkers; j++)
                {
                    char szName[256];
                    strcpy(szName, ptr);
                    int nDataBytes = (int) strlen(szName) + 1;
                    ptr += nDataBytes;
                    zsys_info("Marker Name: %s\n", szName);
                }
            }
            else if(type ==1)   // rigid body
            {
                if(major >= 2)
                {
                    // name
                    char szName[MAX_NAMELENGTH];
                    strcpy(szName, ptr);
                    ptr += strlen(ptr) + 1;
                    zsys_info("Name: %s\n", szName);
                }

                int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                zsys_info("ID : %d\n", ID);

                int parentID = 0; memcpy(&parentID, ptr, 4); ptr +=4;
                zsys_info("Parent ID : %d\n", parentID);

                float xoffset = 0; memcpy(&xoffset, ptr, 4); ptr +=4;
                zsys_info("X Offset : %3.2f\n", xoffset);

                float yoffset = 0; memcpy(&yoffset, ptr, 4); ptr +=4;
                zsys_info("Y Offset : %3.2f\n", yoffset);

                float zoffset = 0; memcpy(&zoffset, ptr, 4); ptr +=4;
                zsys_info("Z Offset : %3.2f\n", zoffset);

            }
            else if(type ==2)   // skeleton
            {
                char szName[MAX_NAMELENGTH];
                strcpy(szName, ptr);
                ptr += strlen(ptr) + 1;
                zsys_info("Name: %s\n", szName);

                int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                zsys_info("ID : %d\n", ID);

                int nRigidBodies = 0; memcpy(&nRigidBodies, ptr, 4); ptr +=4;
                zsys_info("RigidBody (Bone) Count : %d\n", nRigidBodies);

                for(int i=0; i< nRigidBodies; i++)
                {
                    if(major >= 2)
                    {
                        // RB name
                        char szName[MAX_NAMELENGTH];
                        strcpy(szName, ptr);
                        ptr += strlen(ptr) + 1;
                        zsys_info("Rigid Body Name: %s\n", szName);
                    }

                    int ID = 0; memcpy(&ID, ptr, 4); ptr +=4;
                    zsys_info("RigidBody ID : %d\n", ID);

                    int parentID = 0; memcpy(&parentID, ptr, 4); ptr +=4;
                    zsys_info("Parent ID : %d\n", parentID);

                    float xoffset = 0; memcpy(&xoffset, ptr, 4); ptr +=4;
                    zsys_info("X Offset : %3.2f\n", xoffset);

                    float yoffset = 0; memcpy(&yoffset, ptr, 4); ptr +=4;
                    zsys_info("Y Offset : %3.2f\n", yoffset);

                    float zoffset = 0; memcpy(&zoffset, ptr, 4); ptr +=4;
                    zsys_info("Z Offset : %3.2f\n", zoffset);
                }
            }

        }   // next dataset

        zsys_info("End Packet\n-------------\n");

    }
    else
    {
        zsys_info("Unrecognized Packet Type.\n");
    }
}

int NatNet::GetLocalIPAddresses(unsigned long Addresses[], int nMax) {
    return 0;
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
    std::string url = this->host+":"+CMD_PORT;
    zstr_sendm(this->CommandSocket, url.c_str());
    int rc = zsock_send(this->CommandSocket,  "b", (char*)&commandPacket, 4 + commandPacket.nDataBytes);
    if(rc != 0)
    {
        zsys_info("Socket error sending command: %s", szCommand);
    }
    // TODO: needed? will stall actor poll thread as well, so... no?
    /*
    else
    {
        int waitTries = 5;
        while (waitTries--)
        {
            if(gCommandResponse != -1)
                break;
            sleep(30);
        }

        if(gCommandResponse == -1)
        {
            printf("Command response not received (timeout)");
        }
        else if(gCommandResponse == 0)
        {
            printf("Command response received with success");
        }
        else if(gCommandResponse > 0)
        {
            printf("Command response received with errors");
        }
    }
    */

    return gCommandResponse;
}

void NatNet::HandleCommand( sPacket *PacketIn ) {
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
                printf("Response : %s", gCommandResponseString);
                gCommandResponse = 0;   // ok
            }
            break;
        case NAT_UNRECOGNIZED_REQUEST:
            printf("[Client] received 'unrecognized request'\n");
            gCommandResponseSize = 0;
            gCommandResponse = 1;       // err
            break;
        case NAT_MESSAGESTRING:
            printf("[Client] Received message: %s\n", PacketIn->Data.szData);
            break;
    }
}