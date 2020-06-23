//
//  ViewController.h
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ScreenView.h"
#import "KBKeyboardView.h"

#ifdef __IPHONE_13_4
API_AVAILABLE(ios(13.4))
@interface ViewController : UIViewController <UIPointerInteractionDelegate, KBKeyboardViewDelegate>
#else
@interface ViewController : UIViewController <KBKeyboardViewDelegate>
#endif

@property (weak, nonatomic) IBOutlet ScreenView *screenView;
@property (nonatomic, getter=isKeyboardVisible) BOOL keyboardVisible;
@property (weak, nonatomic) IBOutlet UIView *helpView;

- (IBAction)showGestureHelp:(id)sender;
- (IBAction)hideGestureHelp:(id)sender;

@end

