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

@interface ViewController : UIViewController <KBKeyboardViewDelegate>

@property (weak, nonatomic) IBOutlet ScreenView *screenView;
@property (nonatomic, getter=isKeyboardVisible) BOOL keyboardVisible;
@property (weak, nonatomic) IBOutlet UIView *helpView;

- (IBAction)showGestureHelp:(id)sender;
- (IBAction)hideGestureHelp:(id)sender;
- (void)showKeyboard:(id)sender;

@end

#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
@interface ViewController (VisionSupport)
@property (nonatomic, readonly) UIViewController* keyboardViewController;
- (void)initXr;
+ (void)adjustToScreenSize;
- (UIMenu*)keyboardLayoutMenu;
@end
#endif
