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

@interface InterfaceController ()

@end

static NSObject<Emulator> *sharedEmulator = nil;

@implementation InterfaceController
{

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
    UIView *fullScreenView = [self fullScreenView];
    Class emulatorClass = NSClassFromString(@"MacPlus4MEmulator");
    sharedEmulator = [emulatorClass new];
    sharedEmulator.rootViewController = nil;
    sharedEmulator.showAlert = ^(NSString *title, NSString *message) {
        NSLog(@"Alert: %@ - %@", title, message);
    };
    sharedEmulator.dataPath = [NSBundle mainBundle].resourcePath;
    sharedEmulator.screenLayer = fullScreenView.layer;
    sharedEmulator.speed = sharedEmulator.initialSpeed;
    [sharedEmulator.screenLayer setContentsGravity:@"CAGravityResizeAspectFill"];
    [sharedEmulator.screenLayer setAffineTransform:CGAffineTransformScale(CGAffineTransformMakeRotation(M_PI_2), 0.375, 0.375)];
    [sharedEmulator.screenLayer setMinificationFilter:@"CAFilterTrilinear"];
    [sharedEmulator performSelector:@selector(run) withObject:nil afterDelay:0.1];

    id app = [NSClassFromString(@"UIApplication") sharedApplication];
    [app setIdleTimerDisabled:YES];

    TrackPad *trackpad = [[TrackPad alloc] initWithFrame:fullScreenView.bounds];
    [fullScreenView addSubview:trackpad];
}

@end
