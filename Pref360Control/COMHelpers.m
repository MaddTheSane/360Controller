//
//  COMHelpers.m
//  360 Driver
//
//  Created by C.W. Betts on 11/18/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "COMHelpers.h"

NSString* GetDeviceName(io_service_t device)
{
    CFMutableDictionaryRef serviceProperties;
    NSDictionary *properties;
    NSString *deviceName = nil;
    
    if (IORegistryEntryCreateCFProperties(device, &serviceProperties, kCFAllocatorDefault, kNilOptions) != KERN_SUCCESS)
        return nil;
    properties = CFBridgingRelease(serviceProperties);
    deviceName = properties[@kIOHIDProductKey];
    if (deviceName == nil)
        deviceName = properties[@"USB Product Name"];
    return deviceName;
}

HRESULT QueryIOKitInterface(HRESULT ( *QueryInterface)(void *thisPointer, REFIID iid, LPVOID *ppv), void *thisPointer, REFIID uuid, LPVOID *ppv)
{
    return (*QueryInterface)(thisPointer, uuid, ppv);
}

HRESULT ReleaseIOKitInterface(ULONG (STDMETHODCALLTYPE *Release)(void *thisPointer), void *thisPointer)
{
    return (*Release)(thisPointer);
}

