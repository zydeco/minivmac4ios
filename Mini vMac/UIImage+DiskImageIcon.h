//
//  UIImage+DiskImageIcon.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 21/05/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

extern NSString *DidUpdateIconForDiskImageNotification;

@interface UIImage (DiskImageIcon)

+ (UIImage *)imageWithIconForDiskImage:(NSString *)path;

@end
