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

- (NSBezierPath*)getBatteryBody
{
    NSRect bounds = self.bounds;
    NSBezierPath *toRet = [NSBezierPath bezierPath];
    CGFloat leftOffset = bounds.size.width * 3 / 40;
    CGFloat downOffset = bounds.size.height * 5 / 24;
    CGFloat upOffset = bounds.size.height - bounds.size.height * 5 / 24;
    CGFloat rightOffset = bounds.size.width - bounds.size.width * 6 / 40;
    [toRet moveToPoint:NSMakePoint(leftOffset, downOffset)];
    [toRet lineToPoint:NSMakePoint(leftOffset, upOffset)];
    [toRet lineToPoint:NSMakePoint(rightOffset, upOffset)];
    [toRet lineToPoint:NSMakePoint(rightOffset, downOffset)];
    [toRet closePath];
    
    return toRet;
}

- (NSRect)getBatteryEnd
{
    NSRect bounds = self.bounds;
    CGFloat leftOffset = bounds.size.width * 3 / 40;
    CGFloat downOffset = bounds.size.height * 5 / 24;
    CGFloat upOffset = bounds.size.height - bounds.size.height * 5 / 24;

    return NSMakeRect(leftOffset, downOffset, bounds.size.width / 40, upOffset - downOffset);

}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    //NSRect bounds = self.bounds;
    
	// Is the current device a wireless controller?
    if (!_isWirelessController) {
        //If it isn't, don't continue: just return
        return;
    }
    
    [[NSColor greenColor] set];
    
    NSRectFill(dirtyRect);
    
    NSBezierPath *batteryBody = [self getBatteryBody];
    [[NSColor whiteColor] setFill];
    [batteryBody fill];
    [[NSColor blackColor] set];
    [batteryBody stroke];
    
    [[NSBezierPath bezierPathWithRect:[self getBatteryEnd]] fill];
}

@end
