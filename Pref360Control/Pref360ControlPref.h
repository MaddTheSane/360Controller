/*
    MICE Xbox 360 Controller driver for Mac OS X
    Copyright (C) 2006-2013 Colin Munro
    
    Pref360ControlPref.h - definition for the pref pane class
    
    This file is part of Xbox360Controller.

    Xbox360Controller is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Xbox360Controller is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#import <PreferencePanes/PreferencePanes.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <ForceFeedback/ForceFeedback.h>

#import "ARCBridge.h"

@class MyWhole360Controller;
@class MyTrigger;
@class MyBatteryMonitor;
@class MyDeadZoneViewer;
@class DeviceLister;

@interface Pref360ControlPref : NSPreferencePane
#ifdef __i386__
{
@private
    // Window components
    __arcweak NSPopUpButton *_deviceList;
    __arcweak NSButton *_leftLinked;
    __arcweak NSSlider *_leftStickDeadzone;
    __arcweak NSButton *_leftStickInvertX;
    __arcweak NSButton *_leftStickInvertY;
    __arcweak NSButton *_rightLinked;
    __arcweak NSSlider *_rightStickDeadzone;
    __arcweak NSButton *_rightStickInvertX;
    __arcweak NSButton *_rightStickInvertY;
    __arcweak DeviceLister *_deviceLister;
    __arcweak NSButton *_powerOff;
    __arcweak MyWhole360Controller *_wholeController;
    __arcweak MyTrigger *_leftTrigger;
    __arcweak MyTrigger *_rightTrigger;
    __arcweak MyBatteryMonitor *_batteryStatus;
    __arcweak MyDeadZoneViewer *_leftDeadZone;
    __arcweak MyDeadZoneViewer *_rightDeadZone;
    NSPopover *_aboutPopover;

    // Internal info
    mach_port_t masterPort;
    NSMutableArray *deviceArray;
    IOHIDElementCookie axis[6],buttons[15];
    
    IOHIDDeviceInterface122 **device;
    IOHIDQueueInterface **hidQueue;
    FFDeviceObjectReference ffDevice;
    io_registry_entry_t registryEntry;
    
    int largeMotor,smallMotor;
    
    IONotificationPortRef notifyPort;
    CFRunLoopSourceRef notifySource;
    io_iterator_t onIteratorWired, offIteratorWired;
    io_iterator_t onIteratorWireless, offIteratorWireless;
    
    FFEFFECT *effect;
    FFCUSTOMFORCE *customforce;
    FFEffectObjectReference effectRef;

}
#endif
// Window components
@property (arcweak) IBOutlet NSPopUpButton *deviceList;
@property (arcweak) IBOutlet NSButton *leftLinked;
@property (arcweak) IBOutlet NSSlider *leftStickDeadzone;
@property (arcweak) IBOutlet NSButton *leftStickInvertX;
@property (arcweak) IBOutlet NSButton *leftStickInvertY;
@property (arcweak) IBOutlet NSButton *rightLinked;
@property (arcweak) IBOutlet NSSlider *rightStickDeadzone;
@property (arcweak) IBOutlet NSButton *rightStickInvertX;
@property (arcweak) IBOutlet NSButton *rightStickInvertY;
@property (arcweak) IBOutlet DeviceLister *deviceLister;
@property (arcweak) IBOutlet NSButton *powerOff;
@property (arcweak) IBOutlet MyWhole360Controller *wholeController;
@property (arcweak) IBOutlet MyTrigger *leftTrigger;
@property (arcweak) IBOutlet MyTrigger *rightTrigger;
@property (arcweak) IBOutlet MyBatteryMonitor *batteryStatus;
@property (arcweak) IBOutlet MyDeadZoneViewer *leftDeadZone;
@property (arcweak) IBOutlet MyDeadZoneViewer *rightDeadZone;
@property (strong) IBOutlet NSPopover *aboutPopover;

// Internal info
@property (readonly) mach_port_t masterPort;

- (void)eventQueueFired:(void*)sender withResult:(IOReturn)result;

- (void)handleDeviceChange;

- (IBAction)powerOff:(id)sender;
- (IBAction)selectDevice:(id)sender;
- (IBAction)changeSetting:(id)sender;
- (IBAction)showAboutPopover:(id)sender;

@end
