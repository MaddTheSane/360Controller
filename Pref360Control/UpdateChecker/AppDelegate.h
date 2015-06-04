//
//  AppDelegate.h
//  UpdateChecker
//
//  Created by C.W. Betts on 6/4/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>

#define NEXT_RUN_KEY @"NextRunDate"
#define MANUAL_RUN_KEY @"ManualUpdateCheck"
#define UPDATE_URL_KEY @"UpdateFeedURL"
#define SKIPPED_VERSION_KEY @"SkippedVersion"
#define UPDATE_STATUS_NOTIFICATION @"org.mice.UpdateCheckStatus"
#define TIME_INTERVAL_TIL_NEXT_RUN 7*24*60*60

@interface AppDelegate : NSObject <NSApplicationDelegate>
- (void)doUpdateCheck;


@end

