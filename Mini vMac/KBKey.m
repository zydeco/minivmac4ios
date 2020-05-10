//
//  KBKey.m
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 16/03/2014.
//  Copyright (c) 2014-2018 namedfork. All rights reserved.
//

#import "KBKey.h"

const NSUInteger KBKeyControlStateCaps = 1 << 16;
const NSUInteger KBKeyEventStickyKey = 1 << 24;

@implementation KBKey

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.dark = NO;
        self.titleLabel.adjustsFontSizeToFitWidth = YES;
        self.titleLabel.lineBreakMode = NSLineBreakByTruncatingTail;
        self.titleLabel.minimumScaleFactor = 0.5;
        self.titleEdgeInsets = UIEdgeInsetsMake(0, 2, 0, 2);
        UIColor *labelColor;
        if (@available(iOS 13.0, *)) {
            labelColor = [UIColor labelColor];
        } else {
            labelColor = [UIColor darkTextColor];
        }
        [self setTitleColor:labelColor forState:UIControlStateNormal];
        self.tintColor = labelColor;
    }
    return self;
}

- (void)awakeFromNib {
    [super awakeFromNib];
    self.dark = NO;
}

- (void)setDark:(BOOL)dark {
    _dark = dark;
    [self setBackgroundImage:[UIImage imageNamed:@"KBKey"] forState:dark ? UIControlStateHighlighted : UIControlStateNormal];
    [self setBackgroundImage:[UIImage imageNamed:@"KBKeyDark"] forState:dark ? UIControlStateNormal : UIControlStateHighlighted];
}

- (void)setLabel:(NSString *)label {
    self.titleLabel.numberOfLines = [label containsString:@"\n"] ? 2 : 1;
    [self setTitle:label forState:UIControlStateNormal];
}

- (NSString *)label {
    return [self titleForState:UIControlStateNormal];
}

- (void)setTitle:(NSString *)title forState:(UIControlState)state {
    if (title.length > 1 && [title hasPrefix:@"@"] && ![title containsString:@"\n"]) {
        [super setTitle:nil forState:state];
        NSArray<NSString *> *components = [[title substringFromIndex:1] componentsSeparatedByString:@"/"];
        if (state == UIControlStateNormal) {
            if (components.count == 3) {
                [super setImage:[UIImage imageNamed:[components[0] stringByAppendingString:components[1]]] forState:UIControlStateNormal];
                [super setImage:[UIImage imageNamed:[components[0] stringByAppendingString:components[2]]] forState:UIControlStateHighlighted];
            } else if (components.count == 1) {
                UIImage *image = [UIImage imageNamed:components.firstObject];
                [super setImage:image forState:UIControlStateNormal];
                [super setImage:image forState:UIControlStateHighlighted];
            } else {
                NSLog(@"Can't set title for %@: %@", self, title);
            }
        } else {
            [super setImage:[UIImage imageNamed:components.firstObject] forState:state];
        }
        self.imageEdgeInsets = UIEdgeInsetsMake(-2, 0, 0, 0);
    } else if ((id)title != [NSNull null]) {
        [super setTitle:title forState:state];
        [super setImage:nil forState:state];
    }
}

- (instancetype)_sameKey {
    __block KBKey *otherKey = nil;
    [self.superview.subviews enumerateObjectsUsingBlock:^(KBKey *obj, NSUInteger idx, BOOL *stop) {
        if (obj != self && [obj isKindOfClass:[self class]] && obj.scancode == self.scancode) {
            otherKey = obj;
            *stop = YES;
        }
    }];
    return otherKey;
}

@end

@implementation KBHideKey

@end

@implementation KBStickyKey {
  @protected
    BOOL _down;
}

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
        [self addGestureRecognizer:tap];
    }
    return self;
}

- (UIControlState)state {
    return [super state] | (_down ? UIControlStateHighlighted : 0);
}

- (void)setDown:(BOOL)down {
    if (_down != down) {
        _down = down;
        [self sendActionsForControlEvents:KBKeyEventStickyKey];
        [self setNeedsLayout];

        KBStickyKey *otherKey = [self _sameKey];
        if (otherKey != nil) {
            otherKey->_down = down;
            [otherKey setNeedsLayout];
        }
    }
}

- (void)tap:(UIGestureRecognizer *)gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateRecognized) {
        self.down ^= YES;
    }
}

@end

@implementation KBShiftCapsKey {
    BOOL wasCapsLocked;
}

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.dark = YES;
        [self setImage:[UIImage imageNamed:@"KBShiftUp"] forState:UIControlStateNormal];
        [self setImage:[UIImage imageNamed:@"KBShiftDown"] forState:UIControlStateHighlighted];
        [self setImage:[UIImage imageNamed:@"KBCapsLock"] forState:KBKeyControlStateCaps];
        [self setImage:[UIImage imageNamed:@"KBCapsLock"] forState:KBKeyControlStateCaps | UIControlStateHighlighted];
        [self setBackgroundImage:[UIImage imageNamed:@"KBKey"] forState:KBKeyControlStateCaps];
        [self setBackgroundImage:[UIImage imageNamed:@"KBKey"] forState:KBKeyControlStateCaps | UIControlStateHighlighted];

        UITapGestureRecognizer *doubleTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(doubleTap:)];
        doubleTap.numberOfTapsRequired = 2;
        [self addGestureRecognizer:doubleTap];
    }
    return self;
}

- (void)setTitle:(NSString *)title forState:(UIControlState)state {
    // nothing here
}

- (void)setDark:(BOOL)dark {
    [super setDark:YES];
}

- (UIControlState)state {
    return [super state] | (_capsLocked ? KBKeyControlStateCaps : 0);
}

- (void)tap:(UIGestureRecognizer *)gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateRecognized) {
        wasCapsLocked = self.capsLocked;
        typeof(self) sameKey = [self _sameKey];
        if (sameKey != nil) {
            sameKey->wasCapsLocked = wasCapsLocked;
            sameKey.capsLocked = NO;
        }
        self.capsLocked = NO;
        if (wasCapsLocked) {
            _down = YES;
            self.down = NO;
        } else {
            self.down = !self.down;
        }
        [[self _sameKey] setNeedsLayout];
    }
}

- (void)doubleTap:(UIGestureRecognizer *)gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateRecognized) {
        self.down = NO;
        self.capsLocked = !wasCapsLocked;
        typeof(self) sameKey = [self _sameKey];
        if (sameKey != nil) {
            sameKey->wasCapsLocked = wasCapsLocked;
            sameKey.capsLocked = self.capsLocked;
        }
        [self sendActionsForControlEvents:KBKeyEventStickyKey];
        [self setNeedsLayout];
        wasCapsLocked = YES;
        [sameKey setNeedsLayout];
    }
}

@end
