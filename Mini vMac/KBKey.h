//
//  KBKey.h
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 16/03/2014.
//  Copyright (c) 2014 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

extern const NSUInteger KBKeyEventStickyKey;

@interface KBKey : UIButton

@property (nonatomic, copy, nullable) NSString *label;
@property (nonatomic, assign) int16_t scancode;
@property (nonatomic, assign) BOOL dark;

@end

@interface KBStickyKey : KBKey
@property (nonatomic, assign) BOOL down;
@end

@interface KBHideKey : KBKey

@end

@interface KBShiftCapsKey : KBStickyKey

@property (nonatomic, assign) BOOL capsLocked;

@end
