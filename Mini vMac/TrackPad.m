//
//  TrackPad.m
//  Mini vMac for iOS
//
//  Created by Jesús A. Álvarez on 18/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "TrackPad.h"
#import "AppDelegate.h"

#define TRACKPAD_ACCEL_N 1
#define TRACKPAD_ACCEL_T 0.2
#define TRACKPAD_ACCEL_D 20

@implementation TrackPad
{
    NSTimeInterval touchTimeThreshold;
    NSTimeInterval previousClickTime, previousTouchTime;
    CGFloat touchDistanceThreshold;
    CGPoint previousTouchLoc;
    BOOL click, drag;
    NSMutableSet *currentTouches;
}

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        touchTimeThreshold = 0.25;
        touchDistanceThreshold = 16;
        currentTouches = [NSMutableSet setWithCapacity:4];
    }
    return self;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    click = YES;
    CGPoint touchLoc = [touches.anyObject locationInView:self];
    if ((event.timestamp - previousTouchTime < touchTimeThreshold) &&
        fabs(previousTouchLoc.x - touchLoc.x) < touchDistanceThreshold &&
        fabs(previousTouchLoc.y - touchLoc.y) < touchDistanceThreshold) {
        drag = YES;
        [[AppDelegate sharedEmulator] setMouseButton:YES];
    }
    previousTouchTime = event.timestamp;
    previousTouchLoc = touchLoc;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    CGPoint touchLoc = [touches.anyObject locationInView:self];
    CGPoint locDiff = touchLoc;
    locDiff.x -= previousTouchLoc.x;
    locDiff.y -= previousTouchLoc.y;
    // acceleration
    NSTimeInterval timeDiff = 100 * (event.timestamp - previousTouchTime);
    NSTimeInterval accel = TRACKPAD_ACCEL_N / (TRACKPAD_ACCEL_T + ((timeDiff * timeDiff)/TRACKPAD_ACCEL_D));
    locDiff.x *= accel;
    locDiff.y *= accel;
    click = NO;
    [[AppDelegate sharedEmulator] moveMouseX:locDiff.x Y:locDiff.y];
    previousTouchTime = event.timestamp;
    previousTouchLoc = touchLoc;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    CGPoint touchLoc = [touches.anyObject locationInView:self];
    if (click && (event.timestamp - previousTouchTime < touchTimeThreshold)) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(mouseClick) object:nil];
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(mouseUp) object:nil];
        [self performSelector:@selector(mouseClick) withObject:nil afterDelay:touchTimeThreshold];
    }
    click = NO;
    if (drag) {
        [[AppDelegate sharedEmulator] setMouseButton:NO];
        drag = NO;
    }
    
    previousTouchLoc = touchLoc;
    previousTouchTime = event.timestamp;
}

- (void)mouseClick {
    if (drag) {
        return;
    }
    [[AppDelegate sharedEmulator] setMouseButton:YES];
    [self performSelector:@selector(mouseUp) withObject:nil afterDelay:2.0/60.0];
}

- (void)mouseUp {
    [[AppDelegate sharedEmulator] setMouseButton:NO];
}

@end
