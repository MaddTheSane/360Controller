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

NSString* GetDeviceName(io_service_t device);
HRESULT QueryIOKitInterface(HRESULT ( *QueryInterface)(void *thisPointer, REFIID iid, LPVOID *ppv), void *thisPointer, REFIID uuid, LPVOID *ppv);
HRESULT ReleaseIOKitInterface(ULONG (STDMETHODCALLTYPE *Release)(void *thisPointer), void *thisPointer);
