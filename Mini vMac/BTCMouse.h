//
//  BTCMouse.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 04/12/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//


@protocol BTCMouseDelegate
- (void)handleEventWithMove:(CGPoint)move andWheel:(float)wheel andPan:(float)pan andButtons:(int)buttons;
@end

@interface UIApplication (BTCMouse)
- (void)btcMouseSetPanning:(BOOL)panning;
- (void)btcMouseSetZooming:(BOOL)zooming;
- (void)btcMouseSetDelegate:(id<BTCMouseDelegate>)delegate;
- (void)btcMouseSetRawMode:(BOOL)rawMode;
@end

