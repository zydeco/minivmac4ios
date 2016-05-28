//
//  Emulator.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "EmulatorProtocol.h"

@interface Emulator : NSObject <Emulator>

- (void)updateScreen:(CGImageRef)screenImage;

@end
