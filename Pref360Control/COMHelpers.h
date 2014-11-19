//
//  COMHelpers.h
//  360 Driver
//
//  Created by C.W. Betts on 11/18/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/usb/USB.h>

NSString* GetDeviceName(io_service_t device);
HRESULT QueryIOKitInterface(HRESULT ( *QueryInterface)(void *thisPointer, REFIID iid, LPVOID *ppv), void *thisPointer, REFIID uuid, void *ppv);
HRESULT ReleaseIOKitInterface(ULONG (STDMETHODCALLTYPE *Release)(void *thisPointer), void *thisPointer);

IOReturn GetIOKitDeviceType(IOReturn (*GetDeviceClass)(void *aself, UInt8 *devClass), void *aself, UInt8 *devValue);
IOReturn CreateIOKitInterfaceIterator(IOReturn (*CreateInterfaceIterator)(void *aself, IOUSBFindInterfaceRequest *req, io_iterator_t *iter), void *aself, IOUSBFindInterfaceRequest *req, io_iterator_t *iter);

