//
//  MyDeadZoneViewer.m
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import "MyDeadZoneViewer.h"
#import "Pref360StyleKit.h"

@implementation MyDeadZoneViewer
@synthesize val = _val;
@synthesize linked = _linked;

-(void)setVal:(double)value {
    _val = value;
    [self setNeedsDisplay:YES];
}

- (void)setLinked:(BOOL)linked {
    _linked = linked;
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    [Pref360StyleKit drawDeadZoneViewerWithValue:_val / 32768 linked:_linked];
}

@end
