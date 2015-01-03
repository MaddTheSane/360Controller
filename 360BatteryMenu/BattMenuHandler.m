//
//  BattMenuHandler.m
//  360 Driver
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 GitHub. All rights reserved.
//

#import "BattMenuHandler.h"

@implementation BattMenuHandler

- (void)buildStatusMenu
{
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    _statusItem.highlightMode = YES;
    //_statusItem.enabled = NO;
    _statusItem.title = @"Hello";
    NSMenu *menu = [[NSMenu alloc] init];
    NSMenuItem *anItem = [[NSMenuItem alloc] initWithTitle:@"Hello" action:@selector(aQuit:) keyEquivalent:@""];
    anItem.target = self;
    [menu addItem:anItem];
    _statusItem.menu = menu;
}

- (instancetype)init
{
    if (self = [super init]) {
        [self buildStatusMenu];
    }
    return self;
}

- (IBAction)aQuit:(id)sender
{
    self.statusItem = nil;
}

@end
