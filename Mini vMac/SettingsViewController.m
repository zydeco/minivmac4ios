//
//  SettingsViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 07/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "SettingsViewController.h"
#import "AppDelegate.h"

@interface SettingsViewController ()

@end

typedef enum : NSInteger {
    SettingsSectionSpeed,
    SettingsSectionMouse,
    SettingsSectionKeyboard,
    SettingsSectionMachine,
    SettingsSectionAbout
} SettingsSection;

@implementation SettingsViewController
{
    NSArray *keyboardLayouts;
    NSArray<NSBundle*> *emulatorBundles;
    NSString *aboutTitle;
    NSArray<NSDictionary<NSString*,NSString*>*> *aboutItems;
    UITextView *footerView;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    keyboardLayouts = [[NSBundle mainBundle] pathsForResourcesOfType:@"nfkeyboardlayout" inDirectory:@"Keyboard Layouts"];
    emulatorBundles = [AppDelegate sharedInstance].emulatorBundles;
    [self loadCredits];
}

- (void)loadCredits {
    NSDictionary *aboutData = [NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"about" ofType:@"plist"]];
    aboutTitle = aboutData[@"title"];
    aboutItems = aboutData[@"items"];
    footerView = [[UITextView alloc] initWithFrame:CGRectZero];
    NSAttributedString *str = [[NSMutableAttributedString alloc] initWithData:[aboutData[@"footer.html"] dataUsingEncoding:NSUTF8StringEncoding]
                                                                      options:@{NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType,
                                                                                NSCharacterEncodingDocumentAttribute: @(NSUTF8StringEncoding)}
                                                           documentAttributes:nil
                                                                        error:NULL];
    footerView.attributedText = str;
    [footerView sizeToFit];
    footerView.editable = NO;
    footerView.textAlignment = NSTextAlignmentCenter;
    footerView.textColor = [UIColor darkGrayColor];
    footerView.font = [UIFont systemFontOfSize:[UIFont smallSystemFontSize]];
    footerView.backgroundColor = [UIColor clearColor];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [AppDelegate sharedEmulator].running = NO;
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [AppDelegate sharedEmulator].running = YES;
}

- (void)showInsertDisk:(id)sender {
    [[AppDelegate sharedInstance] showInsertDisk:sender];
}

- (void)dismiss:(id)sender {
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)changeSpeed:(UISegmentedControl*)sender {
    if ([sender isKindOfClass:[UISegmentedControl class]]) {
        [[NSUserDefaults standardUserDefaults] setInteger:sender.selectedSegmentIndex forKey:@"speedValue"];
    }
}

- (IBAction)changeMouseType:(UISegmentedControl*)sender {
    if ([sender isKindOfClass:[UISegmentedControl class]]) {
        [[NSUserDefaults standardUserDefaults] setBool:sender.selectedSegmentIndex == 1 forKey:@"trackpad"];
    }
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 5;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    switch (section) {
        case SettingsSectionKeyboard:
            return keyboardLayouts.count;
        case SettingsSectionMachine:
            return emulatorBundles.count;
        case SettingsSectionAbout:
            return aboutItems.count;
        default:
            return 1;
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    switch (section) {
        case SettingsSectionSpeed:
            return NSLocalizedString(@"Speed", nil);
        case SettingsSectionMouse:
            return NSLocalizedString(@"Mouse Type", nil);
        case SettingsSectionMachine:
            return NSLocalizedString(@"Emulated Machine", nil);
        case SettingsSectionKeyboard:
            return NSLocalizedString(@"Keyboard Layout", nil);
        case SettingsSectionAbout:
            return aboutTitle;
        default:return nil;
    }
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
    if (section == SettingsSectionAbout) {
        return footerView;
    } else {
        return nil;
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section == SettingsSectionMachine) {
        return NSLocalizedString(@"Changing the emulated machine requires to relaunch Mini vMac", nil);
    } else {
        return nil;
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
    if (section == SettingsSectionAbout) {
        return footerView.bounds.size.height;
    } else {
        return UITableViewAutomaticDimension;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = nil;
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSInteger section = indexPath.section;
    if (section == SettingsSectionSpeed) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"speed" forIndexPath:indexPath];
        UISegmentedControl *speedControl = (UISegmentedControl*)[cell viewWithTag:128];
        speedControl.selectedSegmentIndex = [defaults integerForKey:@"speedValue"];
    } else if (section == SettingsSectionMouse) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"mouse" forIndexPath:indexPath];
        UISegmentedControl *mouseControl = (UISegmentedControl*)[cell viewWithTag:128];
        mouseControl.selectedSegmentIndex = [defaults boolForKey:@"trackpad"] ? 1 : 0;
    } else if (section == SettingsSectionKeyboard) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"keyboard" forIndexPath:indexPath];
        NSString *layout = keyboardLayouts[indexPath.row];
        cell.textLabel.text = layout.lastPathComponent.stringByDeletingPathExtension;
        BOOL selected = [[defaults stringForKey:@"keyboardLayout"] isEqualToString:layout.lastPathComponent];
        cell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    } else if (section == SettingsSectionMachine) {
        NSBundle *bundle = emulatorBundles[indexPath.row];
        NSString *bundleName = bundle.bundlePath.lastPathComponent.stringByDeletingPathExtension;
        cell = [tableView dequeueReusableCellWithIdentifier:@"machine" forIndexPath:indexPath];
        cell.textLabel.text = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
        cell.detailTextLabel.text = [bundle objectForInfoDictionaryKey:@"CFBundleGetInfoString"];
        NSAssert([bundle.bundlePath hasPrefix:[NSBundle mainBundle].bundlePath], @"emulator bundle is in main bundle");
        NSString *iconName = [[bundle.bundlePath stringByAppendingPathComponent:@"Icon"] substringFromIndex:[NSBundle mainBundle].bundlePath.length+1];
        cell.imageView.image = [UIImage imageNamed:iconName];
        BOOL selected = [[defaults stringForKey:@"machine"] isEqualToString:bundleName];
        cell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    } else if (section == SettingsSectionAbout) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"about" forIndexPath:indexPath];
        NSDictionary<NSString*,NSString*> *item = aboutItems[indexPath.row];
        cell.textLabel.text = item[@"text"];
        NSString *detailText = item[@"detailText"];
        if ([detailText isEqualToString:@"$version"]) {
            NSString *versionString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
            NSString *commitString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"GitVersion"];
            cell.detailTextLabel.text = commitString.length > 0 ? [NSString stringWithFormat:@"%@ (%@)", versionString, commitString] : versionString;
        } else {
            cell.detailTextLabel.text = detailText;
        }
        cell.accessoryType = item[@"link"] == nil ? UITableViewCellAccessoryNone : UITableViewCellAccessoryDisclosureIndicator;
    }
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (indexPath.section == SettingsSectionKeyboard) {
        // selected keyboard layout
        NSString *layout = keyboardLayouts[indexPath.row];
        [defaults setValue:layout.lastPathComponent forKey:@"keyboardLayout"];
        [tableView reloadSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationAutomatic];
    } else if (indexPath.section == SettingsSectionMachine) {
        // selected emulated machine
        NSBundle *bundle = emulatorBundles[indexPath.row];
        NSString *bundleName = bundle.bundlePath.lastPathComponent.stringByDeletingPathExtension;
        [defaults setValue:bundleName forKey:@"machine"];
        [tableView reloadSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationAutomatic];
    } else if (indexPath.section == SettingsSectionAbout) {
        // links in about
        NSString *linkURL = aboutItems[indexPath.row][@"link"];
        if (linkURL != nil) {
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:linkURL]];
        }
    }
}

@end
