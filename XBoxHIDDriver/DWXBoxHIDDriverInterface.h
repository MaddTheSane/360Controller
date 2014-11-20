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
//
//  DWXBoxHIDDriverInterface.h
//  DWXBoxHIDDriver
//
//  Created by Darrell Walisser on Thu May 29 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//
//  v2.0.0 Modified 12/18/2012 to support Standard 32/64-bit architecture. Compiled with Mac OS X 10.6 SDK.
//

#import <IOKit/IOKitLib.h>
#import <libkern/OSTypes.h>
#import <Cocoa/Cocoa.h>
#import "DWXBoxHIDDriverKeys.h"


// misc utility crap

// ObjC and C foundation objects are interchangeable...
#define NSSTR(x) @(x)
#define BOOLtoID(value) @(((BOOL)(value)))
static inline BOOL idToBOOL(id obj) {
	NSNumber *numbObj = obj;
	
	return numbObj.boolValue;
}
#define NSNUM(num) @(num)


@interface DWXBoxHIDDriverInterface : NSObject 
{
    io_object_t   _driver;
    NSDictionary *_ioRegistryProperties;
    NSString     *_deviceType;
    NSDictionary *_deviceOptions;
}

// utility method: get all connected xbox devices
// returns array of DWXBoxHIDDriverInterface objects
+ (NSArray*)interfaces;

+ (instancetype)interfaceWithDriver:(io_object_t)driver;
- (instancetype) initWithDriver:(io_object_t)driver NS_DESIGNATED_INITIALIZER;

@property (readonly) io_object_t driver;			// associated instance of the driver
@property (readonly, copy) NSString *deviceType;
@property (readonly, copy) NSString *productName;
@property (readonly, copy) NSString *manufacturerName;
@property (readonly, copy) NSString *identifier;

// true if the device type has options (currently only the pad has options)
@property (readonly) BOOL hasOptions;

// load a dictionary of options (say a saved configuration) into the ioregistry
- (BOOL)loadOptions:(NSDictionary*)options;

// fetch the current device options from the ioregistry
@property (readonly, copy) NSDictionary *deviceOptions;

// pad options
@property  BOOL invertsYAxis;

@property  BOOL invertsXAxis;

@property  BOOL invertsRyAxis;

@property  BOOL invertsRxAxis;

@property  BOOL clampsButtonValues;

/*
- (BOOL)clampsTriggerValues;
- (void)setClampsTriggerValues:(BOOL)clamps;

- (BOOL)mapsTriggersToButtons;
- (void)setMapsTriggersToButtons:(BOOL)maps;

- (UInt8)triggerButtonThreshold;
- (void)setTriggerButtonThreshold:(UInt8)threshold;
*/

@property  BOOL clampsLeftTriggerValues;

@property  BOOL mapsLeftTriggerToButton;

@property  UInt8 leftTriggerThreshold;

@property  BOOL clampsRightTriggerValues;

@property  BOOL mapsRightTriggerToButton;

@property  UInt8 rightTriggerThreshold;

// remote options

@end
