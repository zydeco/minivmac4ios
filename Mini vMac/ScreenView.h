//
//  ScreenView.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 30/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ScreenView : UIView

@property (nonatomic, readonly) CGRect screenBounds;
@property (nonatomic, readonly) CGSize screenSize;

+ (instancetype)sharedScreenView;

@end
