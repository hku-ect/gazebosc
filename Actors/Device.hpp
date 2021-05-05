//
//  Device.hpp
//  test_101
//
//  Created by Ben Snell on 1/23/19.
//

#ifndef Device_hpp
#define Device_hpp

#include <string>
#include "Utilities.hpp"

class Device {
public:
    
    Device();
    
	/// \brief The elapsed time at which this device was first observed by this addon.
	///
	uint64_t firstObservationTimeMicros = 0;

	/// \brief The current tracking index (in the list of tracked devices). If the device is not being tracked, this will be -1.
	///
    int trackedIndex = -1;
    
    /// \brief Class of the device
	///
    vr::ETrackedDeviceClass type = vr::TrackedDeviceClass_Invalid;
    // POSSIBLE TYPES:
    // TrackedDeviceClass_Invalid           0   invalid id
    // TrackedDeviceClass_HMD               1   head mounted display
    // TrackedDeviceClass_Controller        2   hand controller
    // TrackedDeviceClass_GenericTracker    3   tracker "pucks"
    // TrackedDeviceClass_TrackingReference 4   camera, base stations (lighthouses)
    // TrackedDeviceClass_DisplayRedirect   5   ...
    
    /// \brief Check if this device is a generic tracker "puck".
    ///
    bool isGenericTracker();
    
    /// \brief Check if this device is a handheld controller (with buttons).
    ///
    bool isHandheldController();
    
    /// \brief Check if this device is a head-mounted display (HMD).
    ///
    bool isHeadMountedDisplay();

    /// \brief Serial number of the device
	///
    std::string serialNumber = "";
    
    
	/// \brief Update all transformation parameters, including transformation matrix, position, and quaternion.
    ///
    void updateTransformation(vr::HmdMatrix34_t& _mat, uint64_t _timestampMicros);
    
    // Transformation Parameters
    vr::HmdMatrix34_t mat34;    // the original pose (not very useful by itself)
    glm::mat4 mat44;            // transformation matrix
    glm::vec3 position;            // position
    glm::quat quaternion;        // orientation
    uint64_t timestampMicros;   // time at which these parameters were captured (in microseconds)
    
    // Should these be stored as an ofNode?
    
    
    /// \brief Update the motion parameters.
    ///
    /// These parameters were calculated by SteamVR.
    void updateMotion(vr::HmdVector3_t& _linearVelocity, vr::HmdVector3_t& _angularVelocity);
    glm::vec3 linearVelocity;
    glm::vec3 angularVelocity;
    
    /// \brief Is this device charging?
	///
    bool bCharging = false;
    
	/// \brief What is the fraction of battery power remaining in this device? (1 = full, 0 = empty)
	///
	float batteryFraction = 0;
    
    /// \brief Mark that a new set of data has been set.
    ///
    void markNewData();
    
	/// \brief Event that is pinged when new transformation information has been received.
	///
	// ofEvent<bool> newDataReceived;
    
    
    // Create a method for adding a piece of information to a queue (for mats, velocities, etc.)
    
    
    
	/// \brief Is this device currently connected over bluetooth to its dongle? If the device is not connected, it cannot be tracked.
	///
    bool bConnected = false;
    
    /// \brief Is this device currently being tracked? (Do the base stations see and recongize this device?) If so, location information will be updated.
	///
    bool bTracking = false;
    
    /// \brief Is a firmware update available for this device?
	///
    bool bFirmwareUpdateAvailable = false;
    
    /// \brief Get a string containing all debug information for each relevant parameter contained within this device.
	///
    std::string getDebugString();
    

	/// \brief Is this device being updated with new tracking data?
	bool isActive() { return bTracking; };
    
};

#endif /* Device_hpp */
