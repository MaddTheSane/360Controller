//
//  BattMenuHandler.m
//  360 Driver
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "BattMenuHandler.h"
#import <Foundation/Foundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/usb/IOUSBLib.h>
#include <ForceFeedback/ForceFeedback.h>

#define kShowCondition @"ShowCondition"

static void callbackConnected(void *param,io_iterator_t iterator);
static void callbackDisconnected(void *param,io_iterator_t iterator);

@interface BattMenuHandler ()
@property (strong) NSObject *defaultsObserver;
@end

@implementation BattMenuHandler
{
    mach_port_t masterPort;
    IONotificationPortRef notifyPort;
    CFRunLoopSourceRef notifySource;
    io_iterator_t onIteratorWired;
    io_iterator_t onIteratorWireless;
    io_iterator_t onIteratorOther;
    io_iterator_t offIteratorWired;
    io_iterator_t offIteratorWireless;

}
+ (void)initialize
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSDictionary *usrDict = @{kShowCondition: @(BMHShowConditionBattOnly)};
        [[NSUserDefaults standardUserDefaults] registerDefaults:usrDict];
    });
}

- (void)buildStatusMenu
{
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    _statusItem.highlightMode = YES;
    //_statusItem.enabled = NO;
    NSImage *a360Image = [NSImage imageNamed:@"360ControllerTemplate"];
    [a360Image setTemplate:YES];
    _statusItem.image = a360Image;
    NSMenu *menu = [[NSMenu alloc] init];
    for (NSInteger i = 0; i < 4; i++) {
        NSMenuItem *anItem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Disconnect controller %li", (long)i] action:@selector(disconnect:) keyEquivalent:@""];
        anItem.target = self;
        anItem.tag = i;
        [menu addItem:anItem];
    }
    _statusItem.menu = menu;
}

- (instancetype)init
{
    if (self = [super init]) {
        NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
        self.defaultsObserver =
        [defaultCenter
         addObserverForName:NSUserDefaultsDidChangeNotification
         object:nil queue:[NSOperationQueue mainQueue]
         usingBlock:^(NSNotification *note) {
             NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
             self.showCondition = [defaults integerForKey:kShowCondition];
             switch (self.showCondition) {
                 case BMHShowConditionAlways:
                 case BMHShowConditionBattOnly:
                     if (!self.statusItem) {
                         [self buildStatusMenu];
                     }
                     break;
                     
                 case BMHShowConditionNone:
                     self.statusItem = nil;
                     break;
                     
                 default:
                     break;
             }
         }];
        
        IOMasterPort(MACH_PORT_NULL,&masterPort);
        // Set up notification of USB device addition/removal
        notifyPort=IONotificationPortCreate(masterPort);
        notifySource=IONotificationPortGetRunLoopSource(notifyPort);
        CFRunLoopAddSource([[NSRunLoop mainRunLoop] getCFRunLoop],notifySource,kCFRunLoopCommonModes);
        // Start listening
        // USB devices
        IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching(kIOUSBDeviceClassName), callbackConnected, (__bridge void *)(self), &onIteratorOther);
        callbackConnected((__bridge void *)(self), onIteratorOther);
        // Wired 360 devices
        IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("Xbox360ControllerClass"), callbackConnected, (__bridge void *)(self), &onIteratorWired);
        callbackConnected((__bridge void *)(self), onIteratorWired);
        IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("Xbox360ControllerClass"), callbackDisconnected, (__bridge void *)(self), &offIteratorWired);
        callbackDisconnected((__bridge void *)(self), offIteratorWired);
        // Wireless 360 devices
        IOServiceAddMatchingNotification(notifyPort, kIOFirstMatchNotification, IOServiceMatching("WirelessHIDDevice"), callbackConnected, (__bridge void *)(self), &onIteratorWireless);
        callbackConnected((__bridge void *)(self), onIteratorWireless);
        IOServiceAddMatchingNotification(notifyPort, kIOTerminatedNotification, IOServiceMatching("WirelessHIDDevice"), callbackDisconnected, (__bridge void *)(self), &offIteratorWireless);
        callbackDisconnected((__bridge void *)(self), offIteratorWireless);

        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        self.showCondition = [defaults integerForKey:kShowCondition];
        if (_showCondition != BMHShowConditionNone) {
            [self buildStatusMenu];
        }
    }
    return self;
}

- (void)connectedItem:(io_iterator_t)iterator
{
    
}

- (void)disconnectedItem:(io_iterator_t)iterator
{
    
}

- (void)powerOffObject:(FFDeviceObjectReference)ffDevice
{
    FFEFFESCAPE escape = {0};
    
    if (ffDevice == NULL) {
        return;
    }
    escape.dwSize=sizeof(escape);
    escape.dwCommand=0x03;
    FFDeviceEscape(ffDevice, &escape);
}

- (void)powerOffController:(NSInteger)idx
{
    
}

- (void)dealloc
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center removeObserver:self.defaultsObserver];
    
    // Stop listening
    IOObjectRelease(onIteratorOther);
    IOObjectRelease(onIteratorWired);
    IOObjectRelease(offIteratorWired);
    IOObjectRelease(onIteratorWireless);
    IOObjectRelease(offIteratorWireless);
    CFRunLoopRemoveSource([[NSRunLoop mainRunLoop] getCFRunLoop], notifySource, kCFRunLoopCommonModes);
    CFRunLoopSourceInvalidate(notifySource);
    IONotificationPortDestroy(notifyPort);
}

- (IBAction)disconnect:(id)sender
{
    self.statusItem = nil;
}

@end

void callbackConnected(void *param,io_iterator_t iterator)
{
    BattMenuHandler *handler = (__bridge BattMenuHandler *)(param);
    
    [handler connectedItem:iterator];
}

void callbackDisconnected(void *param,io_iterator_t iterator)
{
    BattMenuHandler *handler = (__bridge BattMenuHandler *)(param);
    
    [handler disconnectedItem:iterator];
}

