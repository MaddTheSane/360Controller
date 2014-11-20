/*
    This file is part of the Xbox HID Driver, Copyright (c) 2007 Darrell Walisser
    walisser@mac.com http://sourceforge.net/projects/xhd

    The Xbox HID Driver is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    The Xbox HID Driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Xbox HID Driver; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
//
//  DWXBoxHIDPrefsLoader.m
//  DWXBoxHIDDriver
//
//  Created by Darrell Walisser on Sun Jun 01 2003.
//  Copyright (c) 2003 __MyCompanyName__. All rights reserved.
//
//  v2.0.0 Modified 12/18/2012 to support Standard 32/64-bit architecture. Compiled with Mac OS X 10.6 SDK.
//

#import "DWXBoxHIDPrefsLoader.h"

#define kConfigsKey @"Configurations"
#define kBindingsKey @"Bindings"

#define kConfigTypeKey @"Type"
#define kConfigSettingsKey @"Settings"

#define kConfigNameDefault @"Default"

#define kDefaultsSuiteIdentifier @"org.walisser.XboxHIDDriver"

@implementation DWXBoxHIDPrefsLoader

+(NSMutableDictionary*)defaults
{
    NSUserDefaults *userDefaults = [ NSUserDefaults standardUserDefaults ];
    [ userDefaults synchronize ];
    NSMutableDictionary *defaults = (NSMutableDictionary*)[ userDefaults persistentDomainForName:kDefaultsSuiteIdentifier ];
        
    if (defaults) {
        defaults = (NSMutableDictionary*)
            CFBridgingRelease(CFPropertyListCreateDeepCopy(
                kCFAllocatorDefault, 
                (CFPropertyListRef)defaults, 
                kCFPropertyListMutableContainers));
        
    }
        
    return defaults;
}

+(void)setDefaults:(NSDictionary*)defaults
{
    NSUserDefaults *userDefaults = [ NSUserDefaults standardUserDefaults ];
    [ userDefaults setPersistentDomain:defaults forName:kDefaultsSuiteIdentifier ];
    [ userDefaults synchronize ];
}

// this needs to be called immediately after the driver loads,
// before any other prefs are set for the device
+(BOOL)createDefaultsForDevice:(DWXBoxHIDDriverInterface*)device
{
    NSMutableDictionary *defaults = [ self defaults ];
    
    if (!defaults)
        defaults = [ NSMutableDictionary dictionary ];
    
    // don't overwrite user setting
    if (!defaults[kConfigsKey][kConfigNameDefault]) {
        
        NSLog(@"create default settings for %@", [ device identifier ]);

        NSMutableDictionary *configs = [ NSMutableDictionary dictionary ];
        NSMutableDictionary *aConfig = [ NSMutableDictionary dictionary ];
        
        aConfig[kConfigSettingsKey] = [ device deviceOptions ];
        aConfig[kConfigTypeKey] = [ device deviceType ];
        configs[kConfigNameDefault] = aConfig;
        
        defaults[kConfigsKey] = configs;
    }
    
    // don't overwrite user setting
    if (!defaults[kBindingsKey][[ device identifier ]]) {
    
        NSLog(@"create default binding for %@", [ device identifier ]);
        
        NSMutableDictionary *bindings = defaults[kBindingsKey];
        if (!bindings)
            bindings = [ NSMutableDictionary dictionary ];
        bindings[[ device identifier ]] = kConfigNameDefault;
        defaults[kBindingsKey] = bindings;
    }
    
    [ DWXBoxHIDPrefsLoader setDefaults:defaults ];
    
    return YES;
}

// list all the config names for a particular device type
+(NSArray*)configNamesForDeviceType:(NSString*)deviceType
{
    NSMutableDictionary *prefs = [ DWXBoxHIDPrefsLoader defaults ];
    NSDictionary *configs = prefs[kConfigsKey];
    NSMutableArray *array = nil;
    
    NSEnumerator *keys = [ configs keyEnumerator ];
    if (keys) {
    
        NSString *key;
        array =  [ NSMutableArray array ];
    
        while ((key = [ keys nextObject ])) {
        
            NSString *type;
            
            type = configs[key][kConfigTypeKey];
            if ([ type isEqualTo:deviceType ])
                [ array addObject:key ];
        }
    }
    
    return array;
}

// get the config name of the specified device
+(NSString*)configNameForDevice:(DWXBoxHIDDriverInterface*)device
{
    NSString *configName;
    
    configName = [ DWXBoxHIDPrefsLoader defaults ][kBindingsKey][[ device identifier ]];
    
    if (!configName)
        configName = kConfigNameDefault;
    
    return configName;
}

// load the current config for the specified device
+(BOOL)loadSavedConfigForDevice:(DWXBoxHIDDriverInterface*)device
{
    NSString *configName = [ DWXBoxHIDPrefsLoader configNameForDevice:device ];
    return [ DWXBoxHIDPrefsLoader loadConfigForDevice:device withName:configName ];
}

// save the current config
+(BOOL)saveConfigForDevice:(DWXBoxHIDDriverInterface*)device
{
    NSDictionary *settings = [ device deviceOptions ];
    NSString *configName = [ DWXBoxHIDPrefsLoader configNameForDevice:device ];
    NSString *configType = [ device deviceType ];
    NSMutableDictionary *defaults = [ DWXBoxHIDPrefsLoader defaults ];
    NSMutableDictionary *config = [ NSMutableDictionary dictionary ];
    
    config[kConfigSettingsKey] = settings;
    config[kConfigTypeKey] = configType;
    
    defaults[kConfigsKey][configName] = config;
    
    [ DWXBoxHIDPrefsLoader setDefaults:defaults ];
        
    return YES;
}

// load named config for device
+(BOOL)loadConfigForDevice:(DWXBoxHIDDriverInterface*)device withName:(NSString*)configName
{
    NSMutableDictionary *defaults = [ DWXBoxHIDPrefsLoader defaults ];
    NSDictionary *config = 
        defaults[kConfigsKey][configName];
    
    // first check that config type matches
    if ([ config[kConfigTypeKey]
                isEqualTo:[ device deviceType ] ]) {
                
    
        // then load the config
        BOOL success = [ device loadOptions:config[kConfigSettingsKey] ];
        if (success) {
            
            // change the binding for the device
            defaults[kBindingsKey][[ device identifier ]] = configName;
                
            [ DWXBoxHIDPrefsLoader setDefaults:defaults ];

            // broadcast a message to other applications that the device's configuration has changed
            [ [ NSDistributedNotificationCenter defaultCenter ]
                postNotificationName:kDWXBoxHIDDeviceConfigurationDidChangeNotification 
                object:kDWDistributedNotificationsObject
                userInfo:nil
                deliverImmediately:YES ];
        }
        
        return success;
    }
    
    return NO;
}

// create a new config with current settings, and make it the device's configuration
+(BOOL)createConfigForDevice:(DWXBoxHIDDriverInterface*)device withName:(NSString*)configName
{
    NSMutableDictionary *defaults = [ DWXBoxHIDPrefsLoader defaults ];

    // change the binding to the new config name
    defaults[kBindingsKey][[ device identifier ]] = configName;
    
    [ DWXBoxHIDPrefsLoader setDefaults:defaults ];
    
    // save the current config with the new name
    return [ DWXBoxHIDPrefsLoader saveConfigForDevice:device ];
}

// delete the specified configuration
+(BOOL)deleteConfigWithName:(NSString*)configName
{
    // don't allow deleting the default config
    if (! [ configName isEqualTo:kConfigNameDefault ] ) {
        
        NSMutableDictionary *defaults = [ DWXBoxHIDPrefsLoader defaults ];
        
        // remove the config
        [ defaults[kConfigsKey]
            removeObjectForKey:configName ];
            
        // change any bindings to the default config
        NSEnumerator *identifiers = [ defaults[kBindingsKey]
            keyEnumerator ];
        NSString *identifier;
        
        while (identifier = [ identifiers nextObject ]) {
        
            if ([ defaults[kBindingsKey][identifier] isEqualTo:configName ]) {
                    
                defaults[kBindingsKey][identifier] = kConfigNameDefault;       
            }
        }
    
        [ DWXBoxHIDPrefsLoader setDefaults:defaults ];

        return YES;
    }
    
    return NO;
}


@end
