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

final class DeviceItem: NSObject {
	let name: String
	let rawDevice: io_service_t
	let ffDevice: FFDeviceObjectReference
	let hidDevice: UnsafeMutablePointer<UnsafeMutablePointer<IOHIDDeviceInterface122>>
	
	required init?(device: io_service_t) {
		var ret: IOReturn = 0
		var plugInInterface: UnsafeMutablePointer<UnsafeMutablePointer<IOCFPlugInInterface>> = nil
		var score: Int32 = 0
		
		ret = IOCreatePlugInInterfaceForService(device, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score)
		if ret != kIOReturnSuccess {
			name = ""
			rawDevice = 0
			ffDevice = nil
			hidDevice = nil
			super.init()
			return nil
		}
		
		var tmpHIDDevice: UnsafeMutablePointer<UnsafeMutablePointer<IOHIDDeviceInterface122>> = nil
		var anotherTmp = UnsafeMutablePointer<LPVOID>(tmpHIDDevice)
		
		ret = QueryIOKitInterface(plugInInterface.memory.memory.QueryInterface, plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID122), anotherTmp)
		ReleaseIOKitInterface(plugInInterface.memory.memory.Release, plugInInterface)
		if ret != kIOReturnSuccess {
			name = ""
			rawDevice = 0
			ffDevice = nil
			hidDevice = nil
			super.init()
			return nil
		}
		var tmpFFdevice: FFDeviceObjectReference = nil
		FFCreateDevice(device, &tmpFFdevice)
		ffDevice = tmpFFdevice
		rawDevice = device
		hidDevice = tmpHIDDevice
		name = GetDeviceName(device)
		
		super.init()
	}
	
	@objc(allocateDeviceItemForDevice:) class func allocateDeviceItem(device: io_service_t) -> Self? {
		if let item = self(device: device) {
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
			ReleaseIOKitInterface(hidDevice.memory.memory.Release, hidDevice)
		}
		if ffDevice != nil {
			FFReleaseDevice(ffDevice)
		}
	}
}
