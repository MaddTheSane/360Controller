/*
    This file is part of the Xbox HID Driver, Copyright (c) 2007 Darrell Walisser
    walisser@mac.com http://sourceforge.net/projects/xhd

    The Xbox HID Driver is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    The Xbox HID Driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Xbox HID Driver; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/*
 *  DWXBoxHIDDriverKeys.h
 *  DWXBoxHIDDriver
 *
 *  Created by Darrell Walisser on Wed May 28 2003.
 *  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
 *
 */

// -- keys for client configuration -------------------------
// ----------------------------------------------------------

#define kClientOptionKeyKey   "OptionKey"
#define kClientOptionValueKey "OptionValue"
#define kClientOptionSetElementsKey "Elements"

// -- keys for XML configuration ----------------------------
// ----------------------------------------------------------

#define kDeviceDataKey "DeviceData"

#define kKnownDevicesKey "KnownDevices"

// device types
#define kDeviceTypePadKey "Pad"   
#define kDeviceTypeIRKey  "IR"
//kDeviceTypeWheel "Wheel"
//kDeviceTypeStick "Stick"
//add more later, maybe

// top-level device properties
#define kDeviceGenericPropertiesKey   "GenericProperties"
#define kDeviceHIDReportDescriptorKey "HIDReportDescriptor"
#define kDeviceUSBStringTableKey      "USBStrings"
#define kDeviceOptionsKey             "Options"
#define kDeviceButtonMapKey           "ButtonMap"

// axes
#define kOptionInvertYAxisKey            "InvertYAxis"
#define kOptionInvertXAxisKey            "InvertXAxis"
#define kOptionInvertRyAxisKey           "InvertRyAxis"
#define kOptionInvertRxAxisKey           "InvertRxAxis"

// buttons
#define kOptionClampButtonsKey           "ClampButtons"

// triggers
#define kOptionClampLeftTriggerKey            "ClampLeftTrigger"
#define kOptionLeftTriggerIsButtonKey         "LeftTriggerIsButton"
#define kOptionLeftTriggerThresholdKey        "LeftTriggerThreshold"

#define kOptionClampRightTriggerKey           "ClampRightTrigger"
#define kOptionRightTriggerIsButtonKey        "RightTriggerIsButton"
#define kOptionRightTriggerThresholdKey "RightTriggerThreshold"

// generic device properties
#define kGenericInterfacesKey      "Interfaces"
#define kGenericEndpointsKey       "Endpoints"
#define kGenericMaxPacketSizeKey   "MaxPacketSize"
#define kGenericPollingIntervalKey "PollingInterval"
#define kGenericAttributesKey      "Attributes"

// general usage keys
#define kVendorKey  "Vendor"
#define kNameKey    "Name"
#define kTypeKey    "Type"

// ------------------------------------------------------
