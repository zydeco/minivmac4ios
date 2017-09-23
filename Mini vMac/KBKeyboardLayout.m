//
//  KBKeyboardLayout.m
//  BasiliskII
//
//  Created by Jesús A. Álvarez on 09/04/2016.
//  Copyright © 2016-2017 namedfork. All rights reserved.
//

#import "KBKeyboardLayout.h"

#if defined(CGFLOAT_IS_DOUBLE) && CGFLOAT_IS_DOUBLE
#define CGFloatValue doubleValue
#else
#define CGFloatValue floatValue
#endif

@interface KBKeyDescriptor : NSObject
{
    @public
    CGRect frame;
    CGFloat fontScale;
    int8_t scancode;
    BOOL dark, sticky;
}
@end

@interface KBKeyboardLayoutBinaryReader : NSObject
{
    NSData *data;
    const void *ptr;
}

@property (nonatomic, assign) NSUInteger position;

+ (BOOL)validBinaryHeader:(NSData*)data;
- (instancetype)initWithData:(NSData*)layoutData;
- (NSString*)readString;
- (uint8_t)readUInt8;
- (int8_t)readInt8;
- (uint16_t)readUInt16;
- (int16_t)readInt16;
- (uint32_t)readUInt32;
- (CGFloat)readFloat;
- (CGRect)readFrameOfSize:(NSUInteger)size withLastFrame:(CGRect)lastFrame;
- (NSArray<NSString*>*)readStrings:(NSUInteger)count emptyStringMarker:(id)emptyStringMarker;

@end

typedef NSArray<KBKeyDescriptor*> KBKeyPlane;
typedef NSMutableArray<KBKeyDescriptor*> KBMutableKeyPlane;
typedef NSArray<KBKeyPlane*> KBKeyMap;
typedef NSMutableArray<KBKeyPlane*> KBMutableKeyMap;

@implementation KBKeyboardLayout
{
    NSMutableDictionary<NSNumber*,NSString*> *labels;
    NSArray<NSString*> *keyPlaneLabels;
    NSMutableDictionary<NSValue*, KBKeyMap*> *keyMaps;
}

- (instancetype)initWithContentsOfFile:(NSString *)path {
    NSData *layoutData = [NSData dataWithContentsOfFile:path];
    if ([KBKeyboardLayoutBinaryReader validBinaryHeader:layoutData]) {
        return [self initWithBinaryRepresentation:layoutData];
    } else if (layoutData.length > 1 && memcmp(layoutData.bytes, "{", 1) == 0) {
        // possibly JSON
        NSError *err = nil;
        NSDictionary *layoutDictionary = [NSJSONSerialization JSONObjectWithData:layoutData options:0 error:&err];
        return [self initWithDictionaryRepresentation:layoutDictionary];
    }
    return nil;
}

- (NSUInteger)numberOfKeyPlanes {
    return keyPlaneLabels.count;
}

- (NSArray<NSValue *> *)availableSizes {
    return keyMaps.allKeys;
}

- (NSString *)labelForScanCode:(NSInteger)scanCode {
    if (scanCode == VKC_HIDE) {
        return @"@KBHide";
    } else if (scanCode < 0 && -scanCode <= keyPlaneLabels.count) {
        // switch keyplane
        return keyPlaneLabels[-scanCode-1];
    }
    return labels[@(scanCode)];
}

- (BOOL)enumerateKeysForSize:(CGSize)size plane:(NSUInteger)plane transform:(CGAffineTransform)transform usingBlock:(void (^)(int8_t, CGRect, CGFloat, BOOL, BOOL))block {
    NSValue *sizeValue = [NSValue valueWithCGSize:size];
    KBKeyPlane *keyPlane = keyMaps[sizeValue][plane];
    for (KBKeyDescriptor *keyDescriptor in keyPlane) {
        block(keyDescriptor->scancode, CGRectApplyAffineTransform(keyDescriptor->frame, transform), keyDescriptor->fontScale, keyDescriptor->dark, keyDescriptor->sticky);
    }
    return keyPlane != nil;
}

#pragma mark - Binary Reading

- (instancetype)initWithBinaryRepresentation:(NSData *)layoutData {
    if (![KBKeyboardLayoutBinaryReader validBinaryHeader:layoutData]) {
        return nil;
    }
    if ((self = [super init])) {
        KBKeyboardLayoutBinaryReader *reader = [[KBKeyboardLayoutBinaryReader alloc] initWithData:layoutData];
        
        // read header
        _identifier = [reader readString];
        _name = [reader readString];
        _type = [reader readUInt8];
        
        // read 128 labels
        labels = [NSMutableDictionary dictionaryWithCapacity:128];
        for (int scancode=0; scancode < 128; scancode++) {
            NSString *label = [reader readString];
            if (label != nil) {
                labels[@(scancode)] = label;
            }
        }
        
        // read keyplane names
        NSUInteger numberOfKeyPlanes = [reader readUInt8];
        keyPlaneLabels = [reader readStrings:numberOfKeyPlanes emptyStringMarker:@""];
        
        // read map index
        NSUInteger numberOfMaps = [reader readUInt8];
        NSMutableDictionary<NSNumber*,NSNumber*> *mapIndex = [NSMutableDictionary dictionaryWithCapacity:numberOfMaps];
        NSMutableArray<NSNumber*> *mapIDs = [NSMutableArray arrayWithCapacity:numberOfMaps];
        for (NSUInteger i=0; i < numberOfMaps; i++) {
            uint32_t mapID = [reader readUInt32];
            uint16_t mapOffset = [reader readUInt16];
            mapIndex[@(mapID)] = @(mapOffset);
            [mapIDs addObject:@(mapID)];
        }
        
        // read maps
        keyMaps = [NSMutableDictionary dictionaryWithCapacity:numberOfMaps];
        NSMutableDictionary<NSNumber*,KBKeyMap*> *mapsByID = [NSMutableDictionary dictionaryWithCapacity:numberOfMaps];
        for (NSNumber *mapID in mapIDs) {
            CGSize mapSize = CGSizeMake((mapID.integerValue >> 16), (mapID.integerValue & 0x7FFF));
            reader.position = mapIndex[mapID].integerValue;
            // read keyplanes
            KBMutableKeyMap *keyMap = [KBMutableKeyMap arrayWithCapacity:numberOfKeyPlanes];
            for (NSUInteger planeNumber = 0; planeNumber < numberOfKeyPlanes; planeNumber++) {
                NSUInteger numberOfKeys = [reader readUInt8];
                if (numberOfKeys == 0) {
                    [keyMap addObject:@[]];
                    continue;
                }
                // read keys
                KBMutableKeyPlane *keyPlane = [KBMutableKeyPlane arrayWithCapacity:numberOfKeys];
                CGRect lastFrame = CGRectZero;
                for (NSUInteger keyNumber = 0; keyNumber < numberOfKeys; keyNumber++) {
                    uint8_t flags = [reader readUInt8];
                    if ((flags & 0xC0) == 0x00) {
                        // key
                        KBKeyDescriptor *key = [KBKeyDescriptor new];
                        key->dark = (flags & 0x20) == 0x20;
                        key->sticky = (flags & 0x10) == 0x10;
                        key->fontScale = (((flags & 0x0C) >> 2) + 1) * 0.25;
                        key->scancode = [reader readInt8];
                        key->frame = [reader readFrameOfSize:(flags & 0x03) + 1 withLastFrame:lastFrame];
                        lastFrame = key->frame;
                        
                        [keyPlane addObject:key];
                    } else if ((flags & 0xC0) == 0x40) {
                        // include
                        NSUInteger includePlaneNumber = flags & 0x0F;
                        uint32_t includeMapNumber = [reader readUInt32];
                        CGAffineTransform includeTransform = CGAffineTransformIdentity;
                        if ((flags & 0x20) == 0x20) {
                            CGFloat scaleX = [reader readFloat];
                            CGFloat scaleY = [reader readFloat];
                            includeTransform = CGAffineTransformScale(includeTransform, scaleX, scaleY);
                        }
                        if ((flags & 0x10) == 0x10) {
                            CGFloat translateX = [reader readInt16];
                            CGFloat translateY = [reader readInt16];
                            includeTransform = CGAffineTransformTranslate(includeTransform, translateX, translateY);
                        }
                        NSUInteger numberOfSkips = [reader readUInt8];
                        NSMutableSet<NSNumber*> *skipKeys = [NSMutableSet setWithCapacity:numberOfSkips];
                        for (NSUInteger i=0; i < numberOfSkips; i++) {
                            [skipKeys addObject:@([reader readInt8])];
                        }
                        
                        KBKeyMap *includeKeyMap = mapsByID[@(includeMapNumber)];
                        if (includeKeyMap.count < includePlaneNumber) {
                            continue;
                        }
                        KBKeyPlane *includeKeyPlane = includeKeyMap[includePlaneNumber];
                        [includeKeyPlane enumerateObjectsUsingBlock:^(KBKeyDescriptor * _Nonnull oldKey, NSUInteger idx, BOOL * _Nonnull stop) {
                            if ([skipKeys containsObject:@(oldKey->scancode)]) {
                                return;
                            }
                            KBKeyDescriptor *key = [KBKeyDescriptor new];
                            key->dark = oldKey->dark;
                            key->sticky = oldKey->sticky;
                            key->frame = CGRectIntegral(CGRectApplyAffineTransform(oldKey->frame, includeTransform));
                            key->fontScale = oldKey->fontScale;
                            key->scancode = oldKey->scancode;
                            [keyPlane addObject:key];
                        }];
                    }
                }
                [keyMap addObject:keyPlane];
            }
            mapsByID[mapID] = keyMap;
            if ((mapID.integerValue & 0x7FFF0000) != 0) {
                // valid size
                keyMaps[[NSValue valueWithCGSize:mapSize]] = keyMap;
            }
        }
    }
    return self;
}

#pragma mark - Dictionary Reading

- (instancetype)initWithDictionaryRepresentation:(NSDictionary *)layoutDictionary {
    if ((self = [super init])) {
        _identifier = layoutDictionary[@"id"];
        _name = layoutDictionary[@"name"];
        _type = (uint8_t)[layoutDictionary[@"adbType"] integerValue];
        
        // read labels
        NSDictionary<NSString*,NSString*> *layoutLabels = layoutDictionary[@"labels"];
        labels = [NSMutableDictionary dictionaryWithCapacity:100];
        [layoutLabels enumerateKeysAndObjectsUsingBlock:^(NSString * _Nonnull key, NSString * _Nonnull obj, BOOL * _Nonnull stop) {
            if (key.integerValue != 0 || [key isEqualToString:@"0"]) {
                labels[@(key.integerValue)] = obj;
            }
        }];
        
        // get keyplane labels
        NSArray *keyPlaneIDs = [[labels.allKeys filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(id  _Nonnull evaluatedObject, NSDictionary<NSString *,id> * _Nullable bindings) {
            return [evaluatedObject integerValue] < 0;
        }]] sortedArrayUsingDescriptors:@[[NSSortDescriptor sortDescriptorWithKey:@"self" ascending:NO]]];
        keyPlaneLabels = [labels objectsForKeys:keyPlaneIDs notFoundMarker:@""];
        [labels removeObjectsForKeys:keyPlaneIDs];
        
        // read sizes
        NSMutableArray *sizes = [layoutDictionary.allKeys filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(id  _Nonnull evaluatedObject, NSDictionary<NSString *,id> * _Nullable bindings) {
            CGSize size = CGSizeFromString(evaluatedObject);
            return !CGSizeEqualToSize(size, CGSizeZero);
        }]].mutableCopy;
        NSMutableDictionary<NSValue*,NSString*> *sizeToKey = [NSMutableDictionary dictionaryWithCapacity:sizes.count];
        for(NSUInteger i=0; i < sizes.count; i++) {
            CGSize size = CGSizeFromString(sizes[i]);
            NSValue *sizeValue = [NSValue valueWithCGSize:size];
            sizeToKey[sizeValue] = sizes[i];
            sizes[i] = sizeValue;
        }
        
        // read layouts
        keyMaps = [NSMutableDictionary dictionaryWithCapacity:sizes.count];
        for (NSValue *sizeValue in sizes) {
            KBMutableKeyMap *keyMap = [NSMutableArray arrayWithCapacity:self.numberOfKeyPlanes];
            for (NSArray *keyPlane in layoutDictionary[sizeToKey[sizeValue]]) {
                [keyMap addObject:[self _loadKeys:keyPlane fromDictionary:layoutDictionary withTransform:CGAffineTransformIdentity skip:nil]];
            }
            keyMaps[sizeValue] = keyMap;
        }
    }
    return self;
}

- (KBKeyPlane*)_loadKeys:(NSArray *)keys fromDictionary:(NSDictionary*)layoutDictionary withTransform:(CGAffineTransform)keyTransform skip:(NSSet*)skipKeys {
    NSMutableArray *keyPlane = [NSMutableArray arrayWithCapacity:keys.count];
    CGRect frame = CGRectZero;
    for (NSDictionary *keyDict in keys) {
        // include
        NSArray *includeArgs = keyDict[@"include"];
        if ([includeArgs isKindOfClass:[NSArray class]] && includeArgs.count >= 2) {
            CGAffineTransform includeTransform = keyTransform;
            NSArray<NSNumber*> *scaleArgs = keyDict[@"scale"];
            if ([scaleArgs isKindOfClass:[NSArray class]] && scaleArgs.count == 2) {
                includeTransform = CGAffineTransformScale(includeTransform, scaleArgs[0].CGFloatValue, scaleArgs[1].CGFloatValue);
            }
            NSArray<NSNumber*> *translateArgs = keyDict[@"translate"];
            if ([translateArgs isKindOfClass:[NSArray class]] && translateArgs.count == 2) {
                includeTransform = CGAffineTransformTranslate(includeTransform, translateArgs[0].CGFloatValue, translateArgs[1].CGFloatValue);
            }
            NSArray *skipArgs = keyDict[@"skip"];
            
            NSString *layoutKey = includeArgs[0];
            NSNumber *planeNumber = includeArgs[1];
            NSArray *addKeys = [self _loadKeys:layoutDictionary[layoutKey][planeNumber.integerValue] fromDictionary:layoutDictionary withTransform:includeTransform skip:skipArgs ? [NSSet setWithArray:skipArgs] : nil];
            [keyPlane addObjectsFromArray:addKeys];
        }
        
        // frame
        NSArray *kvFrame = keyDict[@"frame"];
        frame.origin.x = [kvFrame[0] CGFloatValue];
        if (kvFrame.count > 1 && kvFrame[1] != [NSNull null]) {
            frame.origin.y = [kvFrame[1] CGFloatValue];
        }
        if (kvFrame.count > 2 && kvFrame[2] != [NSNull null]) {
            frame.size.width = [kvFrame[2] CGFloatValue];
        }
        if (kvFrame.count > 3 && kvFrame[3] != [NSNull null]) {
            frame.size.height = [kvFrame[3] CGFloatValue];
        }
        
        // key
        id scancode = keyDict[@"key"];
        if ([skipKeys containsObject:scancode]) {
            continue;
        }
        KBKeyDescriptor *kd = [KBKeyDescriptor new];
        if ([scancode isEqual:@"hide"]) {
            kd->scancode = VKC_HIDE;
        } else if ([scancode isEqual:@"shift/caps"]) {
            kd->scancode = VKC_SHIFT_CAPS;
        } else {
            kd->scancode = (int8_t)[scancode integerValue];
        }
        kd->sticky = [keyDict[@"sticky"] boolValue];
        kd->dark = [keyDict[@"dark"] boolValue];
        kd->frame = CGRectIntegral(CGRectApplyAffineTransform(frame, keyTransform));
        kd->fontScale = keyDict[@"fontScale"] ? [keyDict[@"fontScale"] CGFloatValue] : 1.0;
        
        [keyPlane addObject:kd];
    }
    
    return keyPlane;
}

@end

@implementation KBKeyDescriptor

@end

@implementation KBKeyboardLayoutBinaryReader

- (instancetype)initWithData:(NSData *)layoutData {
    if ((self = [super init])) {
        data = layoutData;
        ptr = data.bytes + 12;
    }
    return self;
}

+ (BOOL)validBinaryHeader:(NSData*)data {
    return data.length > 12 && memcmp(data.bytes, "NFKeyboard\x01\x00", 12) == 0;
}

- (void)_checkReadableBytes:(NSUInteger)size {
    if (ptr + size > data.bytes + data.length) {
        @throw [NSException exceptionWithName:NSRangeException reason:nil userInfo:@{@"pos": @(ptr - data.bytes), @"read": @(size), @"length": @(data.length)}];
    }
}

- (NSUInteger)position {
    return ptr - data.bytes;
}

- (void)setPosition:(NSUInteger)position {
    ptr = data.bytes + position;
}

- (NSString*)readString {
    uint8_t stringLength = [self readUInt8];
    if (stringLength == 0) {
        return nil;
    }
    [self _checkReadableBytes:stringLength];
    NSString *string = [[NSString alloc] initWithBytes:ptr length:stringLength encoding:NSUTF8StringEncoding];
    ptr += stringLength;
    return string;
}

- (uint8_t)readUInt8 {
    [self _checkReadableBytes:1];
    return *(uint8_t*)ptr++;
}

- (int8_t)readInt8 {
    [self _checkReadableBytes:1];
    return *(int8_t*)ptr++;
}

- (uint16_t)readUInt16 {
    [self _checkReadableBytes:2];
    uint16_t value = OSReadLittleInt16(ptr, 0);
    ptr += 2;
    return value;
}

- (int16_t)readInt16 {
    [self _checkReadableBytes:2];
    int16_t value = OSReadLittleInt16(ptr, 0);
    ptr += 2;
    return value;
}

- (uint32_t)readUInt32 {
    [self _checkReadableBytes:4];
    uint32_t value = OSReadLittleInt32(ptr, 0);
    ptr += 4;
    return value;
}

- (CGFloat)readFloat {
    uint32_t value = [self readUInt32];
    return *(float*)&value;
}

- (CGRect)readFrameOfSize:(NSUInteger)size withLastFrame:(CGRect)lastFrame {
    CGRect frame = lastFrame;
    CGFloat *output[4] = {&frame.origin.x, &frame.origin.y, &frame.size.width, &frame.size.height};
    for (int i = 0; i < size; i++) {
        uint16_t value = [self readUInt16];
        if (value != 0x7FFF) {
            *output[i] = (CGFloat)value;
        }
    }
    return frame;
}

- (NSArray<NSString*>*)readStrings:(NSUInteger)count emptyStringMarker:(id)emptyStringMarker {
    NSMutableArray *strings = [NSMutableArray arrayWithCapacity:count];
    while (count--) {
        [strings addObject:[self readString] ?: emptyStringMarker];
    }
    return strings;
}

@end
