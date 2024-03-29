//
//  EmulatorProtocol.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/05/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

@import Foundation;
@import CoreGraphics;
#if TARGET_OS_WATCH
#define CALayer NSObject
@class InterfaceController;
#define UIViewController InterfaceController
@interface NSObject (CALayer)
- (void)setContents:(id)contents;
@end
#else
@import QuartzCore;
#endif

typedef NS_ENUM(NSInteger, EmulatorSpeed) {
    EmulatorSpeedAllOut = -1,
    EmulatorSpeed1x = 0,
    EmulatorSpeed2x = 1,
    EmulatorSpeed4x = 2,
    EmulatorSpeed8x = 3,
    EmulatorSpeed16x = 4,
    EmulatorSpeed32x = 5
};

@protocol Emulator <NSObject>

@property (nonatomic, strong) NSString *dataPath;
@property (nonatomic, assign, getter=isRunning) BOOL running;
@property (nonatomic, assign) EmulatorSpeed speed;
@property (nonatomic, assign) BOOL autoSlow;
@property (nonatomic, readonly) BOOL initialAutoSlow;
@property (nonatomic, readonly) BOOL autoSlowSupported;
@property (nonatomic, weak) CALayer *screenLayer;

@property (nonatomic, readonly) NSBundle *bundle;
@property (nonatomic, readonly) CGSize screenSize;
@property (nonatomic, readonly) NSString *insertDiskNotification, *ejectDiskNotification, *shutdownNotification;
@property (nonatomic, readonly) NSInteger initialSpeed;

@property (nonatomic, readonly) BOOL anyDiskInserted;
@property (nonatomic, readonly) NSString *currentApplication;

@property (nonatomic, strong) void (^showAlert)(NSString *title, NSString *message);
@property (nonatomic, strong) UIViewController *rootViewController;

- (void)run;
- (void)reset;
- (void)interrupt;
- (void)shutdown;

- (void)keyDown:(int)scancode;
- (void)keyUp:(int)scancode;

- (void)setMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)moveMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)setMouseButton:(BOOL)down;

- (BOOL)insertDisk:(NSString*)path;
- (BOOL)isDiskInserted:(NSString*)path;

@end
