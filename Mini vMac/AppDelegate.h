//
//  AppDelegate.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

extern NSString * const MNVMDidInsertDiskNotification;
extern NSString * const MNVMDidEjectDiskNotification;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (assign, nonatomic, getter=isEmulatorRunning) BOOL emulatorRunning;
@property (nonatomic, readonly) NSString *documentsPath;
@property (nonatomic, readonly) NSArray<NSString*> *diskImageExtensions;

+ (instancetype)sharedInstance;
- (void)showAlertWithTitle:(NSString *)title message:(NSString *)message;
- (IBAction)showInsertDisk:(id)sender;
- (IBAction)showSettings:(id)sender;

- (void)setMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)moveMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)setMouseButton:(BOOL)down;

- (void)keyDown:(int)scancode;
- (void)keyUp:(int)scancode;

- (BOOL)insertDisk:(NSString*)path;
- (BOOL)isDiskInserted:(NSString*)path;

- (void)macInterrupt;
- (void)macReset;

@end

