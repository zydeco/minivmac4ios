//
//  KBKeyboardView.h
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 22/03/2014.
//  Copyright (c) 2014-2017 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "KBKeyboardLayout.h"

@class KBKey;

@protocol KBKeyboardViewDelegate <NSObject>
- (void)keyDown:(int)scancode;
- (void)keyUp:(int)scancode;
@optional
- (void)hideKeyboard:(nullable id)sender;
@end

@interface KBKeyboardView : UIView

@property (weak, nonatomic, nullable) id<KBKeyboardViewDelegate> delegate;
@property (nonatomic, strong, nullable) KBKeyboardLayout *layout;

@property (nonatomic, readonly, nonnull) NSArray<KBKey*>* keys;
@property (nonatomic, readonly, nonnull) NSArray<KBKey*>* stickyKeys;

- (instancetype)initWithFrame:(CGRect)frame safeAreaInsets:(UIEdgeInsets)safeAreaInsets;
@end
