//
//  TrackPad.m
//  Mini vMac for iOS
//
//  Created by Jesús A. Álvarez on 18/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import "TrackPad.h"
#import "AppDelegate.h"
@import AudioToolbox;

#define TRACKPAD_ACCEL_N 1
#define TRACKPAD_ACCEL_T 0.2
#define TRACKPAD_ACCEL_D 20

@implementation TrackPad
{
    NSTimeInterval touchTimeThreshold;
    NSTimeInterval previousClickTime, previousTouchTime;
    CGFloat touchDistanceThreshold;
    CGPoint previousTouchLoc;
    BOOL shouldClick;
    BOOL isDragging;
    BOOL supportsForceTouch, didForceClick;
    NSMutableSet *currentTouches;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        touchTimeThreshold = 0.25;
        touchDistanceThreshold = 16;
        currentTouches = [NSMutableSet setWithCapacity:4];
        self.multipleTouchEnabled = YES;
    }
    return self;
}

- (void)willMoveToSuperview:(UIView *)newSuperview {
    [super willMoveToSuperview:newSuperview];
    @try {
        supportsForceTouch = (newSuperview.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable);
    } @catch (NSException *exception) {
        supportsForceTouch = NO;
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [currentTouches unionSet:touches];
    if (currentTouches.count == 1) {
        [self firstTouchBegan:touches.anyObject withEvent:event];
    } else {
        [self startDragging];
    }
}

- (void)firstTouchBegan:(UITouch *)touch withEvent:(UIEvent *)event {
    CGPoint touchLoc = [touch locationInView:self];
    shouldClick = YES;
    if (!isDragging && (event.timestamp - previousTouchTime < touchTimeThreshold) &&
        fabs(previousTouchLoc.x - touchLoc.x) < touchDistanceThreshold &&
        fabs(previousTouchLoc.y - touchLoc.y) < touchDistanceThreshold) {
        [self startDragging];
    }
    previousTouchTime = event.timestamp;
    previousTouchLoc = touchLoc;
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch = touches.anyObject;
    CGPoint touchLoc = [touch locationInView:self];
    previousTouchLoc = [touch previousLocationInView:self];
    // acceleration
    CGPoint locDiff = CGPointMake(touchLoc.x - previousTouchLoc.x, touchLoc.y - previousTouchLoc.y);
    NSTimeInterval timeDiff = 100 * (event.timestamp - previousTouchTime);
    NSTimeInterval accel = TRACKPAD_ACCEL_N / (TRACKPAD_ACCEL_T + ((timeDiff * timeDiff)/TRACKPAD_ACCEL_D));
    locDiff.x *= accel;
    locDiff.y *= accel;

    if (!CGPointEqualToPoint(touchLoc, previousTouchLoc)) {
        shouldClick = NO;
        [[AppDelegate sharedEmulator] moveMouseX:locDiff.x Y:locDiff.y];
    }
    
    previousTouchTime = event.timestamp;
    previousTouchLoc = touchLoc;
    
    if (supportsForceTouch) {
        [self handleForceClick:touch];
    }
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [currentTouches minusSet:touches];
    if (currentTouches.count > 0) {
        return;
    } else if (didForceClick) {
        AudioServicesPlaySystemSound(1519);
        didForceClick = NO;
        [self cancelScheduledClick];
        [self mouseUp];
        return;
    }
    CGPoint touchLoc = [touches.anyObject locationInView:self];
    if (shouldClick && (event.timestamp - previousTouchTime < touchTimeThreshold)) {
        [self cancelScheduledClick];
        [self performSelector:@selector(mouseClick) withObject:nil afterDelay:touchTimeThreshold];
    }
    shouldClick = NO;
    if (isDragging) {
        [self stopDragging];
    }
    
    previousTouchLoc = touchLoc;
    previousTouchTime = event.timestamp;
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [currentTouches minusSet:touches];
    isDragging = NO;
    shouldClick = NO;
    didForceClick = NO;
    [self mouseUp];
}

- (void)startDragging {
    isDragging = YES;
    shouldClick = NO;
    [[AppDelegate sharedEmulator] setMouseButton:YES];
}

- (void)stopDragging {
    isDragging = NO;
    shouldClick = NO;
    [[AppDelegate sharedEmulator] setMouseButton:NO];
}

- (void)handleForceClick:(UITouch *)touch {
    if (touch.force > 3.0 && !didForceClick) {
        AudioServicesPlaySystemSound(1519);
        didForceClick = YES;
        [self startDragging];
    }
}

- (void)mouseClick {
    if (isDragging) {
        return;
    }
    [[AppDelegate sharedEmulator] setMouseButton:YES];
    [self performSelector:@selector(mouseUp) withObject:nil afterDelay:2.0/60.0];
}

- (void)cancelScheduledClick {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(mouseClick) object:nil];
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(mouseUp) object:nil];
}

- (void)mouseUp {
    [[AppDelegate sharedEmulator] setMouseButton:NO];
}

@end
