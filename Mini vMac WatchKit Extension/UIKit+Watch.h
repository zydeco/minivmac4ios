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

typedef NSString *CALayerContentsFilter;
typedef NSString *CALayerContentsGravity;

@interface CALayer : NSObject

@property(strong) id contents;
@property CGRect contentsRect;
@property CGRect contentsCenter;
@property(copy) CALayerContentsFilter minificationFilter;
@property(copy) CALayerContentsGravity contentsGravity;
@property float opacity;
@property BOOL hidden;
@property CGRect bounds;
@property CGRect frame;
@property CGPoint position;
@property CGPoint anchorPoint;
@property BOOL masksToBounds;

+ (instancetype)layer;
- (void)addSublayer:(CALayer *)layer;
- (void)setAffineTransform:(CGAffineTransform)m;

@end

@interface UIResponder : NSObject

@end

@interface UIView : UIResponder
@property(nonatomic,getter=isMultipleTouchEnabled) BOOL multipleTouchEnabled;
@property(nonatomic) CGRect bounds;
@property(nullable, nonatomic, copy) UIColor *backgroundColor;
@property(nullable, readonly, unsafe_unretained) UIView *superview;
@property(nonatomic, readonly, copy) NSArray<__kindof UIView *> *subviews;
@property(nonatomic, readonly, strong) CALayer *layer;
@property(nonatomic) CGPoint center;
@property(nonatomic) BOOL clipsToBounds;

- (instancetype)initWithFrame:(CGRect)frame NS_DESIGNATED_INITIALIZER;
- (void)addSubview:(UIView *)subview;

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

@interface UIViewController : UIResponder

@property(nonatomic, strong) UIView *view;

-(BOOL)prefersStatusBarHidden;

@end

@interface UIWindow : UIView

@property(nullable, nonatomic,strong) UIViewController *rootViewController;

@end

@interface UIApplication : NSObject

@property(class, nonatomic, readonly) UIApplication *sharedApplication;
@property(nullable, nonatomic,readonly) UIWindow *keyWindow;

@end

@interface PUICApplication : UIApplication

+(instancetype)sharedPUICApplication;
-(void)_setStatusBarTimeHidden:(BOOL)hidden animated:(BOOL)animated completion:(void (^_Nullable)(void))completion;

@end

NS_ASSUME_NONNULL_END

#endif /* UIKit_h */
