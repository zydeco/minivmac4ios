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

@interface NSObject (fs_override)
+(id)sharedApplication;
-(id)keyWindow;
-(id)rootViewController;
-(NSArray *)viewControllers;
-(id)view;
-(NSArray *)subviews;
-(id)timeLabel;
-(id)layer;
-(void)addSubview:(id)subview;
-(CGPoint)center;
-(NSString*)timeText;
-(id)sharedPUICApplication;
-(void)_setStatusBarTimeHidden:(BOOL)hidden animated:(BOOL)animated completion:(void (^)(void))completion;
-(void)setAffineTransform:(CGAffineTransform)transform;
-(void)setContentsScale:(CGFloat)value;
-(void)setContentsGravity:(NSString*)gravity;
-(void)setMinificationFilter:(NSString*)filter;
-(void)addTarget:(nullable id)target action:(SEL)action forControlEvents:(NSUInteger)controlEvents;
-(void)setIdleTimerDisabled:(BOOL)disabled;
@end

@interface InterfaceController () <WKExtendedRuntimeSessionDelegate>

@end

static NSObject<Emulator> *sharedEmulator = nil;

@implementation InterfaceController
{
    WKExtendedRuntimeSession *runtimeSession;
}

+ (void)load {
    /* Hack to make the digital time overlay disappear (on watchOS 6) */
    Class CLKTimeFormatter = NSClassFromString(@"CLKTimeFormatter");
    if ([CLKTimeFormatter instancesRespondToSelector:@selector(timeText)]) {
        Method m = class_getInstanceMethod(CLKTimeFormatter, @selector(timeText));
        method_setImplementation(m, imp_implementationWithBlock(^NSString*(id self, SEL _cmd) { return @" "; }));
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
    id fullScreenView = [self fullScreenView];
    if ([fullScreenView respondsToSelector:@selector(timeLabel)]) {
        [[[fullScreenView timeLabel] layer] setOpacity:0];
    }

    /* Hack to make the digital time overlay disappear (on watchOS 7 and 8) */
    Class PUICApplication = NSClassFromString(@"PUICApplication");
    if ([PUICApplication instancesRespondToSelector:@selector(_setStatusBarTimeHidden:animated:completion:)]) {
        [[PUICApplication sharedApplication] _setStatusBarTimeHidden:YES animated:NO completion:nil];
    }

}

- (UIView*)fullScreenView {
    id parentView = [[[[[[NSClassFromString(@"UIApplication") sharedApplication] keyWindow] rootViewController] viewControllers] firstObject] view];
    id view = [self findDescendantViewOfClass:NSClassFromString(@"SPFullScreenView") inView:parentView]; // watchOS 5
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
        sharedEmulator.running = YES;
    }
}

- (void)didDeactivate {
    // This method is called when watch view controller is no longer visible
    [super didDeactivate];
    sharedEmulator.running = NO;
}

- (void)sessionReachabilityDidChange:(WCSession *)session {
    //uint32_t connected = session.activationState == WCSessionActivationStateActivated && session.reachable;
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
    sharedEmulator.screenLayer = fullScreenView.layer;
    sharedEmulator.speed = sharedEmulator.initialSpeed;
    [sharedEmulator.screenLayer setContentsGravity:@"CAGravityResizeAspectFill"];
    [sharedEmulator.screenLayer setAffineTransform:CGAffineTransformScale(CGAffineTransformMakeRotation(M_PI_2), 0.375, 0.375)];
    [sharedEmulator.screenLayer setMinificationFilter:@"CAFilterTrilinear"];
#if TARGET_OS_SIMULATOR
    [sharedEmulator performSelector:@selector(run) withObject:nil afterDelay:0.1];
#endif

    TrackPad *trackpad = [[TrackPad alloc] initWithFrame:fullScreenView.bounds];
    [fullScreenView addSubview:trackpad];

    runtimeSession = [WKExtendedRuntimeSession new];
    runtimeSession.delegate = self;
    [runtimeSession start];
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
        [sharedEmulator performSelectorOnMainThread:@selector(run) withObject:nil waitUntilDone:NO];
    }];
#endif
}

- (void)extendedRuntimeSession:(WKExtendedRuntimeSession *)extendedRuntimeSession didInvalidateWithReason:(WKExtendedRuntimeSessionInvalidationReason)reason error:(NSError *)error {
    NSLog(@"Runtime session invalidated: %@", error);
}

- (void)extendedRuntimeSessionWillExpire:(WKExtendedRuntimeSession *)extendedRuntimeSession {
    NSLog(@"Extended runtime session will expire");
}

@end
