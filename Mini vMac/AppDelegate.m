//
//  AppDelegate.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 27/04/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

@import AVFoundation;
#import "AppDelegate.h"
#import "SettingsViewController.h"
#import "InsertDiskViewController.h"

static AppDelegate *sharedAppDelegate = nil;
static NSObject<Emulator> *sharedEmulator = nil;
NSString *DocumentsChangedNotification = @"documentsChanged";

@interface AppDelegate () <BTCMouseDelegate>

@end

@implementation AppDelegate
{
}

+ (instancetype)sharedInstance {
    return sharedAppDelegate;
}

+ (id<Emulator>)sharedEmulator {
    return sharedEmulator;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    sharedAppDelegate = self;
    if (![self loadEmulator:[[NSUserDefaults standardUserDefaults] stringForKey:@"machine"]]) {
        [self loadEmulator:@"MacPlus4M"];
    }
    [self initDefaults];
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:NULL];
    [sharedEmulator performSelector:@selector(run) withObject:nil afterDelay:0.1];
    
    if ([application respondsToSelector:@selector(btcMouseSetRawMode:)]) {
        [application btcMouseSetRawMode:YES];
        [application btcMouseSetDelegate:self];
    }
    
    // populate documents directory so it shows up in Files
    [[NSFileManager defaultManager] createDirectoryAtPath:self.userKeyboardLayoutsPath withIntermediateDirectories:YES attributes:nil error:nil];
    
    return YES;
}

- (void)initDefaults {
    // default settings
    NSDictionary *layoutForLanguage = @{@"en": @"British.nfkeyboardlayout",
                                        @"es": @"Spanish.nfkeyboardlayout",
                                        @"en-US": @"US.nfkeyboardlayout"};
    NSString *firstLanguage = [NSBundle preferredLocalizationsFromArray:layoutForLanguage.allKeys].firstObject;
    NSDictionary *defaultValues = @{@"trackpad": @([UIDevice currentDevice].userInterfaceIdiom != UIUserInterfaceIdiomPad),
                                    @"keyboardLayout": layoutForLanguage[firstLanguage],
                                    @"machine": @"MacPlus4M",
                                    @"speedValue": @(sharedEmulator.initialSpeed),
                                    @"runInBackground": @NO,
                                    @"autoSlow": @(sharedEmulator.initialAutoSlow),
                                    @"screenFilter": kCAFilterLinear
                                    };
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:defaultValues];
    [defaults setValue:@(sharedEmulator.initialSpeed) forKey:@"speedValue"];
    [defaults addObserver:self forKeyPath:@"speedValue" options:0 context:NULL];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSString *,id> *)change context:(void *)context {
    if (object == [NSUserDefaults standardUserDefaults]) {
        NSUserDefaults *defaults = object;
        if ([keyPath isEqualToString:@"speedValue"]) {
            sharedEmulator.speed = [defaults integerForKey:@"speedValue"];
        } else if ([keyPath isEqualToString:@"autoSlow"]) {
            sharedEmulator.autoSlow = [defaults integerForKey:@"autoSlow"];
        }
    }
}

- (NSString*)emulatorBundlesPath {
    return [NSBundle mainBundle].privateFrameworksPath;
}

- (NSArray<NSBundle*>*)emulatorBundles {
    NSArray<NSString*> *names = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:self.emulatorBundlesPath error:NULL];
    NSMutableArray *emulatorBundles = [NSMutableArray arrayWithCapacity:names.count];
    for (NSString *name in [names pathsMatchingExtensions:@[@"mnvm"]]) {
        NSBundle *bundle = [NSBundle bundleWithPath:[self.emulatorBundlesPath stringByAppendingPathComponent:name]];
        [emulatorBundles addObject:bundle];
    }
    return emulatorBundles;
}

- (BOOL)loadEmulator:(NSString*)name {
    NSString *emulatorBundleName = [name stringByAppendingPathExtension:@"mnvm"];
    NSString *emulatorBundlePath = [self.emulatorBundlesPath stringByAppendingPathComponent:emulatorBundleName];
    NSBundle *emulatorBundle = [NSBundle bundleWithPath:emulatorBundlePath];
    [emulatorBundle load];
    sharedEmulator = [[emulatorBundle principalClass] new];
    sharedEmulator.dataPath = self.documentsPath;
    return sharedEmulator != nil;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults synchronize];
    if ([defaults boolForKey:@"runInBackground"]) {
        // slow down to 1x when in background
        sharedEmulator.speed = EmulatorSpeed1x;
    } else {
        sharedEmulator.running = NO;
    }
    if (sharedEmulator.anyDiskInserted == NO) {
        exit(0);
    }
}

- (void)handleEventWithMove:(CGPoint)move andWheel:(float)wheel andPan:(float)pan andButtons:(int)buttons {
    [sharedEmulator moveMouseX:move.x/2.0 Y:move.y/2.0];
    [sharedEmulator setMouseButton:buttons == 1];
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (sharedEmulator.running) {
        sharedEmulator.speed = [defaults integerForKey:@"speedValue"];
    } else {
        sharedEmulator.running = YES;
    }
}

- (void)showAlertWithTitle:(NSString *)title message:(NSString *)message {
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self showAlertWithTitle:title message:message];
        });
        return;
    }
    if ([UIAlertController class]) {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
        UIViewController *controller = self.window.rootViewController;
        while (controller.presentedViewController) {
            controller = controller.presentedViewController;
        }
        [controller presentViewController:alert animated:YES completion:nil];
    } else {
        UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:title message:message delegate:nil cancelButtonTitle:@"OK" otherButtonTitles: nil];
        [alertView show];
    }
}

#pragma mark - Settings / Insert Disk panels

- (void)showSettings:(id)sender {
    [self.window.rootViewController performSelector:@selector(showSettings:) withObject:sender];
}

- (void)showInsertDisk:(id)sender {
    [self.window.rootViewController performSelector:@selector(showInsertDisk:) withObject:sender];
}

#pragma mark - Files

- (BOOL)isSandboxed {
#if TARGET_IPHONE_SIMULATOR
    return YES;
#else
    static dispatch_once_t onceToken;
    static BOOL sandboxed;
    dispatch_once(&onceToken, ^{
        // not sandboxed if parent of documents directory is "mobile"
        NSString *documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject.stringByStandardizingPath;
        sandboxed = ![documentsPath.stringByDeletingLastPathComponent.lastPathComponent isEqualToString:@"mobile"];
    });
    return sandboxed;
#endif
}

- (NSArray<NSString *> *)diskImageExtensions {
    return @[@"dsk", @"img", @"dc42", @"diskcopy42", @"image"];
}

- (NSString *)documentsPath {
    static dispatch_once_t onceToken;
    static NSString *documentsPath;
    dispatch_once(&onceToken, ^{
        documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject.stringByStandardizingPath;
        if (!self.sandboxed) {
            documentsPath = [documentsPath stringByAppendingPathComponent:@"Mini vMac"].stringByStandardizingPath;
        }
        [[NSFileManager defaultManager] createDirectoryAtPath:documentsPath withIntermediateDirectories:YES attributes:nil error:NULL];
    });
    return documentsPath;
}

- (NSString *)userKeyboardLayoutsPath {
    static dispatch_once_t onceToken;
    static NSString *userKeyboardLayoutsPath;
    dispatch_once(&onceToken, ^{
        userKeyboardLayoutsPath = [self.documentsPath stringByAppendingPathComponent:@"Keyboard Layouts"];
    });
    return userKeyboardLayoutsPath;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
    NSMutableDictionary *options = [NSMutableDictionary dictionaryWithCapacity:2];
    if (sourceApplication) {
        options[UIApplicationOpenURLOptionsSourceApplicationKey] = sourceApplication;
    }
    if (annotation) {
        options[UIApplicationOpenURLOptionsAnnotationKey] = annotation;
    }
    return [self application:application openURL:url options:options];
}

- (BOOL)importFileToDocuments:(NSURL *)url copy:(BOOL)copy {
    if (url.fileURL) {
        // opening file
        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSString *fileName = url.path.lastPathComponent;
        NSString *destinationPath = [self.documentsPath stringByAppendingPathComponent:fileName];
        NSError *error = NULL;
        NSInteger tries = 1;
        while ([fileManager fileExistsAtPath:destinationPath]) {
            NSString *newFileName;
            if (fileName.pathExtension.length > 0) {
                newFileName = [NSString stringWithFormat:@"%@ %d.%@", fileName.stringByDeletingPathExtension, (int)tries, fileName.pathExtension];
            } else {
                newFileName = [NSString stringWithFormat:@"%@ %d", fileName, (int)tries];
            }
            destinationPath = [self.documentsPath stringByAppendingPathComponent:newFileName];
            tries++;
        }
        if (copy) {
            [fileManager copyItemAtPath:url.path toPath:destinationPath error:&error];
        } else {
            [fileManager moveItemAtPath:url.path toPath:destinationPath error:&error];
        }
        if (error) {
            [self showAlertWithTitle:fileName message:error.localizedFailureReason];
        } else {
            NSDictionary *userInfo = @{@"path": destinationPath};
            [[NSNotificationCenter defaultCenter] postNotificationName:DocumentsChangedNotification object:self userInfo:userInfo];
            [self showAlertWithTitle:@"File Import" message:[NSString stringWithFormat:@"%@ imported to Documents", destinationPath.lastPathComponent]];
        }
    }
    return YES;
}

- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey,id> *)options {
    if (url.fileURL) {
        // opening file
        NSString *inboxPath = [self.documentsPath stringByAppendingPathComponent:@"Inbox"];
        if ([url.path.stringByStandardizingPath hasPrefix:inboxPath]) {
            // pre-iOS 11 import through inbox
            [url startAccessingSecurityScopedResource];
            [self importFileToDocuments:url copy:NO];
            [url stopAccessingSecurityScopedResource];
        } else if ([url.path.stringByStandardizingPath hasPrefix:self.documentsPath]) {
            // already in documents - mount
            [sharedEmulator insertDisk:url.path];
        } else if ([options[UIApplicationOpenURLOptionsOpenInPlaceKey] boolValue]) {
            // not in documents - copy
            [url startAccessingSecurityScopedResource];
            [self importFileToDocuments:url copy:YES];
            [url stopAccessingSecurityScopedResource];
        } else {
            return [self importFileToDocuments:url copy:NO];
        }
    }
    return YES;
}

@end
