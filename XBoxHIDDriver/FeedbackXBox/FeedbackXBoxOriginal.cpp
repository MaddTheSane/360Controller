//
//  FeedbackXBoxOriginal.cpp
//  DWXBoxHIDDriver
//
//  Created by C.W. Betts on 11/20/14.
//
//

#include "FeedbackXBoxOriginal.h"

#define LoopGranularity 10000 // Microseconds

static IOCFPlugInInterface functionMapXBox_IOCFPlugInInterface = {
	// Padding required for COM
	NULL,
	// IUnknown
	&FeedbackXBox::sQueryInterface,
	&FeedbackXBox::sAddRef,
	&FeedbackXBox::sRelease,
	// IOCFPlugInInterface
	1,0,    // Version
	&FeedbackXBox::sProbe,
	&FeedbackXBox::sStart,
	&FeedbackXBox::sStop
};

static IOForceFeedbackDeviceInterface functionMapXBox_IOForceFeedbackDeviceInterface = {
	// Padding required for COM
	NULL,
	// IUnknown
	&FeedbackXBox::sQueryInterface,
	&FeedbackXBox::sAddRef,
	&FeedbackXBox::sRelease,
	// IOForceFeedbackDevice
	&FeedbackXBox::sGetVersion,
	&FeedbackXBox::sInitializeTerminate,
	&FeedbackXBox::sDestroyEffect,
	&FeedbackXBox::sDownloadEffect,
	&FeedbackXBox::sEscape,
	&FeedbackXBox::sGetEffectStatus,
	&FeedbackXBox::sGetForceFeedbackCapabilities,
	&FeedbackXBox::sGetForceFeedbackState,
	&FeedbackXBox::sSendForceFeedbackCommand,
	&FeedbackXBox::sSetProperty,
	&FeedbackXBox::sStartEffect,
	&FeedbackXBox::sStopEffect
};

FeedbackXBox::FeedbackXBox() : fRefCount(1),  EffectIndex(1), Stopped(true),
Paused(false), PausedTime(0), LastTime(0), Gain(10000), PrvLeftLevel(0),
PrvRightLevel(0), Actuator(true), Manual(false)
{
	EffectList = FeedbackXBoxEffectVector();
	
	iIOCFPlugInInterface.pseudoVTable = (IUnknownVTbl *) &functionMapXBox_IOCFPlugInInterface;
	iIOCFPlugInInterface.obj = this;
	
	iIOForceFeedbackDeviceInterface.pseudoVTable = (IUnknownVTbl *) &functionMapXBox_IOForceFeedbackDeviceInterface;
	iIOForceFeedbackDeviceInterface.obj = this;
	
	FactoryID = kFeedback360Uuid;
	CFRetain(FactoryID);
	CFPlugInAddInstanceForFactory(FactoryID);
}

FeedbackXBox::~FeedbackXBox()
{
	CFPlugInRemoveInstanceForFactory(FactoryID);
	CFRelease(FactoryID);
}

HRESULT FeedbackXBox::QueryInterface(REFIID iid, LPVOID *ppv)
{
	CFUUIDRef interface = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
	
	if(CFEqual(interface, kIOForceFeedbackDeviceInterfaceID))
		*ppv = &this->iIOForceFeedbackDeviceInterface;
	// IUnknown || IOCFPlugInInterface
	else if(CFEqual(interface, IUnknownUUID) || CFEqual(interface, kIOCFPlugInInterfaceID))
		*ppv = &this->iIOCFPlugInInterface;
	else
		*ppv = NULL;
	
	// Done
	CFRelease(interface);
	if ((*ppv) == NULL) return E_NOINTERFACE;
	else {
		this->iIOCFPlugInInterface.pseudoVTable->AddRef(*ppv);
	}
	return FF_OK;
}

ULONG FeedbackXBox::AddRef()
{
	return ++fRefCount;
}

ULONG FeedbackXBox::Release()
{
	ULONG returnValue = fRefCount - 1;
	if(returnValue > 0) {
		fRefCount = returnValue;
	} else if(returnValue == 0) {
		fRefCount = returnValue;
		delete this;
	} else {
		returnValue = 0;
	}
	return returnValue;
}

IOCFPlugInInterface** FeedbackXBox::Alloc(void)
{
	FeedbackXBox *me = new FeedbackXBox();
	if (!me) {
		return NULL;
	}
	return (IOCFPlugInInterface **)(&me->iIOCFPlugInInterface.pseudoVTable);
}

IOReturn FeedbackXBox::Probe(CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
	if ((service==0)
		|| ((!IOObjectConformsTo(service,"Xbox360ControllerClass"))
			&& (!IOObjectConformsTo(service,"Wireless360Controller")))) return kIOReturnBadArgument;
	return FF_OK;
}

IOReturn FeedbackXBox::Start(CFDictionaryRef propertyTable,io_service_t service)
{
	return FF_OK;
}

IOReturn FeedbackXBox::Stop(void)
{
	return FF_OK;
}

HRESULT FeedbackXBox::SetProperty(FFProperty property, void *value)
{
	if(property != FFPROP_FFGAIN) {
		return FFERR_UNSUPPORTED;
	}
	
	UInt32 NewGain = *((UInt32*)value);
	__block HRESULT Result = FF_OK;
	
	dispatch_sync(Queue, ^{
		if (1 <= NewGain && NewGain <= 10000)
		{
			Gain = NewGain;
		} else {
			Gain = MAX(1, MIN(NewGain, 10000));
			Result = FF_TRUNCATED;
		}
	});
	
	return Result;
}

HRESULT FeedbackXBox::StartEffect(FFEffectDownloadID EffectHandle, FFEffectStartFlag Mode, UInt32 Count)
{
	dispatch_sync(Queue, ^{
		for (FeedbackXBoxEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				effectIterator->Status  = FFEGES_PLAYING;
				effectIterator->PlayCount = Count;
				effectIterator->StartTime = CFAbsoluteTimeGetCurrent();
				Stopped = false;
			} else {
				if (Mode & FFES_SOLO) {
					effectIterator->Status = NULL;
				}
			}
		}
	});
	return FF_OK;
}

HRESULT FeedbackXBox::StopEffect(UInt32 EffectHandle)
{
	dispatch_sync(Queue, ^{
		for (FeedbackXBoxEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				effectIterator->Status = NULL;
				break;
			}
		}
	});
	return FF_OK;
}

HRESULT FeedbackXBox::DownloadEffect(CFUUIDRef EffectType, FFEffectDownloadID *EffectHandle, FFEFFECT *DiEffect, FFEffectParameterFlag Flags)
{
	__block HRESULT Result = FF_OK;
	
	if (Flags & FFEP_NODOWNLOAD)
	{
		return FF_OK;
	}
	
	dispatch_sync(Queue, ^{
		Feedback360Effect *Effect = NULL;
		if (*EffectHandle == 0) {
			EffectList.push_back(Feedback360Effect(EffectIndex++));
			Effect = &(EffectList.back());
			*EffectHandle = Effect->Handle;
		} else {
			for (LONG Index = 0; Index < EffectList.size(); Index++) {
				if (EffectList[Index].Handle == *EffectHandle) {
					Effect = &(EffectList[Index]);
					break;
				}
			}
		}
		
		if (Effect == NULL || Result == -1) {
			Result = FFERR_INTERNAL;
		}
		else {
			Effect->Type = EffectType;
			Effect->DiEffect.dwFlags = DiEffect->dwFlags;
			
			if( Flags & FFEP_DURATION )
			{
				Effect->DiEffect.dwDuration = DiEffect->dwDuration;
			}
			
			if( Flags & FFEP_SAMPLEPERIOD )
			{
				Effect->DiEffect.dwSamplePeriod = DiEffect->dwSamplePeriod;
			}
			
			if( Flags & FFEP_GAIN )
			{
				Effect->DiEffect.dwGain = DiEffect->dwGain;
			}
			
			if( Flags & FFEP_TRIGGERBUTTON )
			{
				Effect->DiEffect.dwTriggerButton = DiEffect->dwTriggerButton;
			}
			
			if( Flags & FFEP_TRIGGERREPEATINTERVAL )
			{
				Effect->DiEffect.dwTriggerRepeatInterval = DiEffect->dwTriggerRepeatInterval;
			}
			
			if( Flags & FFEP_AXES )
			{
				Effect->DiEffect.cAxes  = DiEffect->cAxes;
				Effect->DiEffect.rgdwAxes = NULL;
			}
			
			if( Flags & FFEP_DIRECTION )
			{
				Effect->DiEffect.cAxes   = DiEffect->cAxes;
				Effect->DiEffect.rglDirection = NULL;
			}
			
			if( ( Flags & FFEP_ENVELOPE ) && DiEffect->lpEnvelope != NULL )
			{
				memcpy( &Effect->DiEnvelope, DiEffect->lpEnvelope, sizeof( FFENVELOPE ) );
				if( Effect->DiEffect.dwDuration - Effect->DiEnvelope.dwFadeTime
				   < Effect->DiEnvelope.dwAttackTime )
				{
					Effect->DiEnvelope.dwFadeTime = Effect->DiEnvelope.dwAttackTime;
				}
				Effect->DiEffect.lpEnvelope = &Effect->DiEnvelope;
			}
			
			Effect->DiEffect.cbTypeSpecificParams = DiEffect->cbTypeSpecificParams;
			
			if( Flags & FFEP_TYPESPECIFICPARAMS )
			{
				if(CFEqual(EffectType, kFFEffectType_CustomForce_ID)) {
					memcpy(
						   &Effect->DiCustomForce
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiCustomForce;
				}
				
				else if(CFEqual(EffectType, kFFEffectType_ConstantForce_ID)) {
					memcpy(
						   &Effect->DiConstantForce
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiConstantForce;
				}
				else if(CFEqual(EffectType, kFFEffectType_Square_ID) || CFEqual(EffectType, kFFEffectType_Sine_ID) || CFEqual(EffectType, kFFEffectType_Triangle_ID) || CFEqual(EffectType, kFFEffectType_SawtoothUp_ID) || CFEqual(EffectType, kFFEffectType_SawtoothDown_ID) ) {
					memcpy(
						   &Effect->DiPeriodic
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiPeriodic;
				}
				else if(CFEqual(EffectType, kFFEffectType_RampForce_ID)) {
					memcpy(
						   &Effect->DiRampforce
						   ,DiEffect->lpvTypeSpecificParams
						   ,DiEffect->cbTypeSpecificParams );
					Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiRampforce;
				}
			}
			
			if( Flags & FFEP_STARTDELAY )
			{
				Effect->DiEffect.dwStartDelay = DiEffect->dwStartDelay;
			}
			
			if( Flags & FFEP_START )
			{
				Effect->Status  = FFEGES_PLAYING;
				Effect->PlayCount = 1;
				Effect->StartTime = CFAbsoluteTimeGetCurrent();
			}
			
			if( Flags & FFEP_NORESTART )
			{
				;
			}
			Result = FF_OK;
		}
	});
	return Result;
}

HRESULT FeedbackXBox::GetForceFeedbackState(ForceFeedbackDeviceState *DeviceState)
{
	if (DeviceState->dwSize != sizeof(FFDEVICESTATE))
	{
		return FFERR_INVALIDPARAM;
	}
	
	dispatch_sync(Queue, ^{
		DeviceState->dwState = NULL;
		if( EffectList.size() == 0 )
		{
			DeviceState->dwState |= FFGFFS_EMPTY;
		}
		if( Stopped == true )
		{
			DeviceState->dwState |= FFGFFS_STOPPED;
		}
		if( Paused == true )
		{
			DeviceState->dwState |= FFGFFS_PAUSED;
		}
		if (Actuator == true)
		{
			DeviceState->dwState |= FFGFFS_ACTUATORSON;
		} else {
			DeviceState->dwState |= FFGFFS_ACTUATORSOFF;
		}
		DeviceState->dwState |= FFGFFS_POWERON;
		DeviceState->dwState |= FFGFFS_SAFETYSWITCHOFF;
		DeviceState->dwState |= FFGFFS_USERFFSWITCHON;
		
		DeviceState->dwLoad  = 0;
	});
	
	return FF_OK;
}

HRESULT FeedbackXBox::GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities)
{
	capabilities->ffSpecVer.majorRev=kFFPlugInAPIMajorRev;
	capabilities->ffSpecVer.minorAndBugRev=kFFPlugInAPIMinorAndBugRev;
	capabilities->ffSpecVer.stage=kFFPlugInAPIStage;
	capabilities->ffSpecVer.nonRelRev=kFFPlugInAPINonRelRev;
	capabilities->supportedEffects=FFCAP_ET_CUSTOMFORCE|FFCAP_ET_CONSTANTFORCE|FFCAP_ET_RAMPFORCE|FFCAP_ET_SQUARE|FFCAP_ET_SINE|FFCAP_ET_TRIANGLE|FFCAP_ET_SAWTOOTHUP|FFCAP_ET_SAWTOOTHDOWN;
	capabilities->emulatedEffects=0;
	capabilities->subType=FFCAP_ST_VIBRATION;
	capabilities->numFfAxes=2;
	capabilities->ffAxes[0]=FFJOFS_X;
	capabilities->ffAxes[1]=FFJOFS_Y;
	capabilities->storageCapacity=256;
	capabilities->playbackCapacity=1;
	capabilities->driverVer.majorRev=FeedbackDriverVersionMajor;
	capabilities->driverVer.minorAndBugRev=FeedbackDriverVersionMinor;
	capabilities->driverVer.stage=FeedbackDriverVersionStage;
	capabilities->driverVer.nonRelRev=FeedbackDriverVersionNonRelRev;
	capabilities->firmwareVer.majorRev=1;
	capabilities->firmwareVer.minorAndBugRev=0;
	capabilities->firmwareVer.stage=developStage;
	capabilities->firmwareVer.nonRelRev=0;
	capabilities->hardwareVer.majorRev=1;
	capabilities->hardwareVer.minorAndBugRev=0;
	capabilities->hardwareVer.stage=developStage;
	capabilities->hardwareVer.nonRelRev=0;
	return FF_OK;
}

HRESULT FeedbackXBox::SendForceFeedbackCommand(FFCommandFlag state)
{
	__block HRESULT Result = FF_OK;
	
	dispatch_sync(Queue, ^{
		switch (state) {
			case FFSFFC_RESET:
				EffectList.clear();
				Stopped = true;
				Paused = false;
				break;
				
			case FFSFFC_STOPALL:
				for (FeedbackXBoxEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
				{
					effectIterator->Status = NULL;
				}
				Stopped = true;
				Paused = false;
				break;
				
			case FFSFFC_PAUSE:
				Paused  = true;
				PausedTime = CFAbsoluteTimeGetCurrent();
				break;
				
			case FFSFFC_CONTINUE:
				for (FeedbackXBoxEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
				{
					effectIterator->StartTime += ( CFAbsoluteTimeGetCurrent() - PausedTime );
				}
				Paused = false;
				break;
				
			case FFSFFC_SETACTUATORSON:
				Actuator = true;
				break;
				
			case FFSFFC_SETACTUATORSOFF:
				Actuator = false;
				break;
				
			default:
				Result = FFERR_INVALIDPARAM;
				break;
		}
	});
	//return Result;
	return FF_OK;
}

HRESULT FeedbackXBox::InitializeTerminate(NumVersion APIversion, io_object_t hidDevice, boolean_t begin)
{
	if(begin) {
		if (APIversion.majorRev != kFFPlugInAPIMajorRev)
		{
			// fprintf(stderr,"Feedback: Invalid version\n");
			return FFERR_INVALIDPARAM;
		}
		// From probe
		if( (hidDevice==0)
		   || ((!IOObjectConformsTo(hidDevice,"DWXBoxHIDDriver"))
			   ) )
		{
			// fprintf(stderr,"Feedback: Invalid device\n");
			return FFERR_INVALIDPARAM;
		}
		if(!Device_Initialise(&this->device, hidDevice)) {
			// fprintf(stderr,"Feedback: Failed to initialise\n");
			return FFERR_NOINTERFACE;
		}
		Queue = dispatch_queue_create("com.mice.driver.FeedbackXBox", NULL);
		Timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, Queue);
		dispatch_source_set_timer(Timer, dispatch_walltime(NULL, 0), LoopGranularity*1000, 10);
		dispatch_set_context(Timer, this);
		dispatch_source_set_event_handler_f(Timer, EffectProc);
		dispatch_resume(Timer);
	}
	else {
		dispatch_sync(Queue, ^{
			dispatch_source_cancel(Timer);
			SetForce(0, 0);
			Device_Finalise(&this->device);
		});
		
	}
	return FF_OK;
}

HRESULT FeedbackXBox::DestroyEffect(FFEffectDownloadID EffectHandle)
{
	__block HRESULT Result = FF_OK;
	dispatch_sync(Queue, ^{
		for (FeedbackXBoxEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				EffectList.erase(effectIterator);
				break;
			}
		}
	});
	return Result;
}

HRESULT FeedbackXBox::Escape(FFEffectDownloadID downloadID, FFEFFESCAPE *escape)
{
	if (downloadID!=0) return FFERR_UNSUPPORTED;
	if (escape->dwSize < sizeof(FFEFFESCAPE)) return FFERR_INVALIDPARAM;
	escape->cbOutBuffer=0;
	switch (escape->dwCommand) {
		case 0x00:  // Control motors
			if(escape->cbInBuffer!=1) return FFERR_INVALIDPARAM;
			dispatch_sync(Queue, ^{
				Manual=((unsigned char*)escape->lpvInBuffer)[0]!=0x00;
			});
			break;
			
		case 0x01:  // Set motors
			if (escape->cbInBuffer!=2) return FFERR_INVALIDPARAM;
			dispatch_sync(Queue, ^{
				if(Manual) {
					unsigned char *data=(unsigned char *)escape->lpvInBuffer;
					unsigned char buf[]={0x00,0x05,data[0],0x00,data[1]};
					Device_Send(&this->device,buf,sizeof(buf));
				}
			});
			break;
			
		default:
			fprintf(stderr, "XboxController FF plugin: Unknown escape (%i)\n", (int)escape->dwCommand);
			return FFERR_UNSUPPORTED;
	}
	return FF_OK;
}

void FeedbackXBox::SetForce(LONG LeftLevel, LONG RightLevel)
{
	//fprintf(stderr, "LS: %d; RS: %d\n", (unsigned char)MIN( 255, LeftLevel * Gain / 10000 ), (unsigned char)MIN( 255, RightLevel * Gain / 10000 ));
	unsigned char buf[] = {0x00, 0x05, (unsigned char)MIN(255, LeftLevel * Gain / 10000 ), 0x00, (unsigned char)MIN(255, RightLevel * Gain / 10000 )};
	if (!Manual) Device_Send(&device, buf, sizeof(buf));
}

void FeedbackXBox::EffectProc( void *params )
{
	FeedbackXBox *cThis = (FeedbackXBox *)params;
	
	LONG LeftLevel = 0;
	LONG RightLevel = 0;
	LONG Gain  = cThis->Gain;
	LONG CalcResult =0;
	
	if (cThis->Actuator == true)
	{
		for (FeedbackXBoxEffectIterator effectIterator = cThis->EffectList.begin(); effectIterator != cThis->EffectList.end(); ++effectIterator)
		{
			if((CFAbsoluteTimeGetCurrent() - cThis->LastTime*1000*1000) >= effectIterator->DiEffect.dwSamplePeriod) {
				CalcResult = effectIterator->Calc(&LeftLevel, &RightLevel);
			}
		}
	}
	
	if ((cThis->PrvLeftLevel != LeftLevel || cThis->PrvRightLevel != RightLevel) && (CalcResult != -1))
	{
		//fprintf(stderr, "PL: %d, PR: %d; L: %d, R: %d; \n", cThis->PrvLeftLevel, cThis->PrvRightLevel, LeftLevel, RightLevel);
		cThis->SetForce((unsigned char)MIN(255, LeftLevel * Gain / 10000),(unsigned char)MIN( 255, RightLevel * Gain / 10000 ));
		
		cThis->PrvLeftLevel = LeftLevel;
		cThis->PrvRightLevel = RightLevel;
	}
}

HRESULT FeedbackXBox::GetEffectStatus(FFEffectDownloadID EffectHandle, FFEffectStatusFlag *Status)
{
	dispatch_sync(Queue, ^{
		for (FeedbackXBoxEffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
		{
			if (effectIterator->Handle == EffectHandle)
			{
				*Status = effectIterator->Status;
				break;
			}
		}
	});
	return FF_OK;
}

HRESULT FeedbackXBox::GetVersion(ForceFeedbackVersion *version)
{
	version->apiVersion.majorRev = kFFPlugInAPIMajorRev;
	version->apiVersion.minorAndBugRev = kFFPlugInAPIMinorAndBugRev;
	version->apiVersion.stage = kFFPlugInAPIStage;
	version->apiVersion.nonRelRev = kFFPlugInAPINonRelRev;
	version->plugInVersion.majorRev = FeedbackDriverVersionMajor;
	version->plugInVersion.minorAndBugRev = FeedbackDriverVersionMinor;
	version->plugInVersion.stage = FeedbackDriverVersionStage;
	version->plugInVersion.nonRelRev = FeedbackDriverVersionNonRelRev;
	return FF_OK;
}

// static c->c++ glue functions
HRESULT FeedbackXBox::sQueryInterface(void *self, REFIID iid, LPVOID *ppv)
{
	FeedbackXBox *obj = ((Xbox360InterfaceMap *)self)->obj;
	return obj->QueryInterface(iid, ppv);
}

ULONG FeedbackXBox::sAddRef(void *self)
{
	FeedbackXBox *obj = ( (Xbox360InterfaceMap *) self)->obj;
	return obj->AddRef();
}

ULONG FeedbackXBox::sRelease(void *self)
{
	FeedbackXBox *obj = ( (Xbox360InterfaceMap *) self)->obj;
	return obj->Release();
}

IOReturn FeedbackXBox::sProbe(void *self, CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
	return getThis(self)->Probe(propertyTable, service, order);
}

IOReturn FeedbackXBox::sStart(void *self, CFDictionaryRef propertyTable, io_service_t service)
{
	return getThis(self)->Start(propertyTable, service);
}

IOReturn FeedbackXBox::sStop(void *self)
{
	return getThis(self)->Stop();
}

HRESULT FeedbackXBox::sGetVersion(void * self, ForceFeedbackVersion * version)
{
	return FeedbackXBox::getThis(self)->GetVersion(version);
}

HRESULT FeedbackXBox::sInitializeTerminate(void * self, NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin)
{
	return FeedbackXBox::getThis(self)->InitializeTerminate(forceFeedbackAPIVersion, hidDevice, begin);
}

HRESULT FeedbackXBox::sDestroyEffect(void * self, FFEffectDownloadID downloadID)
{
	return FeedbackXBox::getThis(self)->DestroyEffect(downloadID);
}

HRESULT FeedbackXBox::sDownloadEffect(void * self, CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags)
{
	return FeedbackXBox::getThis(self)->DownloadEffect(effectType, pDownloadID, pEffect, flags);
}

HRESULT FeedbackXBox::sEscape(void * self, FFEffectDownloadID downloadID, FFEFFESCAPE * pEscape)
{
	return FeedbackXBox::getThis(self)->Escape(downloadID, pEscape);
}

HRESULT FeedbackXBox::sGetEffectStatus(void * self, FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode)
{
	return FeedbackXBox::getThis(self)->GetEffectStatus(downloadID, pStatusCode);
}

HRESULT FeedbackXBox::sGetForceFeedbackState(void * self, ForceFeedbackDeviceState * pDeviceState)
{
	return FeedbackXBox::getThis(self)->GetForceFeedbackState(pDeviceState);
}

HRESULT FeedbackXBox::sGetForceFeedbackCapabilities(void * self, FFCAPABILITIES * capabilities)
{
	return FeedbackXBox::getThis(self)->GetForceFeedbackCapabilities(capabilities);
}

HRESULT FeedbackXBox::sSendForceFeedbackCommand(void * self, FFCommandFlag state)
{
	return FeedbackXBox::getThis(self)->SendForceFeedbackCommand(state);
}

HRESULT FeedbackXBox::sSetProperty(void * self, FFProperty property, void * pValue)
{
	return FeedbackXBox::getThis(self)->SetProperty(property, pValue);
}

HRESULT FeedbackXBox::sStartEffect(void * self, FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations)
{
	return FeedbackXBox::getThis(self)->StartEffect(downloadID, mode, iterations);
}

HRESULT FeedbackXBox::sStopEffect(void * self, FFEffectDownloadID downloadID)
{
	return FeedbackXBox::getThis(self)->StopEffect(downloadID);
}

// External factory function
void* ControlXBoxFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
	void* result = NULL;
	if (CFEqual(typeID, kIOForceFeedbackLibTypeID))
		result = (void*)FeedbackXBox::Alloc();
	return result;
}
