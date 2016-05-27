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
EXPORTVAR(ui3b,SpeedValue);
IMPORTPROC SetKeyState(int key, blnr down);
IMPORTPROC MacInterrupt();
IMPORTPROC MacReset();

static AppDelegate *sharedAppDelegate = nil;
NSString * const MNVMDidInsertDiskNotification = @"MNVMDidInsertDisk";
NSString * const MNVMDidEjectDiskNotification = @"MNVMDidEjectDisk";

@interface AppDelegate () <UIViewControllerTransitioningDelegate, UIViewControllerAnimatedTransitioning>

@end

@implementation AppDelegate
{
    UISwipeGestureRecognizerDirection modalPanePresentationDirection;
}

+ (instancetype)sharedInstance {
    return sharedAppDelegate;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    sharedAppDelegate = self;
    [self initDefaults];
    [self performSelector:@selector(runEmulator) withObject:nil afterDelay:0.1];
    return YES;
}

- (void)initDefaults {
    // default settings
    NSDictionary *layoutForLanguage = @{@"en": @"British.nfkeyboardlayout",
                                        @"es": @"Spanish.nfkeyboardlayout",
                                        @"en-US": @"US.nfkeyboardlayout"};
    NSString *firstLanguage = [NSBundle preferredLocalizationsFromArray:layoutForLanguage.allKeys].firstObject;
    NSDictionary *defaultValues = @{@"trackpad": @([UIDevice currentDevice].userInterfaceIdiom != UIUserInterfaceIdiomPad),
                                    @"frameskip": @(0),
                                    @"keyboardLayout": layoutForLanguage[firstLanguage]
                                    };
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:defaultValues];
    [defaults setValue:@(WantInitSpeedValue) forKey:@"speedValue"];
    [defaults addObserver:self forKeyPath:@"speedValue" options:0 context:NULL];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSString *,id> *)change context:(void *)context {
    if (object == [NSUserDefaults standardUserDefaults]) {
        if ([keyPath isEqualToString:@"speedValue"]) {
            SpeedValue = [[NSUserDefaults standardUserDefaults] integerForKey:@"speedValue"];
        }
    }
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
    if ([UIAlertController class]) {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
        UIViewController *controller = self.window.rootViewController;
        [controller presentViewController:alert animated:YES completion:nil];
    } else {
        UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:title message:message delegate:nil cancelButtonTitle:@"OK" otherButtonTitles: nil];
        [alertView show];
    }
}

#pragma mark - Settings / Insert Disk panels

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
        [presentedViewController dismissViewControllerAnimated:YES completion:nil];
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
        UIViewController *viewController = [rootViewController.storyboard instantiateViewControllerWithIdentifier:name];
        viewController.modalTransitionStyle = UIModalTransitionStyleCoverVertical;
        viewController.modalPresentationStyle = UIModalPresentationFormSheet;
        if ([sender isKindOfClass:[UISwipeGestureRecognizer class]] && [UIDevice currentDevice].systemVersion.integerValue >= 8) {
            modalPanePresentationDirection = [(UISwipeGestureRecognizer*)sender direction];
            viewController.transitioningDelegate = self;
        }
        [rootViewController presentViewController:viewController animated:YES completion:nil];
    }
}

- (id<UIViewControllerAnimatedTransitioning>)animationControllerForPresentedController:(UIViewController *)presented presentingController:(UIViewController *)presenting sourceController:(UIViewController *)source {
    return self;
}

- (NSTimeInterval)transitionDuration:(id<UIViewControllerContextTransitioning>)transitionContext {
    return 0.3;
}

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext {
    UIView *containerView = [transitionContext containerView];
    UIView *toView = [transitionContext viewForKey:UITransitionContextToViewKey];
    
    [containerView addSubview:toView];
    switch (modalPanePresentationDirection) {
        case UISwipeGestureRecognizerDirectionLeft:
            toView.transform = CGAffineTransformMakeTranslation(containerView.bounds.size.width, 0);
            break;
        case UISwipeGestureRecognizerDirectionRight:
            toView.transform = CGAffineTransformMakeTranslation(-containerView.bounds.size.width, 0);
            break;
        case UISwipeGestureRecognizerDirectionDown:
            toView.transform = CGAffineTransformMakeTranslation(0, -containerView.bounds.size.height);
            break;
        default:
            toView.transform = CGAffineTransformMakeTranslation(0, containerView.bounds.size.height);
    }
    
    [UIView animateWithDuration:[self transitionDuration:transitionContext] animations:^{
        toView.transform = CGAffineTransformIdentity;
    } completion:^(BOOL finished) {
        [transitionContext completeTransition:finished];
    }];
}

#pragma mark - Files

- (BOOL)isSandboxed {
    static dispatch_once_t onceToken;
    static BOOL sandboxed;
    dispatch_once(&onceToken, ^{
        NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
        sandboxed = ![bundlePath hasPrefix:@"/Applications/"];
    });
    return sandboxed;
}

- (NSArray<NSString *> *)diskImageExtensions {
    return @[@"dsk", @"img", @"dc42", @"diskcopy42"];
}

- (NSString *)documentsPath {
    static dispatch_once_t onceToken;
    static NSString *documentsPath;
    dispatch_once(&onceToken, ^{
        documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject.stringByStandardizingPath;
        if (!self.sandboxed) {
            documentsPath = [documentsPath stringByAppendingPathComponent:@"Mini vMac"].stringByStandardizingPath;
        }
        [[NSFileManager defaultManager] createDirectoryAtPath:documentsPath withIntermediateDirectories:YES attributes:nil error:NULL];
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
    SpeedValue = [[NSUserDefaults standardUserDefaults] integerForKey:@"speedValue"];
    if (SpeedValue > 3) {
        SpeedValue = 3;
    }
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

- (void)macInterrupt {
    MacInterrupt();
}

-(void)macReset {
    MacReset();
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

#pragma mark - Disk Drive

- (BOOL)insertDisk:(NSString *)path {
    return Sony_Insert1(path.stringByStandardizingPath, falseblnr);
}

- (BOOL)isDiskInserted:(NSString *)path {
    return Sony_IsInserted(path.stringByStandardizingPath);
}

@end
