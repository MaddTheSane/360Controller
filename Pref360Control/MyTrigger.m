//
//  MyTrigger.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import "MyTrigger.h"
#import "Pref360StyleKit.h"
#import "ARCBridge.h"

@implementation MyTrigger
@synthesize name = _name;
@synthesize val = _val;

- (void)setName:(NSString *)name {
#if __has_feature(objc_arc)
    _name = name;
#else
    [_name autorelease];
    _name = [name retain];
#endif
    [self setNeedsDisplay:YES];
}

- (void)setVal:(int)value {
    _val = value;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    if (_name == NULL)
        _name = RETAINOBJ(@"");
    [Pref360StyleKit drawTriggerMetterWithIntensity:(_val / 255.0) triggerTitle:_name];
}

@end
