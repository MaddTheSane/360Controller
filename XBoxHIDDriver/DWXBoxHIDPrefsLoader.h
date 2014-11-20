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
//  DWXBoxHIDPrefsLoader.h
//  DWXBoxHIDDriver
//
//  Created by Darrell Walisser on Sun Jun 01 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//
//  v2.0.0 Modified 12/18/2012 to support Standard 32/64-bit architecture. Compiled with Mac OS X 10.6 SDK.
//

#import <Foundation/Foundation.h>
#import "DWXBoxHIDDriverInterface.h"

#define kDWDistributedNotificationsObject @"org.walisser.DWXBoxHIDDriver"
#define kDWXBoxHIDDeviceConfigurationDidChangeNotification @"ConfigDidChange"

@interface DWXBoxHIDPrefsLoader : NSObject 
{
}

// create the default set of settings
+(BOOL)createDefaultsForDevice:(DWXBoxHIDDriverInterface*)device;

// list all the config names for a particular device type
+(NSArray*)configNamesForDeviceType:(NSString*)deviceType;

// get the config name of the specified device
+(NSString*)configNameForDevice:(DWXBoxHIDDriverInterface*)device;

// load the current config for the specified device
+(BOOL)loadSavedConfigForDevice:(DWXBoxHIDDriverInterface*)device;

// save the current config
+(BOOL)saveConfigForDevice:(DWXBoxHIDDriverInterface*)device;

// load named config for device
+(BOOL)loadConfigForDevice:(DWXBoxHIDDriverInterface*)device withName:(NSString*)configName;

// create a new config with specified settings, and make it the device's configuration
+(BOOL)createConfigForDevice:(DWXBoxHIDDriverInterface*)device withName:(NSString*)configName;

// delete the specified configuration
+(BOOL)deleteConfigWithName:(NSString*)configName;
@end
