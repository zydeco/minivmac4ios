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
#import "EmulatorProtocol.h"

@class WCSession;

@interface InterfaceController : WKInterfaceController

@property (class, readonly, strong) id<Emulator> sharedEmulator NS_SWIFT_NAME(emulator);

- (void)sessionReachabilityDidChange:(WCSession *)session;

@end
