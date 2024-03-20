//
//  ScreenView.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 30/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
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
    NSString *screenFilter = [[NSUserDefaults standardUserDefaults] stringForKey:@"screenFilter"];
    videoLayer.magnificationFilter = screenFilter;
    videoLayer.minificationFilter = screenFilter;
    [self updateVideoLayer];
    [self.layer addSublayer:videoLayer];
    [[AppDelegate sharedInstance] addObserver:self forKeyPath:@"sharedEmulator" options:NSKeyValueObservingOptionNew context:NULL];
    [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"screenFilter" options:NSKeyValueObservingOptionNew|NSKeyValueObservingOptionOld context:NULL];
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

- (void)updateVideoLayer {
    videoLayer.contents = nil;
    if ([AppDelegate sharedEmulator]) {
        [AppDelegate sharedEmulator].screenLayer = videoLayer;
        screenSize = [AppDelegate sharedEmulator].screenSize;
    } else {
        screenSize = CGSizeMake(1, 1);
    }
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect viewBounds = self.bounds;
    CGFloat screenScale = MIN(viewBounds.size.width / screenSize.width, viewBounds.size.height / screenSize.height);
    NSString *screenFilter = [[NSUserDefaults standardUserDefaults] stringForKey:@"screenFilter"];
    if ([screenFilter isEqualToString:kCAFilterNearest] && screenScale > 1.0) {
        screenScale = floor(screenScale);
    } else if (screenScale > 1.0 && screenScale <= 1.1) {
        screenScale = 1.0;
    }
    screenBounds = CGRectMake(0, 0, screenSize.width * screenScale, screenSize.height * screenScale);
    screenBounds.origin.x = (viewBounds.size.width - screenBounds.size.width)/2;
    screenBounds = CGRectIntegral(screenBounds);

    if ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad && (viewBounds.size.height - screenBounds.size.height) >= 30.0) {
        // move under multitasking indicator on iPad
        screenBounds.origin.y += 30;
    }
    videoLayer.frame = screenBounds;
    screenBounds.origin.y += self.frame.origin.y;
    BOOL scaleIsIntegral = (floor(screenScale) == screenScale);
    if (scaleIsIntegral) screenFilter = kCAFilterNearest;
    videoLayer.magnificationFilter = screenFilter;
    videoLayer.minificationFilter = screenFilter;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if ([object isEqual:[NSUserDefaults standardUserDefaults]]) {
        if ([keyPath isEqualToString:@"screenFilter"]) {
            NSString *oldValue = change[NSKeyValueChangeOldKey];
            NSString *value = change[NSKeyValueChangeNewKey];
            videoLayer.magnificationFilter = value;
            videoLayer.minificationFilter = value;
            if ([value isEqualToString:kCAFilterNearest] || [oldValue isEqualToString:kCAFilterNearest]) {
                [self setNeedsLayout];
                [self layoutIfNeeded];
            }
        }
    } else if (object == [AppDelegate sharedInstance] && [keyPath isEqualToString:@"sharedEmulator"]) {
        [self updateVideoLayer];
        [self setNeedsLayout];
        [self layoutIfNeeded];
    }
}

- (void)dealloc {
    [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:@"screenFilter" context:NULL];
}

@end
