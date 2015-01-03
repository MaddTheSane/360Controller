//
//  BattMenuHandler.h
//  360 Driver
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import <Cocoa/Cocoa.h>

typedef NS_ENUM(NSInteger, BMHShowCondition) {
    BMHShowConditionNone = 0,
    BMHShowConditionBattOnly,
    BMHShowConditionAlways
};

@interface BattMenuHandler : NSObject
@property (strong) NSStatusItem *statusItem;
@property (nonatomic) BMHShowCondition showCondition;

- (instancetype)init;

@end
