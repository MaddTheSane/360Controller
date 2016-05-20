//
//  DeviceLister.swift
//  360 Driver
//
//  Created by C.W. Betts on 11/18/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

import Cocoa
import IOKit
import IOKit.hid
import IOKit.usb.IOUSBLib
import ForceFeedback

private let TOOL_FILENAME = "DriverTool"


/// Get some sort of CF type for a field in the IORegistry
private func GetDeviceValue(device: io_service_t, key: String) -> AnyObject? {
	let value: AnyObject! = IORegistryEntrySearchCFProperty(device, kIOServicePlane, key, kCFAllocatorDefault, IOOptionBits(kIORegistryIterateRecursively))
	
	return value
}

/// Make sure a name is as nice as possible for eventually going into the XML for the driver
private func SanitiseName(name: String) -> String {
	var output = String()
	for char in name.characters {
		switch char {
		case " ":
			output += "_"
			
		case "a"..."z", "A"..."Z", "0"..."9", "_":
			output += String(char)
			
		default:
			continue
		}
	}
	
	return output
}

/// Get the Device interface for a given IO service
private func GetDeviceInterface(device: io_service_t) -> UnsafeMutablePointer<UnsafeMutablePointer<IOUSBDeviceInterface>> {
	var err: IOReturn
	var ioDev: UnsafeMutablePointer<UnsafeMutablePointer<IOCFPlugInInterface>> = nil
	var dev: UnsafeMutablePointer<UnsafeMutablePointer<IOUSBDeviceInterface>> = nil
	var score: Int32 = 0
	if IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &ioDev, &score) == kIOReturnSuccess && ioDev != nil {
		err = ioDev.memory.memory.QueryInterface(ioDev, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), withUnsafeMutablePointer(&dev, {return UnsafeMutablePointer<LPVOID>($0)}))
		ioDev.memory.memory.Release(ioDev)
		if err == kIOReturnSuccess {
			return dev
		}
	}
	return nil
}

/// Get the Interface interface for a given IO service
private func GetInterfaceInterface(interface: io_service_t) -> UnsafeMutablePointer<UnsafeMutablePointer<IOUSBInterfaceInterface>> {
	var err: IOReturn
	var ioDev: UnsafeMutablePointer<UnsafeMutablePointer<IOCFPlugInInterface>> = nil
	var dev: UnsafeMutablePointer<UnsafeMutablePointer<IOUSBInterfaceInterface>> = nil
	var score: Int32 = 0
	
	if IOCreatePlugInInterfaceForService(interface, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &ioDev, &score) == kIOReturnSuccess && ioDev != nil {
		err = ioDev.memory.memory.QueryInterface(ioDev, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), withUnsafeMutablePointer(&dev, {return UnsafeMutablePointer<LPVOID>($0)}))
		ioDev.memory.memory.Release(ioDev)
		if err == kIOReturnSuccess {
			return dev
		}
	}
	return nil
}

/*
// List of interfaces we expect on a normal Microsoft controller
const struct ControllerInterface {
int numEndpoints;
UInt8 classNum, subClassNum, protocolNum;
} ControllerInterfaces[] = {
//  Endpoints   Class   Subclass    Protocol
{2,         255,    93,         1},
{4,         255,    93,         3},
{1,         255,    93,         2},
{0,         255,    253,        19},
};
*/
// List of interfaces we expect on a normal Microsoft controller
private struct ControllerInterface {
	var numEndpoints: Int32
	var classNum: UInt8
	var subClassNum: UInt8
	var protocolNum: UInt8
}

private let ControllerInterfaces: [ControllerInterface] = [
	//  Endpoints   Class   Subclass    Protocol
	ControllerInterface(numEndpoints: 2, classNum: 255, subClassNum: 93, protocolNum: 1),
	ControllerInterface(numEndpoints: 4, classNum: 255, subClassNum: 93, protocolNum: 3),
	ControllerInterface(numEndpoints: 1, classNum: 255, subClassNum: 93, protocolNum: 2),
	ControllerInterface(numEndpoints: 0, classNum: 255, subClassNum: 253, protocolNum: 19)]


/// Detect if an IO service object is a Microsoft controller by running through and checking some things
private func IsXBox360Controller(device: io_service_t) -> Bool {
	let interface = GetDeviceInterface(device)
	if interface == nil {
		return false
	}
	var iRq = IOUSBFindInterfaceRequest(bInterfaceClass: UInt16(kIOUSBFindInterfaceDontCare), bInterfaceSubClass: UInt16(kIOUSBFindInterfaceDontCare), bInterfaceProtocol: UInt16(kIOUSBFindInterfaceDontCare), bAlternateSetting: UInt16(kIOUSBFindInterfaceDontCare))
	var iterator: io_iterator_t = 0
	var devInterface: io_service_t
	var interfaceInterface: UnsafeMutablePointer<UnsafeMutablePointer<IOUSBInterfaceInterface>> = nil
	
	var interfaceCount: Int32 = 0
	var interfaceNum: UInt8 = 0
	var classNum: UInt8 = 0
	var subClassNum: UInt8 = 0
	var protocolNum: UInt8 = 0
	var endpointCount: UInt8 = 0
	
	var devValid = false
	
	
	// Get the interface to the device
	interface.memory.memory.GetDeviceClass(interface, &classNum)
	interface.memory.memory.GetDeviceSubClass(interface, &subClassNum)
	interface.memory.memory.GetDeviceProtocol(interface, &protocolNum)
	devValid = (classNum == 0xFF) && (subClassNum == 0xFF) && (protocolNum == 0xFF)
	
	if interface.memory.memory.CreateInterfaceIterator(interface, &iRq, &iterator) == kIOReturnSuccess {
		while true {
			devInterface = IOIteratorNext(iterator)
			if devInterface == 0 {
				break
			}
			interfaceInterface = GetInterfaceInterface(devInterface)
			
			if interfaceInterface != nil {
				interfaceInterface.memory.memory.GetInterfaceNumber(interfaceInterface, &interfaceNum)
				interfaceInterface.memory.memory.GetInterfaceClass(interfaceInterface, &classNum)
				interfaceInterface.memory.memory.GetInterfaceSubClass(interfaceInterface, &subClassNum)
				interfaceInterface.memory.memory.GetInterfaceProtocol(interfaceInterface, &protocolNum)
				interfaceInterface.memory.memory.GetNumEndpoints(interfaceInterface, &endpointCount)
				
				if Int(interfaceNum) < ControllerInterfaces.count {
					let castedInterface = Int(interfaceNum)
					let ci = ControllerInterfaces[castedInterface]
					if ci.classNum == classNum && ci.subClassNum == subClassNum && ci.protocolNum == protocolNum && ci.numEndpoints == Int32(endpointCount) {
						// Found another interface in the right place
						interfaceCount += 1
					}
				}
				interfaceInterface.memory.memory.Release(interfaceInterface)
			}
			IOObjectRelease(devInterface);
		}
		IOObjectRelease(iterator);
	}
	
	// Done
	interface.memory.memory.Release(interface)

	return devValid && (interfaceCount >= 3)   // Only 3 in case the security descriptor is missing?
}

class DeviceLister: NSObject, NSTableViewDataSource {
	@IBOutlet var sheet: NSWindow!
	@IBOutlet var list: NSTableView!
	private var changed = false
	weak private var owner: Pref360ControlPref?
	private var entries = [UInt32 : String]()
	private var connected = [UInt32]()
	private var enabled = [UInt32]()
	
	var toolPath: String {
		// Find the path of our tool in our bundle - should it be in the driver's bundle?
		return (owner!.bundle.resourcePath! as NSString).stringByAppendingPathExtension(TOOL_FILENAME) ?? TOOL_FILENAME
	}
	
	func writeToolWithAuthorisation(authorisationRef: AuthorizationRef) -> OSStatus {
		// Pending major re-write, just return noErr
		return noErr;
	}
	
	func readTool() -> String? {
		// Prepare to run the tool
		let task = NSTask()
		task.launchPath = toolPath
		
		// Hook up the pipe to catch the output
		let pipe = NSPipe()
		task.standardOutput = pipe
		let error = NSPipe()
		task.standardError = error
		
		// Run the tool
		task.launch()
		task.waitUntilExit()
		
		// Check result
		if task.terminationStatus != 0 {
			let data = error.fileHandleForReading.readDataToEndOfFile()
			return String(data: data, encoding: NSUTF8StringEncoding)
		}
		
		// Read the data back
		let data = pipe.fileHandleForReading.readDataToEndOfFile()
		if let response = NSString(data: data, encoding: NSUTF8StringEncoding) {
			let lines = response.componentsSeparatedByCharactersInSet(NSCharacterSet.newlineCharacterSet()) as [NSString]
			for line in lines {
				let values = line.componentsSeparatedByString(",") as [NSString]
				if values.count != 3 {
					continue
				}
				let vendor = UInt32(values[1].intValue)
				let product = UInt32(values[2].intValue)
				let akey = UInt32((vendor << 16) | product)
				enabled.append(akey)
				//var entry = entries[akey]
				if entries[akey] == nil {
					entries[akey] = SanitiseName(values[0] as String)
				}
			}
		}
		
		return nil
	}
	
	/// Get the list of devices we've seen from the settings
	func readKnownDevices() -> String? {
		if let known = GetKnownDevices() as? [UInt : String] {
		let keys = known.keys
			for key in keys {
				let castedKey = UInt32(key)
				if entries[castedKey] == nil {
					entries[castedKey] = known[key]!
				}
			}
		}
		
		return nil
	}
	
	/// Find any matching devices currently plugged in
	func readIOKit() -> String? {
		var iterator: io_iterator_t = 0
		var object: io_service_t
		
		IOServiceGetMatchingServices(owner!.masterPort, IOServiceMatching(kIOUSBDeviceClassName), &iterator)
		if iterator != 0 {
			while true {
				object = IOIteratorNext(iterator)
				if object == 0 {
					break
				}
				if IsXBox360Controller(object) {
					
					let vendorValue = GetDeviceValue(object, key: "idVendor") as? NSNumber
					let productValue = GetDeviceValue(object, key: "idProduct") as? NSNumber
					if productValue != nil && vendorValue != nil {
						let vendor = vendorValue!.unsignedShortValue
						let product = productValue!.unsignedShortValue
						let aKey: UInt32 = ((UInt32(vendor) << 16) | UInt32(product))
						
						connected.append(aKey)
						if entries[aKey] == nil {
							var name = GetDeviceValue(object, key: "USB Product Name") as? String
							if name == nil {
								name = String(format: "Unknown_%.4x_%.4x", vendor, product)
							} else {
								name = SanitiseName(name!)
							}
							entries[aKey] = name!
						}
					}
				}
				IOObjectRelease(object);
			}
			IOObjectRelease(iterator);
		}
		return nil;
	}
	
	func showFailure(message: String) {
		let alert = NSAlert()
		
		alert.messageText = "Error"
		alert.informativeText = message
		
		alert.runModal()
	}
	
	func loadDevices() -> Bool {
		func sanitizeEntries() -> [Int : String] {
			let entries = self.entries
			var toRet = [Int : String]()
			for (key, entry) in entries {
				toRet[Int(key)] = entry
			}
			return toRet
		}
		var error: String? = nil
		
		// Initialize
		entries.removeAll()
		connected.removeAll()
		enabled.removeAll()
		
		// These can be done in any order, depending on the behaviour desired
		error = readKnownDevices()
		if error == nil {
			error = readTool()
		}
		if error == nil {
			error = readIOKit()
		}
		
		// Check for errors
		if let unwrapped = error {
			showFailure(unwrapped)
			return false
		}
		
		// Done
		SetKnownDevices(sanitizeEntries());
		list.reloadData()
		self.changed = false
		return true;
	}
	
	var allEntries: [UInt32] {
		var anEtries = Array(entries.keys)
		var moreEntries = [Int]()
		for anEntry in anEtries {
			moreEntries.append(Int(anEntry))
		}
		
		anEtries.sortInPlace { (lhs, rhs) -> Bool in
			return lhs > rhs
		}
		
		return anEtries
	}
	
	func numberOfRowsInTableView(tableView: NSTableView) -> Int {
		return entries.count
	}
	
	func tableView(tableView: NSTableView, objectValueForTableColumn tableColumn: NSTableColumn?, row: Int) -> AnyObject? {
		if let aTableColumn = tableColumn {
			let key = allEntries[row]
			switch aTableColumn.identifier {
			case "enable":
				if enabled.indexOf(key) != nil {
					return true
				} else {
					return false
				}
				
			case "name":
				var colour: NSColor
				if connected.indexOf(key) != nil {
					colour = NSColor.blueColor()
				} else {
					colour = NSColor.blackColor()
				}
				return NSAttributedString(string: entries[key]!, attributes: [NSForegroundColorAttributeName: colour])
				
			default:
				return nil
			}
		} else {
			return nil
		}
	}
	
	func tableView(tableView: NSTableView, setObjectValue object: AnyObject?, forTableColumn tableColumn: NSTableColumn?, row: Int) {
		if let aTableColumn = tableColumn {
			if let anObject: AnyObject = object {
				let key = allEntries[row]
				var contains: Bool
				if let isFound = enabled.indexOf(key) {
					contains = true
				} else {
					contains = false
				}
				if let aBoolVal = anObject as? Bool {
					if aBoolVal {
						if !contains {
							enabled.append(key)
							self.changed = true
						}
					} else {
						if contains {
							func removeObject(value: UInt32) {
								let index = enabled.indexOf(value)!
								self.enabled.removeAtIndex(index)
							}
							removeObject(key)
							self.changed = true
							//enabled.rem
						}
					}
				}
			}
		}
	}
	
	func showWithOwner(pane: Pref360ControlPref) {
		owner = pane;
		if loadDevices() {
			return;
		}
		
		NSApplication.sharedApplication().beginSheet(sheet, modalForWindow: NSApplication.sharedApplication().mainWindow!, modalDelegate: nil, didEndSelector: nil, contextInfo: nil)
	}
	
	/// attempt to authenticate so we can edit the driver's list of supported devices as root
	func trySave() -> Bool {
		func Three60LocalizedString(message: String, comment: String) -> String {
			return NSLocalizedString(message, bundle: NSBundle(forClass: self.dynamicType), comment: comment)
		}
		
		var status: OSStatus = 0
		var authorisationRef: AuthorizationRef = nil
		var success = false
		
		status = AuthorizationCreate(nil, nil, AuthorizationFlags.Defaults, &authorisationRef)
		
		if status != OSStatus(errAuthorizationSuccess) {
			showFailure(Three60LocalizedString("Unable to create authorisation request", comment: ""))
			return false
		}
		var right = AuthorizationItem(name: kAuthorizationRightExecute, valueLength: 0, value: nil, flags: 0)
		var rights = AuthorizationRights(count: 1, items: &right)
		
		status = AuthorizationCopyRights(authorisationRef, &rights, nil, AuthorizationFlags([.Defaults, .InteractionAllowed, .PreAuthorize, .ExtendRights]), nil)
		
		func gotoFail() -> Bool {
			AuthorizationFree(authorisationRef, AuthorizationFlags.DestroyRights)
			return success
		}
		
		if status != OSStatus(errAuthorizationSuccess) {
			showFailure(Three60LocalizedString("Unable to acquire authorisation", comment: ""))
			return gotoFail()
		}
		
		status = writeToolWithAuthorisation(authorisationRef)
		if status != OSStatus(errAuthorizationSuccess) {
			showFailure(Three60LocalizedString("Failed to execute the driver tool", comment: ""))
			return gotoFail()
		}
		
		success = true
		
		return gotoFail()
	}
	
	@IBAction func done(sender: AnyObject?) {
		if (changed) {
			trySave()
		}
		NSApplication.sharedApplication().endSheet(sheet)
		sheet.close()
	}
}
