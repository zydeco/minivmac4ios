//
//  ViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "ViewController.h"
#import "TouchScreen.h"

@interface ViewController ()

@end

@implementation ViewController
{
    UIControl *pointingDeviceView;
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self setUpPointingDevice];
}

- (void)setUpPointingDevice {
    if (pointingDeviceView) {
        [pointingDeviceView removeFromSuperview];
        pointingDeviceView = nil;
    }
    pointingDeviceView = [[TouchScreen alloc] initWithFrame:self.view.bounds];
    pointingDeviceView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view insertSubview:pointingDeviceView aboveSubview:self.screenView];
}

@end
