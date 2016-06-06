//
//  MNVMApplication.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 14/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "MNVMApplication.h"
#import "AppDelegate.h"

@interface UIApplication ()
- (void)handleKeyUIEvent:(UIEvent *)event;
@end

Class keyboardEventClass = nil;

static int8_t usb_to_adb_scancode[] = {
    -1, -1, -1, -1, 0, 11, 8, 2, 14, 3, 5, 4, 34, 38, 40, 37,
    46, 45, 31, 35, 12, 15, 1, 17, 32, 9, 13, 7, 16, 6, 18, 19,
    20, 21, 23, 22, 26, 28, 25, 29, 36, 53, 51, 48, 49, 27, 24, 33,
    30, 42, 42, 41, 39, 10, 43, 47, 44, 57, 122, 120, 99, 118, 96, 97,
    98, 100, 101, 109, 103, 111, 105, 107, 113, 114, 115, 116, 117, 119, 121, 60,
    59, 61, 62, 71, 75, 67, 78, 69, 76, 83, 84, 85, 86, 87, 88, 89,
    91, 92, 82, 65, 50, 55, 126, 81, 105, 107, 113, 106, 64, 79, 80, 90,
    -1, -1, -1, -1, -1, 114, -1, -1, -1, -1, -1, -1, -1, -1, -1, 74,
    72, 73, -1, -1, -1, 95, -1, 94, -1, 93, -1, -1, -1, -1, -1, -1,
    104, 102, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    54, 56, 58, 55, 54, 56, 58, 55, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

@interface UIPhysicalKeyboardEvent : UIPressesEvent

@property (nonatomic, readonly) BOOL _isKeyDown;
@property (nonatomic, readonly) long _keyCode;
@property (nonatomic) int _modifierFlags;
@property(retain, nonatomic) NSString *_unmodifiedInput;
@property(retain, nonatomic) NSString *_modifiedInput;

@end

@implementation MNVMApplication
{
    BOOL physicalCapsLocked;
}

+ (void)load {
    // class is not visible
    keyboardEventClass = NSClassFromString(@"UIPhysicalKeyboardEvent");
}

- (void)handleKeyboardEvent:(UIPhysicalKeyboardEvent *)event {
    long keycode = event._keyCode;
    int scancode = -1;
    
    if (keycode >= 0 && keycode < sizeof(usb_to_adb_scancode)) {
        scancode = usb_to_adb_scancode[keycode];
    }
    
    if (scancode == 57) {
        // caps lock
        if (event._isKeyDown && !physicalCapsLocked) {
            [[AppDelegate sharedEmulator] keyDown:scancode];
            physicalCapsLocked = YES;
        } else if (event._isKeyDown && physicalCapsLocked) {
            [[AppDelegate sharedEmulator] keyUp:scancode];
            physicalCapsLocked = NO;
        }
    } else if (scancode >= 0 && [AppDelegate sharedEmulator].running) {
        if (event._isKeyDown) {
            [self _updateCapsLockStatus:event];
            [[AppDelegate sharedEmulator] keyDown:scancode];
        } else {
            [[AppDelegate sharedEmulator] keyUp:scancode];
        }
    }
}

- (void)_updateCapsLockStatus:(UIPhysicalKeyboardEvent *)event {
    if (event._modifierFlags == 0 && event._unmodifiedInput.length == 1) {
        unichar unmodifiedChar = [event._unmodifiedInput characterAtIndex:0];
        unichar modifiedChar = [event._modifiedInput characterAtIndex:0];
        if ([[NSCharacterSet lowercaseLetterCharacterSet] characterIsMember:unmodifiedChar]) {
            BOOL currentCapsLock = [[NSCharacterSet uppercaseLetterCharacterSet] characterIsMember:modifiedChar];
            if (currentCapsLock != physicalCapsLocked) {
                physicalCapsLocked = currentCapsLock;
                if (physicalCapsLocked) {
                    [[AppDelegate sharedEmulator] keyDown:57];
                } else {
                    [[AppDelegate sharedEmulator] keyUp:57];
                }
            }
        }
    }
}

- (void)handleKeyUIEvent:(UIEvent *)event {
    [super handleKeyUIEvent:event];
    static dispatch_once_t onceToken;
    static BOOL handleKeyboardEvents = YES;
    dispatch_once(&onceToken, ^{
        if ([NSProcessInfo instancesRespondToSelector:@selector(operatingSystemVersion)]) {
            handleKeyboardEvents = [NSProcessInfo processInfo].operatingSystemVersion.majorVersion >= 9;
        } else {
            handleKeyboardEvents = NO;
        }
    });
    BOOL emulatorIsFrontmost = [AppDelegate sharedEmulator].running && [AppDelegate sharedInstance].window.rootViewController.presentedViewController == nil;
    if ([event isKindOfClass:keyboardEventClass] && handleKeyboardEvents && emulatorIsFrontmost) {
        [self handleKeyboardEvent:(UIPhysicalKeyboardEvent*)event];
    }
}

@end
