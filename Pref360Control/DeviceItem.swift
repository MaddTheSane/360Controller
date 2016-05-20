//
//  DeviceItem.swift
//  360 Driver
//
//  Created by C.W. Betts on 11/18/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

import Cocoa
import CoreFoundation;
import IOKit
import IOKit.hid
import IOKit.usb.IOUSBLib
import ForceFeedback

private func getDeviceName(device: io_service_t) -> String? {
	var serviceProperties: Unmanaged<CFMutableDictionary>? = nil
	var deviceName: String? = nil
	
	if (IORegistryEntryCreateCFProperties(device, &serviceProperties, kCFAllocatorDefault, 0 /*kNilOptions*/) != KERN_SUCCESS) {
		return nil;
	}
	let properties = serviceProperties!.takeRetainedValue() as NSDictionary
	deviceName = properties[kIOHIDProductKey] as? String
	if (deviceName == nil) {
		deviceName = properties["USB Product Name"] as? String
	}
	return deviceName
}

private func getControllerType(device: io_service_t) -> ControllerType {
	var parent = io_service_t(0)
	var serviceProperties: Unmanaged<CFMutableDictionaryRef>?
	
	// Check for the DeviceData dictionary in device
	if (IORegistryEntryCreateCFProperties(device, &serviceProperties, kCFAllocatorDefault, 0/*kNilOptions*/) == KERN_SUCCESS)
	{
		let properties = serviceProperties!.takeRetainedValue() as NSDictionary as [NSObject: AnyObject]
		if let deviceData = properties["DeviceData"] as? [String: AnyObject] {
			if let aControllerType = deviceData["ControllerType"] {
				if let bController = aControllerType.intValue {
					return ControllerType(rawValue: UInt(bController)) ?? .Xbox360Controller
				}
			}
		}
	}
	
	// Check for the DeviceData dictionary in the device's parent
	if (IORegistryEntryGetParentEntry(device, kIOServicePlane, &parent) == KERN_SUCCESS)
	{
		if(IORegistryEntryCreateCFProperties(parent, &serviceProperties, kCFAllocatorDefault, 0 /*kNilOptions*/) == KERN_SUCCESS)
		{
			let properties = serviceProperties!.takeRetainedValue() as NSDictionary as [NSObject: AnyObject]
			if let deviceData = properties["DeviceData"] as? [String: AnyObject] {
				if let aControllerType = deviceData["ControllerType"] {
					if let bController = aControllerType.intValue {
						return ControllerType(rawValue: UInt(bController)) ?? .Xbox360Controller
					}
				}
			}
		}
	}
	
	NSLog("Error: couldn't find ControllerType");
	return .Xbox360Controller;
}

final class DeviceItem: NSObject {
	let displayName: String
	let rawDevice: io_service_t
	let controllerType: ControllerType
	let ffDevice: FFDeviceObjectReference
	let hidDevice: UnsafeMutablePointer<UnsafeMutablePointer<IOHIDDeviceInterface122>>
	
	required init?(device: io_service_t) {
		var ret: IOReturn = 0
		var plugInInterface: UnsafeMutablePointer<UnsafeMutablePointer<IOCFPlugInInterface>> = nil
		var score: Int32 = 0
		
		ret = IOCreatePlugInInterfaceForService(device, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score)
		if ret != kIOReturnSuccess {
			return nil
		}
		
		var tmpHIDDevice: UnsafeMutablePointer<UnsafeMutablePointer<IOHIDDeviceInterface122>> = nil
		
		ret = plugInInterface.memory.memory.QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), withUnsafeMutablePointer(&tmpHIDDevice, {return UnsafeMutablePointer<LPVOID>($0)}))
		plugInInterface.memory.memory.Release(plugInInterface)
		if ret != kIOReturnSuccess {
			return nil
		}
		var tmpFFdevice: FFDeviceObjectReference = nil
		FFCreateDevice(device, &tmpFFdevice)
		ffDevice = tmpFFdevice
		rawDevice = device
		hidDevice = tmpHIDDevice
		displayName = getDeviceName(device)!
		controllerType = getControllerType(device)
		
		super.init()
	}
	
	@objc(allocateDeviceItemForDevice:) class func allocateDeviceItem(device: io_service_t) -> Self? {
		if let item = self.init(device: device) {
			return item
		} else {
			IOObjectRelease(device)
			return nil
		}
	}
	
	deinit {
		if rawDevice != 0 {
			IOObjectRelease(rawDevice)
		}
		if hidDevice != nil {
			hidDevice.memory.memory.Release(hidDevice)
		}
		if ffDevice != nil {
			FFReleaseDevice(ffDevice)
		}
	}
}
