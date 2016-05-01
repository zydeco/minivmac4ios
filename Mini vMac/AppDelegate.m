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
IMPORTPROC SetSpeedStopped(blnr stopped);

static AppDelegate *sharedAppDelegate = nil;

@interface AppDelegate ()

@end

@implementation AppDelegate

+ (instancetype)sharedInstance {
    return sharedAppDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [self performSelector:@selector(runEmulator) withObject:nil afterDelay:1.0];
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    SetSpeedStopped(trueblnr);
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    SetSpeedStopped(falseblnr);
}

- (void)runEmulator {
    RunEmulator();
}

@end
