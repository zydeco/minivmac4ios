//
//  HFSDiskImage.m
//  Mini vMac
//
//  Created by Lieven Dekeyser on 13/03/2024.
//

#import "HFSDiskImage.h"
#include "libhfs.h"

@interface NSString (HFSSafe)
- (nullable NSString *)hfsSafeFileName;
- (nullable NSString *)hfsSafeVolumeName;
@end // NSString (HFSSafe)


@interface HFSDiskImage ()
@property (nonatomic, copy) NSString * path;
@end // HFSDiskImage()

@implementation HFSDiskImage
{
	hfsvol * _volume;
}

+ (nullable HFSDiskImage *)createDiskImageWithName:(NSString *)name size:(size_t)volumeSize atPath:(NSString *)path
{
	NSFileManager * fm = [NSFileManager defaultManager];
	if ([fm fileExistsAtPath:path])
	{
		return nil;
	}
	
	
	int fileDescriptor = open(path.fileSystemRepresentation, O_CREAT | O_TRUNC | O_EXCL | O_WRONLY, 0644);
	if (fileDescriptor == -1)
	{
		return nil;
	}
	
	int error = 0;
	if (ftruncate(fileDescriptor, volumeSize))
	{
		error = errno;
	}
	else
	{
		const char * volumeName = [name cStringUsingEncoding:NSMacOSRomanStringEncoding];
		if (volumeName != nil)
		{
			error = hfs_format(path.fileSystemRepresentation, 0, 0, volumeName, 0, NULL);
		}
		else
		{
			error = EINVAL;
		}
	}
	
	close(fileDescriptor);
	
	if (error != 0)
	{
		[fm removeItemAtPath:path error:nil];
		return nil;
	}
	
	return [[self alloc] initWithPath:path];
}

- (nullable instancetype)initWithPath:(NSString *)path
{
	if ((self = [super init]))
	{
		_readOnly = NO;
		_volume = nil;
		self.path = path;
	}
	return self;
}

- (void)dealloc
{
	[self close];
}

- (BOOL)isOpen
{
	return _volume != NULL;
}

- (BOOL)openForReading
{
	return [self _openWithMode:HFS_MODE_RDONLY];
}

- (BOOL)openForReadingAndWriting
{
	return [self _openWithMode:HFS_MODE_RDWR];
}

- (BOOL)_openWithMode:(int)mode
{
	if ([self isOpen])
	{
		return YES;
	}

    _volume = hfs_mount(self.path.fileSystemRepresentation, 0, mode);
    _readOnly = (mode != HFS_MODE_RDWR);
	
	return [self isOpen];
}

- (BOOL)close
{
	if (_volume)
	{
		int error = hfs_umount(_volume);
		if (error == 0) {
			_volume = NULL;
		}
	}
	
	return ![self isOpen];
}

- (BOOL)addFile:(NSString *)sourceFilePath
{
	if (![self isOpen] || [self isReadOnly])
	{
		return NO;
	}
	
	NSString * fileName = [[sourceFilePath lastPathComponent] hfsSafeFileName];
	if (fileName == nil)
	{
		return NO;
	}
	
	const char * targetPath = [[NSString stringWithFormat:@":%@", fileName] cStringUsingEncoding:NSMacOSRomanStringEncoding];
	if (targetPath == NULL)
	{
		return NO;
	}
	
	
	FILE * sourceFile = fopen(sourceFilePath.fileSystemRepresentation, "r");
	if (sourceFile == NULL)
	{
		return NO;
	}
	
	BOOL error = NO;
	hfsfile * file = hfs_create(_volume, targetPath, "SIT!", "SITx"); // FIXME: type and creator from extension
	if (file)
	{
		const size_t bufferSize = HFS_BLOCKSZ;
		uint8_t buffer[bufferSize] = { 0 };
		
		size_t bytesRead = 0;
		while ((bytesRead = fread(buffer, 1, bufferSize, sourceFile)) > 0)
		{
			unsigned long bytesWritten = hfs_write(file, buffer, bytesRead);
			if (bytesWritten < bytesRead)
			{
				error = YES;
				break;
			}
		}
		
		hfs_close(file);
		file = NULL;
	}
	else
	{
		error = YES;
	}
	
	fclose(sourceFile);
	sourceFile = NULL;
	
	return error == NO;
}

@end // HFSDiskImage


@implementation HFSDiskImage (Import)

+ (HFSDiskImage *)importFileIntoTemporaryDiskImage:(NSString *)sourceFile
{
	NSFileManager * fm = [NSFileManager defaultManager];
	NSError * error = nil;
	size_t fileSize = [fm attributesOfItemAtPath:sourceFile error:&error].fileSize;
	if (fileSize == 0)
	{
		return nil;
	}
	
	
	NSString * tempFolder = NSTemporaryDirectory();
	
	NSString * volumeName = [[[sourceFile lastPathComponent] stringByDeletingPathExtension] hfsSafeVolumeName];
	if (volumeName == nil)
	{
		return nil;
	}
	NSString * diskImagePath = [tempFolder stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.img", volumeName]];
	int tries = 1;
	while ([fm fileExistsAtPath:diskImagePath])
	{
		++tries;
		diskImagePath = [tempFolder stringByAppendingPathComponent:[NSString stringWithFormat:@"%@ %d.img", volumeName, tries]];
	}
	
	
	HFSDiskImage * diskImage = [HFSDiskImage createDiskImageWithName:volumeName size:(fileSize + (512*1024)) atPath:diskImagePath];
	if (diskImage == NULL || ![diskImage openForReadingAndWriting])
	{
		return nil;
	}
	
	BOOL result = [diskImage addFile:sourceFile];
	
	[diskImage close];
	
	if (result)
	{
		return diskImage;
	}
	else
	{
		return nil;
	}
	
}

@end // HFSDiskImage (Import)



@implementation NSString (HFSSafe)

- (NSString *)macOSRomanSafeStringGettingLength:(NSUInteger *)outLength
{
	NSData * converted = [self dataUsingEncoding:NSMacOSRomanStringEncoding allowLossyConversion:YES];
	if (converted)
	{
		if (outLength)
		{
			*outLength = [converted length];
		}
		return [[NSString alloc] initWithData:converted encoding:NSMacOSRomanStringEncoding];
	}
	else
	{
		return nil;
	}
}

- (NSString *)macOSRomanSafeStringWithMaxLength:(NSUInteger)maxLength
{
	NSData * converted = [self dataUsingEncoding:NSMacOSRomanStringEncoding allowLossyConversion:YES];
	if (converted)
	{
		NSUInteger convertedLength = [converted length];
		if (convertedLength > maxLength)
		{
			converted = [converted subdataWithRange:NSMakeRange(0, maxLength)];
		}
		
		return [[NSString alloc] initWithData:converted encoding:NSMacOSRomanStringEncoding];
	}
	else
	{
		return nil;
	}
}

- (NSString *)hfsSafeFileName
{
	NSString * noColons = [self stringByReplacingOccurrencesOfString:@":" withString:@"_"];
	NSUInteger convertedStringLength = 0;
	NSString * convertedString = [noColons macOSRomanSafeStringGettingLength:&convertedStringLength];
	if (convertedString)
	{
		if (convertedStringLength <= HFS_MAX_FLEN)
		{
			return convertedString;
		}
		else
		{
			// keep path extension if possible:
			NSUInteger pathExtensionLength = 0;
			NSString * pathExtension = [[self pathExtension] macOSRomanSafeStringGettingLength:&pathExtensionLength];
			if (pathExtension)
			{
				NSInteger remainingLength = HFS_MAX_FLEN - pathExtensionLength - 1;
				if (remainingLength > 2)
				{
					NSString * trimmedName = [[self stringByDeletingPathExtension] macOSRomanSafeStringWithMaxLength:remainingLength];
					return [trimmedName stringByAppendingPathExtension:pathExtension];
					
				}
				else
				{
					return [[self stringByDeletingPathExtension] macOSRomanSafeStringWithMaxLength:HFS_MAX_FLEN];
				}
			}
			else
			{
				return [self macOSRomanSafeStringWithMaxLength:HFS_MAX_FLEN];
			}
		}
	}
	else
	{
		return nil;
	}
}

- (NSString *)hfsSafeVolumeName
{
	NSString * noColons = [self stringByReplacingOccurrencesOfString:@":" withString:@"_"];
	return [noColons macOSRomanSafeStringWithMaxLength:HFS_MAX_VLEN];
}

@end // NSString (HFSSafe)
