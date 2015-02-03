//
//  MyDeadZoneViewer.h
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import <Cocoa/Cocoa.h>

@interface MyDeadZoneViewer : NSView
#ifdef __i386__
{
    double _val;
    BOOL _linked;
}
#endif
@property (nonatomic) double val;
@property (nonatomic) BOOL linked;

@end
