//
//  AppDelegate.m
//  360BatteryMenu
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "AppDelegate.h"

@interface AppDelegate ()
@property BattMenuHandler* menuHandler;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application
    self.menuHandler = [[BattMenuHandler alloc] init];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
}

@end
