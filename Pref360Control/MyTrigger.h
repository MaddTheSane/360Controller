//
//  MyTrigger.h
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import <Cocoa/Cocoa.h>

@interface MyTrigger : NSView
#ifdef __i386__
{
    NSString *_name;
    int _val;
}
#endif
@property (nonatomic, copy) NSString *name;
@property (nonatomic) int val;

@end
