//
//  KBKeyboardView.h
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 22/03/2014.
//  Copyright (c) 2014-2018 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "KBKeyboardLayout.h"

#define KC_COMMAND 55
#define KC_SHIFT 56
#define KC_CAPSLOCK 57
#define KC_OPTION 58
#define KC_CONTROL 59

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
@property (nonatomic, strong, nullable) UIMenu *layoutMenu;

@property (nonatomic, readonly, nonnull) NSArray<KBKey*>* keys;
@property (nonatomic, readonly, nonnull) NSArray<KBKey*>* stickyKeys;

- (instancetype _Nonnull)initWithFrame:(CGRect)frame safeAreaInsets:(UIEdgeInsets)safeAreaInsets;
@end
