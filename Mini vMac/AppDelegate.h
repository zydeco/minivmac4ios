//
//  AppDelegate.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2017 namedfork. All rights reserved.
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
@property (readonly, nonatomic, getter = isSandboxed) BOOL sandboxed;

+ (instancetype)sharedInstance;
+ (id<Emulator>)sharedEmulator;

- (void)showAlertWithTitle:(NSString *)title message:(NSString *)message;
- (IBAction)showInsertDisk:(id)sender;
- (IBAction)showSettings:(id)sender;

@end

