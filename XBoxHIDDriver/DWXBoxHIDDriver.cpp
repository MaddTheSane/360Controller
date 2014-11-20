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
 
 Changed getReport() to read from interface 0, endpoint 0 (input) rather than use
 a HID control request.
 
 Changed GetHIDDescriptor() to read a hardcoded report rather than
 use a HID control request.
 
 Changed GetIndexedString() to read from a hardcoded string table
 instead of using the USB string request
 
 Changed newManufacturerString(), newProductNameString(),
 newSerialNumbeString() to use hardcoded index rather than USB device's
 index.
 
 Added hardcoded tables for aformentioned changes.
 
 05-22-2003 Version 1.1 Changes
 -initial remote control support
 • added IOTimerEventSource to send remote button-up events
 • added generateTimedEvent() method which fires at _xbTimedEventsInterval millisecond intervals (currently 80ms)
 • added button lookup and structure packing to generate remote's report in manipulateReport()
 
 -generic device detection via property list (don't need the product id/vendor id for everything)
 • added isKnownDevice() method to check vendor/product id's
 • added findGenericDevice() method to handle detection of devices with unknown product ids
 • added probe(), which calls the 2 previous methods (utilized by IOKit's driver matching protocol)
 
 -initial options support
 • each device type can contain a dictionary for setting options
 • this is the first step towards a user-configurable driver
 
 -new defaults
 • for compatibility, the analog buttons are clamped to 0-1 (note: to do 0-255 you'd have to edit the hid report descriptor)
 
 -removed hardcoded tables - now stored in property list (use the "hex" tool to format data)
 -removed hardcoded reports - now stored in property list
 -removed GetIndexedString() hack, replaced with property list strings if needed
 -printing out more error messages
 06-16-2003 - version 1.2 beta changes
 
 This is a major overhaul that allows an external program to modify the driver's
 settings. The way this works is that when the driver loads, it uses a default
 set of settings. A daemon application can detect when a driver loads and call
 the IORegistryEntrySetCFProperties() to modify the driver settings. Clients
 also can call IORegistryEntryCreateCFProperties() to see what the current
 settings are.
 
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
 
 10-13-04 Version 1.3 Changes
 In this version I'm trying to further improve compatibility with 3rd-party
 controllers. It turns out that the generic identification routine (findGenericDevice)
 is not generic enough. Probably the only constant between 3rd-party controllers
 is that they will always have the same interface class/subclass (88/66) and two
 interrupt endpoints (one for output/joysticks, one for input/rumble motors).
 
 • Now if findKnownDevice() and findGenericDevice() fail we assume a gamepad is connected.
 - This could be bad if we assume wrong; at worst what happens is we read a report that is either
 to small (hangs driver/application) or too large (incomplete data passed to app). However,
 it seems to me that the majority of Xbox devices conform to the original controller's report
 descriptor (for the obvious purpose of being compatible with all xbox games).
 */

/*
 * This is a basic HID driver to provide support for USB class 3
 * (HID) devices. The initial design allows the original OSX
 * drivers for the Apple USB mouse and keyboard to be used for
 * those devices instead of this driver.
 */
//  v2.0.0 Modified 12/18/2012 to support Standard 32/64-bit architecture. Compiled with Mac OS X 10.6 SDK.
//

#include <mach/mach_types.h>

kern_return_t DWXBoxHIDDriver_start(kmod_info_t * ki, void *d);
kern_return_t DWXBoxHIDDriver_stop(kmod_info_t *ki, void *d);

kern_return_t DWXBoxHIDDriver_start(kmod_info_t * ki, void *d)
{
    return KERN_SUCCESS;
}

kern_return_t DWXBoxHIDDriver_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}

#include <libkern/OSByteOrder.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
//#include <IOKit/IOLib.h>        // For IOLog()...
#include <IOKit/IOMessage.h>

#include <IOKit/hid/IOHIDKeys.h>

#include <IOKit/usb/IOUSBInterface.h>
#include <IOKit/usb/IOUSBPipe.h>

#define DEBUG_LEVEL 7 // 0=disable all logging, 7=full logging
#include <IOKit/usb/IOUSBLog.h>

#include "DWXBoxHIDDriver.h"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(DWXBoxHIDDriver, super)


// Do what is necessary to start device before probe is called.
//
bool
DWXBoxHIDDriver::init(OSDictionary *properties)
{
    USBLog(6, "DWXBoxHIDDriver[%p]::init", this);
    
    if (!super::init(properties))
    {
        return false;
    }
    
    _interface = NULL;
    _buffer = 0;
    _retryCount = kHIDDriverRetryCount;
    _outstandingIO = 0;
    _needToClose = false;
    _maxReportSize = kMaxHIDReportSize;
    _maxOutReportSize = kMaxHIDReportSize;
    _outBuffer = 0;
    _deviceUsage = 0;
    _deviceUsagePage = 0;
    
    _xbDeviceType = 0;
    _xbDeviceVendor = 0;
    _xbDeviceName = 0;
    _xbDeviceHIDReportDescriptor = 0;
    _xbDeviceOptionsDict = 0;
    _xbDeviceButtonMapArray = 0;
    _xbLastButtonPressed = 0;
    //_xbShouldGenerateTimedEvent = false;
    _xbTimedEventsInterval = 80; // milliseconds
    _xbWorkLoop = 0;
    _xbTimerEventSource = 0;
    
    return true;
}

//
// Note: handleStart is not an IOKit thing, but is a IOHIDDevice thing. It is called from
// IOHIDDevice::start after some initialization by that method, but before it calls registerService
// this method needs to open the provider, and make sure to have enough state (basically _interface
// and _device) to be able to get information from the device. we do NOT need to start the interrupt read
// yet, however
//
bool
DWXBoxHIDDriver::handleStart(IOService * provider)
{
    HIDPreparsedDataRef parseData;
    HIDCapabilities 	myHIDCaps;
    UInt8 *         	myHIDDesc;
    UInt32          	hidDescSize;
    IOReturn        	err = kIOReturnSuccess;
    
    USBLog(6, "%s[%p]::handleStart", getName(), this);
    if( !super::handleStart(provider))
    {
        USBError(1, "%s[%p]::handleStart - super::handleStart failed", getName(), this);
        return false;
    }
    
    if( !provider->open(this))
    {
        USBError(1, "%s[%p]::handleStart - unable to open provider", getName(), this);
        return (false);
    }
    
    _interface = OSDynamicCast(IOUSBInterface, provider);
    if (!_interface)
    {
        USBError(1, "%s[%p]::handleStart - no interface", getName(), this);
        return false;
    }
    
    // remember my device
    _device = _interface->GetDevice();
    if (!_device)
    {
        USBError(1, "%s[%p]::handleStart - no device", getName(), this);
        return false;
    }
    
    if (!setupDevice()) {
        
        return false;
    }
    
    // Get the size of the HID descriptor.
    hidDescSize = 0;
    err = GetHIDDescriptor(kUSBReportDesc, 0, NULL, &hidDescSize);
    if ((err != kIOReturnSuccess) || (hidDescSize == 0))
    {
        USBLog(1, "%s[%p]::handleStart : unable to get descriptor size", getName(), this);
        return false;       // Won't be able to set last properties.
    }
    
    myHIDDesc = (UInt8 *)IOMalloc(hidDescSize);
    if (myHIDDesc == NULL)
    {
        USBLog(1, "%s[%p]::handleStart : unable to allocate descriptor", getName(), this);
        return false;
    }
    
    // Get the real report descriptor.
    err = GetHIDDescriptor(kUSBReportDesc, 0, myHIDDesc, &hidDescSize);
    
    if (err == kIOReturnSuccess)
    {
        err = HIDOpenReportDescriptor(myHIDDesc, hidDescSize, &parseData, 0);
        if (err == kIOReturnSuccess)
        {
            err = HIDGetCapabilities(parseData, &myHIDCaps);
            if (err == kIOReturnSuccess)
            {
                // Just get these values!
                _deviceUsage = myHIDCaps.usage;
                _deviceUsagePage = myHIDCaps.usagePage;
                
                _maxOutReportSize = static_cast<unsigned int>(myHIDCaps.outputReportByteLength);            // v2.0.0 Added static_cast<unsigned int>()
                _maxReportSize = (static_cast<unsigned int>(myHIDCaps.inputReportByteLength) > static_cast<unsigned int>(myHIDCaps.featureReportByteLength)) ?            // v2.0.0 Added static_cast<unsigned int>()
                static_cast<unsigned int>(myHIDCaps.inputReportByteLength) : static_cast<unsigned int>(myHIDCaps.featureReportByteLength);            // v2.0.0 Added static_cast<unsigned int>()
            }
            else
            {
                USBError(1, "%s[%p]::handleStart - failed getting capabilities", getName(), this);
            }
            
            HIDCloseReportDescriptor(parseData);
        }
        else
        {
            USBError(1, "%s[%p]::handleStart - failed parsing descriptor", getName(), this);
        }
        
    }
    else
    {
        USBLog(1, "%s[%p]::handleStart : error getting hid descriptor: %X", getName(), this, err);
    }
    
    if (myHIDDesc)
    {
        IOFree(myHIDDesc, hidDescSize);
    }
    
    // Set HID Manager properties in IO registry.
    // Will now be done by IOHIDDevice::start calling newTransportString, etc.
    //    SetProperties();
    return true;
}


void
DWXBoxHIDDriver::handleStop(IOService * provider)
{
    USBLog(7, "%s[%p]::handleStop", getName(), this);
    
    // cleanup timer
    if (_xbWorkLoop) {
        
        if (_xbTimerEventSource) {
            
            _xbTimerEventSource->cancelTimeout();
            _xbWorkLoop->removeEventSource(_xbTimerEventSource);
            _xbTimerEventSource->release();
            _xbTimerEventSource = 0;
        }
    }
    
    if (_outBuffer)
    {
        _outBuffer->release();
        _outBuffer = NULL;
    }
    
    if (_buffer)
    {
        _buffer->release();
        _buffer = NULL;
    }
    
    if (_deviceDeadCheckThread)
    {
        thread_call_cancel(_deviceDeadCheckThread);
        thread_call_free(_deviceDeadCheckThread);
    }
    
    if (_clearFeatureEndpointHaltThread)
    {
        thread_call_cancel(_clearFeatureEndpointHaltThread);
        thread_call_free(_clearFeatureEndpointHaltThread);
    }
    
    super::handleStop(provider);
}


void
DWXBoxHIDDriver::free()
{
    USBLog(6, "%s[%p]::free", getName(), this);
    
    super::free();
}


void
DWXBoxHIDDriver::processPacket(void *data, UInt32 size)
{
    IOLog("Should not be here, DWXBoxHIDDriver::processPacket()\n");
    
    return;
}

/* NOTE: this code that is commented out kernel-paniced my machine. Rather than fix it, I realized the folly in my ways and moved this functionality a user-space program. This is just an experiement in how you could to HID
 configuration after the driver is instantiated - it is incomplete for the Xbox requirements.
 
 bool
 DWXBoxHIDDriver::setElementPropertyRec(OSArray *elements, OSNumber *elementCookie, OSString *key, OSObject *value)
 {
 int i;
 int count;
 
 count = elements->getCount();
 
 for (i = 0; i < count; i++) {
 
 OSDictionary *element;
 
 element = OSDynamicCast(OSDictionary, elements->getObject(i));
 if (element) {
 
 OSNumber *cookie;
 
 cookie = OSDynamicCast(OSNumber, element->getObject(kIOHIDElementCookieKey));
 if (cookie) {
 
 if (cookie->isEqualTo(elementCookie)) {
 
 element->setObject(key, value);
 
 USBLog(6, "%s[%p]::setElementPropertyRec() %s", getName(), this,
 key->getCStringNoCopy());
 
 return true;
 }
 else {
 
 OSArray *subElements;
 
 subElements = OSDynamicCast(OSArray, element->getObject(kIOHIDElementKey));
 if (subElements)
 this->setElementPropertyRec(subElements, elementCookie, key, value);
 }
 }
 }
 
 else {
 USBLog(6, "error 2");
 }
 }
 
 
 return false;
 }
 
 bool
 DWXBoxHIDDriver::setElementProperty(OSNumber *elementCookie, OSString *key, OSObject *value)
 {
 bool error = true;
 OSArray *elements = 0;
 
 elements = OSDynamicCast(OSArray, getProperty(kIOHIDElementKey));
 if (elements) {
 
 error = this->setElementPropertyRec(elements, elementCookie, key, value);
 }
 else {
 USBLog(6, "error 1");
 }
 
 return error;
 }
 
 void
 DWXBoxHIDDriver::reconfigureElements()
 {
 
 if (_xbDeviceType->isEqualTo(kDeviceTypePadKey)) {
 
 OSString *key;
 OSObject *value;
 OSNumber *cookie;
 
 if (_xbDeviceOptions.pad.ClampTriggers) {
 
 USBLog(6, "%s[%p]::reconfigureElements - map triggers to buttons",
 getName(), this);
 
 // map triggers to buttons 15 and 16
 // set min/max to 0/1
 for (int cookieNum = 20, buttonNum = 15;
 cookieNum <= 21 && buttonNum <= 16;
 cookieNum++, buttonNum++) {
 
 cookie = OSNumber::withNumber(cookieNum, 8); // left trigger or right trigger
 if (!cookie)
 return;
 
 // set usage page
 key = OSString::withCString(kIOHIDElementUsagePageKey);
 value = OSNumber::withNumber(kHIDPage_Button, 16);
 if (key && value) {
 setElementProperty(cookie, key, value);
 key->release();
 value->release();
 }
 
 // set usage
 key = OSString::withCString(kIOHIDElementUsageKey);
 value = OSNumber::withNumber(buttonNum, 16); // button 15 or 16
 if (key && value) {
 setElementProperty(cookie, key, value);
 key->release();
 value->release();
 }
 
 // set max to 1
 key = OSString::withCString(kIOHIDElementMaxKey);
 value = OSNumber::withNumber(1, 8);
 if (key && value) {
 setElementProperty(cookie, key, value);
 key->release();
 }
 
 // set scaled max to 1
 key = OSString::withCString(kIOHIDElementScaledMaxKey);
 if (key && value) {
 
 setElementProperty(cookie, key, value);
 key->release();
 cookie->release();
 }
 
 cookie->release();
 }
 }
 else {
 
 // map triggers to z and rz
 // set min/max to 0/255
 }
 }
 }
 
 */

IOReturn 
DWXBoxHIDDriver::setProperties(OSObject *properties)
{
    // called from IORegistryEntrySetCFProperties() from user context
    
    USBLog(6, "%s[%p]::setProperties", getName(), this);
    
    OSDictionary *dict;
    OSString *deviceType;
    
    dict = OSDynamicCast(OSDictionary, properties);
    if (dict) {
        
        // Check if client wants to manipulate device options
        deviceType = OSDynamicCast(OSString, dict->getObject(kTypeKey));
        if (deviceType && _xbDeviceType->isEqualTo(deviceType)) {
            
            USBLog(6, "%s[%p]::setProperties - change properties for a %s device",
                   getName(), this, deviceType->getCStringNoCopy());
            
            OSString *optionKey = OSDynamicCast(OSString, dict->getObject(kClientOptionKeyKey));
            OSObject *optionValue = OSDynamicCast(OSObject, dict->getObject(kClientOptionValueKey));
            
            /*
            // v2.0.0 Changed the way the code is implemented. In Mac OS X 64-bit, there is no way to directly modify the IORegistry.
            // Instead, the solution to a working preference pane is to make a deep copy of the values and modify them. However, I couln't 
            // find enough documentation for implementing the "OSCollection * copyCollection (OSDictionary cycleDict = 0)" code, so I ended 
            // up fixing the problem a different way. Basically I created a new dictionary and called in the default values. Then I overwrote
            // the new values (optionKey & optionValue) with setObject, and finally used setProperty to replace the old IORegistry "Options" property.
            */
            
            _xbDeviceOptionsDict = OSDictionary::withCapacity(0);

            //setDefaultOptions();
            OSBoolean *boolean;
            OSNumber  *number;
            
#define SET_BOOLEAN(prop) \
boolean = OSBoolean::withBoolean(_xbDeviceOptions.pad.prop ); \
if (boolean) { \
_xbDeviceOptionsDict->setObject(kOption ## prop ## Key, boolean); \
boolean->release(); \
}

            SET_BOOLEAN(InvertYAxis)
            SET_BOOLEAN(InvertXAxis)
            SET_BOOLEAN(InvertRyAxis)
            SET_BOOLEAN(InvertRxAxis)
            SET_BOOLEAN(ClampButtons)
            SET_BOOLEAN(ClampLeftTrigger)
            SET_BOOLEAN(ClampRightTrigger)
            
            //SET_BOOLEAN(LeftTriggerIsButton)
            //SET_BOOLEAN(RightTriggerIsButton)
            
#undef SET_BOOLEAN
            
            number = OSNumber::withNumber(_xbDeviceOptions.pad.LeftTriggerThreshold, 8);
            if (number) {
                _xbDeviceOptionsDict->setObject(kOptionLeftTriggerThresholdKey, number);
                number->release();
            }
            
            number = OSNumber::withNumber(_xbDeviceOptions.pad.RightTriggerThreshold, 8);
            if (number) {
                _xbDeviceOptionsDict->setObject(kOptionRightTriggerThresholdKey, number);
                number->release();
            }
            
            // v2.0.0 Overwrite new key & value
            _xbDeviceOptionsDict->setObject(optionKey, optionValue);

            
            if (dict && optionKey && optionValue) {
                
                // v2.0.0 Replace old entry in IORegistry with our new dictionary.
                setProperty(kDeviceOptionsKey, _xbDeviceOptionsDict);
                
                // v2.0.0 (causes kernel panic in 64-bit) It is no longer possible to directly modify the IORegistry.
                // More information here: http://lists.apple.com/archives/darwin-drivers/2009/Sep/msg00015.html
                // update properties
                //_xbDeviceOptionsDict->setObject(optionKey, optionValue);
                
                // rescan properties for options
                setDeviceOptions();
                
                // Change elements structure to reflect changes
                //reconfigureElements();
                
                return kIOReturnSuccess;
            }
        }
        else {
            
            USBLog(6, "%s[%p]::setProperties - changing HID elements",
                   getName(), this);
            
            // check if client wants to change the HID elements structure
            OSArray *newElements = OSDynamicCast(OSArray, dict->getObject(kClientOptionSetElementsKey));
            if (newElements) {
                
                setProperty(kIOHIDElementKey, newElements);
                
                return kIOReturnSuccess;
            }
        }
    }
    
    return kIOReturnError;
}

void
DWXBoxHIDDriver::generateTimedEvent(OSObject *object, IOTimerEventSource *tes)
{
    DWXBoxHIDDriver *me = OSDynamicCast(DWXBoxHIDDriver, object);
    if (me) {
        
        //USBLog(1, "should generate event here...");
        if (me->_xbDeviceType->isEqualTo(kDeviceTypeIRKey)) {
            
            if (me->_buffer) {
                
                void *bytes;
                int len;
                
                bytes = me->_buffer->getBytesNoCopy();
                len = static_cast<unsigned int>(me->_buffer->getLength());  // v2.0.0 Added static_cast<unsigned int>()
                if (len == sizeof(XBRemoteReport)) {
                    
                    memset(bytes, 0, len);
                    me->handleReport(me->_buffer);
                    me->_xbLastButtonPressed = 0;
                }
            }
        }
    }
}

void
DWXBoxHIDDriver::setDefaultOptions()
{
    if (_xbDeviceType->isEqualTo(kDeviceTypePadKey)) {
        
        // fill in defaults
        _xbDeviceOptions.pad.InvertYAxis = true;
        _xbDeviceOptions.pad.InvertXAxis = false;
        _xbDeviceOptions.pad.InvertRyAxis = true;
        _xbDeviceOptions.pad.InvertRxAxis = false;
        _xbDeviceOptions.pad.ClampButtons = true;
        _xbDeviceOptions.pad.ClampLeftTrigger = false;
        _xbDeviceOptions.pad.ClampRightTrigger = false;
        //_xbDeviceOptions.pad.TriggersAreButtons = false;
        _xbDeviceOptions.pad.LeftTriggerThreshold = 1;
        _xbDeviceOptions.pad.RightTriggerThreshold = 1;
        
        // create options dict and populate it with defaults
        _xbDeviceOptionsDict = OSDictionary::withCapacity(9);
        if (_xbDeviceOptionsDict) {
            
            OSBoolean *boolean;
            OSNumber  *number;
            
#define SET_BOOLEAN(prop) \
boolean = OSBoolean::withBoolean(_xbDeviceOptions.pad.prop ); \
if (boolean) { \
_xbDeviceOptionsDict->setObject(kOption ## prop ## Key, boolean); \
boolean->release(); \
}
            
            SET_BOOLEAN(InvertYAxis)
            SET_BOOLEAN(InvertXAxis)
            SET_BOOLEAN(InvertRyAxis)
            SET_BOOLEAN(InvertRxAxis)
            SET_BOOLEAN(ClampButtons)
            SET_BOOLEAN(ClampLeftTrigger)
            SET_BOOLEAN(ClampRightTrigger)
            
            //SET_BOOLEAN(LeftTriggerIsButton)
            //SET_BOOLEAN(RightTriggerIsButton)
            
#undef SET_BOOLEAN
            
            number = OSNumber::withNumber(_xbDeviceOptions.pad.LeftTriggerThreshold, 8);
            if (number) {
                _xbDeviceOptionsDict->setObject(kOptionLeftTriggerThresholdKey, number);
                number->release();
            }
            
            number = OSNumber::withNumber(_xbDeviceOptions.pad.RightTriggerThreshold, 8);
            if (number) {
                _xbDeviceOptionsDict->setObject(kOptionRightTriggerThresholdKey, number);
                number->release();
            }
        }
    }
    else {
        
        _xbDeviceOptionsDict = OSDictionary::withCapacity(1);
    }
    
    // add options dict to our properties
    if (_xbDeviceOptionsDict)
        setProperty(kDeviceOptionsKey, _xbDeviceOptionsDict);
}

void
DWXBoxHIDDriver::setDeviceOptions()
{
    if (_xbDeviceType->isEqualTo(kDeviceTypePadKey)) {
        
        // override defaults with xml settings
        if (_xbDeviceOptionsDict) {
            
            OSBoolean *boolean;
            OSNumber  *number;
            
#define GET_BOOLEAN(field) \
boolean = OSDynamicCast(OSBoolean, _xbDeviceOptionsDict->getObject(kOption ## field ## Key)); \
if (boolean) \
_xbDeviceOptions.pad.field = boolean->getValue();
            
#define GET_UINT8_NUMBER(field) \
number = OSDynamicCast(OSNumber, _xbDeviceOptionsDict->getObject(kOption ## field ## Key)); \
if (number) \
_xbDeviceOptions.pad.field = number->unsigned8BitValue();
            
            // axis inversion
            GET_BOOLEAN(InvertYAxis)
            GET_BOOLEAN(InvertXAxis)
            GET_BOOLEAN(InvertRyAxis)
            GET_BOOLEAN(InvertRxAxis)
            
            // triggers
            GET_BOOLEAN(ClampLeftTrigger)
            GET_BOOLEAN(ClampRightTrigger)
            //GET_BOOLEAN(LeftTriggerIsButton)
            //GET_BOOLEAN(RightTriggerIsButton)
            GET_UINT8_NUMBER(LeftTriggerThreshold)
            GET_UINT8_NUMBER(RightTriggerThreshold)
            
            // buttons
            GET_BOOLEAN(ClampButtons)
            
#undef GET_BOOLEAN
#undef GET_UINT8_NUMBER
        }
    }
}

bool
DWXBoxHIDDriver::setupDevice()
{
    // called from handleStart()
    OSDictionary *deviceDict = 0;
    
    // Load the driver's deviceData dictionary
    OSDictionary *dataDict = OSDynamicCast(OSDictionary, getProperty(kDeviceDataKey));
    if (!dataDict) {
        
        USBError(1, "%s[%p]::setupDevice - no data dictionary", getName(), this);
        return false;
    }
    
    // Check that we have a device type (just in case...)
    if (_xbDeviceType) {
        
        // ...and put in our properties for clients to see
        setProperty(kTypeKey, _xbDeviceType);
    }
    
    // Get device-specific dictionary
    deviceDict = OSDynamicCast(OSDictionary, dataDict->getObject(_xbDeviceType));
    if (!deviceDict) {
        
        USBError(1, "%s[%p]::setupDevice - no device support dictionary", getName(), this);
        return false;
    }
    
    // Finally load the HID descriptor
    _xbDeviceHIDReportDescriptor = OSDynamicCast(OSData, deviceDict->getObject(kDeviceHIDReportDescriptorKey));
    if (!_xbDeviceHIDReportDescriptor ||
        _xbDeviceHIDReportDescriptor->getLength() <= 0) {
        
        USBLog(1, "%s[%p]::setupDevice - no hid descriptor for device", getName(), this);
        return false;
    }
    
    // Set the option dictionary too (note: can be NULL if device type has no options)
    // _xbDeviceOptionsDict = OSDynamicCast(OSDictionary, deviceDict->getObject(kDeviceOptionsKey));
    
    // Get the button map (remote control only - can be NULL)
    _xbDeviceButtonMapArray = OSDynamicCast(OSArray, deviceDict->getObject(kDeviceButtonMapKey));
    
    // If the device is a remote control, setup a timer for generating button-release events
    if (_xbDeviceType->isEqualTo(kDeviceTypeIRKey)) {
        
        _xbWorkLoop = getWorkLoop();
        if (_xbWorkLoop) {
            
            _xbTimerEventSource = IOTimerEventSource::timerEventSource(this, &generateTimedEvent);
            if (_xbTimerEventSource) {
                
                if (kIOReturnSuccess != _xbWorkLoop->addEventSource(_xbTimerEventSource)) {
                    
                    USBLog(1, "%s[%p]::setupDevice - couldn't establish a timer", getName(), this);
                    return false;
                }
            }
        }
    }
    
    // Set default device options
    setDefaultOptions();
    
    // Build _xbDeviceOptions structure from the device's option dictionary
    // setDeviceOptions();
    
    return true;
}


bool
DWXBoxHIDDriver::manipulateReport(IOBufferMemoryDescriptor *report)
{
    // change the report before it's sent to the HID layer
    // return true if report should be sent to HID layer,
    // so that we can ignore certain reports
    if (_xbDeviceType->isEqualTo(kDeviceTypePadKey) &&
        report->getLength() == sizeof(XBPadReport)) {
        
        XBPadReport *raw = (XBPadReport*)(report->getBytesNoCopy());
        
#define INVERT_AXIS(name) \
SInt16 name = (raw->name ## hi << 8) | raw->name ## lo; \
name = -(name + 1); \
raw->name ## hi = name >> 8; \
raw->name ## lo = name & 0xFF;
        
        if (_xbDeviceOptions.pad.InvertYAxis) {
            INVERT_AXIS(ly)
        }
        
        if (_xbDeviceOptions.pad.InvertRyAxis) {
            INVERT_AXIS(ry)
        }
        
        if (_xbDeviceOptions.pad.InvertXAxis) {
            INVERT_AXIS(lx)
        }
        
        if (_xbDeviceOptions.pad.InvertRxAxis) {
            INVERT_AXIS(rx)
        }
        
#undef INVERT_AXIS
        
        if (_xbDeviceOptions.pad.ClampButtons) {
            
            if (raw->a != 0)
                raw->a = 1;
            if (raw->b != 0)
                raw->b = 1;
            if (raw->x != 0)
                raw->x = 1;
            if (raw->y != 0)
                raw->y = 1;
            if (raw->black != 0)
                raw->black = 1;
            if (raw->white != 0)
                raw->white = 1;
        }
        
        if (_xbDeviceOptions.pad.ClampLeftTrigger) {
            
            UInt8 threshold = _xbDeviceOptions.pad.LeftTriggerThreshold;
            
            if (raw->lt < threshold)
                raw->lt = 0;
            else
                raw->lt = 1;
        }
        else
            if (_xbDeviceOptions.pad.LeftTriggerThreshold > 1) {
                
                int threshold = _xbDeviceOptions.pad.LeftTriggerThreshold;
                
                if (raw->lt < threshold) {
                    
                    raw->lt = 0;
                }
                else
                    if (threshold < 255) {
                        
                        // use this system of equations to scale values from 1-255
                        // also note the divide-by-zero check caught above
                        // 1 = a(threshold) + b
                        // 255 = a(255) + b
                        
                        raw->lt = (254*raw->lt + 255*(1 - threshold)) / (255 - threshold);
                    }
                    else {
                        
                        raw->lt = 255;
                    }
            }
        
        if (_xbDeviceOptions.pad.ClampRightTrigger) {
            
            UInt8 threshold = _xbDeviceOptions.pad.RightTriggerThreshold;
            
            if (raw->rt < threshold)
                raw->rt = 0;
            else
                raw->rt = 1;
        }
        else
            if (_xbDeviceOptions.pad.RightTriggerThreshold > 1) {
                
                UInt8 threshold = _xbDeviceOptions.pad.RightTriggerThreshold;
                
                if (raw->rt < threshold) {
                    raw->rt = 0;
                }
                else
                    if (threshold < 255) {
                        
                        // use this system of equations to scale values from 1-255
                        // 1 = a(threshold) + b
                        // 255 = a(255) + b
                        
                        raw->rt = (254*raw->rt + 255*(1 - threshold)) / (255 - threshold);
                    }
                    else {
                        
                        raw->rt = 255;
                    }
            }
    }
    else
        if (_xbDeviceType->isEqualTo(kDeviceTypeIRKey) &&
            report->getLength() == sizeof(XBActualRemoteReport) &&
            _xbDeviceButtonMapArray) {
            
            XBActualRemoteReport *raw = (XBActualRemoteReport*)(report->getBytesNoCopy());
            UInt8 scancode = raw->scancode;
            UInt8 testScancode;
            XBRemoteReport *converted = (XBRemoteReport*)raw;
            OSNumber *number;
            
            if (scancode == _xbLastButtonPressed)
                return false; // remote sends many events when holding down a button.. skip 'em
            else
                _xbLastButtonPressed = scancode;
            
            //USBLog(6, "handle remote control: scancode=%d", scancode);
            
#define SET_REPORT_FIELD(field, index) \
number = OSDynamicCast(OSNumber, _xbDeviceButtonMapArray->getObject(index)); \
if (number) { \
testScancode = number->unsigned8BitValue(); \
if (scancode == testScancode) { \
converted->field = 1; \
} \
else \
converted->field = 0; \
}
            
            SET_REPORT_FIELD(select, kRemoteSelect)
            SET_REPORT_FIELD(up, kRemoteUp)
            SET_REPORT_FIELD(down, kRemoteDown)
            SET_REPORT_FIELD(left, kRemoteLeft)
            SET_REPORT_FIELD(right, kRemoteRight)
            SET_REPORT_FIELD(title, kRemoteTitle)
            SET_REPORT_FIELD(info, kRemoteInfo)
            SET_REPORT_FIELD(menu, kRemoteMenu)
            SET_REPORT_FIELD(back, kRemoteBack)
            SET_REPORT_FIELD(display, kRemoteDisplay)
            SET_REPORT_FIELD(play, kRemotePlay)
            SET_REPORT_FIELD(stop, kRemoteStop)
            SET_REPORT_FIELD(pause, kRemotePause)
            SET_REPORT_FIELD(reverse, kRemoteReverse)
            SET_REPORT_FIELD(forward, kRemoteForward)
            SET_REPORT_FIELD(skipBackward, kRemoteSkipBackward)
            SET_REPORT_FIELD(skipForward, kRemoteSkipForward)
            SET_REPORT_FIELD(kp0, kRemoteKP0)
            SET_REPORT_FIELD(kp1, kRemoteKP1)
            SET_REPORT_FIELD(kp2, kRemoteKP2)
            SET_REPORT_FIELD(kp3, kRemoteKP3)
            SET_REPORT_FIELD(kp4, kRemoteKP4)
            SET_REPORT_FIELD(kp5, kRemoteKP5)
            SET_REPORT_FIELD(kp6, kRemoteKP6)
            SET_REPORT_FIELD(kp7, kRemoteKP7)
            SET_REPORT_FIELD(kp8, kRemoteKP8)
            SET_REPORT_FIELD(kp9, kRemoteKP9)
            
#undef SET_REPORT_FIELD
        }
    
    return true;
}

bool DWXBoxHIDDriver::isKnownDevice(IOService *provider)
{
    ///
    // Check for a known vendor and product id
    ///
    bool isKnown = false;
    
    IOUSBInterface *interface = OSDynamicCast(IOUSBInterface, provider);
    
    if (interface) {
        
        IOUSBDevice *device = interface->GetDevice();
        if (device) {
            
            char productID[8], vendorID[8];
            
            // get product and vendor id
            printf(vendorID, "%d", device->GetVendorID());	//  v2.0.0 changed sprintf to printf
            printf(productID, "%d", device->GetProductID());	//  v2.0.0 changed sprintf to printf
            
            OSDictionary *dataDict = OSDynamicCast(OSDictionary, getProperty(kDeviceDataKey));
            if (dataDict) {
                
                OSDictionary *vendors = OSDynamicCast(OSDictionary, dataDict->getObject(kKnownDevicesKey));
                if (vendors) {
                    
                    OSDictionary *vendor = OSDynamicCast(OSDictionary, vendors->getObject(vendorID));
                    if (vendor) {
                        
                        OSDictionary *product = OSDynamicCast(OSDictionary, vendor->getObject(productID));
                        if (product) {
                            
                            OSString *typeName, *deviceName, *vendorName;
                            
                            typeName = OSDynamicCast(OSString, product->getObject(kTypeKey));
                            deviceName = OSDynamicCast(OSString, product->getObject(kNameKey));
                            vendorName = OSDynamicCast(OSString, vendor->getObject(kVendorKey));
                            
                            USBLog(4,  "%s[%p]::isKnownDevice found %s %s",
                                   getName(), this, vendorName->getCStringNoCopy(), deviceName->getCStringNoCopy());
                            
                            isKnown = true;
                            
                            if (typeName)
                                _xbDeviceType = typeName;
                            else
                                isKnown = false;
                            
                            _xbDeviceName = deviceName;
                            _xbDeviceVendor = vendorName;
                        }
                    }
                }
            }
        }
    }
    
    return isKnown;
}

bool DWXBoxHIDDriver::findGenericDevice(IOService *provider)
{
    // This attempts to identify a supported "generic" device by walking the device's property
    // tree and comparing it to a known standard (Microsoft)
    
    // Unfortunately, this doesn't always work because some devices have slightly different specs
    // than the Microsoft controllers
    
    IOUSBInterface 		*interface   = 0;
    IOUSBDevice 		*device      = 0;
    OSDictionary 		*deviceDataDict = 0;        // root dictionary for all device types
    OSDictionary        *specificDeviceDict = 0;    // pad, IR, wheel, etc
    OSDictionary        *genericPropertiesDict = 0; // tree of properties that can identify device with unknown vendor/product id
    OSArray             *genericInterfaceArray = 0; // array of interfaces
    OSDictionary        *genericInterfaceDict = 0;  // interface
    OSArray             *genericEndpointArray = 0;  // array of endpoints
    OSDictionary        *genericEndpointDict = 0;   // endpoint
    
    char                const *typesList[] = { kDeviceTypePadKey, kDeviceTypeIRKey, NULL }; //  v2.0.0 added "const"
    
    bool				foundGenericDevice = false;
    
    interface = OSDynamicCast(IOUSBInterface, provider);
    if (interface) {
        
        device = interface->GetDevice();
        if (device) {
            
            deviceDataDict = OSDynamicCast(OSDictionary, getProperty(kDeviceDataKey));
            if (deviceDataDict) {
                
                for (int i = 0; typesList[i] != NULL; i++) {
                    
                    specificDeviceDict = OSDynamicCast(OSDictionary, deviceDataDict->getObject(typesList[i]));
                    if (specificDeviceDict) {
                        
                        genericPropertiesDict = OSDynamicCast(OSDictionary, specificDeviceDict->getObject(kDeviceGenericPropertiesKey));
                        if (genericPropertiesDict) {
                            
                            genericInterfaceArray = OSDynamicCast(OSArray, genericPropertiesDict->getObject(kGenericInterfacesKey));
                            if (genericInterfaceArray) {
                                
                                int numInterfaces = genericInterfaceArray->getCount();
                                int numActualInterfaces = 0;
                                bool allEndpointsMatched = true;
                                IOUSBFindInterfaceRequest request;
                                IOUSBInterface *foundInterface;
                                
                                request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
                                request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
                                request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
                                request.bAlternateSetting = kIOUSBFindInterfaceDontCare;
                                
                                foundInterface = device->FindNextInterface(NULL,&request);
                                
                                for (int j = 0; j < numInterfaces; j++) {
                                    
                                    if (foundInterface) {
                                        
                                        USBLog(6, "%s[%p]::findGenericDevice - checking interface: %d", getName(), this, j);
                                        
                                        foundInterface->retain();
                                        numActualInterfaces++;
                                        
                                        genericInterfaceDict = OSDynamicCast(OSDictionary, genericInterfaceArray->getObject(j));
                                        if (genericInterfaceDict) {
                                            
                                            genericEndpointArray = OSDynamicCast(OSArray,
                                                                                 genericInterfaceDict->getObject(kGenericEndpointsKey));
                                            if (genericEndpointArray) {
                                                
                                                int numEndpoints = genericEndpointArray->getCount();
                                                int numActualEndpoints = 0;
                                                
                                                for (int k = 0; k < numEndpoints; k++) {
                                                    
                                                    bool endPointMatched = false;
                                                    
                                                    USBLog(6, "%s[%p]::findGenericDevice - checking endpoint: %d of %d",
                                                           getName(), this, k, foundInterface->GetNumEndpoints());
                                                    
                                                    // check that index is within bounds
                                                    if (k < foundInterface->GetNumEndpoints()) {
                                                        
                                                        genericEndpointDict = OSDynamicCast(OSDictionary, genericEndpointArray->getObject(k));
                                                        if (genericEndpointDict) {
                                                            
                                                            UInt8 transferType = 0, pollingInterval = 0;
                                                            UInt16 maxPacketSize = 0;
                                                            IOReturn kr = kIOReturnError;
                                                            
                                                            UInt8 genericAttributes = 0;
                                                            UInt16 genericMaxPacketSize = 0;
                                                            UInt8 genericPollingInterval = 0;
                                                            UInt8 genericDirection = 0;
                                                            UInt8 genericIndex = 0;
                                                            
                                                            OSNumber *number;
                                                            
                                                            // read dictionary attributes
                                                            number = OSDynamicCast(OSNumber,
                                                                                   genericEndpointDict->getObject(kGenericAttributesKey));
                                                            if (number)
                                                                genericAttributes = number->unsigned8BitValue();
                                                            
                                                            number = OSDynamicCast(OSNumber,
                                                                                   genericEndpointDict->getObject(kGenericMaxPacketSizeKey));
                                                            if (number)
                                                                genericMaxPacketSize = number->unsigned16BitValue();
                                                            
                                                            number = OSDynamicCast(OSNumber,
                                                                                   genericEndpointDict->getObject(kGenericPollingIntervalKey));
                                                            if (number)
                                                                genericPollingInterval = number->unsigned8BitValue();
                                                            
                                                            if (genericAttributes & 0x80)
                                                                genericDirection = kUSBIn;
                                                            else
                                                                genericDirection = kUSBOut;
                                                            
                                                            genericIndex = genericAttributes & 0xF;
                                                            
                                                            // read device attributes
                                                            kr = foundInterface->GetEndpointProperties(0, genericIndex,
                                                                                                       genericDirection, &transferType, &maxPacketSize, &pollingInterval);
                                                            
                                                            if (kIOReturnSuccess == kr) {
                                                                
                                                                numActualEndpoints++;
                                                                
                                                                // compare device's endpoint to dictionary entry's endpoint
                                                                if (maxPacketSize == genericMaxPacketSize &&
                                                                    pollingInterval == genericPollingInterval) {
                                                                    endPointMatched = true;
                                                                    
                                                                    USBLog(6, "%s[%p]::findGenericDevice - endpoint %d matched mps=%d int=%d",
                                                                           getName(), this, k, genericMaxPacketSize, genericPollingInterval);
                                                                }
                                                                else {
                                                                    
                                                                    USBLog(6, "%s[%p]::findGenericDevice - endpoint %d rejected mps=%d int=%d",
                                                                           getName(), this, k, genericMaxPacketSize, genericPollingInterval);
                                                                }
                                                            }
                                                        }
                                                    }
                                                    
                                                    if (!endPointMatched)
                                                        allEndpointsMatched = false;
                                                }
                                                
                                                if (numEndpoints != numActualEndpoints)
                                                    allEndpointsMatched = false;
                                            }
                                        }
                                        
                                        IOUSBInterface *saveInterface = foundInterface; // save so we can call release() on it later
                                        
                                        request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
                                        request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
                                        request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
                                        request.bAlternateSetting = kIOUSBFindInterfaceDontCare;
                                        
                                        foundInterface = device->FindNextInterface(foundInterface, &request);
                                        saveInterface->release();
                                    }
                                }
                                
                                if (numInterfaces == numActualInterfaces &&
                                    allEndpointsMatched) {
                                    
                                    foundGenericDevice = true;
                                    
                                    if (typesList[i])
                                        _xbDeviceType = OSString::withCString(typesList[i]);
                                    else
                                        foundGenericDevice = false;
                                    
                                    _xbDeviceVendor = OSDynamicCast(OSString, specificDeviceDict->getObject(kVendorKey));
                                    _xbDeviceName   = OSDynamicCast(OSString, specificDeviceDict->getObject(kNameKey));
                                    if (!_xbDeviceVendor || !_xbDeviceName)
                                        foundGenericDevice = false;
                                    
                                    if (foundGenericDevice) {
                                        
                                        USBLog(3, "%s[%p]::findGenericDevice - found %s %s", getName(), this,
                                               _xbDeviceVendor->getCStringNoCopy(), _xbDeviceName->getCStringNoCopy());
                                        
                                        break; // we're done, return from for loop
                                    }
                                }
                            }
                        }
                    }
                    
                }
            }
        }
    }
    
    return foundGenericDevice;
}

IOService* DWXBoxHIDDriver::probe(IOService *provider, SInt32 *score)
{
    if (this->isKnownDevice(provider)) {
        
        USBLog(3,  "%s[%p]::probe found known device", getName(), this);
        
        // pump up our probe score, since we're probably the best driver
        *score += 10000;
    }
    else
        if (this->findGenericDevice(provider)) {
            
            // there might be a better driver, so don't increase the score
            USBLog(3,  "%s[%p]::probe found generic device", getName(), this);
            *score += 1000;
        }
        else {
            
            // device is unknown *and* doesn't match known generic properties,
            // we can assume it is a controller (usually!) If we're wrong
            // the application using HID might hang, or perhaps the kernel
            // will crash. On the upside I'll have less complaints about
            // controllers not working because their vendor/product IDs
            // are not compiled into the driver.
            USBLog(3,  "%s[%p]::probe didn't find supported device, taking a risk here", getName(), this);
            
            _xbDeviceType   = OSString::withCString("Pad");
            _xbDeviceVendor = OSString::withCString("Unknown");
            _xbDeviceName   = OSString::withCString("Generic Controller");
            
            *score += 100;
        }
    
    return this;
}



// ***********************************************************************************
// ************************ HID Driver Dispatch Table Functions *********************
// **********************************************************************************

IOReturn
DWXBoxHIDDriver::GetReport(UInt8 inReportType, UInt8 inReportID, UInt8 *vInBuf, UInt32 *vInSize)
{
    return kIOReturnSuccess;
}

IOReturn
DWXBoxHIDDriver::getReport( IOMemoryDescriptor * report,
                           IOHIDReportType      reportType,
                           IOOptionBits         options )
{
    //UInt8     reportID;
    IOReturn        ret = kIOReturnSuccess;
    UInt8       usbReportType;
    //IOUSBDevRequestDesc requestPB;
    
    IncrementOutstandingIO();
    
    // Get the reportID from the lower 8 bits of options
    ////
    //reportID = (UInt8) ( options & 0x000000ff);
    
    // And now save the report type
    //
    usbReportType = HIDMgr2USBReportType(reportType);
    
    USBLog(6, "%s[%p]::getReport (type=%d len=%u)", getName(), this,
        usbReportType, (unsigned int) report->getLength());	//  v2.0.0 added (unsigned int)
    
    if (kUSBIn == usbReportType || kUSBNone == usbReportType) {
        
        // don't support this on remote controls - it can block indefinitely until a button is pressed
        if (! _xbDeviceType->isEqualTo(kDeviceTypeIRKey))
            ret = _interruptPipe->Read(report);
    }
    else {
        
        USBLog(3, "%s[%p]::getReport (type=%d len=%u): error operation unsupported", getName(), this,
            usbReportType, (unsigned int) report->getLength());	//  v2.0.0 added (unsigned int)
        ret = kIOReturnError;
    }
    
    //--- Fill out device request form
    //
    //requestPB.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBClass, kUSBInterface);
    //requestPB.bRequest = kHIDRqGetReport;
    //requestPB.wValue = (usbReportType << 8) | reportID;
    //requestPB.wIndex = _interface->GetInterfaceNumber();
    //requestPB.wLength = report->getLength();
    //requestPB.pData = report;
    //requestPB.wLenDone = 0;
    
    //ret = _device->DeviceRequest(&requestPB);
    //if ( ret != kIOReturnSuccess )
    //        USBLog(3, "%s[%p]::getReport request failed; err = 0x%x)", getName(), this, ret);
    
    DecrementOutstandingIO();
    return ret;
}


// DEPRECATED
//
IOReturn
DWXBoxHIDDriver::SetReport(UInt8 outReportType, UInt8 outReportID, UInt8 *vOutBuf, UInt32 vOutSize)
{
    return kIOReturnSuccess;
}

IOReturn
DWXBoxHIDDriver::setReport( IOMemoryDescriptor *    report,
                           IOHIDReportType         reportType,
                           IOOptionBits            options)
{
    UInt8       reportID;
    IOReturn    ret;
    UInt8       usbReportType;
    IOUSBDevRequestDesc requestPB;
    
    IncrementOutstandingIO();
    
    // Get the reportID from the lower 8 bits of options
    //
    reportID = (UInt8) ( options & 0x000000ff);
    
    // And now save the report type
    //
    usbReportType = HIDMgr2USBReportType(reportType);
    
    // If we have an interrupt out pipe, try to use it for output type of reports.
    if ( kHIDOutputReport == usbReportType && _interruptOutPipe )
    {
#if ENABLE_HIDREPORT_LOGGING
        USBLog(3, "%s[%p]::setReport sending out interrupt out pipe buffer (%p,%d):", getName(), this, report, report->getLength() );
        LogMemReport(report);
#endif
        ret = _interruptOutPipe->Write(report);
        if (ret == kIOReturnSuccess)
        {
            DecrementOutstandingIO();
            return ret;
        }
        else
        {
            USBLog(3, "%s[%p]::setReport _interruptOutPipe->Write failed; err = 0x%x)", getName(), this, ret);
        }
    }
    
    // If we did not succeed using the interrupt out pipe, we may still be able to use the control pipe.
    // We'll let the family check whether it's a disjoint descriptor or not (but right now it doesn't do it)
    //
#if ENABLE_HIDREPORT_LOGGING
    USBLog(3, "%s[%p]::SetReport sending out control pipe:", getName(), this);
    LogMemReport( report);
#endif
    
    //--- Fill out device request form
    requestPB.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
    requestPB.bRequest = kHIDRqSetReport;
    requestPB.wValue = (usbReportType << 8) | reportID;
    requestPB.wIndex = _interface->GetInterfaceNumber();
    requestPB.wLength = report->getLength();
    requestPB.pData = report;
    requestPB.wLenDone = 0;
    
    ret = _device->DeviceRequest(&requestPB);
    if (ret != kIOReturnSuccess)
        USBLog(3, "%s[%p]::setReport request failed; err = 0x%x)", getName(), this, ret);
    
    DecrementOutstandingIO();
    return ret;
}


// HIDGetHIDDescriptor is used to get a specific HID descriptor from a HID device
// (such as a report descriptor).
IOReturn
DWXBoxHIDDriver::GetHIDDescriptor(UInt8 inDescriptorType, UInt8 inDescriptorIndex, UInt8 *vOutBuf, UInt32 *vOutSize)
{
    //IOUSBDevRequest       requestPB;
    IOUSBHIDDescriptor      *theHIDDesc;
    IOUSBHIDReportDesc      *hidTypeSizePtr;    // For checking owned descriptors.
    UInt8           *descPtr;
    UInt32          providedBufferSize;
    UInt16          descSize;
    UInt8           descType;
    UInt8           typeIndex;
    UInt8           numberOwnedDesc;
    IOReturn        err = kIOReturnSuccess;
    Boolean         foundIt;
    
    if (!vOutSize)
        return  kIOReturnBadArgument;
    
    if (!_interface)
    {
        USBLog(2, "%s[%p]::GetHIDDescriptor - no _interface", getName(), this);
        return kIOReturnNotFound;
    }
    
    // From the interface descriptor, get the HID descriptor.
    // theHIDDesc = (IOUSBHIDDescriptor *)_interface->FindNextAssociatedDescriptor(NULL, kUSBHIDDesc);
    if (!_xbDeviceHIDReportDescriptor)
        return kIOReturnError;
    
    UInt16 descDataSize = _xbDeviceHIDReportDescriptor->getLength();
    
    IOUSBHIDDescriptor hidDescriptor =
    {
        sizeof(IOUSBHIDDescriptor),   // descLen
        kUSBHIDDesc,                  // descType
        0x0111,                       // descVersNum (1.11)
        0,                            // hidCountryCode
        1,                            // hidNumDescriptors
        kUSBReportDesc,               // hidDescriptorType - Table 7.1.2
        static_cast<UInt8>(descDataSize & 0xFF),          // hidDescriptorLengthLo	//  v2.0.0 added static_cast<UInt8>
        static_cast<UInt8>((descDataSize & 0xFF00) >> 8)  // hidDescriptorLengthHi	//  v2.0.0 added static_cast<UInt8>
    };
    
    theHIDDesc = (IOUSBHIDDescriptor*)&hidDescriptor;
    
    if (theHIDDesc == NULL)
    {
        USBLog(2, "%s[%p]::GetHIDDescriptor - FindNextAssociatedDescriptor(NULL, kUSBHIDDesc) failed", getName(), this);
        return kIOReturnNotFound;
    }
    
    //USBLog (6, "HID Descriptor: descLen=%d\n\tdescType=0x%X\n\thidCountryCode=0x%X\n\thidNumDescriptors=%d\n\thidDescriptorType=0x%X\n\thidDescriptorLengthLo=%d\n\thidDescriptorLengthHi=%d\n", theHIDDesc->descLen, theHIDDesc->descType, theHIDDesc->hidCountryCode, theHIDDesc->hidNumDescriptors, theHIDDesc->hidDescriptorType, theHIDDesc->hidDescriptorLengthLo, theHIDDesc->hidDescriptorLengthHi);
    
    // Remember the provided buffer size
    providedBufferSize = *vOutSize;
    
    // Are we looking for just the main HID descriptor?
    if (inDescriptorType == kUSBHIDDesc || (inDescriptorType == 0 && inDescriptorIndex == 0))
    {
        descSize = theHIDDesc->descLen;
        descPtr = (UInt8*)theHIDDesc;
        
        // No matter what, set the return size to the actual size of the data.
        *vOutSize = descSize;
        
        // If the provided size is 0, they are just asking for the size, so don't return an error.
        if (providedBufferSize == 0)
            err = kIOReturnSuccess;
        // Otherwise, if the buffer too small, return buffer too small error.
        else if (descSize > providedBufferSize)
            err = kIOReturnNoSpace;
        // Otherwise, if the buffer nil, return that error.
        else if (vOutBuf == NULL)
            err = kIOReturnBadArgument;
        // Otherwise, looks good, so copy the deiscriptor.
        else
        {
            //IOLog("  Copying HIDDesc w/ vOutBuf = 0x%x, descPtr = 0x%x, and descSize = 0x%x.\n", vOutBuf, descPtr, descSize);
            memcpy(vOutBuf, descPtr, descSize);
        }
    }
    else
    {
        // Looking for a particular type of descriptor.
        // The HID descriptor tells how many endpoint and report descriptors it contains.
        numberOwnedDesc = ((IOUSBHIDDescriptor *)theHIDDesc)->hidNumDescriptors;
        hidTypeSizePtr = (IOUSBHIDReportDesc *)&((IOUSBHIDDescriptor *)theHIDDesc)->hidDescriptorType;
        //USBLog (6, "     %d owned descriptors start at %08x\n", numberOwnedDesc, (unsigned int)hidTypeSizePtr);
        
        typeIndex = 0;
        foundIt = false;
        err = kIOReturnNotFound;
        for (UInt8 i = 0; i < numberOwnedDesc; i++)
        {
            descType = hidTypeSizePtr->hidDescriptorType;
            
            //USBLog (6, "descType=0x%X lengthhi=%d lengthlo=%d", descType,
            //    hidTypeSizePtr->hidDescriptorLengthHi, hidTypeSizePtr->hidDescriptorLengthLo);
            
            // Are we indexing for a specific type?
            if (inDescriptorType != 0)
            {
                if (inDescriptorType == descType)
                {
                    if (inDescriptorIndex == typeIndex)
                    {
                        foundIt = true;
                        //USBLog (6, "found it!", descType);
                    }
                    else
                    {
                        typeIndex++;
                    }
                }
            }
            
            // Otherwise indexing across descriptors in general.
            // (If looking for any type, index must be 1 based or we'll get HID descriptor.)
            else if (inDescriptorIndex == i + 1)
            {
                //IOLog("  said we found it because inDescriptorIndex = 0x%x.\n", inDescriptorIndex);
                typeIndex = i;
                foundIt = true;
            }
            
            if (foundIt)
            {
                err = kIOReturnSuccess;     // Maybe
                //IOLog("     Found the requested owned descriptor, %d.\n", i);
                descSize = (hidTypeSizePtr->hidDescriptorLengthHi << 8) + hidTypeSizePtr->hidDescriptorLengthLo;
                
                //USBLog (6, "descSize=%d", descSize);
                
                // Did we just want the size or the whole descriptor?
                // No matter what, set the return size to the actual size of the data.
                *vOutSize = descSize;   // OSX: Won't get back if we return an error!
                
                // If the provided size is 0, they are just asking for the size, so don't return an error.
                if (providedBufferSize == 0)
                    err = kIOReturnSuccess;
                // Otherwise, if the buffer too small, return buffer too small error.
                else if (descSize > providedBufferSize)
                    err = kIOReturnNoSpace;
                // Otherwise, if the buffer nil, return that error.
                else if (vOutBuf == NULL)
                    err = kIOReturnBadArgument;
                // Otherwise, looks good, so copy the descriptor.
                else
                {
                    if (!_device)
                    {
                        USBLog(2, "%s[%p]::GetHIDDescriptor - no _device", getName(), this);
                        return kIOReturnNotFound;
                    }
                    
                    if (descSize == _xbDeviceHIDReportDescriptor->getLength() &&
                        _xbDeviceHIDReportDescriptor->getBytesNoCopy()) {
                        
                        memcpy(vOutBuf, _xbDeviceHIDReportDescriptor->getBytesNoCopy(), descSize);
                    }
                    else {
                        
                        USBLog(2, "%s[%p]::GetHIDDescriptor - hid report desc wrong size", getName(), this);
                        return kIOReturnError;
                    }
                    
                    //IOLog("  Requesting new desscriptor.\n");
                    /*
                     requestPB.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBInterface);
                     requestPB.bRequest = kUSBRqGetDescriptor;
                     requestPB.wValue = (inDescriptorType << 8) + typeIndex;     // type and index
                     requestPB.wIndex = _interface->GetInterfaceNumber();
                     requestPB.wLength = descSize;
                     requestPB.pData = vOutBuf;                      // So we don't have to do any allocation here.
                     err = _device->DeviceRequest(&requestPB, 5000, 0);
                     if (err != kIOReturnSuccess)
                     {
                     USBLog(3, "%s[%p]::GetHIDDescriptor Final request failed; err = 0x%x", getName(), this, err);
                     return err;
                     }
                     */
                }
                break;  // out of for i loop.
            }
            // Make sure we add 3 bytes not 4 regardless of struct alignment.
            hidTypeSizePtr = (IOUSBHIDReportDesc *)(((UInt8 *)hidTypeSizePtr) + 3);
        }
    }
    return err;
}

IOReturn
DWXBoxHIDDriver::newReportDescriptor(IOMemoryDescriptor ** desc) const
{
    IOBufferMemoryDescriptor * bufferDesc = NULL;
    IOReturn ret = kIOReturnNoMemory;
    DWXBoxHIDDriver * me = (DWXBoxHIDDriver *) this;
    
    // Get the proper HID report descriptor size.
    UInt32 inOutSize = 0;
    ret = me->GetHIDDescriptor(kUSBReportDesc, 0, NULL, &inOutSize);
    
    if ( ret == kIOReturnSuccess &&  inOutSize != 0)
    {
        bufferDesc = IOBufferMemoryDescriptor::withCapacity(inOutSize, kIODirectionOutIn);
    }
    
    if (bufferDesc)
    {
        ret = me->GetHIDDescriptor(kUSBReportDesc, 0, (UInt8 *)bufferDesc->getBytesNoCopy(), &inOutSize);
        
        if ( ret != kIOReturnSuccess )
        {
            bufferDesc->release();
            bufferDesc = NULL;
        }
    }
    
    *desc = bufferDesc;
    
    return ret;
}


OSString *
DWXBoxHIDDriver::newTransportString() const
{
    return OSString::withCString("USB");
}


OSNumber *
DWXBoxHIDDriver::newPrimaryUsageNumber() const
{
    return OSNumber::withNumber(_deviceUsage, 32);
}


OSNumber *
DWXBoxHIDDriver::newPrimaryUsagePageNumber() const
{
    return OSNumber::withNumber(_deviceUsagePage, 32);
}


OSNumber *
DWXBoxHIDDriver::newVendorIDNumber() const
{
    UInt16 vendorID = 0;
    
    if (_device != NULL)
        vendorID = _device->GetVendorID();
    
    return OSNumber::withNumber(vendorID, 16);
}


OSNumber *
DWXBoxHIDDriver::newProductIDNumber() const
{
    UInt16 productID = 0;
    
    if (_device != NULL)
        productID = _device->GetProductID();
    
    return OSNumber::withNumber(productID, 16);
}


OSNumber *
DWXBoxHIDDriver::newVersionNumber() const
{
    UInt16 releaseNum = 0;
    
    if (_device != NULL)
        releaseNum = _device->GetDeviceRelease();
    
    return OSNumber::withNumber(releaseNum, 16);
}


UInt32
DWXBoxHIDDriver::getMaxReportSize()
{
    return _maxReportSize;
}


OSString *
DWXBoxHIDDriver::newManufacturerString() const
{
    char    manufacturerString[256];
    UInt32  strSize;
    UInt8   index;
    IOReturn    err;
    
    manufacturerString[0] = 0;
    
    index = _device->GetManufacturerStringIndex();
    strSize = sizeof(manufacturerString);
    
    err = GetIndexedString(index, (UInt8 *)manufacturerString, &strSize);
    
    if ( err == kIOReturnSuccess )
        return OSString::withCString(manufacturerString);
    else
        if (_xbDeviceVendor)
            return OSString::withString(_xbDeviceVendor);
        else
            return NULL;
}


OSString *
DWXBoxHIDDriver::newProductString() const
{
    char    productString[256];
    UInt32  strSize;
    UInt8   index;
    IOReturn    err;
    
    productString[0] = 0;
    
    index = _device->GetProductStringIndex();
    strSize = sizeof(productString);
    
    err = GetIndexedString(index, (UInt8 *)productString, &strSize);
    
    if ( err == kIOReturnSuccess )
        return OSString::withCString(productString);
    else
        if (_xbDeviceName)
            return OSString::withString(_xbDeviceName);
        else
            return NULL;
}


OSString *
DWXBoxHIDDriver::newSerialNumberString() const
{
    char    serialNumberString[256];
    UInt32  strSize;
    UInt8   index;
    IOReturn    err;
    
    serialNumberString[0] = 0;
    
    index = _device->GetSerialNumberStringIndex();
    strSize = sizeof(serialNumberString);
    
    err = GetIndexedString(index, (UInt8 *)serialNumberString, &strSize);
    
    if ( err == kIOReturnSuccess )
        return OSString::withCString(serialNumberString);
    else
        return NULL;
}


OSNumber *
DWXBoxHIDDriver::newLocationIDNumber() const
{
    OSNumber * newLocationID = NULL;
    
    if (_interface != NULL)
    {
        OSNumber * locationID = (OSNumber *)_interface->getProperty(kUSBDevicePropertyLocationID);
        if ( locationID )
            // I should be able to just duplicate locationID, but no OSObject::clone() or such.
            newLocationID = OSNumber::withNumber(locationID->unsigned32BitValue(), 32);
    }
    
    return newLocationID;
}


IOReturn
DWXBoxHIDDriver::GetIndexedString(UInt8 index, UInt8 *vOutBuf, UInt32 *vOutSize, UInt16 lang) const
{
    char    strBuf[256];
    UInt16  strLen = sizeof(strBuf) - 1;    // GetStringDescriptor MaxLen = 255
    UInt32  outSize = *vOutSize;
    IOReturn    err;
    
    // Valid string index?
    if (index == 0)
    {
        return kIOReturnBadArgument;
    }
    
    // Valid language?
    if (lang == 0)
    {
        lang = 0x409;   // Default is US English.
    }
    
    err = _device->GetStringDescriptor((UInt8)index, strBuf, strLen, (UInt16)lang);
    // When string is returned, it has been converted from Unicode and is null terminated!
    
    /*
     err = kIOReturnSuccess;
     
     if (index > 0 && index < kPadNumStrings) {
     
     strncpy(strBuf, gPadStringTable[index], sizeof(strBuf));
     strBuf[sizeof(strBuf)-1] = '\0'; // strncpy doesn't \0 terminate when limit is reached
     }
     else {
     
     err = kIOReturnError;
     }
     */
    
    if (err != kIOReturnSuccess)
    {
        return err;
    }
    
    // We return the length of the string plus the null terminator,
    // but don't say a null string is 1 byte long.
    strLen = (strBuf[0] == 0) ? 0 : strlen(strBuf) + 1;
    
    if (outSize == 0)
    {
        *vOutSize = strLen;
        return kIOReturnSuccess;
    }
    else if (outSize < strLen)
    {
        return kIOReturnMessageTooLarge;
    }
    
    strlcpy((char *)vOutBuf, strBuf, sizeof(vOutBuf)); // v2.0.0 Changed strncpy to strlcpy and added sizeof(vOutBuf)
    *vOutSize = strLen;
    return kIOReturnSuccess;
}

OSString *
DWXBoxHIDDriver::newIndexedString(UInt8 index) const
{
    char string[256];
    UInt32 strSize;
    IOReturn    err = kIOReturnSuccess;
    
    string[0] = 0;
    strSize = sizeof(string);
    
    err = GetIndexedString(index, (UInt8 *)string, &strSize );
    
    if ( err == kIOReturnSuccess )
        return OSString::withCString(string);
    else
        return NULL;
}


IOReturn
DWXBoxHIDDriver::message( UInt32 type, IOService * provider,  void * argument )
{
    IOReturn    err = kIOReturnSuccess;
    
    err = super::message (type, provider, argument);
    
    switch ( type )
    {
        case kIOMessageServiceIsTerminated:
            USBLog(5, "%s[%p]: service is terminated - ignoring", getName(), this);
            break;
            
        case kIOUSBMessagePortHasBeenReset:
            USBLog(3, "%s[%p]: received kIOUSBMessagePortHasBeenReset", getName(), this);
            _retryCount = kHIDDriverRetryCount;
            _deviceIsDead = FALSE;
            _deviceHasBeenDisconnected = FALSE;
            
            IncrementOutstandingIO();
            err = _interruptPipe->Read(_buffer, &_completion);
            if (err != kIOReturnSuccess)
            {
                DecrementOutstandingIO();
                USBLog(3, "%s[%p]::message - err (%x) in interrupt read", getName(), this, err);
                // _interface->close(this); will be done in didTerminate
            }
            break;
            
        default:
            break;
    }
    
    return kIOReturnSuccess;
}


bool
DWXBoxHIDDriver::willTerminate( IOService * provider, IOOptionBits options )
{
    // this method is intended to be used to stop any pending I/O and to make sure that
    // we have begun getting our callbacks in order. by the time we get here, the
    // isInactive flag is set, so we really are marked as being done. we will do in here
    // what we used to do in the message method (this happens first)
    USBLog(3, "%s[%p]::willTerminate isInactive = %d", getName(), this, isInactive());
    if (_interruptPipe)
        _interruptPipe->Abort();
    
    return super::willTerminate(provider, options);
}



bool
DWXBoxHIDDriver::didTerminate( IOService * provider, IOOptionBits options, bool * defer )
{
    // this method comes at the end of the termination sequence. Hopefully, all of our outstanding IO is complete
    // in which case we can just close our provider and IOKit will take care of the rest. Otherwise, we need to
    // hold on to the device and IOKit will terminate us when we close it later
    USBLog(3, "%s[%p]::didTerminate isInactive = %d, outstandingIO = %u", getName(), this, isInactive(), (unsigned int) _outstandingIO);	//  v2.0.0 added (unsigned int)
    if (!_outstandingIO)
        _interface->close(this);
    else
        _needToClose = true;
    return super::didTerminate(provider, options, defer);
}



bool
DWXBoxHIDDriver::start(IOService *provider)
{
    IOReturn        err = kIOReturnSuccess;
    IOWorkLoop      *wl = NULL;
    
    USBLog(7, "%s[%p]::start", getName(), this);
    IncrementOutstandingIO();           // make sure that once we open we don't close until start is open
    bool ret = super::start(provider);
    if (!ret) {
        USBLog(1, "%s[%p]::start - failed to start provider", getName(), this);
    }
    if (ret)
        do {
            // OK - at this point IOHIDDevice has successfully started, and now we need to start out interrupt pipe
            // read. we have not initialized a bunch of this stuff yet, because we needed to wait to see if
            // IOHIDDevice::start succeeded or not
            IOUSBFindEndpointRequest    request;
            
            USBLog(7, "%s[%p]::start - getting _gate", getName(), this);
            _gate = IOCommandGate::commandGate(this);
            
            if(!_gate)
            {
                USBError(1, "%s[%p]::start - unable to create command gate", getName(), this);
                break;
            }
            
            wl = getWorkLoop();
            if (!wl)
            {
                USBError(1, "%s[%p]::start - unable to find my workloop", getName(), this);
                break;
            }
            
            if (wl->addEventSource(_gate) != kIOReturnSuccess)
            {
                USBError(1, "%s[%p]::start - unable to add gate to work loop", getName(), this);
                break;
            }
            
            // Errata for ALL Saitek devices.  Do a SET_IDLE 0 call
            if ( (_device->GetVendorID()) == 0x06a3 )
                SetIdleMillisecs(0);
            
            request.type = kUSBInterrupt;
            request.direction = kUSBOut;
            _interruptOutPipe = _interface->FindNextPipe(NULL, &request);
            
            request.type = kUSBInterrupt;
            request.direction = kUSBIn;
            _interruptPipe = _interface->FindNextPipe(NULL, &request);
            
            if(!_interruptPipe)
            {
                USBError(1, "%s[%p]::start - unable to get interrupt pipe", getName(), this);
                break;
            }
            
            _maxReportSize = getMaxReportSize();
            if (_maxReportSize)
            {
                _buffer = IOBufferMemoryDescriptor::withCapacity(_maxReportSize, kIODirectionIn);
                if ( !_buffer )
                {
                    USBError(1, "%s[%p]::start - unable to get create buffer", getName(), this);
                    break;
                }
            }
            
            
            // allocate a thread_call structure
            _deviceDeadCheckThread = thread_call_allocate((thread_call_func_t)CheckForDeadDeviceEntry, (thread_call_param_t)this);
            _clearFeatureEndpointHaltThread = thread_call_allocate((thread_call_func_t)ClearFeatureEndpointHaltEntry, (thread_call_param_t)this);
            
            if ( !_deviceDeadCheckThread || !_clearFeatureEndpointHaltThread )
            {
                USBError(1, "[%s]%p: could not allocate all thread functions", getName(), this);
                break;
            }
            
            err = StartFinalProcessing();
            if (err != kIOReturnSuccess)
            {
                USBError(1, "%s[%p]::start - err (%x) in StartFinalProcessing", getName(), this, err);
                break;
            }
            
            USBError(1, "%s[%p]::start -  USB HID Device @ %d (0x%lx)", getName(), this, _device->GetAddress(), strtoul(_device->getLocation(), (char **)NULL, 16));	//  v2.0.0 changed "%x" to "%lx"
            
            DecrementOutstandingIO();       // release the hold we put on at the beginning
            
            return true;
            
        } while (false);
    
    USBError(1, "%s[%p]::start - aborting startup", getName(), this);
    if (_gate)
    {
        if (wl)
            wl->removeEventSource(_gate);
        
        _gate->release();
        _gate = NULL;
    }
    
    if (_deviceDeadCheckThread)
        thread_call_free(_deviceDeadCheckThread);
    
    if (_clearFeatureEndpointHaltThread)
        thread_call_free(_clearFeatureEndpointHaltThread);
    
    if (_interface)
        _interface->close(this);
    
    DecrementOutstandingIO();       // release the hold we put on at the beginning
    return false;
}


//=============================================================================================
//
//  InterruptReadHandlerEntry is called to process any data coming in through our interrupt pipe
//
//=============================================================================================
//
void
DWXBoxHIDDriver::InterruptReadHandlerEntry(OSObject *target, void *param, IOReturn status, UInt32 bufferSizeRemaining)
{
    DWXBoxHIDDriver *   me = OSDynamicCast(DWXBoxHIDDriver, target);
    
    if (!me)
        return;
    
    me->InterruptReadHandler(status, bufferSizeRemaining);
    me->DecrementOutstandingIO();
}


void
DWXBoxHIDDriver::InterruptReadHandler(IOReturn status, UInt32 bufferSizeRemaining)
{
    bool        queueAnother = true;
    bool        timeToGoAway = false;
    IOReturn        err = kIOReturnSuccess;
    
    switch (status)
    {
        case kIOReturnOverrun:
            USBLog(3, "%s[%p]::InterruptReadHandler kIOReturnOverrun error", getName(), this);
            // This is an interesting error, as we have the data that we wanted and more...  We will use this
            // data but first we need to clear the stall and reset the data toggle on the device.  We will not
            // requeue another read because our _clearFeatureEndpointHaltThread will requeue it.  We then just
            // fall through to the kIOReturnSuccess case.
            // 01-18-02 JRH If we are inactive, then ignore this
            if (!isInactive())
            {
                //
                // First, clear the halted bit in the controller
                //
                _interruptPipe->ClearStall();
                
                // And call the device to reset the endpoint as well
                //
                IncrementOutstandingIO();
                thread_call_enter(_clearFeatureEndpointHaltThread);
            }
            queueAnother = false;
            timeToGoAway = false;
            
            // Fall through to process the data.
            
        case kIOReturnSuccess:
            // Reset the retry count, since we had a successful read
            //
            _retryCount = kHIDDriverRetryCount;
            
            // Handle the data
            //
#if ENABLE_HIDREPORT_LOGGING
            USBLog(6, "%s[%p]::InterruptReadHandler report came in:", getName(), this);
            LogMemReport(_buffer);
#endif
            if (manipulateReport(_buffer))
                handleReport(_buffer);
            
            if (_xbDeviceType->isEqualTo(kDeviceTypeIRKey))
                if (_xbTimerEventSource) {
                    _xbTimerEventSource->cancelTimeout();
                    _xbTimerEventSource->setTimeoutMS(_xbTimedEventsInterval);
                }
            
            if (isInactive())
                queueAnother = false;
            
            break;
            
        case kIOReturnNotResponding:
            USBLog(3, "%s[%p]::InterruptReadHandler kIOReturnNotResponding error", getName(), this);
            // If our device has been disconnected or we're already processing a
            // terminate message, just go ahead and close the device (i.e. don't
            // queue another read.  Otherwise, go check to see if the device is
            // around or not.
            //
            if ( _deviceHasBeenDisconnected || isInactive() )
            {
                queueAnother = false;
                timeToGoAway = true;
            }
            else
            {
                USBLog(3, "%s[%p]::InterruptReadHandler Checking to see if HID device is still connected", getName(), this);
                IncrementOutstandingIO();
                thread_call_enter(_deviceDeadCheckThread);
                
                // Before requeueing, we need to clear the stall
                //
                _interruptPipe->ClearStall();
            }
            
            break;
            
        case kIOReturnAborted:
            // This generally means that we are done, because we were unplugged, but not always
            //
            if (isInactive() || _deviceIsDead )
            {
                USBLog(3, "%s[%p]::InterruptReadHandler error kIOReturnAborted (expected)", getName(), this);
                queueAnother = false;
                timeToGoAway = true;
            }
            else
            {
                USBLog(3, "%s[%p]::InterruptReadHandler error kIOReturnAborted. Try again.", getName(), this);
            }
            break;
            
        case kIOReturnUnderrun:
        case kIOUSBPipeStalled:
        case kIOUSBLinkErr:
        case kIOUSBNotSent2Err:
        case kIOUSBNotSent1Err:
        case kIOUSBBufferUnderrunErr:
        case kIOUSBBufferOverrunErr:
        case kIOUSBWrongPIDErr:
        case kIOUSBPIDCheckErr:
        case kIOUSBDataToggleErr:
        case kIOUSBBitstufErr:
        case kIOUSBCRCErr:
            // These errors will halt the endpoint, so before we requeue the interrupt read, we have
            // to clear the stall at the controller and at the device.  We will not requeue the read
            // until after we clear the ENDPOINT_HALT feature.  We need to do a callout thread because
            // we are executing inside the gate here and we cannot issue a synchronous request.
            USBLog(3, "%s[%p]::InterruptReadHandler OHCI error (0x%x) reading interrupt pipe", getName(), this, status);
            // 01-18-02 JRH If we are inactive, then ignore this
            if (!isInactive())
            {
                // First, clear the halted bit in the controller
                //
                _interruptPipe->ClearStall();
                
                // And call the device to reset the endpoint as well
                //
                IncrementOutstandingIO();
                thread_call_enter(_clearFeatureEndpointHaltThread);
            }
            // We don't want to requeue the read here, AND we don't want to indicate that we are done
            //
            queueAnother = false;
            break;
        default:
            // We should handle other errors more intelligently, but
            // for now just return and assume the error is recoverable.
            USBLog(3, "%s[%p]::InterruptReadHandler error (0x%x) reading interrupt pipe", getName(), this, status);
            if (isInactive())
                queueAnother = false;
            
            break;
    }
    
    if ( queueAnother )
    {
        // Queue up another one before we leave.
        //
        IncrementOutstandingIO();
        err = _interruptPipe->Read(_buffer, &_completion);
        if ( err != kIOReturnSuccess)
        {
            // This is bad.  We probably shouldn't continue on from here.
            USBError(1, "%s[%p]::InterruptReadHandler immediate error 0x%x queueing read\n", getName(), this, err);
            DecrementOutstandingIO();
            timeToGoAway = true;
        }
    }
}


//=============================================================================================
//
//  CheckForDeadDevice is called when we get a kIODeviceNotResponding error in our interrupt pipe.
//  This can mean that (1) the device was unplugged, or (2) we lost contact
//  with our hub.  In case (1), we just need to close the driver and go.  In
//  case (2), we need to ask if we are still attached.  If we are, then we update
//  our retry count.  Once our retry count (3 from the 9 sources) are exhausted, then we
//  issue a DeviceReset to our provider, with the understanding that we will go
//  away (as an interface).
//
//=============================================================================================
//
void
DWXBoxHIDDriver::CheckForDeadDeviceEntry(OSObject *target)
{
    DWXBoxHIDDriver *   me = OSDynamicCast(DWXBoxHIDDriver, target);
    
    if (!me)
        return;
    
    me->CheckForDeadDevice();
    me->DecrementOutstandingIO();
}

void
DWXBoxHIDDriver::CheckForDeadDevice()
{
    IOReturn            err = kIOReturnSuccess;
    
    // Are we still connected?  Don't check again if we're already
    // checking
    //
    if ( _interface && _device && !_deviceDeadThreadActive)
    {
        _deviceDeadThreadActive = TRUE;
        
        err = _device->message(kIOUSBMessageHubIsDeviceConnected, NULL, 0);
        
        if ( kIOReturnSuccess == err )
        {
            // Looks like the device is still plugged in.  Have we reached our retry count limit?
            //
            if ( --_retryCount == 0 )
            {
                _deviceIsDead = TRUE;
                USBLog(3, "%s[%p]: Detected an kIONotResponding error but still connected.  Resetting port", getName(), this);
                
                if (_interruptPipe)
                    _interruptPipe->Abort();  // This will end up closing the interface as well.
                
                // OK, let 'er rip.  Let's do the reset thing
                //
                _device->ResetDevice();
            }
        }
        else
        {
            // Device is not connected -- our device has gone away.  The message kIOServiceIsTerminated
            // will take care of shutting everything down.
            //
            _deviceHasBeenDisconnected = TRUE;
            USBLog(5, "%s[%p]: CheckForDeadDevice: device has been unplugged", getName(), this);
        }
        _deviceDeadThreadActive = FALSE;
    }
}


//=============================================================================================
//
//  ClearFeatureEndpointHaltEntry is called when we get an OHCI error from our interrupt read
//  (except for kIOReturnNotResponding  which will check for a dead device).  In these cases
//  we need to clear the halted bit in the controller AND we need to reset the data toggle on the
//  device.
//
//=============================================================================================
//
void
DWXBoxHIDDriver::ClearFeatureEndpointHaltEntry(OSObject *target)
{
    DWXBoxHIDDriver *   me = OSDynamicCast(DWXBoxHIDDriver, target);
    
    if (!me)
        return;
    
    me->ClearFeatureEndpointHalt();
    me->DecrementOutstandingIO();
}

void
DWXBoxHIDDriver::ClearFeatureEndpointHalt( )
{
    IOReturn            status;
    IOUSBDevRequest     request;
    
    // Clear out the structure for the request
    //
    bzero( &request, sizeof(IOUSBDevRequest));
    
    // Build the USB command to clear the ENDPOINT_HALT feature for our interrupt endpoint
    //
    request.bmRequestType   = USBmakebmRequestType(kUSBNone, kUSBStandard, kUSBEndpoint);
    request.bRequest        = kUSBRqClearFeature;
    request.wValue      = 0;    // Zero is ENDPOINT_HALT
    request.wIndex      = _interruptPipe->GetEndpointNumber() | 0x80 ; // bit 7 sets the direction of the endpoint to IN
    request.wLength     = 0;
    request.pData       = NULL;
    
    // Send the command over the control endpoint
    //
    status = _device->DeviceRequest(&request, 5000, 0);
    
    if ( status )
    {
        USBLog(3, "%s[%p]::ClearFeatureEndpointHalt -  DeviceRequest returned: 0x%x", getName(), this, status);
    }
    
    // Now that we've sent the ENDPOINT_HALT clear feature, we need to requeue the interrupt read.  Note
    // that we are doing this even if we get an error from the DeviceRequest.
    //
    IncrementOutstandingIO();
    status = _interruptPipe->Read(_buffer, &_completion);
    if ( status != kIOReturnSuccess)
    {
        // This is bad.  We probably shouldn't continue on from here.
        USBLog(3, "%s[%p]::ClearFeatureEndpointHalt -  immediate error %d queueing read", getName(), this, status);
        DecrementOutstandingIO();
        // _interface->close(this); this will be done in didTerminate
    }
}



IOReturn
DWXBoxHIDDriver::ChangeOutstandingIO(OSObject *target, void *param1, void *param2, void *param3, void *param4)
{
    DWXBoxHIDDriver *me = OSDynamicCast(DWXBoxHIDDriver, target);
    UInt64  direction = (UInt64)param1;	//  v2.0.0 changed UInt32 to UInt64
    
    if (!me)
    {
        USBLog(1, "DWXBoxHIDDriver::ChangeOutstandingIO - invalid target");
        return kIOReturnSuccess;
    }
    switch (direction)
    {
        case 1:
            me->_outstandingIO++;
            break;
            
        case -1:
            if (!--me->_outstandingIO && me->_needToClose)
            {
                USBLog(3, "%s[%p]::ChangeOutstandingIO isInactive = %d, outstandingIO = %u - closing device",
                       me->getName(), me, me->isInactive(), (unsigned int) me->_outstandingIO);	//  v2.0.0 added (unsigned int)
                me->_interface->close(me);
            }
            break;
            
        default:
            USBLog(1, "%s[%p]::ChangeOutstandingIO - invalid direction", me->getName(), me);
    }
    return kIOReturnSuccess;
}


void
DWXBoxHIDDriver::DecrementOutstandingIO(void)
{
    if (!_gate)
    {
        if (!--_outstandingIO && _needToClose)
        {
            USBLog(3, "%s[%p]::DecrementOutstandingIO isInactive = %d, outstandingIO = %u - closing device",
                   getName(), this, isInactive(), (unsigned int) _outstandingIO);	//  v2.0.0 added (unsigned int)
            _interface->close(this);
        }
        return;
    }
    _gate->runAction(ChangeOutstandingIO, (void*)-1);
}


void
DWXBoxHIDDriver::IncrementOutstandingIO(void)
{
    if (!_gate)
    {
        _outstandingIO++;
        return;
    }
    _gate->runAction(ChangeOutstandingIO, (void*)1);
}


//
// StartFinalProcessing
//
// This method may have a confusing name. This is not talking about Final Processing of the driver (as in
// the driver is going away or something like that. It is talking about FinalProcessing of the start method.
// It is called as the very last thing in the start method, and by default it issues a read on the interrupt
// pipe.
//
IOReturn
DWXBoxHIDDriver::StartFinalProcessing(void)
{
    IOReturn    err = kIOReturnSuccess;
    
    _completion.target = (void *)this;
    _completion.action = (IOUSBCompletionAction) &DWXBoxHIDDriver::InterruptReadHandlerEntry;
    _completion.parameter = (void *)0;
    
    IncrementOutstandingIO();
    err = _interruptPipe->Read(_buffer, &_completion);
    if (err != kIOReturnSuccess)
    {
        DecrementOutstandingIO();
        USBError(1, "%s[%p]::StartFinalProcessing - err (%x) in interrupt read, retain count %d after release",
                 getName(), this, err, getRetainCount());
    }
    return err;
}


IOReturn
DWXBoxHIDDriver::SetIdleMillisecs(UInt16 msecs)
{
    IOReturn            err = kIOReturnSuccess;
    IOUSBDevRequest     request;
    
    request.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
    request.bRequest = kHIDRqSetIdle;  //See USBSpec.h
    request.wValue = (msecs/4) << 8;
    request.wIndex = _interface->GetInterfaceNumber();
    request.wLength = 0;
    request.pData = NULL;
    
    err = _device->DeviceRequest(&request, 5000, 0);
    if (err != kIOReturnSuccess)
    {
        USBLog(3, "%s[%p]: DWXBoxHIDDriver::SetIdleMillisecs returned error 0x%x",getName(), this, err);
    }
    
    return err;
    
}


#if ENABLE_HIDREPORT_LOGGING
void
DWXBoxHIDDriver::LogMemReport(IOMemoryDescriptor * reportBuffer)
{
    IOByteCount reportSize;
    char outBuffer[1024];
    char in[1024];
    char *out;
    char inChar;
    
    out = (char *)&outBuffer;
    reportSize = reportBuffer->getLength();
    reportBuffer->readBytes(0, in, reportSize );
    if (reportSize > 256) reportSize = 256;
    
    for (unsigned int i = 0; i < reportSize; i++)
    {
        inChar = in[i];
        *out++ = ' ';
        *out++ = GetHexChar(inChar >> 4);
        *out++ = GetHexChar(inChar & 0x0F);
    }
    
    *out = 0;
    
    USBLog(6, outBuffer);
}

char
DWXBoxHIDDriver::GetHexChar(char hexChar)
{
    char hexChars[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    return hexChars[0x0F & hexChar];
}
#endif


OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  0);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  1);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  2);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  3);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  4);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  5);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  6);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  7);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  8);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver,  9);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 10);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 11);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 12);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 13);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 14);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 15);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 16);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 17);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 18);
OSMetaClassDefineReservedUnused(DWXBoxHIDDriver, 19);

