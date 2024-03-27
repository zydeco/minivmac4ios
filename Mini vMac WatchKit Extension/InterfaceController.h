//
//  InterfaceController.h
//  Mini vMac WatchKit Extension
//
//  Created by Jesús A. Álvarez on 14/10/2018.
//  Copyright © 2018 namedfork. All rights reserved.
//

#import <WatchKit/WatchKit.h>
#import <Foundation/Foundation.h>
#import <SpriteKit/SpriteKit.h>

@class WCSession;

@interface InterfaceController : WKInterfaceController

- (void)sessionReachabilityDidChange:(WCSession *)session;

@end
