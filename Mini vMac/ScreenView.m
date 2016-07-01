//
//  ScreenView.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 30/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "ScreenView.h"
#import "AppDelegate.h"

static ScreenView *sharedScreenView = nil;

@implementation ScreenView
{
    CGImageRef screenImage;
    CGRect screenBounds;
    CGSize screenSize;
    CALayer *videoLayer;
}

- (void)awakeFromNib {
    [super awakeFromNib];
    sharedScreenView = self;
    videoLayer = [CALayer layer];
    [AppDelegate sharedEmulator].screenLayer = videoLayer;
    if ([AppDelegate sharedEmulator]) {
        screenSize = [AppDelegate sharedEmulator].screenSize;
    } else {
        screenSize = CGSizeMake(1, 1);
    }
    [self.layer addSublayer:videoLayer];
}

+ (instancetype)sharedScreenView {
    return sharedScreenView;
}

- (CGRect)screenBounds {
    return screenBounds;
}

- (CGSize)screenSize {
    return screenSize;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect viewBounds = self.bounds;
    CGFloat screenScale = MAX(screenSize.width / viewBounds.size.width, screenSize.height / viewBounds.size.height);
    screenBounds = CGRectMake(0, 0, screenSize.width / screenScale, screenSize.height / screenScale);
    screenBounds.origin.x = (viewBounds.size.width - screenBounds.size.width)/2;
    screenBounds = CGRectIntegral(screenBounds);
    videoLayer.frame = screenBounds;
}

@end
