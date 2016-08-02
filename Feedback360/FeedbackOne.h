//
//  FeedbackOne.h
//  360 Driver
//
//  Created by C.W. Betts on 1/8/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#ifndef ___60_Driver__FeedbackOne__
#define ___60_Driver__FeedbackOne__

#include "FeedbackBase.h"
#include "Feedback360Effect.h"
#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <vector>
#include "devlink.h"

class FeedbackOne : public FeedbackBase {
    CFUUIDRef FactoryID;
    

    virtual IOReturn Probe ( CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
    virtual IOReturn Start ( CFDictionaryRef propertyTable, io_service_t service );
    virtual IOReturn Stop ( void );
    
    virtual HRESULT GetVersion(ForceFeedbackVersion * version);
    virtual HRESULT InitializeTerminate(NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin);
    virtual HRESULT DestroyEffect(FFEffectDownloadID downloadID);
    virtual HRESULT DownloadEffect(CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags);
    virtual HRESULT Escape(FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape);
    virtual HRESULT GetEffectStatus(FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode);
    virtual HRESULT GetForceFeedbackState(ForceFeedbackDeviceState * pDeviceState);
    virtual HRESULT GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities);
    virtual HRESULT SendForceFeedbackCommand(FFCommandFlag state);
    virtual HRESULT SetProperty(FFProperty property, void * pValue);
    virtual HRESULT StartEffect(FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations);
    virtual HRESULT StopEffect(UInt32 downloadID);

public:
    static IOCFPlugInInterface** Alloc(void);
    
    FeedbackOne();
    ~FeedbackOne();
    
private:
    //disable copy constructor
    FeedbackOne(FeedbackOne &src);
    void operator = (FeedbackOne &src);
    
    typedef std::vector<Feedback360Effect> Feedback360EffectVector;
    typedef Feedback360EffectVector::iterator Feedback360EffectIterator;
    
    // interfacing
    DeviceLink          device;
    
    // GCD queue and timer
    dispatch_queue_t    Queue;
    dispatch_source_t   Timer;
    
    // effects handling
    Feedback360EffectVector EffectList;
    UInt32              EffectIndex;
    
    DWORD   Gain;
    bool    Actuator;
    
    LONG            PrvLeftLevel, PrvRightLevel, PrvTriggerLevel;
    bool            Stopped;
    bool            Paused;
    bool            Manual;
    double          LastTime;
    double          PausedTime;
    
    // event loop func
    static void EffectProc( void *params );
    
    void            SetForce(LONG LeftLevel, LONG RightLevel, LONG triggerLevel);

};


// 3A816735-D3CE-4788-BE87-811B33BFB4CA
#define kFeedbackOneUUID CFUUIDGetConstantUUIDWithBytes(kCFAllocatorSystemDefault, 0x3A, 0x81, 0x67, 0x35, 0xD3, 0xCE, 0x47, 0x88, 0xBE, 0x87, 0x81, 0x1B, 0x33, 0xBF, 0xB4, 0xCA)

extern "C" {
    void* ControlXboxOneFactory(CFAllocatorRef allocator, CFUUIDRef uuid);
}

#endif /* defined(___60_Driver__FeedbackOne__) */
