//
//  DeviceList.hpp
//  test_101
//
//  Created by Ben Snell on 1/24/19.
//

#ifndef DeviceList_hpp
#define DeviceList_hpp

#include <vector>
#include <map>
#include "Device.hpp"
#include "Utilities.hpp"

// This class handles adding new devices into a list and provides helpers for retrieving relevant devices and their parameters.

// ETrackedDeviceProperties that have no relevance include:
// vr::ETrackedDeviceProperty::Prop_NeverTracked_Bool
// vr::ETrackedDeviceProperty::Prop_DeviceIsWireless_Bool

class DeviceList {
public:
    
    DeviceList();
    
    /// \brief Update all devices' information. Pass the IVRSystem pointer here.
    ///
    bool update(vr::IVRSystem *system);
    
    /// \brief Get a pointer to a vector with pointers to all observed devices.
    ///
    std::vector< Device* >* getDevices();
    
    /// \brief Returns a vector with pointers to all observed generic tracker devices.
    ///
    std::vector< Device* >* getTrackers();
    
    
private:
    
    // List of all devices which have ever been observed
    std::vector< Device* > allDevices;
    // mapping from a device's serial number to its index in this vector
    std::map< std::string, int > serial2index;

	// Get a device with a specific serial number or create it if it doesn't exist
	Device* getDevice(vr::IVRSystem* system, int trackedIndex, std::string serial);

	// Copies of all generic trackers ever observed are kept in here.
    std::vector< Device* > trackers;


	// Check if a serial number is new
	bool isNewDevice(std::string serial);

	// Get a device that already exists (must check to make sure it exists!)
	Device* getExistingDevice(std::string serial);

	// Add new device (assumes device already has serial number inside
	void addNewDevice(Device* device);
    
    // Holds the poses returned each time we look for tracked devices
    std::vector< vr::TrackedDevicePose_t > poses;
    
	// Update the general power, wireless, charging, firmware, etc. properties of a device
	void updateGeneralProperties(vr::IVRSystem* system, int trackedIndex, Device* device);
};

#endif /* DeviceList_hpp */
