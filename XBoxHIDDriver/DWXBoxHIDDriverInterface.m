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
//  DWXboxHIDDriverControl.m
//  DWXBoxHIDDriver
//
//  Created by Darrell Walisser on Thu May 29 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//
//  v2.0.0 Modified 12/18/2012 to support Standard 32/64-bit architecture. Compiled with Mac OS X 10.6 SDK.
//

#import "DWXBoxHIDDriverInterface.h"
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDUsageTables.h>

#define kCookiePadFirstFaceButton 14
#define kCookiePadLastFaceButton  19
//#define kCookiePadFirstTrigger    20
//#define kCookiePadLastTrigger     21
#define kCookiePadLeftTrigger     20
#define kCookiePadRightTrigger    21

@implementation DWXBoxHIDDriverInterface

#pragma mark --- private methods ---
- (BOOL)getDeviceProperties
{
    CFMutableDictionaryRef ioRegistryProperties = 0; 

    // get the ioregistry properties
    if (kIOReturnSuccess != 
        IORegistryEntryCreateCFProperties(
            _driver,
            &ioRegistryProperties,
            kCFAllocatorDefault,
            0))
        return NO;

    if (!ioRegistryProperties)
        return NO;

    _ioRegistryProperties = (NSMutableDictionary*)CFBridgingRelease(ioRegistryProperties);

    // set the device type
    _deviceType = _ioRegistryProperties[NSSTR(kTypeKey)];
    if (!_deviceType)
        return NO;

    _deviceOptions = _ioRegistryProperties[NSSTR(kDeviceOptionsKey)];
    
    return YES;
}

- (void)setOptionWithKey:(NSString*)key andValue:(id)value
{
    NSDictionary *request;
    IOReturn ret;
    
    request = @{NSSTR(kTypeKey): _deviceType,
        NSSTR(kClientOptionKeyKey): key,
        NSSTR(kClientOptionValueKey): value};

    ret = IORegistryEntrySetCFProperties (_driver, (__bridge CFDictionaryRef)request);
    if (ret != kIOReturnSuccess)
        NSLog(@"Failed setting driver properties: 0x%x", ret);
}

- (NSMutableDictionary*)elementWithCookieRec:(int)cookie elements:(NSArray*)elements
{
    NSUInteger i, count;
    
    for (i = 0, count = [ elements count ]; i < count; i++) {
    
        NSMutableDictionary *element = elements[i];
        NSArray *subElements;
        
        if (cookie == [ element[NSSTR(kIOHIDElementCookieKey)] intValue ]) {
        
            return element;
        }
        else {
        
            subElements = element[NSSTR(kIOHIDElementKey)];
        
            if (subElements) {
            
                element = [ self elementWithCookieRec:cookie elements:subElements ];
                if (element)
                    return element;
            }
        }
    }
    
    return nil;
}

- (NSMutableDictionary*)elementWithCookie:(int)cookie
{
    NSArray *elements = _ioRegistryProperties[NSSTR(kIOHIDElementKey)];
    if (elements) {
    
        return [ self elementWithCookieRec:cookie elements:elements ];
    }
    
    return nil;
}

- (void)commitElements
{
    NSArray *elements = _ioRegistryProperties[NSSTR(kIOHIDElementKey)];
    IOReturn ret;
        
    if (elements) {
    
        NSDictionary *request = @{NSSTR(kClientOptionSetElementsKey): elements};
        
        if (request) {
            
            ret = IORegistryEntrySetCFProperties (_driver, (__bridge CFDictionaryRef)request);
            if (ret != kIOReturnSuccess)
                NSLog(@"Failed setting driver properties: 0x%x", ret);
        }
    }
}

#pragma mark --- interface methods ---

+ (NSArray*)interfaces
{
    IOReturn 				result = kIOReturnSuccess;
	mach_port_t 			masterPort = 0;
	io_iterator_t 			objectIterator = 0;
	CFMutableDictionaryRef 	matchDictionary = 0;
	io_object_t 			driver = 0;
	NSMutableArray          *interfaceList = nil;
    
	result = IOMasterPort (bootstrap_port, &masterPort);
	if (kIOReturnSuccess != result)
	{
		NSLog(@"IOMasterPort error with bootstrap_port.");
		return nil;
	}

	/* Set up a matching dictionary to search I/O Registry by class name for all HID class devices. */
	matchDictionary = IOServiceMatching ("DWXBoxHIDDriver");
	if (matchDictionary == NULL)
	{
		NSLog(@"Failed to get CFMutableDictionaryRef via IOServiceMatching.");
		return nil;
	}
	
	/*/ Now search I/O Registry for matching devices. */
	result = IOServiceGetMatchingServices (masterPort, matchDictionary, &objectIterator);
	if (kIOReturnSuccess != result)
	{
		NSLog(@"Couldn't create an object iterator.");
		return nil;
	}
    
	if (0 == objectIterator) /* there are no joysticks */
	{
		return nil;
	}
    
	interfaceList = [ [ NSMutableArray alloc ] init ];
    
    /* IOServiceGetMatchingServices consumes a reference to the dictionary, so we don't need to release the dictionary ref. */
	while ((driver = IOIteratorNext (objectIterator)))
	{
        id intf = [ DWXBoxHIDDriverInterface interfaceWithDriver:driver ];
        if (intf)
            [ interfaceList addObject:intf];
    }
    
    IOObjectRelease(objectIterator);
    
    return interfaceList;
}

+ (instancetype)interfaceWithDriver:(io_object_t)driver
{
    DWXBoxHIDDriverInterface *instance;
    
    instance = [ [ [self class] alloc ] initWithDriver:driver ];

    return instance;
}

- (instancetype)initWithDriver:(io_object_t)driver
{
    io_name_t 	className;
     
    self = [ super init ];
    if (!self)
        return nil;
        
        
    IOObjectRetain(driver);
    _driver = driver;

    // check that driver is DWXBoxHIDDriver
    if (kIOReturnSuccess != IOObjectGetClass(_driver, className))
        return nil;

    if (0 != strcmp(className, "DWXBoxHIDDriver"))
        return nil;

    if (! [ self getDeviceProperties ])
        return nil;
        
    return self;
}

- (void)dealloc
{
    
    IOObjectRelease(_driver);
}

- (io_object_t)driver
{
    return _driver;
}

- (NSString*)deviceType
{
    return _deviceType;
}

- (NSString*)productName
{
    return _ioRegistryProperties[NSSTR(kIOHIDProductKey)];
}

- (NSString*)manufacturerName
{
    return _ioRegistryProperties[NSSTR(kIOHIDManufacturerKey)];
}

- (NSString*)identifier
{
    return [ NSString stringWithFormat:@"%@-%x", [ self deviceType ],
        [ _ioRegistryProperties[NSSTR(kIOHIDLocationIDKey)] intValue ] ];
}	

- (BOOL)hasOptions
{
    return _deviceOptions != nil && [ _deviceOptions count ] > 0;
}

- (BOOL)loadOptions:(NSDictionary*)options
{
    // use a little trick here to avoid code duplication...
    // temporarily set the options dictionary so that the "get" methods
    // pull their values from there, but the "set" methods
    // change the values in the driver instance (ioregistry),
    // which is what this method is supposed to do!
    NSDictionary *saveOptions = _deviceOptions;
    _deviceOptions = options;
    if (_deviceOptions && [ _deviceType isEqualTo:NSSTR(kDeviceTypePadKey) ]) {
    
        BOOL invertsYAxis, invertsXAxis, invertsRyAxis, invertsRxAxis,
            clampsButtons, clampsLeftTrigger, clampsRightTrigger,
            mapsLeftTrigger, mapsRightTrigger;
        UInt8 leftTriggerThreshold, rightTriggerThreshold;
        
        // the set* methods refetch the ioreg properties, so we have to
        // get all the values up front before setting anything
        invertsYAxis = [ self invertsYAxis ];
        invertsXAxis = [ self invertsXAxis ];
        invertsRxAxis = [ self invertsRxAxis ];
        invertsRyAxis = [ self invertsRyAxis ];
        clampsButtons = [ self clampsButtonValues ];
        clampsLeftTrigger = [ self clampsLeftTriggerValues ];
        clampsRightTrigger = [ self clampsRightTriggerValues ];
        mapsLeftTrigger = [ self mapsLeftTriggerToButton ];
        mapsRightTrigger = [ self mapsRightTriggerToButton ];
        leftTriggerThreshold = [ self leftTriggerThreshold ];
        rightTriggerThreshold = [ self rightTriggerThreshold ];
        
        [ self setInvertsYAxis:invertsYAxis ];
        [ self setInvertsXAxis:invertsXAxis ];
        [ self setInvertsRyAxis:invertsRyAxis ];
        [ self setInvertsRxAxis:invertsRxAxis ];
        [ self setClampsButtonValues:clampsButtons ];
        [ self setClampsLeftTriggerValues:clampsLeftTrigger ];
        [ self setClampsRightTriggerValues:clampsRightTrigger ];
        [ self setMapsLeftTriggerToButton:mapsLeftTrigger ];
        [ self setMapsRightTriggerToButton:mapsRightTrigger ];
        [ self setLeftTriggerThreshold:leftTriggerThreshold ];
        [ self setRightTriggerThreshold:rightTriggerThreshold ];
                        
        return YES;
    }
    _deviceOptions = saveOptions;
    
    return NO;
}

- (NSDictionary*)deviceOptions
{
    return _deviceOptions;
}

// pad options
- (BOOL)invertsYAxis
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionInvertYAxisKey)]);
}

- (void)setInvertsYAxis:(BOOL)inverts
{
    [ self setOptionWithKey:NSSTR(kOptionInvertYAxisKey) andValue:BOOLtoID(inverts) ];
    [ self getDeviceProperties ];
    
}

- (BOOL)invertsXAxis
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionInvertXAxisKey)]);
}

- (void)setInvertsXAxis:(BOOL)inverts
{
    [ self setOptionWithKey:NSSTR(kOptionInvertXAxisKey) andValue:BOOLtoID(inverts) ];
    [ self getDeviceProperties ];
}

- (BOOL)invertsRyAxis
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionInvertRyAxisKey)]);
}

- (void)setInvertsRyAxis:(BOOL)inverts
{
    [ self setOptionWithKey:NSSTR(kOptionInvertRyAxisKey) andValue:BOOLtoID(inverts) ];
    [ self getDeviceProperties ];
}

- (BOOL)invertsRxAxis
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionInvertRxAxisKey)]);
}

- (void)setInvertsRxAxis:(BOOL)inverts
{
    [ self setOptionWithKey:NSSTR(kOptionInvertRxAxisKey) andValue:BOOLtoID(inverts) ];
    [ self getDeviceProperties ];
}

- (BOOL)clampsButtonValues
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionClampButtonsKey)]);
}

- (void)setClampsButtonValues:(BOOL)clamps
{
    int max = 0;
    int cookie;
    NSMutableDictionary *element;
    
    if (clamps)
        max = 1;   // set elements min/max to 0/1
    else
        max = 255; // set elements min/max to 0/255
    
    for (cookie = kCookiePadFirstFaceButton; cookie <= kCookiePadLastFaceButton; cookie++) {
    
        element = [ self elementWithCookie:cookie ];
        if (element) {
        
            element[NSSTR(kIOHIDElementMaxKey)] = NSNUM(max);
            element[NSSTR(kIOHIDElementScaledMaxKey)] = NSNUM(max);
        }
    }
                
    // update elements structure in ioregistry/driver
    [ self commitElements ];
    [ self setOptionWithKey:NSSTR(kOptionClampButtonsKey) andValue:BOOLtoID(clamps) ];
    [ self getDeviceProperties ];
}

- (BOOL)clampsLeftTriggerValues
{    
    return idToBOOL(_deviceOptions[NSSTR(kOptionClampLeftTriggerKey)]);
}

- (void)setClampsLeftTriggerValues:(BOOL)clamps
{
    int max = 0;
    int cookie;
    NSMutableDictionary *element;
    
    if (clamps)
        max = 1;   // set elements min/max to 0/1
    else
        max = 255; // set elements min/max to 0/255
    
    cookie = kCookiePadLeftTrigger;
    
    element = [ self elementWithCookie:cookie ];
    if (element) {
    
        element[NSSTR(kIOHIDElementMaxKey)] = NSNUM(max);
        element[NSSTR(kIOHIDElementScaledMaxKey)] = NSNUM(max);
    }
    
    [ self commitElements ];
    [ self setOptionWithKey:NSSTR(kOptionClampLeftTriggerKey) andValue:BOOLtoID(clamps) ];
    [ self getDeviceProperties ];
}

- (BOOL)mapsLeftTriggerToButton
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionLeftTriggerIsButtonKey)]);
}

- (void)setMapsLeftTriggerToButton:(BOOL)maps
{
    int usagePage;
    int usage;
    int cookie;
    NSMutableDictionary *element;
    
    if (maps) {
        usagePage = kHIDPage_Button;
        usage = 15;
    }
    else {
        usagePage = kHIDPage_GenericDesktop;
        usage = kHIDUsage_GD_Z;
    }
    
    cookie = kCookiePadLeftTrigger;
    element = [ self elementWithCookie:cookie ];
    if (element) {
        
        element[NSSTR(kIOHIDElementUsagePageKey)] = NSNUM(usagePage);
        element[NSSTR(kIOHIDElementUsageKey)] = NSNUM(usage);
    }
    
    [ self commitElements ];
    [ self setOptionWithKey:NSSTR(kOptionLeftTriggerIsButtonKey) andValue:BOOLtoID(maps) ];
    [ self getDeviceProperties ];
}

- (UInt8)leftTriggerThreshold
{
    return [ _deviceOptions[NSSTR(kOptionLeftTriggerThresholdKey)] unsignedCharValue ];
}

- (void)setLeftTriggerThreshold:(UInt8)threshold
{
    [ self setOptionWithKey:NSSTR(kOptionLeftTriggerThresholdKey) andValue:NSNUM(threshold) ];
    [ self getDeviceProperties ];
}


- (BOOL)clampsRightTriggerValues
{    
    return idToBOOL(_deviceOptions[NSSTR(kOptionClampRightTriggerKey)]);
}

- (void)setClampsRightTriggerValues:(BOOL)clamps
{
    int max = 0;
    int cookie;
    NSMutableDictionary *element;
    
    if (clamps)
        max = 1;   // set elements min/max to 0/1
    else
        max = 255; // set elements min/max to 0/255
    
    cookie = kCookiePadRightTrigger;
    
    element = [ self elementWithCookie:cookie ];
    if (element) {
    
        element[NSSTR(kIOHIDElementMaxKey)] = NSNUM(max);
        element[NSSTR(kIOHIDElementScaledMaxKey)] = NSNUM(max);
    }
    
    [ self commitElements ];
    [ self setOptionWithKey:NSSTR(kOptionClampRightTriggerKey) andValue:BOOLtoID(clamps) ];
    [ self getDeviceProperties ];
}

- (BOOL)mapsRightTriggerToButton
{
    return idToBOOL(_deviceOptions[NSSTR(kOptionRightTriggerIsButtonKey)]);
}

- (void)setMapsRightTriggerToButton:(BOOL)maps
{
    int usagePage;
    int usage;
    int cookie;
    NSMutableDictionary *element;
    
    if (maps) {
        usagePage = kHIDPage_Button;
        usage = 16;
    }
    else {
        usagePage = kHIDPage_GenericDesktop;
        usage = kHIDUsage_GD_Rz;
    }
    
    cookie = kCookiePadRightTrigger;
    element = [ self elementWithCookie:cookie ];
    if (element) {
        
        element[NSSTR(kIOHIDElementUsagePageKey)] = NSNUM(usagePage);
        element[NSSTR(kIOHIDElementUsageKey)] = NSNUM(usage);
    }
    
    [ self commitElements ];
    [ self setOptionWithKey:NSSTR(kOptionRightTriggerIsButtonKey) andValue:BOOLtoID(maps) ];
    [ self getDeviceProperties ];
}

- (UInt8)rightTriggerThreshold
{
    return [ _deviceOptions[NSSTR(kOptionRightTriggerThresholdKey)] unsignedCharValue ];
}

- (void)setRightTriggerThreshold:(UInt8)threshold
{
    [ self setOptionWithKey:NSSTR(kOptionRightTriggerThresholdKey) andValue:NSNUM(threshold) ];
    [ self getDeviceProperties ];
}

@end

