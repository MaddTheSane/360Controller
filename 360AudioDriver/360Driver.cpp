//
//  360Driver.cpp
//  360 Driver
//
//  Created by C.W. Betts on 4/5/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#include "360Driver.h"
#include <CoreAudio/AudioServerPlugIn.h>
#include <pthread.h>


static AudioServerPlugInDriverInterface	gAudioServerPlugInDriverInterface =
{
	NULL,
	Three60Driver::SQueryInterface,
	Three60Driver::SAddRef,
	Three60Driver::SRelease,
	Three60Driver::SInitialize,
	Three60Driver::SCreateDevice,
	Three60Driver::SDestroyDevice,
	Three60Driver::SAddDeviceClient,
	Three60Driver::SRemoveDeviceClient,
	Three60Driver::SPerformDeviceConfigurationChange,
	Three60Driver::SAbortDeviceConfigurationChange,
	Three60Driver::SHasProperty,
	Three60Driver::SIsPropertySettable,
	Three60Driver::SGetPropertyDataSize,
	Three60Driver::SGetPropertyData,
	Three60Driver::SSetPropertyData,
	Three60Driver::SStartIO,
	Three60Driver::SStopIO,
	Three60Driver::SGetZeroTimeStamp,
	Three60Driver::SWillDoIOOperation,
	Three60Driver::SBeginIOOperation,
	Three60Driver::SDoIOOperation,
	Three60Driver::SEndIOOperation
};


Three60Driver::Three60Driver() : RefCount(1)
{
    iIOCFPlugInInterface.pseudoVTable = (IUnknownVTbl *) &gAudioServerPlugInDriverInterface;
    iIOCFPlugInInterface.obj = this;
	
	gPlugIn_StateMutex = PTHREAD_MUTEX_INITIALIZER;
	
	factoryID = k360AudioDriverUUID;
	CFRetain(factoryID);
	CFPlugInAddInstanceForFactory(factoryID);

}

Three60Driver::~Three60Driver()
{
	CFPlugInRemoveInstanceForFactory(factoryID);
	CFRelease(factoryID);
}

HRESULT Three60Driver::QueryInterface(REFIID inUUID, LPVOID *outInterface)
{
	//	This function is called by the HAL to get the interface to talk to the plug-in through.
	//	AudioServerPlugIns are required to support the IUnknown interface and the
	//	AudioServerPlugInDriverInterface. As it happens, all interfaces must also provide the
	//	IUnknown interface, so we can always just return the single interface we made with
	//	gAudioServerPlugInDriverInterfacePtr regardless of which one is asked for.
	
	//	declare the local variables
	HRESULT theAnswer = 0;
	CFUUIDRef theRequestedUUID = NULL;
	
	//	make a CFUUIDRef from inUUID
	theRequestedUUID = CFUUIDCreateFromUUIDBytes(NULL, inUUID);
	//FailWithAction(theRequestedUUID == NULL, theAnswer = kAudioHardwareIllegalOperationError, Done, "NullAudio_QueryInterface: failed to create the CFUUIDRef");
	
	if(CFEqual(theRequestedUUID, IUnknownUUID) || CFEqual(theRequestedUUID, kAudioServerPlugInDriverInterfaceUUID)) {
		pthread_mutex_lock(&gPlugIn_StateMutex);
		AddRef();
		pthread_mutex_unlock(&gPlugIn_StateMutex);
		*outInterface = &this->iIOCFPlugInInterface;
	} else {
		theAnswer = E_NOINTERFACE;
	}
	
	//	make sure to release the UUID we created
	CFRelease(theRequestedUUID);
	
Done:
	return theAnswer;
}

ULONG Three60Driver::AddRef()
{
	ULONG theAnswer = 0;
	pthread_mutex_lock(&gPlugIn_StateMutex);
	if(RefCount < UINT32_MAX) {
		++RefCount;
	}
	theAnswer = RefCount;
	pthread_mutex_unlock(&gPlugIn_StateMutex);

	return theAnswer;
}

ULONG Three60Driver::Release()
{
	ULONG theAnswer = 0;
	
	pthread_mutex_lock(&gPlugIn_StateMutex);
	if(RefCount > 0) {
		--RefCount;
		//	Note that we don't do anything special if the refcount goes to zero as the HAL
		//	will never fully release a plug-in it opens. We keep managing the refcount so that
		//	the API semantics are correct though.
	}
	theAnswer = RefCount;
	pthread_mutex_unlock(&gPlugIn_StateMutex);

	return theAnswer;
}

#pragma mark - static implementation

HRESULT Three60Driver::SQueryInterface(void *thisPointer, REFIID iid, LPVOID *ppv)
{
	return getThis(thisPointer)->QueryInterface(iid, ppv);
}

ULONG Three60Driver::SAddRef(void *thisPointer)
{
	return getThis(thisPointer)->AddRef();
}

ULONG Three60Driver::SRelease(void *thisPointer)
{
	return getThis(thisPointer)->Release();
}

IOCFPlugInInterface** Three60Driver::Alloc()
{
    Three60Driver *me = new Three60Driver;
    if (!me) {
        return NULL;
    }
    return (IOCFPlugInInterface **)(&me->iIOCFPlugInInterface.pseudoVTable);
}

OSStatus Three60Driver::SInitialize(AudioServerPlugInDriverRef inDriver, AudioServerPlugInHostRef inHost)
{
	return getThis(inDriver)->Initialize(inHost);
}

OSStatus Three60Driver::SCreateDevice(AudioServerPlugInDriverRef inDriver, CFDictionaryRef inDescription, const AudioServerPlugInClientInfo* inClientInfo, AudioObjectID* outDeviceObjectID)
{
	return getThis(inDriver)->CreateDevice(inDescription, inClientInfo, outDeviceObjectID);
}

OSStatus Three60Driver::SDestroyDevice(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID)
{
	return getThis(inDriver)->DestroyDevice(inDeviceObjectID);
}

OSStatus Three60Driver::SAddDeviceClient(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo)
{
	return getThis(inDriver)->AddDeviceClient(inDeviceObjectID, inClientInfo);
}

OSStatus Three60Driver::SRemoveDeviceClient(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo)
{
	return getThis(inDriver)->RemoveDeviceClient(inDeviceObjectID, inClientInfo);
}

OSStatus Three60Driver::SPerformDeviceConfigurationChange(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo)
{
	return getThis(inDriver)->PerformDeviceConfigurationChange(inDeviceObjectID, inChangeAction, inChangeInfo);
}

OSStatus Three60Driver::SAbortDeviceConfigurationChange(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo)
{
	return getThis(inDriver)->AbortDeviceConfigurationChange(inDeviceObjectID, inChangeAction, inChangeInfo);
}

Boolean Three60Driver::SHasProperty(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress)
{
	return getThis(inDriver)->HasProperty(inObjectID, inClientProcessID, inAddress);
}

OSStatus Three60Driver::SIsPropertySettable(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable)
{
	return getThis(inDriver)->IsPropertySettable(inObjectID, inClientProcessID, inAddress, outIsSettable);
}

OSStatus Three60Driver::SGetPropertyDataSize(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize)
{
	return getThis(inDriver)->GetPropertyDataSize(inObjectID, inClientProcessID, inAddress, inQualifierDataSize, inQualifierData, outDataSize);
}

OSStatus Three60Driver::SGetPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData)
{
	return getThis(inDriver)->GetPropertyData(inObjectID, inClientProcessID, inAddress, inQualifierDataSize, inQualifierData, inDataSize, outDataSize, outData);
}

OSStatus Three60Driver::SSetPropertyData(AudioServerPlugInDriverRef inDriver, AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData)
{
	return getThis(inDriver)->SetPropertyData(inObjectID, inClientProcessID, inAddress, inQualifierDataSize, inQualifierData, inDataSize, inData);
}

OSStatus Three60Driver::SStartIO(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID)
{
	return getThis(inDriver)->StartIO(inDeviceObjectID, inClientID);
}

OSStatus Three60Driver::SStopIO(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID)
{
	return getThis(inDriver)->StopIO(inDeviceObjectID, inClientID);
}

OSStatus Three60Driver::SGetZeroTimeStamp(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, Float64* outSampleTime, UInt64* outHostTime, UInt64* outSeed)
{
	return getThis(inDriver)->GetZeroTimeStamp(inDeviceObjectID, inClientID, outSampleTime, outHostTime, outSeed);
}

OSStatus Three60Driver::SWillDoIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, Boolean* outWillDo, Boolean* outWillDoInPlace)
{
	return getThis(inDriver)->WillDoIOOperation(inDeviceObjectID, inClientID, inOperationID, outWillDo, outWillDoInPlace);
}

OSStatus Three60Driver::SBeginIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo)
{
	return getThis(inDriver)->BeginIOOperation(inDeviceObjectID, inClientID, inOperationID, inIOBufferFrameSize, inIOCycleInfo);
}

OSStatus Three60Driver::SDoIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, AudioObjectID inStreamObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo, void* ioMainBuffer, void* ioSecondaryBuffer)
{
	return getThis(inDriver)->DoIOOperation(inDeviceObjectID, inStreamObjectID, inClientID, inOperationID, inIOBufferFrameSize, inIOCycleInfo, ioMainBuffer, ioSecondaryBuffer);
}

OSStatus Three60Driver::SEndIOOperation(AudioServerPlugInDriverRef inDriver, AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo)
{
	return getThis(inDriver)->EndIOOperation(inDeviceObjectID, inClientID, inOperationID, inIOBufferFrameSize, inIOCycleInfo);
}

#pragma mark - class implementation
OSStatus Three60Driver::Initialize(AudioServerPlugInHostRef inHost)
{
	return noErr;
}

OSStatus Three60Driver::CreateDevice(CFDictionaryRef inDescription, const AudioServerPlugInClientInfo* inClientInfo, AudioObjectID* outDeviceObjectID)
{
	return noErr;
}

OSStatus Three60Driver::DestroyDevice(AudioObjectID inDeviceObjectID)
{
	return noErr;
}

OSStatus Three60Driver::AddDeviceClient(AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo)
{
	return noErr;
}

OSStatus Three60Driver::RemoveDeviceClient(AudioObjectID inDeviceObjectID, const AudioServerPlugInClientInfo* inClientInfo)
{
	return noErr;
}

OSStatus Three60Driver::PerformDeviceConfigurationChange(AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo)
{
	return noErr;
}

OSStatus Three60Driver::AbortDeviceConfigurationChange(AudioObjectID inDeviceObjectID, UInt64 inChangeAction, void* inChangeInfo)
{
	return noErr;
}

Boolean Three60Driver::HasProperty(AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress)
{
	return noErr;
}

OSStatus Three60Driver::IsPropertySettable(AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, Boolean* outIsSettable)
{
	if (!outIsSettable) {
		return -1;
	}
	*outIsSettable = false;
	return noErr;
}

OSStatus Three60Driver::GetPropertyDataSize(AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32* outDataSize)
{
	return noErr;
}

OSStatus Three60Driver::GetPropertyData(AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, UInt32* outDataSize, void* outData)
{
	return noErr;
}

OSStatus Three60Driver::SetPropertyData(AudioObjectID inObjectID, pid_t inClientProcessID, const AudioObjectPropertyAddress* inAddress, UInt32 inQualifierDataSize, const void* inQualifierData, UInt32 inDataSize, const void* inData)
{
	return noErr;
}

OSStatus Three60Driver::StartIO(AudioObjectID inDeviceObjectID, UInt32 inClientID)
{
	return noErr;
}

OSStatus Three60Driver::StopIO(AudioObjectID inDeviceObjectID, UInt32 inClientID)
{
	return noErr;
}

OSStatus Three60Driver::GetZeroTimeStamp(AudioObjectID inDeviceObjectID, UInt32 inClientID, Float64* outSampleTime, UInt64* outHostTime, UInt64* outSeed)
{
	return noErr;
}

OSStatus Three60Driver::WillDoIOOperation(AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, Boolean* outWillDo, Boolean* outWillDoInPlace)
{
	return noErr;
}

OSStatus Three60Driver::BeginIOOperation(AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo)
{
	return noErr;
}

OSStatus Three60Driver::DoIOOperation(AudioObjectID inDeviceObjectID, AudioObjectID inStreamObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo, void* ioMainBuffer, void* ioSecondaryBuffer)
{
	return noErr;
}

OSStatus Three60Driver::EndIOOperation(AudioObjectID inDeviceObjectID, UInt32 inClientID, UInt32 inOperationID, UInt32 inIOBufferFrameSize, const AudioServerPlugInIOCycleInfo* inIOCycleInfo)
{
	return noErr;
}


extern void* Audio360Factory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    void* theAnswer = NULL;
    if(CFEqual(typeID, kAudioServerPlugInTypeUUID)) {
		theAnswer = Three60Driver::Alloc();
    }
    return theAnswer;
}
