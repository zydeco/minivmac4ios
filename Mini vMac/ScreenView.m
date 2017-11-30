//
//  ScreenView.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 30/04/2016.
//  Copyright © 2016-2017 namedfork. All rights reserved.
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
    [AppDelegate sharedEmulator].screenLayer = videoLayer;
    if ([AppDelegate sharedEmulator]) {
        screenSize = [AppDelegate sharedEmulator].screenSize;
    } else {
        screenSize = CGSizeMake(1, 1);
    }
    [self.layer addSublayer:videoLayer];
    [[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"screenFilter" options:NSKeyValueObservingOptionNew context:NULL];
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
    if (screenScale > 0.9 && screenScale <= 1.0) {
        screenScale = 1.0;
    }
    screenBounds = CGRectMake(0, 0, screenSize.width / screenScale, screenSize.height / screenScale);
    screenBounds.origin.x = (viewBounds.size.width - screenBounds.size.width)/2;
    screenBounds = CGRectIntegral(screenBounds);
    videoLayer.frame = screenBounds;
    BOOL scaleIsIntegral = (floor(screenScale) == screenScale);
    NSString *screenFilter = scaleIsIntegral ? kCAFilterNearest : [[NSUserDefaults standardUserDefaults] stringForKey:@"screenFilter"];
    videoLayer.magnificationFilter = screenFilter;
    videoLayer.minificationFilter = screenFilter;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if ([object isEqual:[NSUserDefaults standardUserDefaults]]) {
        if ([keyPath isEqualToString:@"screenFilter"]) {
            NSString *value = change[NSKeyValueChangeNewKey];
            videoLayer.magnificationFilter = value;
            videoLayer.minificationFilter = value;
        }
    }
}

- (void)dealloc {
    [[NSUserDefaults standardUserDefaults] removeObserver:self forKeyPath:@"screenFilter" context:NULL];
}
@end
