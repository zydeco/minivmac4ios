//
//  AppDelegate.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "EmulatorProtocol.h"
#import "BTCMouse.h"

extern NSString *DocumentsChangedNotification;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (nonatomic, readonly) NSString *documentsPath;
@property (nonatomic, readonly) NSString *userKeyboardLayoutsPath;
@property (nonatomic, readonly) NSArray<NSString*> *diskImageExtensions;
@property (nonatomic, readonly) NSArray<NSBundle*> *emulatorBundles;
@property (nonatomic, readonly) NSString *emulatorBundlesPath;
@property (readonly, nonatomic, getter = isSandboxed) BOOL sandboxed;
@property (readonly, nonatomic) id<Emulator> sharedEmulator;

@property (class, readonly, strong) AppDelegate *sharedInstance NS_SWIFT_NAME(shared);
@property (class, readonly, strong) id<Emulator> sharedEmulator NS_SWIFT_NAME(emulator);
- (void)loadAndStartEmulator;

- (void)showAlertWithTitle:(NSString *)title message:(NSString *)message;
- (IBAction)showInsertDisk:(id)sender;
- (IBAction)showSettings:(id)sender;
- (IBAction)showGestureHelp:(id)sender;

@end

