//
//  TrackPad.h
//  Mini vMac for iOS
//
//  Created by Jesús A. Álvarez on 18/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

#if TARGET_OS_WATCH
#import "UIKit+Watch.h"
@interface TrackPad : UIView
#else
@interface TrackPad : UIControl
#endif

@end
