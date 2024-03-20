//
//  HFSDiskImage.h
//  Mini vMac
//
//  Created by Lieven Dekeyser on 13/03/2024.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HFSDiskImage : NSObject

@property (nonatomic, readonly, copy) NSString * path;
@property (nonatomic, readonly, getter=isOpen) BOOL open;
@property (nonatomic, readonly, getter=isReadOnly) BOOL readOnly;

+ (nullable HFSDiskImage *)createDiskImageWithName:(NSString *)name size:(size_t)volumeSize atPath:(NSString *)path;

- (nullable instancetype)initWithPath:(NSString *)path;

- (instancetype)init __unavailable;
+ (instancetype)new __unavailable;

- (BOOL)openForReading;
- (BOOL)openForReadingAndWriting;

- (BOOL)close;

- (BOOL)addFile:(NSString *)sourceFile;

@end // HFSDiskImage


@interface HFSDiskImage (Import)

+ (nullable HFSDiskImage *)importFileIntoTemporaryDiskImage:(NSString *)sourceFile;

@end // HFSDiskImage (Import)


NS_ASSUME_NONNULL_END
