//
//  MyBatteryView.m
//  360 Driver
//
//  Created by C.W. Betts on 11/19/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#import "MyBatteryView.h"

// 32 * 24
// left: 3 right: 2 top, bottom 5

@implementation MyBatteryView
@synthesize chargeLevel = _chargeLevel;
@synthesize wirelessController = _isWirelessController;

- (void)setChargeLevel:(NSInteger)chargeLevel
{
    NSParameterAssert(chargeLevel <= UCHAR_MAX);
    _chargeLevel = chargeLevel;
    self.needsDisplay = true;
}

- (void)setWirelessController:(BOOL)wirelessController
{
    _isWirelessController = wirelessController;
    self.needsDisplay = true;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
	// Is the current device a wireless controller?
    if (!_isWirelessController) {
        //If it isn't, don't continue: just return
        return;
    }
    
    [[NSColor greenColor] set];
    
    NSRectFill(dirtyRect);
}

@end
