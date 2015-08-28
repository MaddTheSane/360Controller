//
//  360Driver.h
//  360 Driver
//
//  Created by C.W. Betts on 4/5/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#ifndef ___60_Driver___60Driver__
#define ___60_Driver___60Driver__

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <IOKit/IOCFPlugIn.h>

// CD4E2F92-C3C7-414D-A622-6431A41B0447
#define k360AudioDriverUUID CFUUIDGetConstantUUIDWithBytes(kCFAllocatorSystemDefault, 0xCD, 0x4E, 0x2F, 0x92, 0xC3, 0xC7, 0x41, 0x4D, 0xA6, 0x22, 0x64, 0x31, 0xA4, 0x1B, 0x04, 0x47)


class Three60Driver : IUnknown {
	void *_reserved;
	ULONG RefCount;
	
	typedef struct _Audio360InterfaceMap
    {
        IUnknownVTbl *pseudoVTable;
        Three60Driver *obj;
    } Audio360InterfaceMap;

	Audio360InterfaceMap iIOCFPlugInInterface;
	static inline Three60Driver *getThis (void *oSelf) { return (Three60Driver *) ((Audio360InterfaceMap *) oSelf)->obj; }

public:
	
	Three60Driver();
	virtual ~Three60Driver();
	
	static IOCFPlugInInterface** Alloc();
	
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
	
    static HRESULT SQueryInterface(void *thisPointer, REFIID iid, LPVOID *ppv);
    static ULONG SAddRef(void *thisPointer);
    static ULONG SRelease(void *thisPointer);
    
    
#pragma mark Basic Operations

	OSStatus Initialize(AudioServerPlugInHostRef inHost);
    OSStatus CreateDevice(CFDictionaryRef                     inDescription,
						  const AudioServerPlugInClientInfo*  inClientInfo,
						  AudioObjectID*                      outDeviceObjectID);
    OSStatus DestroyDevice(AudioObjectID               inDeviceObjectID);
    OSStatus AddDeviceClient(AudioObjectID                       inDeviceObjectID,
							 const AudioServerPlugInClientInfo*  inClientInfo);
    OSStatus RemoveDeviceClient(AudioObjectID                       inDeviceObjectID,
								const AudioServerPlugInClientInfo*  inClientInfo);
    
    OSStatus PerformDeviceConfigurationChange(AudioObjectID               inDeviceObjectID,
											  UInt64                      inChangeAction,
											  void*                       inChangeInfo);
	
    OSStatus AbortDeviceConfigurationChange(AudioObjectID               inDeviceObjectID,
											UInt64                      inChangeAction,
											void*                       inChangeInfo);
	
#pragma mark Property Operations
	
    Boolean HasProperty(AudioObjectID                       inObjectID,
						pid_t                               inClientProcessID,
						const AudioObjectPropertyAddress*   inAddress);
	
    OSStatus IsPropertySettable(AudioObjectID                       inObjectID,
								pid_t                               inClientProcessID,
								const AudioObjectPropertyAddress*   inAddress,
								Boolean*                            outIsSettable);
	
	OSStatus GetPropertyDataSize(AudioObjectID                       inObjectID,
								 pid_t                               inClientProcessID,
								 const AudioObjectPropertyAddress*   inAddress,
								 UInt32                              inQualifierDataSize,
								 const void*                         inQualifierData,
								 UInt32*                             outDataSize);
	
    OSStatus GetPropertyData(AudioObjectID                       inObjectID,
							 pid_t                               inClientProcessID,
							 const AudioObjectPropertyAddress*   inAddress,
							 UInt32                              inQualifierDataSize,
							 const void*                         inQualifierData,
							 UInt32                              inDataSize,
							 UInt32*                             outDataSize,
							 void*                               outData);
	
    OSStatus SetPropertyData(AudioObjectID                       inObjectID,
							 pid_t                               inClientProcessID,
							 const AudioObjectPropertyAddress*   inAddress,
							 UInt32                              inQualifierDataSize,
							 const void*                         inQualifierData,
							 UInt32                              inDataSize,
							 const void*                         inData);
	
#pragma mark IO Operations
	
    OSStatus StartIO(AudioObjectID               inDeviceObjectID,
					 UInt32                      inClientID);
    
    OSStatus StopIO(AudioObjectID               inDeviceObjectID,
					UInt32                      inClientID);
    
    OSStatus GetZeroTimeStamp(AudioObjectID               inDeviceObjectID,
							  UInt32                      inClientID,
							  Float64*                    outSampleTime,
							  UInt64*                     outHostTime,
							  UInt64*                     outSeed);
	
    OSStatus WillDoIOOperation(AudioObjectID               inDeviceObjectID,
							   UInt32                      inClientID,
							   UInt32                      inOperationID,
							   Boolean*                    outWillDo,
							   Boolean*                    outWillDoInPlace);
	
    OSStatus BeginIOOperation(AudioObjectID                       inDeviceObjectID,
							  UInt32                              inClientID,
							  UInt32                              inOperationID,
							  UInt32                              inIOBufferFrameSize,
							  const AudioServerPlugInIOCycleInfo* inIOCycleInfo);
	
    OSStatus DoIOOperation(AudioObjectID                       inDeviceObjectID,
						   AudioObjectID                       inStreamObjectID,
						   UInt32                              inClientID,
						   UInt32                              inOperationID,
						   UInt32                              inIOBufferFrameSize,
						   const AudioServerPlugInIOCycleInfo* inIOCycleInfo,
						   void*                               ioMainBuffer,
						   void*                               ioSecondaryBuffer);
	
    OSStatus EndIOOperation(AudioObjectID                       inDeviceObjectID,
							UInt32                              inClientID,
							UInt32                              inOperationID,
							UInt32                              inIOBufferFrameSize,
							const AudioServerPlugInIOCycleInfo* inIOCycleInfo);

#pragma mark - static implementations

	static OSStatus SInitialize(AudioServerPlugInDriverRef          inDriver,
								AudioServerPlugInHostRef inHost);
	static OSStatus SCreateDevice(AudioServerPlugInDriverRef          inDriver,
								  CFDictionaryRef                     inDescription,
								  const AudioServerPlugInClientInfo*  inClientInfo,
								  AudioObjectID*                      outDeviceObjectID);
	static OSStatus SDestroyDevice(AudioServerPlugInDriverRef          inDriver,
								   AudioObjectID               inDeviceObjectID);
	static OSStatus SAddDeviceClient(AudioServerPlugInDriverRef          inDriver,
									 AudioObjectID                       inDeviceObjectID,
									 const AudioServerPlugInClientInfo*  inClientInfo);
	static OSStatus SRemoveDeviceClient(AudioServerPlugInDriverRef          inDriver,
										AudioObjectID                       inDeviceObjectID,
										const AudioServerPlugInClientInfo*  inClientInfo);
	
	static OSStatus SPerformDeviceConfigurationChange(AudioServerPlugInDriverRef          inDriver,
													  AudioObjectID               inDeviceObjectID,
													  UInt64                      inChangeAction,
													  void*                       inChangeInfo);
	
	static OSStatus SAbortDeviceConfigurationChange(AudioServerPlugInDriverRef          inDriver,
													AudioObjectID               inDeviceObjectID,
													UInt64                      inChangeAction,
													void*                       inChangeInfo);
	
	static Boolean SHasProperty(AudioServerPlugInDriverRef          inDriver,
								AudioObjectID                       inObjectID,
								pid_t                               inClientProcessID,
								const AudioObjectPropertyAddress*   inAddress);
	
	static OSStatus SIsPropertySettable(AudioServerPlugInDriverRef          inDriver,
										AudioObjectID                       inObjectID,
										pid_t                               inClientProcessID,
										const AudioObjectPropertyAddress*   inAddress,
										Boolean*                            outIsSettable);
	
	static OSStatus SGetPropertyDataSize(AudioServerPlugInDriverRef          inDriver,
										 AudioObjectID                       inObjectID,
										 pid_t                               inClientProcessID,
										 const AudioObjectPropertyAddress*   inAddress,
										 UInt32                              inQualifierDataSize,
										 const void*                         inQualifierData,
										 UInt32*                             outDataSize);
	
	static OSStatus SGetPropertyData(AudioServerPlugInDriverRef          inDriver,
									 AudioObjectID                       inObjectID,
									 pid_t                               inClientProcessID,
									 const AudioObjectPropertyAddress*   inAddress,
									 UInt32                              inQualifierDataSize,
									 const void*                         inQualifierData,
									 UInt32                              inDataSize,
									 UInt32*                             outDataSize,
									 void*                               outData);
	
	static OSStatus SSetPropertyData(AudioServerPlugInDriverRef          inDriver,
									 AudioObjectID                       inObjectID,
									 pid_t                               inClientProcessID,
									 const AudioObjectPropertyAddress*   inAddress,
									 UInt32                              inQualifierDataSize,
									 const void*                         inQualifierData,
									 UInt32                              inDataSize,
									 const void*                         inData);
	
	static OSStatus SStartIO(AudioServerPlugInDriverRef          inDriver,
							 AudioObjectID               inDeviceObjectID,
							 UInt32                      inClientID);
	
	static OSStatus SStopIO(AudioServerPlugInDriverRef          inDriver,
							AudioObjectID               inDeviceObjectID,
							UInt32                      inClientID);
	
	static OSStatus SGetZeroTimeStamp(AudioServerPlugInDriverRef          inDriver,
									  AudioObjectID               inDeviceObjectID,
									  UInt32                      inClientID,
									  Float64*                    outSampleTime,
									  UInt64*                     outHostTime,
									  UInt64*                     outSeed);
	
	static OSStatus SWillDoIOOperation(AudioServerPlugInDriverRef          inDriver,
									   AudioObjectID               inDeviceObjectID,
									   UInt32                      inClientID,
									   UInt32                      inOperationID,
									   Boolean*                    outWillDo,
									   Boolean*                    outWillDoInPlace);
	
	static OSStatus SBeginIOOperation(AudioServerPlugInDriverRef          inDriver,
							  AudioObjectID                       inDeviceObjectID,
							  UInt32                              inClientID,
							  UInt32                              inOperationID,
							  UInt32                              inIOBufferFrameSize,
							  const AudioServerPlugInIOCycleInfo* inIOCycleInfo);
	
	static OSStatus SDoIOOperation(AudioServerPlugInDriverRef          inDriver,
								   AudioObjectID                       inDeviceObjectID,
						   AudioObjectID                       inStreamObjectID,
						   UInt32                              inClientID,
						   UInt32                              inOperationID,
						   UInt32                              inIOBufferFrameSize,
						   const AudioServerPlugInIOCycleInfo* inIOCycleInfo,
						   void*                               ioMainBuffer,
						   void*                               ioSecondaryBuffer);
	
	static OSStatus SEndIOOperation(AudioServerPlugInDriverRef          inDriver,
									AudioObjectID                       inDeviceObjectID,
									UInt32                              inClientID,
									UInt32                              inOperationID,
									UInt32                              inIOBufferFrameSize,
									const AudioServerPlugInIOCycleInfo* inIOCycleInfo);

	
#pragma mark -
private:
	pthread_mutex_t			gPlugIn_StateMutex;
	CFUUIDRef				factoryID;
};

extern "C" {
	extern void* Audio360Factory(CFAllocatorRef allocator, CFUUIDRef typeID);
}

#endif /* defined(___60_Driver___60Driver__) */
