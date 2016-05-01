//
//  AppDelegate.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "AppDelegate.h"
#include "CNFGRAPI.h"
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"

IMPORTPROC RunEmulator(void);
IMPORTFUNC blnr GetSpeedStopped(void);
IMPORTPROC SetSpeedStopped(blnr stopped);
IMPORTPROC SetMouseButton(blnr down);
IMPORTPROC SetMouseLoc(ui4r h, ui4r v);
IMPORTPROC SetMouseDelta(ui4r dh, ui4r dv);

static AppDelegate *sharedAppDelegate = nil;

@interface AppDelegate ()

@end

@implementation AppDelegate

+ (instancetype)sharedInstance {
    return sharedAppDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    sharedAppDelegate = self;
    [self performSelector:@selector(runEmulator) withObject:nil afterDelay:1.0];
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    SetSpeedStopped(trueblnr);
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    SetSpeedStopped(falseblnr);
}

#pragma mark - Emulation

- (void)runEmulator {
    RunEmulator();
}

- (BOOL)isEmulatorRunning {
    return !GetSpeedStopped();
}

- (void)setEmulatorRunning:(BOOL)emulatorRunning {
    SetSpeedStopped(emulatorRunning);
}

- (void)setMouseX:(NSInteger)x Y:(NSInteger)y {
    SetMouseLoc(x, y);
}

- (void)moveMouseX:(NSInteger)x Y:(NSInteger)y {
    SetMouseDelta(x, y);
}

- (void)setMouseButton:(BOOL)down {
    SetMouseButton(down);
}

@end
