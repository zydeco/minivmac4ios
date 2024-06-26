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
#import "ViewController.h"

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
    [self initDefaults];
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient error:NULL];
    [self loadAndStartEmulator];
    
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
                                    @"screenFilter": kCAFilterLinear,
                                    @"autoShowGestureHelp": @YES,
                                    @"recentDisks": @[]
                                    };
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults registerDefaults:defaultValues];
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
    sharedEmulator.rootViewController = self.window.rootViewController;
    sharedEmulator.showAlert = ^(NSString *title, NSString *message) {
        [self showAlertWithTitle:title message:message];
    };
    sharedEmulator.dataPath = self.documentsPath;
#if defined(TARGET_OS_VISION) && TARGET_OS_VISION == 1
    [ViewController adjustToScreenSize];
#endif
    return sharedEmulator != nil;
}

- (void)loadAndStartEmulator {
    [self willChangeValueForKey:@"sharedEmulator"];
    if (sharedEmulator) {
        NSBundle *bundle = sharedEmulator.bundle;
        id<Emulator> oldEmulator = sharedEmulator;
        sharedEmulator = nil;
        [oldEmulator shutdown];
        [bundle unload];
    }
    if (![self loadEmulator:[[NSUserDefaults standardUserDefaults] stringForKey:@"machine"]]) {
        [self loadEmulator:@"MacPlus4M"];
    }
    [self didChangeValueForKey:@"sharedEmulator"];
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if ([defaults integerForKey:@"speedValue"] > sharedEmulator.initialSpeed) {
        [defaults setValue:@(sharedEmulator.initialSpeed) forKey:@"speedValue"];
    } else {
        sharedEmulator.speed = [defaults integerForKey:@"speedValue"];
    }
    [sharedEmulator performSelector:@selector(run) withObject:nil afterDelay:0.1];
}

- (id<Emulator>)sharedEmulator {
    return sharedEmulator;
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
    BOOL wasRunning = sharedEmulator.isRunning;
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        [sharedEmulator setRunning:wasRunning];
    }]];
    UIViewController *controller = self.window.rootViewController;
    while (controller.presentedViewController) {
        controller = controller.presentedViewController;
    }
    [controller presentViewController:alert animated:YES completion:nil];
}

- (void)application:(UIApplication *)application performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem completionHandler:(void (^)(BOOL))completionHandler {
    BOOL success = NO;
    if ([shortcutItem.type isEqualToString:@"disk"]) {
        NSString *fileName = (NSString*)shortcutItem.userInfo[@"disk"];
        NSString *filePath = [self.documentsPath stringByAppendingPathComponent:fileName];
        if ([[NSFileManager defaultManager] fileExistsAtPath:filePath] && ![sharedEmulator isDiskInserted:filePath]) {
            success = YES;
            [sharedEmulator performSelector:@selector(insertDisk:) withObject:filePath afterDelay:1.0];
        }
    }
    completionHandler(success);
}

#pragma mark - Settings / Insert Disk / Help

- (void)showSettings:(id)sender {
    [self.window.rootViewController performSelector:@selector(showSettings:) withObject:sender];
}

- (void)showInsertDisk:(id)sender {
    [self.window.rootViewController performSelector:@selector(showInsertDisk:) withObject:sender];
}

- (void)showGestureHelp:(id)sender {
    [self.window.rootViewController performSelector:@selector(showGestureHelp:) withObject:sender];
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

- (NSArray<NSString *> *)keyboardLayoutPaths {
    NSArray *keyboardLayouts = [[NSBundle mainBundle] pathsForResourcesOfType:@"nfkeyboardlayout" inDirectory:@"Keyboard Layouts"];
    NSString *userKeyboardLayoutsPath = [AppDelegate sharedInstance].userKeyboardLayoutsPath;
    NSArray *userKeyboardLayouts = [[[NSFileManager defaultManager] contentsOfDirectoryAtPath:userKeyboardLayoutsPath error:nil] filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"pathExtension.lowercaseString = %@", @"nfkeyboardlayout"]];
    if (userKeyboardLayouts.count > 0) {
        keyboardLayouts = [keyboardLayouts arrayByAddingObjectsFromArray:userKeyboardLayouts];
    }
    return keyboardLayouts;
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

#pragma mark - Making a Scene

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options {
    for (NSUserActivity *activity in options.userActivities) {
        if ([activity.activityType isEqualToString:@"net.namedfork.keyboard"]) {
            return [UISceneConfiguration configurationWithName:@"Keyboard" sessionRole:UIWindowSceneSessionRoleApplication];
        }
    }
    if ([self sceneWithName:@"Default"] == nil) {
        [[AppDelegate sharedEmulator] setRunning:YES];
        return [UISceneConfiguration configurationWithName:@"Default" sessionRole:UIWindowSceneSessionRoleApplication];
    }
    return nil;
}

- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions {
    if ([self sceneWithName:@"Default"] == nil) {
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"runInBackground"] == NO) {
            [[AppDelegate sharedEmulator] setRunning:NO];
        }
        UIScene *keyboardScene = [self sceneWithName:@"Keyboard"];
        if (keyboardScene != nil) {
            // if only keyboard is left, close it too
            [application requestSceneSessionDestruction:keyboardScene.session options:nil errorHandler:nil];
        }
    }
}

- (UIScene*)sceneWithName:(NSString*)name {
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if ([scene.session.configuration.name isEqualToString:name]) {
            return scene;
        }
    }
    return nil;
}

@end
