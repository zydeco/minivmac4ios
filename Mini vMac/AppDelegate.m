//
//  AppDelegate.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "AppDelegate.h"
#import "SettingsViewController.h"
#import "InsertDiskViewController.h"
#include "CNFGRAPI.h"
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"

IMPORTPROC RunEmulator(void);
IMPORTFUNC blnr GetSpeedStopped(void);
IMPORTPROC SetSpeedStopped(blnr stopped);
IMPORTPROC SetMouseButton(blnr down);
IMPORTPROC SetMouseLoc(ui4r h, ui4r v);
IMPORTPROC SetMouseDelta(ui4r dh, ui4r dv);
IMPORTFUNC blnr Sony_Insert1(NSString *filePath, blnr silentfail);
IMPORTFUNC blnr Sony_IsInserted(NSString *filePath);

static AppDelegate *sharedAppDelegate = nil;
NSString * const MNVMDidInsertDiskNotification = @"MNVMDidInsertDisk";
NSString * const MNVMDidEjectDiskNotification = @"MNVMDidEjectDisk";

@interface AppDelegate ()

@end

@implementation AppDelegate

+ (instancetype)sharedInstance {
    return sharedAppDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    sharedAppDelegate = self;
    [self performSelector:@selector(runEmulator) withObject:nil afterDelay:0.1];
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    self.emulatorRunning = NO;
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    self.emulatorRunning = YES;
}

- (void)showAlertWithTitle:(NSString *)title message:(NSString *)message {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self showAlertWithTitle:title message:message];
        });
        return;
    }
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    UIViewController *controller = self.window.rootViewController;
    [controller presentViewController:alert animated:YES completion:nil];
}

- (void)showSettings:(id)sender {
    [self showModalPanel:@"settings" sender:sender];
}

- (void)showInsertDisk:(id)sender {
    [self showModalPanel:@"disk" sender:sender];
}

- (void)showModalPanel:(NSString*)name sender:(id)sender {
    Class classToShow, otherClass;
    if ([name isEqualToString:@"settings"]) {
        classToShow = [SettingsViewController class];
        otherClass = [InsertDiskViewController class];
    } else {
        classToShow = [InsertDiskViewController class];
        otherClass = [SettingsViewController class];
    }
    
    UIViewController *rootViewController = self.window.rootViewController;
    UIViewController *presentedViewController = rootViewController.presentedViewController;
    UIViewController *presentedTopViewController = [presentedViewController isKindOfClass:[UINavigationController class]] ? [(UINavigationController*)presentedViewController topViewController] : nil;
    
    if ([presentedTopViewController isKindOfClass:classToShow]) {
        return;
    } else if ([presentedTopViewController isKindOfClass:otherClass]) {
        // flip
        UIViewController *viewController = [rootViewController.storyboard instantiateViewControllerWithIdentifier:name];
        viewController.modalTransitionStyle = UIModalTransitionStyleFlipHorizontal;
        viewController.modalPresentationStyle = UIModalPresentationFormSheet;
        UIView *windowSnapshotView = [self.window snapshotViewAfterScreenUpdates:NO];
        [self.window addSubview:windowSnapshotView];
        UIView *oldPanelSnapshotView = [presentedViewController.view snapshotViewAfterScreenUpdates:NO];
        [viewController.view addSubview:oldPanelSnapshotView];
        [rootViewController dismissViewControllerAnimated:NO completion:^{
            [rootViewController presentViewController:viewController animated:NO completion:^{
                UIView *emptyView = [[UIView alloc] initWithFrame:viewController.view.bounds];
                [windowSnapshotView removeFromSuperview];
                viewController.modalTransitionStyle = UIModalTransitionStyleCoverVertical;
                [UIView transitionFromView:oldPanelSnapshotView
                                    toView:emptyView
                                  duration:0.5
                                   options:UIViewAnimationOptionTransitionFlipFromRight
                                completion:^(BOOL finished) {
                                    [emptyView removeFromSuperview];
                                }];
            }];
        }];
    } else {
        [self.window.rootViewController performSegueWithIdentifier:name sender:sender];
    }
}

#pragma mark - Files

- (NSArray<NSString *> *)diskImageExtensions {
    return @[@"dsk", @"img", @"dc42", @"diskcopy42"];
}

- (NSString *)documentsPath {
    static dispatch_once_t onceToken;
    static NSString *documentsPath;
    dispatch_once(&onceToken, ^{
        documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject.stringByStandardizingPath;
        [[NSFileManager defaultManager] createDirectoryAtPath:documentsPath withIntermediateDirectories:YES attributes:nil error:NULL];
    });
    return documentsPath;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    if (url.fileURL) {
        // opening file
        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSString *fileName = url.path.lastPathComponent;
        NSString *destinationPath = [self.documentsPath stringByAppendingPathComponent:fileName];
        NSError *error = NULL;
        NSInteger tries = 1;
        while ([fileManager fileExistsAtPath:destinationPath]) {
            NSString *newFileName;
            if (fileName.pathExtension.length > 0) {
                newFileName = [NSString stringWithFormat:@"%@ %d.%@", fileName.stringByDeletingPathExtension, (int)tries, fileName.pathExtension];
            } else {
                newFileName = [NSString stringWithFormat:@"%@ %d", fileName, (int)tries];
            }
            destinationPath = [self.documentsPath stringByAppendingPathComponent:newFileName];
            tries++;
        }
        [fileManager moveItemAtPath:url.path toPath:destinationPath error:&error];
        if (error) {
            [self showAlertWithTitle:fileName message:error.localizedFailureReason];
        } else {
            [self showAlertWithTitle:@"File Import" message:[NSString stringWithFormat:@"%@ imported to Documents", destinationPath.lastPathComponent]];
        }
    }
    return YES;
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

#pragma mark - Disk Drive

- (BOOL)insertDisk:(NSString *)path {
    return Sony_Insert1(path.stringByStandardizingPath, falseblnr);
}

- (BOOL)isDiskInserted:(NSString *)path {
    return Sony_IsInserted(path.stringByStandardizingPath);
}

@end
