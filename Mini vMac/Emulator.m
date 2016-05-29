//
//  Emulator.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "Emulator.h"
#include "SYSDEPNS.h"
#include "ENDIANAC.h"
#include "MYOSGLUE.h"

IMPORTPROC RunEmulator(void);
IMPORTFUNC blnr GetSpeedStopped(void);
IMPORTPROC SetSpeedStopped(blnr stopped);
IMPORTPROC SetMouseButton(blnr down);
IMPORTPROC SetMouseLoc(ui4r h, ui4r v);
IMPORTPROC SetMouseDelta(ui4r dh, ui4r dv);
IMPORTFUNC blnr Sony_Insert1(NSString *filePath, blnr silentfail);
IMPORTFUNC blnr Sony_IsInserted(NSString *filePath);
IMPORTPROC SetKeyState(int key, blnr down);

static Emulator *sharedEmulator = nil;
static dispatch_once_t onceToken;

@implementation Emulator

@synthesize dataPath;

+ (instancetype)sharedEmulator {
    dispatch_once(&onceToken, ^{
        sharedEmulator = [self new];
    });
    return sharedEmulator;
}

- (instancetype)init {
    if ((self = [super init])) {
        dispatch_once(&onceToken, ^{
            sharedEmulator = self;
        });
    }
    return self;
}
- (void)run {
    SpeedValue = [[NSUserDefaults standardUserDefaults] integerForKey:@"speedValue"];
    if (SpeedValue > 3) {
        SpeedValue = 3;
    }
    RunEmulator();
}

- (NSInteger)initialSpeed {
    return WantInitSpeedValue;
}

- (NSBundle *)bundle {
    return [NSBundle bundleForClass:self.class];
}

- (NSInteger)speed {
    return SpeedValue;
}

- (void)setSpeed:(NSInteger)speed {
    SpeedValue = speed;
}

- (BOOL)isRunning {
    return !GetSpeedStopped();
}

- (void)setRunning:(BOOL)running {
    SetSpeedStopped(running ? falseblnr : trueblnr);
}

- (void)interrupt {
    WantMacInterrupt = trueblnr;
}

- (void)reset {
    WantMacReset = trueblnr;
}

#pragma mark - Screen

@synthesize screenLayer;

- (CGSize)screenSize {
    return CGSizeMake(vMacScreenWidth, vMacScreenHeight);
}

- (void)updateScreen:(CGImageRef)screenImage {
    screenLayer.contents = (__bridge id)screenImage;
}

#pragma mark - Disk

@synthesize insertDiskNotification, ejectDiskNotification;

- (BOOL)anyDiskInserted {
    return AnyDiskInserted();
}

- (BOOL)isDiskInserted:(NSString *)path {
    return Sony_IsInserted(path);
}

- (BOOL)insertDisk:(NSString *)path {
    return Sony_Insert1(path, false);
}

- (NSString *)insertDiskNotification {
    return @"didInsertDisk";
}

- (NSString *)ejectDiskNotification {
    return @"didEjectDisk";
}

#pragma mark - Keyboard

- (int)translateScanCode:(int)scancode {
    switch (scancode) {
        case 54: return 59; // left control
        case 59: return 70; // arrow left
        case 60: return 66; // arrow right
        case 61: return 72; // arrow down
        case 62: return 77; // arrow up
        default: return scancode;
    }
}

- (void)keyDown:(int)scancode {
    SetKeyState([self translateScanCode:scancode], true);
}

- (void)keyUp:(int)scancode {
    SetKeyState([self translateScanCode:scancode], false);
}

#pragma mark - Mouse

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
