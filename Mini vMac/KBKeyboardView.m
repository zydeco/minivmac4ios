//
//  KBKeyboardView.m
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 22/03/2014.
//  Copyright (c) 2014-2018 namedfork. All rights reserved.
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
        if (@available(iOS 13.0, *)) {
            self.backgroundColor = [UIColor clearColor];
            UIVisualEffectView *backgroundView = [[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThickMaterial]];
            backgroundView.frame = self.bounds;
            [self addSubview: backgroundView];
        } else {
            self.backgroundColor = [UIColor colorWithRed:0xEB / 255.0 green:0xF0 / 255.0 blue:0xF7 / 255.0 alpha:0.9];
        }
        modifiers = [NSMutableSet setWithCapacity:4];
        keysDown = [NSMutableIndexSet indexSet];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame {
    return [self initWithFrame:frame safeAreaInsets:UIEdgeInsetsZero];
}

- (BOOL)isCompactKeyboardSize:(CGSize)size {
    return size.width < 768.0;
}

- (CGSize)findBestSizeForWidth:(CGFloat)preferredWidth inArray:(NSArray<NSValue*>*)sizes {
    CGSize selectedSize = CGSizeZero;
    for (NSValue *key in sizes) {
        CGSize size = key.CGSizeValue;
        if (size.width > selectedSize.width && size.width <= preferredWidth) {
            selectedSize = size;
            if (size.width == preferredWidth) {
                // exact match
                break;
            }
        }
    }

    // still not found, use smallest width
    if (CGSizeEqualToSize(selectedSize, CGSizeZero)) {
        selectedSize = sizes.firstObject.CGSizeValue;
    }
    
    return selectedSize;
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
    
    // find best size in class
    BOOL isCompactSize = self.traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassCompact;
    NSArray<NSValue*> *sizes = [[layout.availableSizes filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSValue *size, NSDictionary<NSString *,id> * _Nullable bindings) {
        return [self isCompactKeyboardSize:size.CGSizeValue] == isCompactSize;
    }]] sortedArrayUsingComparator:^NSComparisonResult(NSValue * _Nonnull size1, NSValue * _Nonnull size2) {
        return size1.CGSizeValue.width - size2.CGSizeValue.width;
    }];
    selectedSize = [self findBestSizeForWidth:preferredWidth inArray:sizes];
    
    defaultKeyTransform = CGAffineTransformIdentity;
    fontSize = [self isCompactKeyboardSize:selectedSize] ? 22.0 : 30.0;
    if (preferredWidth != selectedSize.width || preferredWidth != frameWidth) {
        // adjust width
        if (frameWidth / selectedSize.width > 2 || frameWidth / selectedSize.width < 0.5) {
            // iphone keyboard on ipad?
            defaultKeyTransform = CGAffineTransformMakeScale(frameWidth / selectedSize.width, frameWidth / selectedSize.width);
        } else if (frameWidth < selectedSize.width) {
            // shrink keyboard
            defaultKeyTransform = CGAffineTransformMakeScale(frameWidth / selectedSize.width, 1.0);
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

- (void)_addKeyWithFrame:(CGRect)keyFrame scanCode:(int8_t)scancode fontScale:(CGFloat)fontScale dark:(BOOL)dark sticky:(BOOL)sticky toKeyPlane:(NSMutableArray*)keyPlane {
    KBKey *key = nil;
    if (@available(iOS 11, *)) {
        keyFrame.origin.x += safeAreaInsets.left;
    }
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
        fontScale *= [self isCompactKeyboardSize:selectedSize] ? 0.6 : 0.65;
    }
    key.label = label;
    key.titleLabel.font = [UIFont systemFontOfSize:self->fontSize * fontScale weight:&UIFontWeightRegular ? UIFontWeightRegular : 1.0];
    [keyPlane addObject:key];
}

- (NSArray *)_loadKeyPlane:(NSUInteger)plane {
    NSMutableArray *keyPlane = [NSMutableArray arrayWithCapacity:64];
    [_layout enumerateKeysForSize:selectedSize plane:plane transform:defaultKeyTransform usingBlock:^(int8_t scancode, CGRect keyFrame, CGFloat fontScale, BOOL dark, BOOL sticky) {
        [self _addKeyWithFrame:keyFrame scanCode:scancode fontScale:fontScale dark:dark sticky:sticky toKeyPlane:keyPlane];
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
