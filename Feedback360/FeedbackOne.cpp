//
//  FeedbackOne.cpp
//  360 Driver
//
//  Created by C.W. Betts on 1/8/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#include "FeedbackOne.h"
using std::min;
using std::max;
#define LoopGranularity 10000 // Microseconds

IOCFPlugInInterface** FeedbackOne::Alloc(void)
{
    FeedbackOne *me = new FeedbackOne();
    if (!me) {
        return NULL;
    }
    return (IOCFPlugInInterface **)(&me->iIOCFPlugInInterface.pseudoVTable);
}

FeedbackOne::FeedbackOne(): FeedbackBase(), EffectIndex(1), Stopped(true),
Paused(false), PausedTime(0), LastTime(0), Gain(10000), PrvLeftLevel(0),
PrvRightLevel(0), PrvTriggerLevel(0), Actuator(true), Manual(false)
{
    EffectList = Feedback360EffectVector();
    
    FactoryID = kFeedbackOneUUID;
    CFRetain(FactoryID);
    CFPlugInAddInstanceForFactory(FactoryID);
}

FeedbackOne::~FeedbackOne()
{
    CFPlugInRemoveInstanceForFactory(FactoryID);
    CFRelease(FactoryID);
}

HRESULT FeedbackOne::Escape(FFEffectDownloadID downloadID, FFEFFESCAPE *escape)
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
                    unsigned char buf[]={0x00,0x04,data[0],data[1]};
                    Device_Send(&this->device,buf,sizeof(buf));
                }
            });
            break;
            
        case 0x02:  // Set LED
            if (escape->cbInBuffer!=1) return FFERR_INVALIDPARAM;
        {
            dispatch_sync(Queue, ^{
                unsigned char *data=(unsigned char *)escape->lpvInBuffer;
                unsigned char buf[]={0x01,0x03,data[0]};
                Device_Send(&this->device,buf,sizeof(buf));
            });
        }
            break;
            
        case 0x03:  // Power off
        {
            dispatch_sync(Queue, ^{
                unsigned char buf[] = {0x02, 0x02};
                Device_Send(&this->device, buf, sizeof(buf));
            });
        }
            break;
            
        default:
            fprintf(stderr, "Xbox360Controller FF plugin: Unknown escape (%i)\n", (int)escape->dwCommand);
            return FFERR_UNSUPPORTED;
    }
    return FF_OK;
}

IOReturn FeedbackOne::Probe(CFDictionaryRef propertyTable, io_service_t service, SInt32 *order)
{
    if ((service==0)
        || ((!IOObjectConformsTo(service,"XboxOneControllerClass")))) return kIOReturnBadArgument;
    return S_OK;
}

IOReturn FeedbackOne::Start ( CFDictionaryRef propertyTable, io_service_t service )
{
    return 0;
}

IOReturn FeedbackOne::Stop ( void )
{
    return 0;
}

HRESULT FeedbackOne::GetVersion(ForceFeedbackVersion * version)
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

HRESULT FeedbackOne::InitializeTerminate(NumVersion forceFeedbackAPIVersion, io_object_t hidDevice, boolean_t begin)
{
    if(begin) {
        if (forceFeedbackAPIVersion.majorRev != kFFPlugInAPIMajorRev)
        {
            // fprintf(stderr,"Feedback: Invalid version\n");
            return FFERR_INVALIDPARAM;
        }
        // From probe
        if( (hidDevice==0) || IOObjectConformsTo(hidDevice,"Xbox360ControllerClass")
               ||  IOObjectConformsTo(hidDevice,"Wireless360Controller") ||
           !IOObjectConformsTo(hidDevice,"XboxOneControllerClass"))
        {
            // fprintf(stderr,"Feedback: Invalid device\n");
            return FFERR_INVALIDPARAM;
        }
        if(!Device_Initialise(&this->device, hidDevice)) {
            // fprintf(stderr,"Feedback: Failed to initialise\n");
            return FFERR_NOINTERFACE;
        }
        Queue = dispatch_queue_create("com.mice.driver.Feedback360", NULL);
        Timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, Queue);
        dispatch_source_set_timer(Timer, dispatch_walltime(NULL, 0), LoopGranularity*1000, 10);
        dispatch_set_context(Timer, this);
        dispatch_source_set_event_handler_f(Timer, EffectProc);
        dispatch_resume(Timer);
    }
    else {
        dispatch_sync(Queue, ^{
            dispatch_source_cancel(Timer);
            SetForce(0, 0, 0);
            Device_Finalise(&this->device);
        });
        
    }
    return FF_OK;
}

HRESULT FeedbackOne::DestroyEffect(FFEffectDownloadID downloadID)
{
    __block HRESULT Result = FF_OK;
    dispatch_sync(Queue, ^{
        for (Feedback360EffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
        {
            if (effectIterator->Handle == downloadID)
            {
                EffectList.erase(effectIterator);
                break;
            }
        }
    });
    return Result;
}

HRESULT FeedbackOne::DownloadEffect(CFUUIDRef effectType, FFEffectDownloadID *pDownloadID, FFEFFECT * pEffect, FFEffectParameterFlag flags)
{
    __block HRESULT Result = FF_OK;
    
    if (flags & FFEP_NODOWNLOAD)
    {
        return FF_OK;
    }
    
    dispatch_sync(Queue, ^{
        Feedback360Effect *Effect = NULL;
        if (*pDownloadID == 0) {
            EffectList.push_back(Feedback360Effect(EffectIndex++));
            Effect = &(EffectList.back());
            *pDownloadID = Effect->Handle;
        } else {
            for (LONG Index = 0; Index < EffectList.size(); Index++) {
                if (EffectList[Index].Handle == *pDownloadID) {
                    Effect = &(EffectList[Index]);
                    break;
                }
            }
        }
        
        if (Effect == NULL || Result == -1) {
            Result = FFERR_INTERNAL;
        }
        else {
            Effect->Type = effectType;
            Effect->DiEffect.dwFlags = pEffect->dwFlags;
            
            if( flags & FFEP_DURATION )
            {
                Effect->DiEffect.dwDuration = pEffect->dwDuration;
            }
            
            if( flags & FFEP_SAMPLEPERIOD )
            {
                Effect->DiEffect.dwSamplePeriod = pEffect->dwSamplePeriod;
            }
            
            if( flags & FFEP_GAIN )
            {
                Effect->DiEffect.dwGain = pEffect->dwGain;
            }
            
            if( flags & FFEP_TRIGGERBUTTON )
            {
                Effect->DiEffect.dwTriggerButton = pEffect->dwTriggerButton;
            }
            
            if( flags & FFEP_TRIGGERREPEATINTERVAL )
            {
                Effect->DiEffect.dwTriggerRepeatInterval = pEffect->dwTriggerRepeatInterval;
            }
            
            if( flags & FFEP_AXES )
            {
                Effect->DiEffect.cAxes  = pEffect->cAxes;
                Effect->DiEffect.rgdwAxes = NULL;
            }
            
            if( flags & FFEP_DIRECTION )
            {
                Effect->DiEffect.cAxes   = pEffect->cAxes;
                Effect->DiEffect.rglDirection = NULL;
            }
            
            if( ( flags & FFEP_ENVELOPE ) && pEffect->lpEnvelope != NULL )
            {
                memcpy( &Effect->DiEnvelope, pEffect->lpEnvelope, sizeof( FFENVELOPE ) );
                if( Effect->DiEffect.dwDuration - Effect->DiEnvelope.dwFadeTime
                   < Effect->DiEnvelope.dwAttackTime )
                {
                    Effect->DiEnvelope.dwFadeTime = Effect->DiEnvelope.dwAttackTime;
                }
                Effect->DiEffect.lpEnvelope = &Effect->DiEnvelope;
            }
            
            Effect->DiEffect.cbTypeSpecificParams = pEffect->cbTypeSpecificParams;
            
            if( flags & FFEP_TYPESPECIFICPARAMS )
            {
                if(CFEqual(effectType, kFFEffectType_CustomForce_ID)) {
                    memcpy(
                           &Effect->DiCustomForce
                           ,pEffect->lpvTypeSpecificParams
                           ,pEffect->cbTypeSpecificParams );
                    Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiCustomForce;
                }
                
                else if(CFEqual(effectType, kFFEffectType_ConstantForce_ID)) {
                    memcpy(
                           &Effect->DiConstantForce
                           ,pEffect->lpvTypeSpecificParams
                           ,pEffect->cbTypeSpecificParams );
                    Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiConstantForce;
                }
                else if(CFEqual(effectType, kFFEffectType_Square_ID) || CFEqual(effectType, kFFEffectType_Sine_ID) || CFEqual(effectType, kFFEffectType_Triangle_ID) || CFEqual(effectType, kFFEffectType_SawtoothUp_ID) || CFEqual(effectType, kFFEffectType_SawtoothDown_ID) ) {
                    memcpy(
                           &Effect->DiPeriodic
                           ,pEffect->lpvTypeSpecificParams
                           ,pEffect->cbTypeSpecificParams );
                    Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiPeriodic;
                }
                else if(CFEqual(effectType, kFFEffectType_RampForce_ID)) {
                    memcpy(
                           &Effect->DiRampforce
                           ,pEffect->lpvTypeSpecificParams
                           ,pEffect->cbTypeSpecificParams );
                    Effect->DiEffect.lpvTypeSpecificParams = &Effect->DiRampforce;
                }
            }
            
            if( flags & FFEP_STARTDELAY )
            {
                Effect->DiEffect.dwStartDelay = pEffect->dwStartDelay;
            }
            
            if( flags & FFEP_START )
            {
                Effect->Status  = FFEGES_PLAYING;
                Effect->PlayCount = 1;
                Effect->StartTime = CurrentTimeUsingMach();
            }
            
            if( flags & FFEP_NORESTART )
            {
                ;
            }
            Result = FF_OK;
        }
    });
    return Result;
}

HRESULT FeedbackOne::GetEffectStatus(FFEffectDownloadID downloadID, FFEffectStatusFlag * pStatusCode)
{
    dispatch_sync(Queue, ^{
        for (Feedback360EffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
        {
            if (effectIterator->Handle == downloadID)
            {
                *pStatusCode = effectIterator->Status;
                break;
            }
        }
    });
    return FF_OK;
}

HRESULT FeedbackOne::GetForceFeedbackState(ForceFeedbackDeviceState * pDeviceState)
{
    if (pDeviceState->dwSize != sizeof(FFDEVICESTATE))
    {
        return FFERR_INVALIDPARAM;
    }
    
    dispatch_sync(Queue, ^{
        pDeviceState->dwState = NULL;
        if( EffectList.size() == 0 )
        {
            pDeviceState->dwState |= FFGFFS_EMPTY;
        }
        if( Stopped == true )
        {
            pDeviceState->dwState |= FFGFFS_STOPPED;
        }
        if( Paused == true )
        {
            pDeviceState->dwState |= FFGFFS_PAUSED;
        }
        if (Actuator == true)
        {
            pDeviceState->dwState |= FFGFFS_ACTUATORSON;
        } else {
            pDeviceState->dwState |= FFGFFS_ACTUATORSOFF;
        }
        pDeviceState->dwState |= FFGFFS_POWERON;
        pDeviceState->dwState |= FFGFFS_SAFETYSWITCHOFF;
        pDeviceState->dwState |= FFGFFS_USERFFSWITCHON;
        
        pDeviceState->dwLoad  = 0;
    });
    
    return FF_OK;
}

HRESULT FeedbackOne::GetForceFeedbackCapabilities(FFCAPABILITIES *capabilities)
{
    capabilities->ffSpecVer.majorRev=kFFPlugInAPIMajorRev;
    capabilities->ffSpecVer.minorAndBugRev=kFFPlugInAPIMinorAndBugRev;
    capabilities->ffSpecVer.stage=kFFPlugInAPIStage;
    capabilities->ffSpecVer.nonRelRev=kFFPlugInAPINonRelRev;
    capabilities->supportedEffects=FFCAP_ET_CUSTOMFORCE|FFCAP_ET_CONSTANTFORCE|FFCAP_ET_RAMPFORCE|FFCAP_ET_SQUARE|FFCAP_ET_SINE|FFCAP_ET_TRIANGLE|FFCAP_ET_SAWTOOTHUP|FFCAP_ET_SAWTOOTHDOWN;
    capabilities->emulatedEffects=0;
    capabilities->subType=FFCAP_ST_VIBRATION;
    capabilities->numFfAxes=3;
    capabilities->ffAxes[0]=FFJOFS_X;
    capabilities->ffAxes[1]=FFJOFS_Y;
    capabilities->ffAxes[2]=FFJOFS_Z;
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

HRESULT FeedbackOne::SendForceFeedbackCommand(FFCommandFlag state)
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
                for (Feedback360EffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
                {
                    effectIterator->Status = NULL;
                }
                Stopped = true;
                Paused = false;
                break;
                
            case FFSFFC_PAUSE:
                Paused  = true;
                PausedTime = CurrentTimeUsingMach();
                break;
                
            case FFSFFC_CONTINUE:
                for (Feedback360EffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
                {
                    effectIterator->StartTime += ( CurrentTimeUsingMach() - PausedTime );
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

HRESULT FeedbackOne::SetProperty(FFProperty property, void * pValue)
{
    if(property != FFPROP_FFGAIN) {
        return FFERR_UNSUPPORTED;
    }
    
    UInt32 NewGain = *((UInt32*)pValue);
    __block HRESULT Result = FF_OK;
    
    dispatch_sync(Queue, ^{
        if (1 <= NewGain && NewGain <= 10000)
        {
            Gain = NewGain;
        } else {
            Gain = max((UInt32)1, min(NewGain, (UInt32)10000));
            Result = FF_TRUNCATED;
        }
    });
    
    return Result;
}

HRESULT FeedbackOne::StartEffect(FFEffectDownloadID downloadID, FFEffectStartFlag mode, UInt32 iterations)
{
    dispatch_sync(Queue, ^{
        for (Feedback360EffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
        {
            if (effectIterator->Handle == downloadID)
            {
                effectIterator->Status  = FFEGES_PLAYING;
                effectIterator->PlayCount = iterations;
                effectIterator->StartTime = CurrentTimeUsingMach();
                Stopped = false;
            } else {
                if (mode & FFES_SOLO) {
                    effectIterator->Status = NULL;
                }
            }
        }
    });
    return FF_OK;
}

HRESULT FeedbackOne::StopEffect(UInt32 downloadID)
{
    dispatch_sync(Queue, ^{
        for (Feedback360EffectIterator effectIterator = EffectList.begin() ; effectIterator != EffectList.end(); ++effectIterator)
        {
            if (effectIterator->Handle == downloadID)
            {
                effectIterator->Status = NULL;
                break;
            }
        }
    });
    return FF_OK;
}

void* ControlXboxOneFactory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    void* result = NULL;
    if (CFEqual(typeID, kIOForceFeedbackLibTypeID))
        result = (void*)FeedbackOne::Alloc();
    return result;
}

void FeedbackOne::EffectProc( void *params )
{
    FeedbackOne *cThis = (FeedbackOne *)params;
    
    LONG LeftLevel = 0;
    LONG RightLevel = 0;
    LONG TriggerLevel = 0;
    LONG Gain  = cThis->Gain;
    LONG CalcResult = 0;
    
    if (cThis->Actuator == true)
    {
        for (Feedback360EffectIterator effectIterator = cThis->EffectList.begin(); effectIterator != cThis->EffectList.end(); ++effectIterator)
        {
            if(((CurrentTimeUsingMach() - cThis->LastTime)*1000*1000) >= effectIterator->DiEffect.dwSamplePeriod) {
                CalcResult = effectIterator->Calc(&LeftLevel, &RightLevel);
            }
        }
    }
    
    if ((cThis->PrvLeftLevel != LeftLevel || cThis->PrvRightLevel != RightLevel || cThis->PrvTriggerLevel != TriggerLevel) && (CalcResult != -1))
    {
        //fprintf(stderr, "PL: %d, PR: %d; L: %d, R: %d; \n", cThis->PrvLeftLevel, cThis->PrvRightLevel, LeftLevel, RightLevel);
        cThis->SetForce((unsigned char)min(SCALE_MAX, LeftLevel * Gain / 10000),(unsigned char)min(SCALE_MAX, RightLevel * Gain / 10000 ), (unsigned char)min(SCALE_MAX, TriggerLevel * Gain / 10000));
        
        cThis->PrvLeftLevel = LeftLevel;
        cThis->PrvRightLevel = RightLevel;
        cThis->PrvTriggerLevel = TriggerLevel;
    }
}

void FeedbackOne::SetForce(LONG LeftLevel, LONG RightLevel, LONG triggerLevel)
{
    //fprintf(stderr, "LS: %d; RS: %d\n", (unsigned char)MIN( 255, LeftLevel * Gain / 10000 ), (unsigned char)MIN( 255, RightLevel * Gain / 10000 ));
    unsigned char buf[] = {0x00, 0x04, (unsigned char)min(SCALE_MAX, LeftLevel * (LONG)Gain / 10000 ), (unsigned char)min(SCALE_MAX, RightLevel * (LONG)Gain / 10000 )};
    if (!Manual) Device_Send(&device, buf, sizeof(buf));
}
