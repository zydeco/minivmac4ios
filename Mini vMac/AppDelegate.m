//
//  AppDelegate.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2017 namedfork. All rights reserved.
//

@import AVFoundation;
#import "AppDelegate.h"
#import "SettingsViewController.h"
#import "InsertDiskViewController.h"

static AppDelegate *sharedAppDelegate = nil;
static NSObject<Emulator> *sharedEmulator = nil;
NSString *DocumentsChangedNotification = @"documentsChanged";

@interface AppDelegate () <UIViewControllerTransitioningDelegate, UIViewControllerAnimatedTransitioning, BTCMouseDelegate>

@end

@implementation AppDelegate
{
    UISwipeGestureRecognizerDirection modalPanePresentationDirection;
}

+ (instancetype)sharedInstance {
    return sharedAppDelegate;
}

+ (id<Emulator>)sharedEmulator {
    return sharedEmulator;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    sharedAppDelegate = self;
    if (![self loadEmulator:[[NSUserDefaults standardUserDefaults] stringForKey:@"machine"]]) {
        [self loadEmulator:@"MacPlus4M"];
    }
    [self initDefaults];
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:NULL];
    [sharedEmulator performSelector:@selector(run) withObject:nil afterDelay:0.1];
    
    if ([application respondsToSelector:@selector(btcMouseSetRawMode:)]) {
        [application btcMouseSetRawMode:YES];
        [application btcMouseSetDelegate:self];
    }
    return YES;
}

- (void)initDefaults {
    // default settings
    NSDictionary *layoutForLanguage = @{@"en": @"British.nfkeyboardlayout",
                                        @"es": @"Spanish.nfkeyboardlayout",
                                        @"en-US": @"US.nfkeyboardlayout"};
    NSString *firstLanguage = [NSBundle preferredLocalizationsFromArray:layoutForLanguage.allKeys].firstObject;
    NSDictionary *defaultValues = @{@"trackpad": @([UIDevice currentDevice].userInterfaceIdiom != UIUserInterfaceIdiomPad),
                                    @"keyboardLayout": layoutForLanguage[firstLanguage],
                                    @"machine": @"MacPlus4M",
                                    @"speedValue": @(sharedEmulator.initialSpeed),
                                    @"runInBackground": @NO,
                                    @"autoSlow": @(sharedEmulator.initialAutoSlow),
                                    @"screenFilter": kCAFilterLinear
                                    };
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:defaultValues];
    [defaults setValue:@(sharedEmulator.initialSpeed) forKey:@"speedValue"];
    [defaults addObserver:self forKeyPath:@"speedValue" options:0 context:NULL];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSString *,id> *)change context:(void *)context {
    if (object == [NSUserDefaults standardUserDefaults]) {
        NSUserDefaults *defaults = object;
        if ([keyPath isEqualToString:@"speedValue"]) {
            sharedEmulator.speed = [defaults integerForKey:@"speedValue"];
        } else if ([keyPath isEqualToString:@"autoSlow"]) {
            sharedEmulator.autoSlow = [defaults integerForKey:@"autoSlow"];
        }
    }
}

- (NSArray<NSBundle*>*)emulatorBundles {
    NSString *pluginsPath = [NSBundle mainBundle].builtInPlugInsPath;
    NSArray<NSString*> *names = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pluginsPath error:NULL];
    NSMutableArray *emulatorBundles = [NSMutableArray arrayWithCapacity:names.count];
    for (NSString *name in [names pathsMatchingExtensions:@[@"mnvm"]]) {
        NSBundle *bundle = [NSBundle bundleWithPath:[pluginsPath stringByAppendingPathComponent:name]];
        [emulatorBundles addObject:bundle];
    }
    return emulatorBundles;
}

- (BOOL)loadEmulator:(NSString*)name {
    NSString *emulatorBundleName = [name stringByAppendingPathExtension:@"mnvm"];
    NSString *emulatorBundlePath = [[NSBundle mainBundle].builtInPlugInsPath stringByAppendingPathComponent:emulatorBundleName];
    NSBundle *emulatorBundle = [NSBundle bundleWithPath:emulatorBundlePath];
    [emulatorBundle load];
    sharedEmulator = [[emulatorBundle principalClass] new];
    sharedEmulator.dataPath = self.documentsPath;
    return sharedEmulator != nil;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults synchronize];
    if ([defaults boolForKey:@"runInBackground"]) {
        // slow down to 1x when in background
        sharedEmulator.speed = EmulatorSpeed1x;
    } else {
        sharedEmulator.running = NO;
    }
    if (sharedEmulator.anyDiskInserted == NO) {
        exit(0);
    }
}

- (void)handleEventWithMove:(CGPoint)move andWheel:(float)wheel andPan:(float)pan andButtons:(int)buttons {
    [sharedEmulator moveMouseX:move.x/2.0 Y:move.y/2.0];
    [sharedEmulator setMouseButton:buttons == 1];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (sharedEmulator.running) {
        sharedEmulator.speed = [defaults integerForKey:@"speedValue"];
    } else {
        sharedEmulator.running = YES;
    }
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
        while (controller.presentedViewController) {
            controller = controller.presentedViewController;
        }
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
        if ([sender isKindOfClass:[UISwipeGestureRecognizer class]] && NSFoundationVersionNumber >= NSFoundationVersionNumber_iOS_8_0) {
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
#if TARGET_IPHONE_SIMULATOR
    return YES;
#else
    static dispatch_once_t onceToken;
    static BOOL sandboxed;
    dispatch_once(&onceToken, ^{
        // not sandboxed if parent of documents directory is "mobile"
        NSString *documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject.stringByStandardizingPath;
        sandboxed = ![documentsPath.stringByDeletingLastPathComponent.lastPathComponent isEqualToString:@"mobile"];
    });
    return sandboxed;
#endif
}

- (NSArray<NSString *> *)diskImageExtensions {
    return @[@"dsk", @"img", @"dc42", @"diskcopy42", @"image"];
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
            NSDictionary *userInfo = @{@"path": destinationPath};
            [[NSNotificationCenter defaultCenter] postNotificationName:DocumentsChangedNotification object:self userInfo:userInfo];
            [self showAlertWithTitle:@"File Import" message:[NSString stringWithFormat:@"%@ imported to Documents", destinationPath.lastPathComponent]];
        }
    }
    return YES;
}

@end
