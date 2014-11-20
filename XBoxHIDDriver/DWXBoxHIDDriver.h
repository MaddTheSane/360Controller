/*
 Portions Copyright (c) 1999-2003 Apple Computer, Inc. All Rights Reserved.
 
 
 This file contains Original Code and/or Modifications of Original Code as defined in and that are subject to the Apple Public Source License Version 1.2 (the 'License'). You may not use this file except in compliance with the License. Please obtain a copy of the License at http://www.apple.com/publicsource and read it before using this file.
 
 
 The Original Code and all software distributed under the License are distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see the License for the specific language governing rights and limitations under the License.
 */

/*
 How to Obtain this Code:
 Point your browser at http://homepage.mac.com/walisser/xboxhiddriver
 
 Modifications to Original Code
 05-16-2003 Added manipulateReport() method to allow subclasses to modify the hid report
 before it's passed to HIDDevice's handleReport()
 
 05-22-2003 - new methods (see implementation file for comments)
 • isKnownDevice()
 • findGenericDevice()
 • probe()
 • setDeviceOptions()
 • generateTimedEvent()
 - new fields
 • all have _xb prefix, used throughout implementation
 - new definitions
 • property list keys for configuration and data
 • structures to abstract reports for remote and controller
 • enumerations for device type and remote control key map
 06-16-2003 - version 1.2 beta changes
 • moved property list keys into a DWXBoxHIDDriverKeys.h
 • removed device type enumeration - use string instead
 • new pad options:
 - bool InvertXAxis;
 - bool InvertRyAxis;
 - bool InvertRxAxis;
 - bool ClampButtons;
 - bool ClampLeftTrigger;
 - bool ClampRightTrigger;
 - UInt8 LeftTriggerThreshold;
 - UInt8 RightTriggerThreshold;
 • new methods to handle prefs setting from user space
 - setProperties( OSObject * properties );
 - setDefaultOptions();
 - setupDevice();
 
 Background:
 This class is a modified IOUSBHIDDriver for use with XBox USB devices. I wanted to
 just subclass IOUSBHIDDriver, but because the XBox has no HID descriptor (or HID
 report descriptor), and GetHIDDescriptor() is a private method, I had to create
 a complete copy.
 
 I think this really sucks, and hope that Apple's HID implementation will grow to more easily
 support USB devices that are not HID-compatible.
 
 Problems in the Code:
 
 It would be really, really, really nice if Apple's HID implementation actually consulted
 the String_Index items in the report descriptor...I've complained about this on the Apple
 usb list so we'll see.
 
 Force feedback needs to be looked into as well. Update (05/22/03): Linux kernel's iforce support
 may provide clues into faking Immersion Corp's behavior for the ForceFeedback framework
 in 10.2.3...
 
 Previous Problems (for historical reference)
 05-16-2003 (Version 1.0)
 Right now, I can only support (presumably) the large XBox controller and variants. Each
 variant needs to be added to the IOKitPersonalities or it won't be matched to this
 driver.
 
 What I would *like* to do is put almost all of the device-specific crap (hid report
 descriptor, string table, etc in the Info.plist so that it will be possible to support
 new devices without touching the code or recompiling the kext ("you can doooit!").
 */
//  v2.0.0 Modified 12/18/2012 to support Standard 32/64-bit architecture. Compiled with Mac OS X 10.6 SDK.
//
 
#include <IOKit/IOService.h>

#include <IOKit/hid/IOHIDKeys.h>

#ifndef IOUSBHIDDRIVER_H
#define IOUSBHIDDRIVER_H

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOTimerEventSource.h>

#include <IOKit/hid/IOHIDDevice.h>

#include <IOKit/usb/IOUSBBus.h>
#include <IOKit/usb/IOUSBInterface.h>
#include <IOKit/usb/USB.h>

#include "DWXBoxHIDDriverKeys.h"

// xbox device type
/*
 typedef enum {
 
 kDeviceTypeUnknown = 0,
 kDeviceTypePad,
 kDeviceTypeIR,
 kDeviceTypeStick,
 kDeviceTypeWheel
 
 } XBoxDeviceType;
 */

// remote control keys (index into ButtonMapping table which is generated
// and stored in the driver's property list)
typedef enum {
    
    kRemoteDisplay = 0,
    kRemoteReverse,
    kRemotePlay,
    kRemoteForward,
    kRemoteSkipBackward,
    kRemoteStop,
    kRemotePause,
    kRemoteSkipForward,
    kRemoteTitle,
    kRemoteUp,
    kRemoteInfo,
    kRemoteLeft,
    kRemoteSelect,
    kRemoteRight,
    kRemoteMenu,
    kRemoteDown,
    kRemoteBack,
    kRemoteKP1,
    kRemoteKP2,
    kRemoteKP3,
    kRemoteKP4,
    kRemoteKP5,
    kRemoteKP6,
    kRemoteKP7,
    kRemoteKP8,
    kRemoteKP9,
    kRemoteKP0,
    kNumRemoteButtons
} XBoxRemoteKey;

// this structure describes the (fabricated) remote report
// that is passed up to the hid layer
typedef struct {
    
    // note: fields within byte are in reverse order
    // first byte
    UInt8 menu:1;
    UInt8 info:1;
    UInt8 title:1;
    UInt8 right:1;
    UInt8 left:1;
    UInt8 down:1;
    UInt8 up:1;
    UInt8 select:1;
    
    // second byte
    UInt8 skipBackward:1;
    UInt8 forward:1;
    UInt8 reverse:1;
    UInt8 pause:1;
    UInt8 stop:1;
    UInt8 play:1;
    UInt8 display:1;
    UInt8 back:1;
    
    // third byte
    UInt8 kp6:1;
    UInt8 kp5:1;
    UInt8 kp4:1;
    UInt8 kp3:1;
    UInt8 kp2:1;
    UInt8 kp1:1;
    UInt8 kp0:1;
    UInt8 skipForward:1;
    
    // fourth byte
    UInt8 r1:5; // constant
    UInt8 kp9:1;
    UInt8 kp8:1;
    UInt8 kp7:1;
    
    // constant
    UInt8 r2;
    UInt8 r3;
    
} XBRemoteReport;

// this describes the actual hid report that we have to parse
typedef struct
{
    UInt8 r1, r2;
    UInt8 scancode;
    UInt8 r3, r4, r5;
    
} XBActualRemoteReport;

// this checks that the structures are of the same size
typedef int _sizeCheck[ (sizeof(XBRemoteReport) == sizeof(XBActualRemoteReport)) * 2 - 1];

// this structure represents the gampad's raw report
typedef struct {
    
    UInt8
    r1,      // reserved
    r2,      // report length (useless)
    buttons, // up, down, left, right, start, back, left-click, right-click
    r3,      // reserved
    a,
    b,
    x,
    y,
    black,
    white,
    lt,     // left trigger
    rt;     // right trigger
    
    // lo/hi bits of signed 16-bit axes
    UInt8
    lxlo, lxhi,
    lylo, lyhi,
    rxlo, rxhi,
    rylo, ryhi;
    
} XBPadReport;

#define ENABLE_HIDREPORT_LOGGING    0 

// Report types from low level USB:
//  from USBSpec.h:
//    enum {
//        kHIDRtInputReport     = 1,
//        kHIDRtOutputReport        = 2,
//        kHIDRtFeatureReport       = 3
//    };
//
//  from IOHIDDescriptorParser.h:
//    // types of HID reports (input, output, feature)
//    enum
//    {
//        kHIDInputReport           =   1,
//        kHIDOutputReport,
//        kHIDFeatureReport,
//        kHIDUnknownReport     =   255
//    };
//
// Report types from high level HID Manager:
//  from IOHIDKeys.h:
//    enum IOHIDReportType
//    {
//        kIOHIDReportTypeInput = 0,
//        kIOHIDReportTypeOutput,
//        kIOHIDReportTypeFeature,
//        kIOHIDReportTypeCount
//    };
//
#define HIDMgr2USBReportType(x) (x + 1)
#define USB2HIDMgrReportType(x) (x - 1)


// Note: In other Neptune files, kMaxHIDReportSize was defined as 64. But Ferg & Keithen were unable to
// find that value in the USB HID 1.1 specs. Brent had previously changed it to 256 in the OS 9 HID Driver
// to  allow for reports spanning multiple packets. 256 may be no more a hard and fast limit, but it's
// working for now in OS 9.
#define kMaxHIDReportSize 256           // Max packet size = 8 for low speed & 64 for high speed.
#define kHIDDriverRetryCount    3


class DWXBoxHIDDriver : public IOHIDDevice
{
    OSDeclareDefaultStructors(DWXBoxHIDDriver)
    
    IOUSBInterface *    _interface;
    IOUSBDevice *       _device;
    IOUSBPipe *         _interruptPipe;
    UInt32          _maxReportSize;
    IOBufferMemoryDescriptor *  _buffer;
    IOUSBCompletion     _completion;
    UInt32          _retryCount;
    thread_call_t       _deviceDeadCheckThread;
    thread_call_t       _clearFeatureEndpointHaltThread;
    bool            _deviceDeadThreadActive;
    bool            _deviceIsDead;
    bool            _deviceHasBeenDisconnected;
    bool            _needToClose;
    UInt32          _outstandingIO;
    IOCommandGate *     _gate;
    IOUSBPipe *         _interruptOutPipe;
    UInt32          _maxOutReportSize;
    IOBufferMemoryDescriptor *  _outBuffer;
    UInt32          _deviceUsage;
    UInt32          _deviceUsagePage;
    
    // xbox additions
    OSString *      _xbDeviceType;
    OSString *      _xbDeviceVendor;
    OSString *      _xbDeviceName;
    OSData *        _xbDeviceHIDReportDescriptor;
    OSDictionary *  _xbDeviceOptionsDict;
    OSArray *       _xbDeviceButtonMapArray;
    UInt8           _xbLastButtonPressed;
    
    // timing stuff (for synthesizing events - currently only for remote control)
    //bool            _xbShouldGenerateTimedEvent;
    UInt16          _xbTimedEventsInterval;
    IOWorkLoop *    _xbWorkLoop;
    IOTimerEventSource * _xbTimerEventSource;
    
    // xbox device options
    union {
        struct {
            bool InvertYAxis;        // invert sticks (default = true for Y, false for X)
            bool InvertXAxis;
            bool InvertRyAxis;
            bool InvertRxAxis;
            bool ClampButtons;       // clamp face buttons to 0-1 (default = true)
            bool ClampLeftTrigger;      // clamp triggers to 0-1 (default = false)
            bool ClampRightTrigger;
            
            //bool LeftTriggerIsButton; // triggers are mapped to buttons, not axis (default = false)
            //bool RightTriggerIsButton;
            
            UInt8 LeftTriggerThreshold;  // point at which trigger press is realized (default = 1)
            UInt8 RightTriggerThreshold;
        } pad;
        // add more devices here...
    } _xbDeviceOptions;
    
    struct ExpansionData
    {
    };
    ExpansionData *_expansionData;
    static void         InterruptReadHandlerEntry(OSObject *target, void *param, IOReturn status, UInt32 bufferSizeRemaining);
    void            InterruptReadHandler(IOReturn status, UInt32 bufferSizeRemaining);
    
    static void         CheckForDeadDeviceEntry(OSObject *target);
    void            CheckForDeadDevice();
    
    static void         ClearFeatureEndpointHaltEntry(OSObject *target);
    void            ClearFeatureEndpointHalt(void);
    
    virtual void processPacket(void *data, UInt32 size);
    
    virtual void free();
    
    static IOReturn ChangeOutstandingIO(OSObject *target, void *arg0, void *arg1, void *arg2, void *arg3);
    
public:
    // IOService methods
    virtual bool    init(OSDictionary *properties);
    virtual bool    start(IOService * provider);
    virtual bool    didTerminate( IOService * provider, IOOptionBits options, bool * defer );
    virtual bool    willTerminate( IOService * provider, IOOptionBits options );
    
    // IOHIDDevice methods
    virtual bool    handleStart(IOService * provider);
    virtual void    handleStop(IOService *  provider);
    
    virtual IOReturn newReportDescriptor(
                                         IOMemoryDescriptor ** descriptor ) const;
    
    virtual OSString * newTransportString() const;
    virtual OSNumber * newPrimaryUsageNumber() const;
    virtual OSNumber * newPrimaryUsagePageNumber() const;
    
    virtual OSNumber * newVendorIDNumber() const;
    
    virtual OSNumber * newProductIDNumber() const;
    
    virtual OSNumber * newVersionNumber() const;
    
    virtual OSString * newManufacturerString() const;
    
    virtual OSString * newProductString() const;
    
    virtual OSString * newSerialNumberString() const;
    
    virtual OSNumber * newLocationIDNumber() const;
    
    virtual IOReturn    getReport( IOMemoryDescriptor * report,
                                  IOHIDReportType      reportType,
                                  IOOptionBits         options = 0 );
    
    virtual IOReturn    setReport( IOMemoryDescriptor * report,
                                  IOHIDReportType      reportType,
                                  IOOptionBits         options = 0 );
    
    virtual IOReturn    message( UInt32 type, IOService * provider,  void * argument = 0 );
    
    // HID driver methods
    virtual OSString * newIndexedString(UInt8 index) const;
    
    virtual UInt32 getMaxReportSize();
    
    virtual void    DecrementOutstandingIO(void);
    virtual void    IncrementOutstandingIO(void);
    virtual IOReturn    StartFinalProcessing();
    virtual IOReturn    SetIdleMillisecs(UInt16 msecs);
    
    // new stuff
    
    // driver or subclasses can change the format of the report here
    // for example, to reverse the Y axis values
    // return value indicates if event should be sent to HID layer or not
    virtual bool manipulateReport(IOBufferMemoryDescriptor *report);
    
    // check the device product/vendor id's
    virtual bool isKnownDevice(IOService *provider);
    
    // fallback: probe device for #of interfaces, endpoints, etc
    virtual bool findGenericDevice(IOService *provider);
    
    // use active matching to determine the device type (gamepad, joystick, etc..)
    // so we can support 3rd-party devices
    virtual IOService* probe(IOService *service, SInt32 *score);
    
    
    // callback for timer event source
    static void generateTimedEvent(OSObject *object, IOTimerEventSource *tes);
    
    virtual IOReturn setProperties( OSObject * properties );
    
    // create and publish default option settings
    virtual void setDefaultOptions();
    
    // set device-specific options from our property list
    virtual void setDeviceOptions();
    
    // in handleStart() do any initialization we need here
    virtual bool setupDevice();
    
    /*
     virtual bool setElementPropertyRec(OSArray *elements, OSNumber *elementCookie, OSString *key, OSObject *value);
     
     virtual bool setElementProperty(OSNumber *elementCookie, OSString *key, OSObject *value);
     
     virtual void reconfigureElements();
     */
    
private:    // Should these be protected or virtual?
    IOReturn GetHIDDescriptor(UInt8 inDescriptorType, UInt8 inDescriptorIndex, UInt8 *vOutBuf, UInt32 *vOutSize);
    IOReturn GetReport(UInt8 inReportType, UInt8 inReportID, UInt8 *vInBuf, UInt32 *vInSize);
    IOReturn SetReport(UInt8 outReportType, UInt8 outReportID, UInt8 *vOutBuf, UInt32 vOutSize);
    IOReturn GetIndexedString(UInt8 index, UInt8 *vOutBuf, UInt32 *vOutSize, UInt16 lang = 0x409) const;

#if ENABLE_HIDREPORT_LOGGING
    void LogBufferReport(char *report, UInt32 len);
    void LogMemReport(IOMemoryDescriptor * reportBuffer);
    char GetHexChar(char hexChar);
#endif
 
public:
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  0);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  1);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  2);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  3);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  4);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  5);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  6);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  7);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  8);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver,  9);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 10);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 11);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 12);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 13);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 14);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 15);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 16);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 17);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 18);
    OSMetaClassDeclareReservedUnused(DWXBoxHIDDriver, 19);
};

#endif  // IOUSBHIDDRIVER_H
