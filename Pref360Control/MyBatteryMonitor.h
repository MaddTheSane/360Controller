//
//  MyBatteryMonitor.h
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import <Cocoa/Cocoa.h>

@interface MyBatteryMonitor : NSView
#ifdef __i386__
{
    int _bars;
}
#endif
@property (nonatomic) int bars;

@end
