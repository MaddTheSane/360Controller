//
//  BattMenuHandler.m
//  360 Driver
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "BattMenuHandler.h"

#define kShowCondition @"ShowCondition"

@interface BattMenuHandler ()
@property (strong) NSObject *defaultsObserver;
@end

@implementation BattMenuHandler

+ (void)initialize
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSDictionary *usrDict = @{kShowCondition: @(BMHShowConditionBattOnly)};
        [[NSUserDefaults standardUserDefaults] registerDefaults:usrDict];
    });
}

- (void)buildStatusMenu
{
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    _statusItem.highlightMode = YES;
    //_statusItem.enabled = NO;
    NSImage *a360Image = [NSImage imageNamed:@"360ControllerTemplate"];
    [a360Image setTemplate:YES];
    _statusItem.image = a360Image;
    NSMenu *menu = [[NSMenu alloc] init];
    NSMenuItem *anItem = [[NSMenuItem alloc] initWithTitle:@"Hello" action:@selector(aQuit:) keyEquivalent:@""];
    anItem.target = self;
    [menu addItem:anItem];
    _statusItem.menu = menu;
}

- (instancetype)init
{
    if (self = [super init]) {
        self.defaultsObserver =
        [[NSNotificationCenter defaultCenter]
         addObserverForName:NSUserDefaultsDidChangeNotification
         object:nil queue:[NSOperationQueue mainQueue]
         usingBlock:^(NSNotification *note) {
             NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
             self.showCondition = [defaults integerForKey:kShowCondition];
             switch (self.showCondition) {
                 case BMHShowConditionAlways:
                 case BMHShowConditionBattOnly:
                     if (!self.statusItem) {
                         [self buildStatusMenu];
                     }
                     break;
                     
                 case BMHShowConditionNone:
                     self.statusItem = nil;
                     break;
                     
                 default:
                     break;
             }
         }];
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        self.showCondition = [defaults integerForKey:kShowCondition];
        if (_showCondition != BMHShowConditionNone) {
            [self buildStatusMenu];
        }
    }
    return self;
}

- (void)dealloc
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center removeObserver:self.defaultsObserver];
}

- (IBAction)aQuit:(id)sender
{
    self.statusItem = nil;
}

@end
