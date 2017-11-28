//
//  ViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2017 namedfork. All rights reserved.
//

#import "ViewController.h"
#import "TouchScreen.h"
#import "TrackPad.h"
#import "AppDelegate.h"
#import "KBKeyboardView.h"
#import "KBKeyboardLayout.h"

@interface ViewController ()

@end

@implementation ViewController
{
    KBKeyboardView *keyboardView;
    UISwipeGestureRecognizer *showKeyboardGesture, *hideKeyboardGesture, *insertDiskGesture, *showSettingsGesture;
    UIControl *pointingDeviceView;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self installKeyboardGestures];
    insertDiskGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:[AppDelegate sharedInstance] action:@selector(showInsertDisk:)];
    insertDiskGesture.direction = UISwipeGestureRecognizerDirectionLeft;
    insertDiskGesture.numberOfTouchesRequired = 2;
    [self.view addGestureRecognizer:insertDiskGesture];
    
    showSettingsGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:[AppDelegate sharedInstance] action:@selector(showSettings:)];
    showSettingsGesture.direction = UISwipeGestureRecognizerDirectionRight;
    showSettingsGesture.numberOfTouchesRequired = 2;
    [self.view addGestureRecognizer:showSettingsGesture];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(emulatorDidShutDown:) name:[AppDelegate sharedEmulator].shutdownNotification object:nil];
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self setUpPointingDevice];
    [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"trackpad" options:0 context:NULL];
}

- (void)viewDidDisappear:(BOOL)animated {
    [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:@"trackpad"];
}

- (void)setUpPointingDevice {
    if (pointingDeviceView) {
        [pointingDeviceView removeFromSuperview];
        pointingDeviceView = nil;
    }
    BOOL useTrackPad = [[NSUserDefaults standardUserDefaults] boolForKey:@"trackpad"];
    Class pointingDeviceClass = useTrackPad ? [TrackPad class] : [TouchScreen class];
    pointingDeviceView = [[pointingDeviceClass alloc] initWithFrame:self.view.bounds];
    pointingDeviceView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view insertSubview:pointingDeviceView aboveSubview:self.screenView];
    if ([UIApplication instancesRespondToSelector:@selector(btcMouseSetRawMode:)]) {
        [[UIApplication sharedApplication] btcMouseSetRawMode:YES];
    }
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event {
    if (motion == UIEventSubtypeMotionShake) {
        [[AppDelegate sharedInstance] showInsertDisk:self];
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSString *,id> *)change context:(void *)context {
    if (object == [NSUserDefaults standardUserDefaults]) {
        if ([keyPath isEqualToString:@"keyboardLayout"] && keyboardView != nil) {
            BOOL keyboardWasVisible = self.keyboardVisible;
            [self setKeyboardVisible:NO animated:NO];
            [keyboardView removeFromSuperview];
            keyboardView = nil;
            if (keyboardWasVisible) {
                [self setKeyboardVisible:YES animated:NO];
            }
        } else if ([keyPath isEqualToString:@"trackpad"]) {
            [self setUpPointingDevice];
        }
    }
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    if (self.keyboardVisible) {
        [self setKeyboardVisible:NO animated:NO];
        [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context) {
            [self setKeyboardVisible:YES animated:YES];
        }];
    }
}

- (void)emulatorDidShutDown:(NSNotification*)notification {
    UILabel *shutdownLabel = [[UILabel alloc] initWithFrame:self.view.bounds];
    shutdownLabel.text = NSLocalizedString(@"the emulated Mac has shut down\ntap to restart", nil);
    shutdownLabel.textColor = [UIColor whiteColor];
    [self.view addSubview:shutdownLabel];
    shutdownLabel.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [shutdownLabel addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(restartEmulator:)]];
    shutdownLabel.numberOfLines = -1;
    shutdownLabel.textAlignment = NSTextAlignmentCenter;
    shutdownLabel.userInteractionEnabled = YES;
    [UIView animateWithDuration:0.5 animations:^{
        self.screenView.alpha = 0.5;
    }];
    [self hideKeyboard:notification];
}

- (void)restartEmulator:(UITapGestureRecognizer*)gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateRecognized) {
        [UIView animateWithDuration:0.5 animations:^{
            self.screenView.alpha = 1.0;
        }];
        [gestureRecognizer.view removeFromSuperview];
        id emulator = [AppDelegate sharedEmulator];
        [emulator performSelector:@selector(run) withObject:nil afterDelay:0.1];
    }
}

#pragma mark - Keyboard

- (void)installKeyboardGestures {
    showKeyboardGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(showKeyboard:)];
    showKeyboardGesture.direction = UISwipeGestureRecognizerDirectionUp;
    showKeyboardGesture.numberOfTouchesRequired = 2;
    [self.view addGestureRecognizer:showKeyboardGesture];
    
    hideKeyboardGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(hideKeyboard:)];
    hideKeyboardGesture.direction = UISwipeGestureRecognizerDirectionDown;
    hideKeyboardGesture.numberOfTouchesRequired = 2;
    [self.view addGestureRecognizer:hideKeyboardGesture];
}

- (BOOL)isKeyboardVisible {
    return keyboardView != nil && CGRectIntersectsRect(keyboardView.frame, self.view.bounds) && !keyboardView.hidden;
}

- (void)setKeyboardVisible:(BOOL)keyboardVisible {
    [self setKeyboardVisible:keyboardVisible animated:YES];
}

- (void)showKeyboard:(id)sender {
    [self setKeyboardVisible:YES animated:YES];
}

- (void)hideKeyboard:(id)sender {
    [self setKeyboardVisible:NO animated:YES];
}

- (void)setKeyboardVisible:(BOOL)visible animated:(BOOL)animated {
    if (self.keyboardVisible == visible) {
        return;
    }
    
    if (visible) {
        [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"keyboardLayout" options:0 context:NULL];
        [self loadKeyboardView];
        if (keyboardView.layout == nil) {
            [keyboardView removeFromSuperview];
            return;
        }
        [self.view addSubview:keyboardView];
        keyboardView.hidden = NO;
        CGRect finalFrame = CGRectMake(0.0, self.view.bounds.size.height - keyboardView.bounds.size.height, keyboardView.bounds.size.width, keyboardView.bounds.size.height);
        if (animated) {
            keyboardView.frame = CGRectOffset(finalFrame, 0.0, finalFrame.size.height);
            [UIView animateWithDuration:0.3 delay:0.0 options:UIViewAnimationOptionCurveEaseOut animations:^{
                keyboardView.frame = finalFrame;
            } completion:nil];
        } else {
            keyboardView.frame = finalFrame;
        }
    } else {
        [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:@"keyboardLayout"];
        if (animated) {
            CGRect finalFrame = CGRectMake(0.0, self.view.bounds.size.height, keyboardView.bounds.size.width, keyboardView.bounds.size.height);
            [UIView animateWithDuration:0.3 delay:0.0 options:UIViewAnimationOptionCurveEaseOut animations:^{
                keyboardView.frame = finalFrame;
            } completion:^(BOOL finished) {
                if (finished) {
                    keyboardView.hidden = YES;
                }
            }];
        } else {
            keyboardView.hidden = YES;
        }
    }
    
}

- (void)loadKeyboardView {
    if (keyboardView != nil && keyboardView.bounds.size.width != self.view.bounds.size.width) {
        // keyboard needs resizing
        [keyboardView removeFromSuperview];
        keyboardView = nil;
    }
    
    if (keyboardView == nil) {
        UIEdgeInsets safeAreaInsets = UIEdgeInsetsZero;
        if (@available(iOS 11, *)) {
            safeAreaInsets = self.view.safeAreaInsets;
        }
        keyboardView = [[KBKeyboardView alloc] initWithFrame:self.view.bounds safeAreaInsets:safeAreaInsets];
        keyboardView.layout = [self keyboardLayout];
        keyboardView.delegate = self;
    }
}

- (KBKeyboardLayout*)keyboardLayout {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *layoutName = [defaults stringForKey:@"keyboardLayout"];
    NSString *layoutPath = [[NSBundle mainBundle] pathForResource:layoutName ofType:nil inDirectory:@"Keyboard Layouts"];
    if (layoutPath == nil) {
        NSLog(@"Layout not found: %@", layoutPath);
    }
    return layoutPath ? [[KBKeyboardLayout alloc] initWithContentsOfFile:layoutPath] : nil;
}

- (void)keyDown:(int)scancode {
    [[AppDelegate sharedEmulator] keyDown:scancode];
}

- (void)keyUp:(int)scancode {
    [[AppDelegate sharedEmulator] keyUp:scancode];
}

@end
