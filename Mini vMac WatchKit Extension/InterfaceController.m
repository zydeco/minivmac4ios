//
//  InterfaceController.m
//  Mini vMac WatchKit Extension
//
//  Created by Jesús A. Álvarez on 14/10/2018.
//  Copyright © 2018 namedfork. All rights reserved.
//

@import Foundation;
@import UIKit;
@import ObjectiveC.runtime;
@import WatchConnectivity;
@import AVFAudio;

#import "UIKit+Watch.h"
#import "InterfaceController.h"
#import "EmulatorProtocol.h"
#import "TrackPad.h"

@interface NSObject ()
@property(nonatomic, copy) NSArray<__kindof UIViewController *> *viewControllers;
- (UIView*)timeLabel;
- (NSString*)timeText;
@end

@interface InterfaceController () <WKExtendedRuntimeSessionDelegate>

@end

static NSObject<Emulator> *sharedEmulator = nil;

@implementation InterfaceController
{
    WKExtendedRuntimeSession *runtimeSession;
    BOOL hasStartedEmulator;
}

+ (void)load {
    /* Hack to make the digital time overlay disappear (on watchOS 6) */
    Class CLKTimeFormatter = NSClassFromString(@"CLKTimeFormatter");
    if ([CLKTimeFormatter instancesRespondToSelector:@selector(timeText)]) {
        Method m = class_getInstanceMethod(CLKTimeFormatter, @selector(timeText));
        method_setImplementation(m, imp_implementationWithBlock(^NSString*(id self, SEL _cmd) { return @" "; }));
    }
    /* hide status bar on watchOS 10 */
    Class clsUIViewController = NSClassFromString(@"UIViewController");
    if ([clsUIViewController instancesRespondToSelector:@selector(prefersStatusBarHidden)]) {
        Method m = class_getInstanceMethod(clsUIViewController, @selector(prefersStatusBarHidden));
        method_setImplementation(m, imp_implementationWithBlock(^BOOL(id self, SEL _cmd) { return YES; }));
    }
}

+ (id<Emulator>)sharedEmulator {
    return sharedEmulator;
}

- (void)awakeWithContext:(id)context {
    [super awakeWithContext:context];
}

- (void)hideTimeLabel {
    /* Hack to make the digital time overlay disappear (on watchOS 5) */
    UIView *fullScreenView = [self fullScreenView];
    if ([fullScreenView respondsToSelector:@selector(timeLabel)]) {
        fullScreenView.timeLabel.layer.opacity = 0.0;
    }

    /* Hack to make the digital time overlay disappear (on watchOS 7 and 8) */
    Class clsPUICApplication = NSClassFromString(@"PUICApplication");
    if ([clsPUICApplication instancesRespondToSelector:@selector(_setStatusBarTimeHidden:animated:completion:)]) {
        PUICApplication *app = (PUICApplication*)[clsPUICApplication sharedApplication];
        [app _setStatusBarTimeHidden:YES animated:NO completion:nil];
    }

}

- (UIView*)fullScreenView {
    UIApplication *app = [NSClassFromString(@"UIApplication") sharedApplication];
    UIView *parentView = app.keyWindow.rootViewController.viewControllers.firstObject.view;
    UIView *view = [self findDescendantViewOfClass:NSClassFromString(@"SPFullScreenView") inView:parentView]; // watchOS 5
    if (view == nil) {
        view = [self findDescendantViewOfClass:NSClassFromString(@"SPInterfaceRemoteView") inView:parentView]; // watchOS 6
    }
    return view;
}

- (id)findDescendantViewOfClass:(Class)viewClass inView:(id)parentView {
    for (NSObject *view in [parentView subviews]) {
        if ([view isKindOfClass:viewClass]) {
            return view;
        } else {
            id foundView = [self findDescendantViewOfClass:viewClass inView:view];
            if (foundView != nil) return foundView;
        }
    }
    return nil;
}

- (void)didAppear {
    [self hideTimeLabel];
    if (sharedEmulator == nil) {
        [self loadAndStartEmulator];
    } else {
        sharedEmulator.running = YES;
    }
}

- (void)willActivate {
    if ([WKExtension sharedExtension].applicationState == WKApplicationStateActive) {
        [self startRuntimeSession];
    }
}

- (void)didDeactivate {
    // This method is called when watch view controller is no longer visible
    [super didDeactivate];
    [runtimeSession invalidate];
    sharedEmulator.running = NO;
}

- (void)sessionReachabilityDidChange:(WCSession *)session {
    //uint32_t connected = session.activationState == WCSessionActivationStateActivated && session.reachable;
}

- (void)startRuntimeSession {
    runtimeSession = [WKExtendedRuntimeSession new];
    runtimeSession.delegate = self;
    [runtimeSession start];
}

- (void)startOrResumeEmulator {
    if (!hasStartedEmulator) {
        hasStartedEmulator = YES;
        [sharedEmulator performSelectorOnMainThread:@selector(run) withObject:nil waitUntilDone:NO];
    } else {
        sharedEmulator.running = YES;
    }
}

- (void)loadAndStartEmulator {
#ifdef LTOVRTCP_SERVER
    setenv("LTOVRTCP_SERVER", LTOVRTCP_SERVER, 1);
#endif
    UIView *fullScreenView = [self fullScreenView];
    Class emulatorClass = NSClassFromString(@"MacPlus4MEmulator");
    sharedEmulator = [emulatorClass new];
    sharedEmulator.rootViewController = self;
    sharedEmulator.showAlert = ^(NSString *title, NSString *message) {
        [self presentAlertControllerWithTitle:title message:message preferredStyle:WKAlertControllerStyleAlert actions:@[
            [WKAlertAction actionWithTitle:@"OK" style:WKAlertActionStyleDefault handler:^{}]]];
    };

    NSString *documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject.stringByStandardizingPath;
    NSString *resourcePath = [NSBundle mainBundle].resourcePath;
    NSFileManager *fm = [NSFileManager defaultManager];
    for (NSString *fileName in @[@"vMac.rom", @"disk1.dsk"]) {
        NSString *srcPath = [resourcePath stringByAppendingPathComponent:fileName];
        NSString *dstPath = [documentsPath stringByAppendingPathComponent:fileName];
        if ([fm fileExistsAtPath:srcPath] && ![fm fileExistsAtPath:dstPath]) {
            [fm copyItemAtPath:srcPath toPath:dstPath error:NULL];
        }
    }
    sharedEmulator.dataPath = documentsPath;

    // screen
    CALayer *screenLayer = [NSClassFromString(@"CALayer") layer];
    [fullScreenView.layer addSublayer:screenLayer];
    screenLayer.frame = fullScreenView.layer.bounds;
    sharedEmulator.screenLayer = screenLayer;
    sharedEmulator.speed = sharedEmulator.initialSpeed;
    [screenLayer setContentsGravity:@"CAGravityResizeAspectFill"];
    CGFloat scale = [self bestScaleForScreen];
    [screenLayer setAffineTransform:CGAffineTransformScale(CGAffineTransformMakeRotation(M_PI_2), scale, scale)];
    [screenLayer setMinificationFilter:@"CAFilterTrilinear"];

    // trackpad
    TrackPad *trackpad = [[TrackPad alloc] initWithFrame:CGRectMake(0, 0, 512, 342)];
    [fullScreenView addSubview:trackpad];
    trackpad.center = fullScreenView.center;
    [trackpad.layer setAffineTransform:CGAffineTransformScale(CGAffineTransformMakeRotation(M_PI_2), 0.5, 0.5)];
    trackpad.layer.masksToBounds = YES;
    fullScreenView.clipsToBounds = NO;

    [self startRuntimeSession];
}

- (CGFloat)bestScaleForScreen {
    CGSize screenSize = [UIScreen mainScreen].bounds.size;
    NSInteger screenWidthAndHeight = (NSInteger)(screenSize.width) * 1000 + (NSInteger)(screenSize.height);
    // manually selected scales to account for non-square screens
    switch (screenWidthAndHeight) {
            // 38 40 41 42 44 45 49
        case 136170: // 38mm
            return 0.33;
        case 176215: // 41mm
            return 0.40;
        case 184224: // 44mm
            return 0.42;
        case 198242: // 45mm
        case 205251: // 49mm
            return 0.455;
        case 162197: // 40mm
        case 156195: // 42mm
        default:
            return 0.375;
    }
}

- (void)extendedRuntimeSessionDidStart:(WKExtendedRuntimeSession *)extendedRuntimeSession {
#if TARGET_OS_SIMULATOR == 0
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback
                                            mode:AVAudioSessionModeDefault
                              routeSharingPolicy:AVAudioSessionRouteSharingPolicyLongFormAudio
                                         options:0
                                           error:NULL];
    [[AVAudioSession sharedInstance] activateWithOptions:0 completionHandler:^(BOOL activated, NSError * _Nullable error) {
        // network only works on watchOS when there's an active audio session
        [self startOrResumeEmulator];
    }];
#else
    [self startOrResumeEmulator];
#endif
}

- (void)extendedRuntimeSession:(WKExtendedRuntimeSession *)extendedRuntimeSession didInvalidateWithReason:(WKExtendedRuntimeSessionInvalidationReason)reason error:(NSError *)error {
    NSLog(@"Runtime session invalidated: %@", error);
}

- (void)extendedRuntimeSessionWillExpire:(WKExtendedRuntimeSession *)extendedRuntimeSession {
    NSLog(@"Extended runtime session will expire");
}

@end
