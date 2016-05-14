//
//  KBKeyboardLayout.h
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 09/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

#define VKC_SHIFT_CAPS -127
#define VKC_HIDE -128

@interface KBKeyboardLayout : NSObject

@property (nonatomic, readonly) NSString *identifier;
@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly) uint8_t type;
@property (nonatomic, readonly) NSUInteger numberOfKeyPlanes;
@property (nonatomic, readonly) NSArray<NSValue*> *availableSizes;

- (instancetype)initWithContentsOfFile:(NSString*)path;
- (NSString*)labelForScanCode:(NSInteger)scanCode;
- (BOOL)enumerateKeysForSize:(CGSize)size plane:(NSUInteger)plane transform:(CGAffineTransform)transform usingBlock:(void(^)(int8_t scancode, CGRect frame, CGFloat fontScale, BOOL dark, BOOL sticky))block;

@end
