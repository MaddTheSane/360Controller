//
//  AppDelegate.m
//  UpdateChecker
//
//  Created by C.W. Betts on 6/4/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "AppDelegate.h"

@interface AppDelegate ()
@property (weak) IBOutlet NSWindow *window;
@property (strong) NSDate *lastRunDate;
@property (strong) SUAppcastItem *latest;
@property (strong) NSString *downloadPath;
@property (strong) SUAppcast *appcast;
@property (strong) SUUpdater *updater;
@end

@implementation AppDelegate
{
    BOOL	manualRun;
}
@synthesize lastRunDate;
@synthesize latest;
@synthesize downloadPath;
@synthesize appcast;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    self.lastRunDate = [defaults objectForKey:NEXT_RUN_KEY];
    
    if (![lastRunDate isEqualToDate:[NSDate distantFuture]]) {
        [defaults setObject:[NSDate dateWithTimeIntervalSinceNow:TIME_INTERVAL_TIL_NEXT_RUN] forKey:NEXT_RUN_KEY];
    }
    
    manualRun = [defaults boolForKey:MANUAL_RUN_KEY];
    [defaults removeObjectForKey:MANUAL_RUN_KEY];
    [defaults synchronize];
    [self doUpdateCheck];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
}

static id SUInfoValueForKey(NSString *key)
{
    return [[NSBundle mainBundle] objectForInfoDictionaryKey:key];
}


- (void)doUpdateCheck
{
    self.updater = [[SUUpdater alloc] initForBundle:[NSBundle mainBundle]];
    _updater.delegate = self;
    
    if (manualRun) {
        [self.updater checkForUpdates:nil];
    } else {
        [self.updater checkForUpdatesInBackground];
    }
}

- (NSString *)feedURLStringForUpdater:(SUUpdater *)updater
{
    return SUInfoValueForKey(UPDATE_URL_KEY);
}

@end
