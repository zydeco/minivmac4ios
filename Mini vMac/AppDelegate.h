//
//  AppDelegate.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (assign, nonatomic, getter=isEmulatorRunning) BOOL emulatorRunning;
@property (nonatomic, readonly) NSString *documentsPath;

+ (instancetype)sharedInstance;
- (void)showAlertWithTitle:(NSString *)title message:(NSString *)message;

- (void)setMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)moveMouseX:(NSInteger)x Y:(NSInteger)y;
- (void)setMouseButton:(BOOL)down;

@end

