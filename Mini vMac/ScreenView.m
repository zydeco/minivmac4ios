//
//  ScreenView.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 30/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "ScreenView.h"
#import "CNFGGLOB.h"

static ScreenView *sharedScreenView = nil;

@implementation ScreenView
{
    CGImageRef screenImage;
    CGRect screenBounds;
    CGSize screenSize;
}

- (void)awakeFromNib {
    [super awakeFromNib];
    sharedScreenView = self;
    
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

- (void)updateScreen:(CGImageRef)newScreenImage {
    CGImageRelease(screenImage);
    screenImage = CGImageRetain(newScreenImage);
    [self setNeedsDisplay];
}

- (void)layoutSubviews {
    screenSize = CGSizeMake(vMacScreenWidth, vMacScreenHeight);
    CGRect viewBounds = self.bounds;
    CGFloat screenScale = MAX(screenSize.width / viewBounds.size.width, screenSize.height / viewBounds.size.height);
    screenBounds = CGRectMake(0, 0, screenSize.width / screenScale, screenSize.height / screenScale);
    screenBounds.origin.x = (viewBounds.size.width - screenBounds.size.width)/2;
    screenBounds = CGRectIntegral(screenBounds);
    [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect {
    // Draw screenImage
    CGImageRef imageRef = CGImageRetain(screenImage);
    if (imageRef) {
        CGContextRef ctx = UIGraphicsGetCurrentContext();
        CGContextTranslateCTM(ctx, 0, screenBounds.size.height);
        CGContextScaleCTM(ctx, 1.0, -1.0);
        CGContextDrawImage(ctx, screenBounds, imageRef);
        CGImageRelease(imageRef);
    }
}

@end
