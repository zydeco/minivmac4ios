//
//  ViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import "ViewController.h"
#import "TouchScreen.h"
#import "TrackPad.h"
#import "AppDelegate.h"
#import "KBKeyboardView.h"
#import "KBKeyboardLayout.h"

@interface ViewController () <UIAdaptivePresentationControllerDelegate, UIPointerInteractionDelegate>

@end

static int8_t usb_to_adb_scancode[] = {
    -1, -1, -1, -1, 0, 11, 8, 2, 14, 3, 5, 4, 34, 38, 40, 37,
    46, 45, 31, 35, 12, 15, 1, 17, 32, 9, 13, 7, 16, 6, 18, 19,
    20, 21, 23, 22, 26, 28, 25, 29, 36, 53, 51, 48, 49, 27, 24, 33,
    30, 42, 42, 41, 39, 10, 43, 47, 44, 57, 122, 120, 99, 118, 96, 97,
    98, 100, 101, 109, 103, 111, 105, 107, 113, 114, 115, 116, 117, 119, 121, 60,
    59, 61, 62, 71, 75, 67, 78, 69, 76, 83, 84, 85, 86, 87, 88, 89,
    91, 92, 82, 65, 50, 55, 126, 81, 105, 107, 113, 106, 64, 79, 80, 90,
    -1, -1, -1, -1, -1, 114, -1, -1, -1, -1, -1, -1, -1, -1, -1, 74,
    72, 73, -1, -1, -1, 95, -1, 94, -1, 93, -1, -1, -1, -1, -1, -1,
    104, 102, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    54, 56, 58, 55, 54, 56, 58, 55, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

@implementation ViewController
{
    KBKeyboardView *keyboardView;
    UISwipeGestureRecognizer *showKeyboardGesture, *hideKeyboardGesture, *insertDiskGesture, *showSettingsGesture;
    UIControl *pointingDeviceView;
    UIViewController *_keyboardViewController;
    BOOL physicalCapsLocked;
    UIPointerInteraction *pointerInteraction;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(emulatorDidShutDown:) name:[AppDelegate sharedEmulator].shutdownNotification object:nil];
    
#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
    [self initXr];
#else
    [self scheduleHelpPresentationIfNeededAfterDelay:6.0];
    [self installGestures];
#endif
}

#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
- (UIViewController *)keyboardViewController {
    if (keyboardView == nil) {
        [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"keyboardLayout" options:0 context:NULL];
        KBKeyboardLayout *layout = [self keyboardLayout];
        CGSize keyboardSize = CGSizeZero;
        for (NSValue *size in layout.availableSizes) {
            if (size.CGSizeValue.width > keyboardSize.width) {
                keyboardSize = size.CGSizeValue;
            }
        }
        keyboardView = [[KBKeyboardView alloc] initWithFrame:CGRectMake(0, 0, keyboardSize.width, keyboardSize.height)];
        keyboardView.layoutMenu = [self keyboardLayoutMenu];
        keyboardView.layout = layout;
        keyboardView.delegate = self;
    }
    if (_keyboardViewController == nil) {
        _keyboardViewController = [UIViewController alloc];
        _keyboardViewController.view = keyboardView;
        _keyboardViewController.preferredContentSize = keyboardView.frame.size;
    } else if (_keyboardViewController.view != keyboardView) {
        _keyboardViewController.view = keyboardView;
    }
    return _keyboardViewController;
}
#endif

- (void)installGestures {
    [self installKeyboardGestures];
    insertDiskGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(showInsertDisk:)];
    insertDiskGesture.direction = UISwipeGestureRecognizerDirectionLeft;
    insertDiskGesture.numberOfTouchesRequired = 2;
    [self.view addGestureRecognizer:insertDiskGesture];

    showSettingsGesture = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(showSettings:)];
    showSettingsGesture.direction = UISwipeGestureRecognizerDirectionRight;
    showSettingsGesture.numberOfTouchesRequired = 2;
    [self.view addGestureRecognizer:showSettingsGesture];

}

- (void)showSettings:(id)sender {
    [self performSegueWithIdentifier:@"settings" sender:sender];
}

- (void)showInsertDisk:(id)sender {
    [self performSegueWithIdentifier:@"disk" sender:sender];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    [self cancelHelpPresentation];
    if ([@[@"disk", @"settings"] containsObject:segue.identifier]) {
        segue.destinationViewController.presentationController.delegate = self;
        if (self.presentedViewController != nil) {
            [self dismissViewControllerAnimated:YES completion:nil];
        }
    }
#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
    if ([segue.identifier isEqualToString:@"disk"]) {
        [(UINavigationController*)segue.destinationViewController setToolbarHidden:YES];
    }
#endif
}

- (void)presentationControllerDidDismiss:(UIPresentationController *)presentationController {
    // hack to fix the presenting view controller not returning back to normal size on iOS 13
    // when the sheet is dismissed interactively (ie dragging it down) after a contextual menu
    // has been shown, which breaks some autolayout constraints. when this happens, the
    // presenting view controller won't resize back to normal during the interactive
    // dismissing of the presented sheet
    if (@available(iOS 14, *)) {
        // it works correctly on iOS 14
    } else {
        self.view.superview.transform = CGAffineTransformIdentity;
    }
}

#if !defined(TARGET_OS_VISION) || TARGET_OS_VISION == 0
- (BOOL)prefersStatusBarHidden {
    UIScreen *screen = self.view.window.screen;
    return CGRectEqualToRect(screen.bounds, self.view.window.bounds);
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}
#endif

- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures {
    return UIRectEdgeAll;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self setUpPointingDevice];
    [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"trackpad" options:0 context:NULL];
}

- (void)viewDidDisappear:(BOOL)animated {
    [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:@"trackpad"];
}

- (BOOL)prefersHomeIndicatorAutoHidden {
    return YES;
}

- (void)motionEnded:(UIEventSubtype)motion withEvent:(UIEvent *)event {
    if (motion == UIEventSubtypeMotionShake) {
        [[AppDelegate sharedInstance] showInsertDisk:self];
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSString *,id> *)change context:(void *)context {
    if (object == [NSUserDefaults standardUserDefaults]) {
        if ([keyPath isEqualToString:@"keyboardLayout"] && keyboardView != nil) {
#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
            // FIXME: do this nicelier
            keyboardView = nil;
            [self keyboardViewController];
#else
            BOOL keyboardWasVisible = self.keyboardVisible;
            [self setKeyboardVisible:NO animated:NO];
            [keyboardView removeFromSuperview];
            keyboardView = nil;
            if (keyboardWasVisible) {
                [self setKeyboardVisible:YES animated:NO];
            }
#endif
        } else if ([keyPath isEqualToString:@"trackpad"]) {
            [self setUpPointingDevice];
        }
    }
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
    if (self.keyboardVisible) {
        // willTransitionToTraitCollection... is caled before us, so keyboard will already be hidden here in a trait collection transition
        [self setKeyboardVisible:NO animated:NO];
        [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context) {
            [self setKeyboardVisible:YES animated:YES];
        }];
    }
}

- (void)willTransitionToTraitCollection:(UITraitCollection *)newCollection withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [super willTransitionToTraitCollection:newCollection withTransitionCoordinator:coordinator];
    if (self.keyboardVisible) {
        [self setKeyboardVisible:NO animated:NO];
        [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext>  _Nonnull context) {
            [self setKeyboardVisible:YES animated:YES];
        }];
    }
}

- (void)emulatorDidShutDown:(NSNotification*)notification {
    if (notification.object == [AppDelegate sharedEmulator]) {
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

#pragma mark - Gesture Help

- (void)showGestureHelp:(id)sender {
#if !defined(TARGET_OS_VISION) || TARGET_OS_VISION == 0
    [self setGestureHelpHidden:NO];
#endif
}

- (void)hideGestureHelp:(id)sender {
    [self setGestureHelpHidden:YES];
}

- (void)setGestureHelpHidden:(BOOL)hidden {
    if (self.helpView.hidden == hidden) {
        return;
    } else if (!hidden) {
        // prepare to show
        self.helpView.alpha = 0.0;
        self.helpView.hidden = NO;
    }
    [UIView animateWithDuration:0.2
                     animations:^{
        self.helpView.alpha = hidden ? 0.0 : 1.0;
    }
                     completion:^(BOOL finished) {
        self.helpView.hidden = hidden;
    }];
}

- (void)showGestureHelpIfNeeded:(id)sender {
    // show help if no disks have been inserted
    if (![AppDelegate sharedEmulator].anyDiskInserted) {
        [self showGestureHelp:sender];
    }
}

- (void)scheduleHelpPresentationIfNeededAfterDelay:(NSTimeInterval)delay {
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"autoShowGestureHelp"]) {
        [self performSelector:@selector(showGestureHelpIfNeeded:) withObject:self afterDelay:delay];
    }
}

- (void)cancelHelpPresentation {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(showGestureHelpIfNeeded:) object:self];
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
#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
    return _keyboardViewController.view.window != nil;
#else
    return keyboardView != nil && CGRectIntersectsRect(keyboardView.frame, self.view.bounds) && !keyboardView.hidden;
#endif
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
#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
    if (visible) {
        UISceneSessionActivationRequest *request = [UISceneSessionActivationRequest requestWithRole:UIWindowSceneSessionRoleApplication];
        request.userActivity = [[NSUserActivity alloc] initWithActivityType:@"net.namedfork.keyboard"];
        request.userActivity.targetContentIdentifier = @"net.namedfork.keyboard";
        [[UIApplication sharedApplication] activateSceneSessionForRequest:request errorHandler:^(NSError * _Nonnull error) {
            NSLog(@"Activation error: %@", error);
        }];
    } else {
        UIScene *keyboardScene = [[AppDelegate sharedInstance] sceneWithName:@"Keyboard"];
        if (keyboardScene != nil) {
            [[UIApplication sharedApplication] requestSceneSessionDestruction:keyboardScene.session options:nil errorHandler:nil];
        }
    }
#else
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
                self->keyboardView.frame = finalFrame;
            } completion:nil];
        } else {
            keyboardView.frame = finalFrame;
        }
    } else {
        [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:@"keyboardLayout"];
        if (animated) {
            CGRect finalFrame = CGRectMake(0.0, self.view.bounds.size.height, keyboardView.bounds.size.width, keyboardView.bounds.size.height);
            [UIView animateWithDuration:0.3 delay:0.0 options:UIViewAnimationOptionCurveEaseOut animations:^{
                self->keyboardView.frame = finalFrame;
            } completion:^(BOOL finished) {
                if (finished) {
                    self->keyboardView.hidden = YES;
                }
            }];
        } else {
            keyboardView.hidden = YES;
        }
    }
#endif
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
    NSString *layoutPath = [[[AppDelegate sharedInstance] userKeyboardLayoutsPath] stringByAppendingPathComponent:layoutName];
    if (![[NSFileManager defaultManager] fileExistsAtPath:layoutPath]) {
        layoutPath = [[NSBundle mainBundle] pathForResource:layoutName ofType:nil inDirectory:@"Keyboard Layouts"];
    }
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

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        [self handlePressEvent:press];
    }
}

- (void)pressesChanged:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        [self handlePressEvent:press];
    }
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        [self handlePressEvent:press];
    }
}

- (void)handlePressEvent:(UIPress *)event {
    long keycode = event.key.keyCode;
    int scancode = -1;

    BOOL emulatorIsFrontmost = [AppDelegate sharedEmulator].running && [AppDelegate sharedInstance].window.rootViewController.presentedViewController == nil;
    BOOL isKeyDown = (event.phase == UIPressPhaseBegan);

    if (keycode >= 0 && keycode < sizeof(usb_to_adb_scancode)) {
        scancode = usb_to_adb_scancode[keycode];
    }

    if (scancode == KC_CAPSLOCK) {
        // caps lock
        if (isKeyDown && !physicalCapsLocked) {
            [[AppDelegate sharedEmulator] keyDown:KC_CAPSLOCK];
            physicalCapsLocked = YES;
        } else if (isKeyDown && physicalCapsLocked) {
            [[AppDelegate sharedEmulator] keyUp:KC_CAPSLOCK];
            physicalCapsLocked = NO;
        }
    } else if (scancode >= 0 && emulatorIsFrontmost) {
        [self _updateCapsLockStatus:event];
        if (isKeyDown) {
            [[AppDelegate sharedEmulator] keyDown:scancode];
        } else {
            [[AppDelegate sharedEmulator] keyUp:scancode];
        }
    }
}

- (void)_updateCapsLockStatus:(UIPress *)event {
    BOOL currentCapsLock = (event.key.modifierFlags & UIKeyModifierAlphaShift) != 0;
    if (currentCapsLock != physicalCapsLocked) {
        physicalCapsLocked = currentCapsLock;
        if (physicalCapsLocked) {
            [[AppDelegate sharedEmulator] keyDown:KC_CAPSLOCK];
        } else {
            [[AppDelegate sharedEmulator] keyUp:KC_CAPSLOCK];
        }
    }
}

#pragma mark - Mouse

- (void)setUpPointingDevice {
    if (pointingDeviceView) {
        [pointingDeviceView removeFromSuperview];
        pointingDeviceView = nil;
    }

    if (pointerInteraction == nil) {
        pointerInteraction = [[UIPointerInteraction alloc] initWithDelegate: self];
        [self.view addInteraction:pointerInteraction];
        UIHoverGestureRecognizer *hoverGestureRecognizer = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(hover:)];
        [self.view addGestureRecognizer:hoverGestureRecognizer];
    }


#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
    Class pointingDeviceClass = [TouchScreen class];
#else
    BOOL useTrackPad = [[NSUserDefaults standardUserDefaults] boolForKey:@"trackpad"];
    Class pointingDeviceClass = useTrackPad ? [TrackPad class] : [TouchScreen class];
#endif
    pointingDeviceView = [[pointingDeviceClass alloc] initWithFrame:self.view.bounds];
    pointingDeviceView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view insertSubview:pointingDeviceView aboveSubview:self.screenView];
    if ([UIApplication instancesRespondToSelector:@selector(btcMouseSetRawMode:)]) {
        [[UIApplication sharedApplication] btcMouseSetRawMode:YES];
    }
}

- (Point)mouseLocForCGPoint:(CGPoint)point {
    Point mouseLoc;
    CGRect screenBounds = self.screenView.screenBounds;
    CGSize screenSize = self.screenView.screenSize;
    mouseLoc.h = (point.x - screenBounds.origin.x) * (screenSize.width/screenBounds.size.width);
    mouseLoc.v = (point.y - screenBounds.origin.y) * (screenSize.height/screenBounds.size.height);
    return mouseLoc;
}

- (UIPointerRegion *)pointerInteraction:(UIPointerInteraction *)interaction regionForRequest:(UIPointerRegionRequest *)request defaultRegion:(UIPointerRegion *)defaultRegion {
    return defaultRegion;
}

- (UIPointerStyle *)pointerInteraction:(UIPointerInteraction *)interaction styleForRegion:(UIPointerRegion *)region {
    return [UIPointerStyle hiddenPointerStyle];
}

- (void)hover:(UIHoverGestureRecognizer *)hoverGestureRecognizer {
    Point mouseLoc = [self mouseLocForCGPoint:[hoverGestureRecognizer locationInView:self.screenView]];
    [[AppDelegate sharedEmulator] setMouseX:mouseLoc.h Y:mouseLoc.v];
}

@end
