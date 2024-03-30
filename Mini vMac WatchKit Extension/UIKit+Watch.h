//
//  UIKit+Watch.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 2024-03-27.
//  Copyright © 2024 namedfork. All rights reserved.
//

#ifndef UIKit_Watch_h
#define UIKit_Watch_h

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN


@interface UIResponder : NSObject

@end

@interface UIView : UIResponder
@property(nonatomic,getter=isMultipleTouchEnabled) BOOL multipleTouchEnabled;
@property(nonatomic) CGRect bounds;

- (instancetype)initWithFrame:(CGRect)frame NS_DESIGNATED_INITIALIZER;

@end

@interface UITouch : NSObject

@property(nonatomic,readonly) CGFloat force;

- (CGPoint)locationInView:(nullable UIView *)view;
- (CGPoint)previousLocationInView:(nullable UIView *)view;

@end

@interface UIEvent : NSObject

@property(nonatomic,readonly) NSTimeInterval  timestamp;

@end

@interface UIScreen : NSObject

@property(nonatomic, readonly) CGRect bounds;

+ (instancetype)mainScreen;

@end

NS_ASSUME_NONNULL_END

#endif /* UIKit_h */
