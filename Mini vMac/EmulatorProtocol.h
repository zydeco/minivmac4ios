//
//  EmulatorProtocol.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

@import Foundation;
@import CoreGraphics;
@import QuartzCore;

@protocol Emulator <NSObject>

@property (nonatomic, strong) NSString *dataPath;
@property (nonatomic, assign, getter=isRunning) BOOL running;
@property (nonatomic, assign) NSInteger speed;
@property (nonatomic, weak) CALayer *screenLayer;

@property (nonatomic, readonly) CGSize screenSize;
@property (nonatomic, readonly) NSString *insertDiskNotification, *ejectDiskNotification;
@property (nonatomic, readonly) NSInteger initialSpeed;

@property (nonatomic, readonly) BOOL anyDiskInserted;

+ (instancetype)sharedEmulator;

- (void)run;
- (void)reset;
- (void)interrupt;

- (void)keyDown:(int)scancode;
- (void)keyUp:(int)scancode;

- (void)setMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)moveMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)setMouseButton:(BOOL)down;

- (BOOL)insertDisk:(NSString*)path;
- (BOOL)isDiskInserted:(NSString*)path;

@end
