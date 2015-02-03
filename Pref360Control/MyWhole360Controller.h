//
//  MyWhole360Controller.h
//  360 Driver
//
//  Created by Pierre TACCHI on 21/01/15.
//

#import <Cocoa/Cocoa.h>

@interface MyWhole360Controller : NSView
#ifdef __i386__
{
    BOOL _aPressed;
    BOOL _bPressed;
    BOOL _xPressed;
    BOOL _yPressed;
    
    BOOL _leftPressed;
    BOOL _upPressed;
    BOOL _rightPressed;
    BOOL _downPressed;
    
    BOOL _backPressed;
    BOOL _startPressed;
    BOOL _homePressed;
    
    BOOL _lbPressed;
    BOOL _rbPressed;
    BOOL _leftStickPressed;
    BOOL _rightStickPressed;
    
    CGPoint _leftStickPosition;
    CGPoint _rightStickPosition;
    
    int _leftStickXPos;
    int _leftStickYPos;
    int _rightStickXPos;
    int _rightStickYPos;
}
#endif

@property (nonatomic) BOOL aPressed;
@property (nonatomic) BOOL bPressed;
@property (nonatomic) BOOL xPressed;
@property (nonatomic) BOOL yPressed;

@property (nonatomic) BOOL leftPressed;
@property (nonatomic) BOOL upPressed;
@property (nonatomic) BOOL rightPressed;
@property (nonatomic) BOOL downPressed;

@property (nonatomic) BOOL backPressed;
@property (nonatomic) BOOL startPressed;
@property (nonatomic) BOOL homePressed;

@property (nonatomic) BOOL lbPressed;
@property (nonatomic) BOOL rbPressed;
@property (nonatomic) BOOL leftStickPressed;
@property (nonatomic) BOOL rightStickPressed;

@property (nonatomic) CGPoint leftStickPosition;
@property (nonatomic) CGPoint rightStickPosition;

@property (nonatomic) int leftStickXPos;
@property (nonatomic) int leftStickYPos;
@property (nonatomic) int rightStickXPos;
@property (nonatomic) int rightStickYPos;

- (void)reset;

@end
