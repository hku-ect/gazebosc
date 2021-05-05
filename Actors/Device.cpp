//
//  Device.cpp
//  test_101
//
//  Created by Ben Snell on 1/23/19.
//

#include "Device.hpp"

//--------------------------------------------------------------
Device::Device() {
    // TODO: find some other value to check this with...
	firstObservationTimeMicros = clock();
}

//--------------------------------------------------------------
void Device::updateTransformation(vr::HmdMatrix34_t& _mat, uint64_t _timestampMicros) {

    // Set the new values
    mat34 = _mat;
    mat44 = getTransformationMatrix(mat34);
    position = getPosition(mat34);
    quaternion = getQuaternion(mat34);
    timestampMicros = _timestampMicros;
    
    // Should these values be added to a fixed length queue?
    
}

//--------------------------------------------------------------
bool Device::isGenericTracker() {
    return type == vr::TrackedDeviceClass_GenericTracker;
}

//--------------------------------------------------------------
bool Device::isHandheldController() {
    return type == vr::TrackedDeviceClass_Controller;
}

//--------------------------------------------------------------
bool Device::isHeadMountedDisplay() {
    return type == vr::TrackedDeviceClass_HMD;
}

//--------------------------------------------------------------
void Device::updateMotion(vr::HmdVector3_t& _linearVelocity, vr::HmdVector3_t& _angularVelocity) {
    
    linearVelocity[0] = _linearVelocity.v[0];
    linearVelocity[1] = _linearVelocity.v[1];
    linearVelocity[2] = _linearVelocity.v[2];
    
    angularVelocity[0] = _angularVelocity.v[0];
    angularVelocity[1] = _angularVelocity.v[1];
    angularVelocity[2] = _angularVelocity.v[2];
}

//--------------------------------------------------------------
void Device::markNewData() {
    
    // Push an update to all listeners that new data has been received for this one device
	// bool tmp = true;
	// ofNotifyEvent(newDataReceived, tmp);
}

//--------------------------------------------------------------
std::string Device::getDebugString() {
    std::string s;
	
    //TODO: Fix debug string
    /*
    s.append( "serial number\t " + serialNumber + "\n" );
	s.append("device type  \t" + getETrackedDeviceClassString(type) + "\n");
	s.append("connected?   \t" + bConnected + "\n");
	s.append("tracking?    \t" + bTracking + "\n");
	s.append("tracked index\t" + trackedIndex + "\n");
	s.append("timestamp    \t" + timestampMicros + "\n");
	s.append(setprecision(3) + std::fixed + std::showpos);
	s.append("position     \t" + position.x + "\t" + position.y + "\t" + position.z + "\n");
	s.append("quaternion   \t" + quaternion[0] + "\t" + quaternion[1] + "\t" + quaternion[2] + "\t" + quaternion[3] << "\n");
	s.append("velocity     \t" + linearVelocity + "\n");
	s.append("ang velocity \t" + angularVelocity + "\n");
	//s.append(std::noshowpos + setprecision(2));
	s.append("charging?    \t" + bCharging + "\n");
	s.append("battery %    \t" + batteryFraction + "\n");
	s.append("firmwr avail?\t" + bFirmwareUpdateAvailable);
    */

	return s;
}

//--------------------------------------------------------------


//--------------------------------------------------------------



