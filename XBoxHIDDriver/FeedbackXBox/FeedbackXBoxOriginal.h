//
//  FeedbackXBoxOriginal.h
//  DWXBoxHIDDriver
//
//  Created by C.W. Betts on 11/20/14.
//
//

#ifndef __DWXBoxHIDDriver__FeedbackXBoxOriginal__
#define __DWXBoxHIDDriver__FeedbackXBoxOriginal__

#include <ForceFeedback/IOForceFeedbackLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <vector>

#include "devlink.h"
#include "Feedback360Effect.h"

#define FeedbackDriverVersionMajor      1
#define FeedbackDriverVersionMinor      0
#define FeedbackDriverVersionStage      developStage
#define FeedbackDriverVersionNonRelRev  0

class FeedbackXBox : IUnknown
{
public:
	// constructor/destructor
	FeedbackXBox(void);
	virtual ~FeedbackXBox(void);
	
private:
	//disable copy constructor
	FeedbackXBox(FeedbackXBox &src);
	void operator = (FeedbackXBox &src);
	
	UInt32 fRefCount;
	
	typedef struct _Xbox360InterfaceMap
	{
		IUnknownVTbl *pseudoVTable;
		FeedbackXBox *obj;
	} Xbox360InterfaceMap;
	
	// IOCFPlugin interfacing variables and functions
public:
	static IOCFPlugInInterface** Alloc(void);
	
	// static functions called by the ForceFeedback API
	static HRESULT  sQueryInterface(void *self, REFIID iid, LPVOID *ppv);
	static ULONG sAddRef(void *self);
	static ULONG sRelease(void *self);
	
	static IOReturn sProbe ( void * self, CFDictionaryRef propertyTable, io_service_t service, SInt32 * order );
	static IOReturn sStart ( void * self, CFDictionaryRef propertyTable, io_service_t service );
	static IOReturn sStop ( void * self );
	
	static HRESULT  sGetVersion(void * interface, ForceFeedbackVersion * version);
	static HRESULT  sInitializeTerminate(void * interface, NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin );
	static HRESULT  sDestroyEffect(void * interface, FFEffectDownloadID downloadID );
	static HRESULT  sDownloadEffect(  void * interface, CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags );
	static HRESULT  sEscape( void * interface, FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape );
	static HRESULT  sGetEffectStatus( void * interface, FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode );
	static HRESULT  sGetForceFeedbackState( void * interface, ForceFeedbackDeviceState * pDeviceState );
	static HRESULT  sGetForceFeedbackCapabilities( void * interface, FFCAPABILITIES *capabilities );
	static HRESULT  sSendForceFeedbackCommand( void * interface, FFCommandFlag state );
	static HRESULT  sSetProperty( void * interface, FFProperty property, void * pValue );
	static HRESULT  sStartEffect( void * interface, FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations );
	static HRESULT  sStopEffect( void * interface, FFEffectDownloadID downloadID );
	
	virtual HRESULT QueryInterface(REFIID iid, LPVOID* ppv);
	virtual ULONG   AddRef(void);
	virtual ULONG   Release(void);
	
private:
	typedef std::vector<Feedback360Effect> FeedbackXBoxEffectVector;
	typedef FeedbackXBoxEffectVector::iterator FeedbackXBoxEffectIterator;
	// helper function
	static inline FeedbackXBox *getThis (void *self) { return (FeedbackXBox *) ((Xbox360InterfaceMap *) self)->obj; }
	
	// interfacing
	Xbox360InterfaceMap iIOCFPlugInInterface;
	Xbox360InterfaceMap iIOForceFeedbackDeviceInterface;
	DeviceLink          device;
	
	// GCD queue and timer
	dispatch_queue_t    Queue;
	dispatch_source_t   Timer;
	
	// effects handling
	FeedbackXBoxEffectVector EffectList;
	UInt32              EffectIndex;
	
	DWORD   Gain;
	bool    Actuator;
	
	LONG            PrvLeftLevel, PrvRightLevel;
	bool            Stopped;
	bool            Paused;
	bool            Manual;
	CFAbsoluteTime  LastTime;
	CFAbsoluteTime  PausedTime;
	CFUUIDRef       FactoryID;
	
	void            SetForce(LONG LeftLevel, LONG RightLevel);
	
	// event loop func
	static void EffectProc( void *params );
	
	// actual member functions ultimately called by the FF API (through the static functions)
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
	virtual HRESULT StopEffect(FFEffectDownloadID downloadID);
};

// 55D89979-FD2A-4753-BD1C-BB3FE8750C0C
#define kFeedback360Uuid CFUUIDGetConstantUUIDWithBytes(kCFAllocatorSystemDefault, 0x55, 0xD8, 0x99, 0x79, 0xFD, 0x2A, 0x47, 0x53, 0xBD, 0x1C, 0xBB, 0x3F, 0xE8, 0x75, 0x0C, 0x0C)

// Factory function
extern "C" {
	void* ControlXBoxFactory(CFAllocatorRef allocator, CFUUIDRef uuid) __attribute__((visibility("default")));
}

#endif /* defined(__DWXBoxHIDDriver__FeedbackXBoxOriginal__) */
