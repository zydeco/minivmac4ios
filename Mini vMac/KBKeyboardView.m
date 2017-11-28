//
//  KBKeyboardView.m
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 22/03/2014.
//  Copyright (c) 2014-2017 namedfork. All rights reserved.
//

#import "KBKeyboardView.h"
#import "KBKey.h"

#define KC_COMMAND 55
#define KC_SHIFT 56
#define KC_CAPSLOCK 57
#define KC_OPTION 58
#define KC_CONTROL 59

@implementation KBKeyboardView {
    NSMutableArray *keyPlanes;
    NSMutableSet *modifiers;
    NSMutableIndexSet *keysDown;
    CGAffineTransform defaultKeyTransform;
    CGFloat fontSize;
    CGSize selectedSize;
    UIEdgeInsets safeAreaInsets;
}

- (instancetype)initWithFrame:(CGRect)frame safeAreaInsets:(UIEdgeInsets)insets {
    self = [super initWithFrame:frame];
    if (self) {
        safeAreaInsets = insets;
        self.backgroundColor = [UIColor colorWithRed:0xEB / 255.0 green:0xF0 / 255.0 blue:0xF7 / 255.0 alpha:0.9];
        modifiers = [NSMutableSet setWithCapacity:4];
        keysDown = [NSMutableIndexSet indexSet];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
    return [self initWithFrame:frame safeAreaInsets:UIEdgeInsetsZero];
}

- (void)setLayout:(KBKeyboardLayout *)layout {
    if ([_layout isEqual:layout]) {
        return;
    }
    _layout = layout;

    // find preferred size (same width or smaller)
    CGRect safeFrame = UIEdgeInsetsInsetRect(self.frame, safeAreaInsets);
    CGFloat frameWidth = safeFrame.size.width;
    CGFloat preferredWidth = frameWidth;
    selectedSize = CGSizeZero;
    for (NSValue *key in layout.availableSizes) {
        CGSize size = key.CGSizeValue;
        if (!CGSizeEqualToSize(size, CGSizeZero) && size.width > selectedSize.width && size.width <= preferredWidth) {
            selectedSize = size;
        }
    }

    // try sideways
    if (CGSizeEqualToSize(selectedSize, CGSizeZero)) {
        preferredWidth = safeFrame.size.height;
        for (NSValue *key in layout.availableSizes) {
            CGSize size = key.CGSizeValue;
            if (!CGSizeEqualToSize(size, CGSizeZero) && size.width > selectedSize.width && size.width <= preferredWidth) {
                selectedSize = size;
            }
        }
    }
    
    // still not found
    if (CGSizeEqualToSize(selectedSize, CGSizeZero)) {
        return;
    }
    
    defaultKeyTransform = CGAffineTransformIdentity;
    fontSize = [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone ? 22.0 : 30.0;
    if (preferredWidth != selectedSize.width || preferredWidth != frameWidth) {
        // adjust width
        if (frameWidth / selectedSize.width > 2 || frameWidth / selectedSize.width < 0.5) {
            // iphone keyboard on ipad?
            defaultKeyTransform = CGAffineTransformMakeScale(frameWidth / selectedSize.width, frameWidth / selectedSize.width);
        } else if (frameWidth < selectedSize.width) {
            // shrink keyboard
            defaultKeyTransform = CGAffineTransformMakeScale(frameWidth / selectedSize.width, 1.33333);
        } else {
            // iPhone keyboard on bigger phone
            CGFloat wScale = safeFrame.size.width / selectedSize.width;
            defaultKeyTransform = CGAffineTransformMakeScale(wScale, 1.0);
        }
    }
    self.frame = CGRectMake(0, 0, safeFrame.size.width + safeAreaInsets.right + safeAreaInsets.left, selectedSize.height * defaultKeyTransform.d + safeAreaInsets.bottom);

    // init keyplanes array
    NSUInteger numberOfKeyPlanes = layout.numberOfKeyPlanes;
    keyPlanes = [NSMutableArray arrayWithCapacity:numberOfKeyPlanes];
    for (int i = 0; i < numberOfKeyPlanes; i++) {
        [keyPlanes addObject:[NSNull null]];
    }
    
    [self switchToKeyPlane:0];
}

- (NSArray *)_loadKeyPlane:(NSUInteger)plane {
    NSMutableArray *keyPlane = [NSMutableArray arrayWithCapacity:64];
    [_layout enumerateKeysForSize:selectedSize plane:plane transform:defaultKeyTransform usingBlock:^(int8_t scancode, CGRect keyFrame, CGFloat fontScale, BOOL dark, BOOL sticky) {
        KBKey *key = nil;
        keyFrame.origin.x += safeAreaInsets.left;
        if (scancode == VKC_HIDE) {
            key = [[KBHideKey alloc] initWithFrame:keyFrame];
            [key addTarget:self action:@selector(hideKeyboard:) forControlEvents:UIControlEventTouchUpInside];
        } else if (scancode == VKC_SHIFT_CAPS) {
            key = [[KBShiftCapsKey alloc] initWithFrame:keyFrame];
            key.scancode = KC_SHIFT;
            [key addTarget:self action:@selector(capsKey:) forControlEvents:KBKeyEventStickyKey];
        } else if (sticky) {
            key = [[KBStickyKey alloc] initWithFrame:keyFrame];
            key.scancode = scancode;
            [key addTarget:self action:@selector(stickyKey:) forControlEvents:KBKeyEventStickyKey];
        } else {
            key = [[KBKey alloc] initWithFrame:keyFrame];
            key.scancode = scancode;
            [key addTarget:self action:@selector(keyDown:) forControlEvents:UIControlEventTouchDown];
            [key addTarget:self action:@selector(keyUp:) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchDragExit | UIControlEventTouchCancel];
        }
        key.dark = dark;
        NSString *label = [_layout labelForScanCode:scancode];
        if ([label containsString:@"\n"]) {
            fontScale *= [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone ? 0.6 : 0.65;
        }
        key.label = label;
        key.titleLabel.font = [UIFont systemFontOfSize:fontSize * fontScale weight:&UIFontWeightRegular ? UIFontWeightRegular : 1.0];
        [keyPlane addObject:key];
    }];
    return keyPlane;
}

- (NSArray<KBKey *> *)keys {
    return [self.subviews filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self isKindOfClass: %@", [KBKey class]]];
}

- (NSArray<KBKey *> *)stickyKeys {
    return [self.subviews filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"self isKindOfClass: %@", [KBStickyKey class]]];
}

- (void)switchToKeyPlane:(NSInteger)idx {
    if (idx < 0 || idx >= keyPlanes.count) {
        return;
    }

    // remove previous keys
    [self.keys makeObjectsPerformSelector:@selector(removeFromSuperview)];

    // load keyplane
    if (keyPlanes[idx] == [NSNull null]) {
        keyPlanes[idx] = [self _loadKeyPlane:idx];
    }
    
    // add keys
    for (KBKey *key in keyPlanes[idx]) {
        if ([key isKindOfClass:[KBStickyKey class]]) {
            // match modifiers
            KBStickyKey *stickyKey = (KBStickyKey *)key;
            stickyKey.down = [keysDown containsIndex:key.scancode];
            if ([key isKindOfClass:[KBShiftCapsKey class]]) {
                KBShiftCapsKey *shiftCapsKey = (KBShiftCapsKey *)key;
                shiftCapsKey.capsLocked = [keysDown containsIndex:KC_CAPSLOCK];
            }
        }
        [self addSubview:key];
    }
}

- (void)keyDown:(KBKey *)key {
    int16_t scancode = key.scancode;
    if (scancode >= 0 && ![keysDown containsIndex:scancode]) {
        [keysDown addIndex:scancode];
        [self.delegate keyDown:scancode];
    } else if (scancode < 0) {
        [self switchToKeyPlane:(-scancode) - 1];
    }
}

- (void)keyUp:(KBKey *)key {
    int16_t scancode = key.scancode;
    if (scancode >= 0 && [keysDown containsIndex:scancode]) {
        [keysDown removeIndex:scancode];
        [self.delegate keyUp:scancode];
        // up sticky keys
        [self.stickyKeys setValue:@NO forKey:@"down"];
    }
}

- (void)hideKeyboard:(KBHideKey *)key {
    if ([self.delegate respondsToSelector:@selector(hideKeyboard:)]) {
        [self.delegate hideKeyboard:key];
    } else {
        self.hidden = YES;
    }
}

- (void)stickyKey:(KBStickyKey *)key {
    if (key.down && ![keysDown containsIndex:key.scancode]) {
        [keysDown addIndex:key.scancode];
        [self.delegate keyDown:key.scancode];
    } else if (!key.down && [keysDown containsIndex:key.scancode]) {
        [keysDown removeIndex:key.scancode];
        [self.delegate keyUp:key.scancode];
    }
}

- (void)capsKey:(KBShiftCapsKey *)key {
    if (key.capsLocked && ![keysDown containsIndex:KC_CAPSLOCK]) {
        [keysDown addIndex:KC_CAPSLOCK];
        [self.delegate keyDown:KC_CAPSLOCK];
    } else if ([keysDown containsIndex:KC_CAPSLOCK]) {
        [keysDown removeIndex:KC_CAPSLOCK];
        [self.delegate keyUp:KC_CAPSLOCK];
    } else {
        [self stickyKey:key];
    }
}

@end
