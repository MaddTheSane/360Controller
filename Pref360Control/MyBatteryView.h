//
//  MyBatteryView.h
//  360 Driver
//
//  Created by C.W. Betts on 11/19/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface MyBatteryView : NSView
@property unsigned char chargeLevel;
@property (getter=isWirelessController) BOOL wirelessController;
@end
