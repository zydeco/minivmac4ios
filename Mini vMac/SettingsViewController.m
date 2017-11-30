//
//  SettingsViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 07/05/2016.
//  Copyright © 2016-2017 namedfork. All rights reserved.
//

#import "SettingsViewController.h"
#import "AppDelegate.h"

@interface SettingsViewController ()

@end

typedef enum : NSInteger {
    SettingsSectionSpeed,
    SettingsSectionMouse,
    SettingsSectionKeyboard,
    SettingsSectionDisplay,
    SettingsSectionMachine,
    SettingsSectionAbout
} SettingsSection;

@implementation SettingsViewController
{
    NSArray *keyboardLayouts;
    NSArray<NSBundle*> *emulatorBundles;
    NSMutableArray *machineList; // NSString (header) or NSBundle (emulator bundle)
    NSMutableSet<NSBundle*> *groupedEmulatorBundles;
    NSBundle *selectedEmulatorBundle;
    NSString *aboutTitle;
    NSArray<NSDictionary<NSString*,NSString*>*> *aboutItems;
    UITextView *footerView;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    keyboardLayouts = [[NSBundle mainBundle] pathsForResourcesOfType:@"nfkeyboardlayout" inDirectory:@"Keyboard Layouts"];
    [self loadEmulatorBundles];
    [self loadCredits];
}

- (void)loadEmulatorBundles {
    emulatorBundles = [AppDelegate sharedInstance].emulatorBundles;
    NSMutableDictionary<NSString*,NSMutableArray<NSBundle*>*> *bundlesByName = [NSMutableDictionary dictionaryWithCapacity:emulatorBundles.count];
    NSString *selectedBundleName = [[NSUserDefaults standardUserDefaults] stringForKey:@"machine"];
    for (NSBundle *bundle in emulatorBundles) {
        NSString *bundleName = bundle.bundlePath.lastPathComponent.stringByDeletingPathExtension;
        if ([selectedBundleName isEqualToString:bundleName]) {
            selectedEmulatorBundle = bundle;
        }
        NSString *displayName = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
        if (bundlesByName[displayName] == nil) {
            bundlesByName[displayName] = [NSMutableArray arrayWithCapacity:1];
        }
        [bundlesByName[displayName] addObject:bundle];
    }
    NSArray *sortedNames = [bundlesByName.allKeys sortedArrayUsingSelector:@selector(compare:)];
    machineList = [NSMutableArray arrayWithCapacity:emulatorBundles.count];
    groupedEmulatorBundles = [NSMutableSet setWithCapacity:emulatorBundles.count];
    for (NSString *name in sortedNames) {
        NSArray<NSBundle*>* bundles = bundlesByName[name];
        if (bundles.count > 1) {
            [machineList addObject:name];
            [groupedEmulatorBundles addObjectsFromArray:bundles];
        }
        [machineList addObjectsFromArray:bundles];
    }
}

- (void)loadCredits {
    NSDictionary *aboutData = [NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"about" ofType:@"plist"]];
    aboutTitle = aboutData[@"title"];
    aboutItems = aboutData[@"items"];
    footerView = [[UITextView alloc] initWithFrame:CGRectZero];
    NSString *footerHTML = [aboutData[@"footer.html"] stringByReplacingOccurrencesOfString:@"$mnvmversion" withString:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"MNVMVersion"]];
    NSAttributedString *str = [[NSAttributedString alloc] initWithData:[footerHTML dataUsingEncoding:NSUTF8StringEncoding]
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
    if ([UIApplication instancesRespondToSelector:@selector(btcMouseSetRawMode:)]) {
        [[UIApplication sharedApplication] btcMouseSetRawMode:NO];
    }
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
        EmulatorSpeed speedValue = sender.selectedSegmentIndex == sender.numberOfSegments - 1 ? EmulatorSpeedAllOut : sender.selectedSegmentIndex;
        [[NSUserDefaults standardUserDefaults] setInteger:speedValue forKey:@"speedValue"];
    }
}

- (IBAction)changeMouseType:(UISegmentedControl*)sender {
    if ([sender isKindOfClass:[UISegmentedControl class]]) {
        [[NSUserDefaults standardUserDefaults] setBool:sender.selectedSegmentIndex == 1 forKey:@"trackpad"];
    }
}

- (IBAction)changeScreenScaling:(UISegmentedControl*)sender {
    if ([sender isKindOfClass:[UISegmentedControl class]]) {
        NSString *filter = @[kCAFilterNearest, kCAFilterLinear, kCAFilterTrilinear][sender.selectedSegmentIndex];
        [[NSUserDefaults standardUserDefaults] setObject:filter forKey:@"screenFilter"];
    }
}

- (void)changeRunInBackground:(UISwitch*)sender {
    if ([sender isKindOfClass:[UISwitch class]]) {
        [[NSUserDefaults standardUserDefaults] setBool:sender.on forKey:@"runInBackground"];
    }
}

- (void)changeAutoSlow:(UISwitch*)sender {
    if ([sender isKindOfClass:[UISwitch class]]) {
        [[NSUserDefaults standardUserDefaults] setBool:sender.on forKey:@"autoSlow"];
    }
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 6;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    switch (section) {
        case SettingsSectionSpeed: {
            NSDictionary *capabilities = [selectedEmulatorBundle objectForInfoDictionaryKey:@"MNVMCapabilities"];
            BOOL hasAutoSlow = [capabilities[@"AutoSlow"] boolValue];
            return hasAutoSlow ? 3 : 2;
        }
        case SettingsSectionKeyboard:
            return keyboardLayouts.count;
        case SettingsSectionMachine:
            return machineList.count;
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
        case SettingsSectionDisplay:
            return NSLocalizedString(@"Display Scaling", nil);
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
    if (section == SettingsSectionSpeed) {
        return NSLocalizedString(@"Faster speeds and running in background drain the battery faster", nil);
    } else if (section == SettingsSectionMachine) {
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
        if (indexPath.row == 0) {
            cell = [tableView dequeueReusableCellWithIdentifier:@"speed" forIndexPath:indexPath];
            UISegmentedControl *speedControl = (UISegmentedControl*)[cell viewWithTag:128];
            EmulatorSpeed speedValue = [defaults integerForKey:@"speedValue"];
            speedControl.selectedSegmentIndex = speedValue == EmulatorSpeedAllOut ? speedControl.numberOfSegments - 1 : speedValue;
        } else if (indexPath.row == 1) {
            cell = [self switchCellForTableView:tableView indexPath:indexPath action:@selector(changeRunInBackground:) on:[defaults boolForKey:@"runInBackground"]];
            cell.textLabel.text = NSLocalizedString(@"Run in background", nil);
        } else if (indexPath.row == 2) {
            cell = [self switchCellForTableView:tableView indexPath:indexPath action:@selector(changeAutoSlow:) on:[defaults boolForKey:@"autoSlow"]];
            cell.textLabel.text = NSLocalizedString(@"AutoSlow", nil);
        }
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
        id item = machineList[indexPath.row];
        BOOL rowIsHeader = [item isKindOfClass:[NSString class]];
        BOOL rowHasHeader = [groupedEmulatorBundles containsObject:item];
        NSBundle *bundle = rowIsHeader ? machineList[indexPath.row + 1] : item;
        NSString *bundleName = bundle.bundlePath.lastPathComponent.stringByDeletingPathExtension;
        cell = [tableView dequeueReusableCellWithIdentifier:@"machine" forIndexPath:indexPath];
        if (rowIsHeader) {
            cell.textLabel.text = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
            cell.detailTextLabel.text = NSLocalizedString(@"multiple configurations available", nil);
        } else if (rowHasHeader) {
            cell.textLabel.text = [bundle objectForInfoDictionaryKey:@"CFBundleGetInfoString"];
            cell.detailTextLabel.text = nil;
        } else {
            cell.textLabel.text = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
            cell.detailTextLabel.text = [bundle objectForInfoDictionaryKey:@"CFBundleGetInfoString"];
        }
        
        if (rowHasHeader) {
            cell.imageView.image = nil;
            cell.indentationLevel = 1;
        } else {
            NSString *iconName = [NSString stringWithFormat:@"PlugIns/%@.mnvm/Icon", bundleName];
            cell.imageView.image = [UIImage imageNamed:iconName];
            cell.indentationLevel = 0;
        }
        cell.accessoryType = (item == selectedEmulatorBundle) ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
        cell.selectionStyle = rowIsHeader ? UITableViewCellSelectionStyleNone : UITableViewCellSelectionStyleDefault;
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
    } else if (section == SettingsSectionDisplay) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"display" forIndexPath:indexPath];
        UISegmentedControl *filterControl = (UISegmentedControl*)[cell viewWithTag:128];
        NSString *filter = [defaults stringForKey:@"screenFilter"];
        if ([filter isEqualToString:kCAFilterNearest]) {
            filterControl.selectedSegmentIndex = 0;
        } else if ([filter isEqualToString:kCAFilterTrilinear]) {
            filterControl.selectedSegmentIndex = 2;
        } else {
            filterControl.selectedSegmentIndex = 1;
        }
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
        NSBundle *bundle = machineList[indexPath.row];
        if (bundle == selectedEmulatorBundle || ![bundle isKindOfClass:[NSBundle class]]) {
            return;
        }
        NSString *bundleName = bundle.bundlePath.lastPathComponent.stringByDeletingPathExtension;
        [defaults setValue:bundleName forKey:@"machine"];
        NSUInteger lastSelectedIndex = [machineList indexOfObject:selectedEmulatorBundle];
        if (lastSelectedIndex != NSNotFound) {
            [tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:lastSelectedIndex inSection:SettingsSectionMachine]].accessoryType = UITableViewCellAccessoryNone;
        }
        UITableViewCell *currentCell = [tableView cellForRowAtIndexPath:indexPath];
        currentCell.accessoryType = UITableViewCellAccessoryCheckmark;
        selectedEmulatorBundle = bundle;
        [tableView reloadSections:[NSIndexSet indexSetWithIndex:SettingsSectionSpeed] withRowAnimation:UITableViewRowAnimationAutomatic];
    } else if (indexPath.section == SettingsSectionAbout) {
        // links in about
        NSString *linkURL = aboutItems[indexPath.row][@"link"];
        if (linkURL != nil) {
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:linkURL]];
        }
    }
}

- (UITableViewCell*)switchCellForTableView:(UITableView*)tableView indexPath:(NSIndexPath*)indexPath action:(SEL)action on:(BOOL)on {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"toggle" forIndexPath:indexPath];
    UISwitch *cellSwitch = (UISwitch*)cell.accessoryView;
    if (cellSwitch == nil) {
        cellSwitch = [UISwitch new];
        cell.accessoryView = cellSwitch;
    } else {
        [cellSwitch removeTarget:nil action:nil forControlEvents:UIControlEventAllEvents];
    }
    cellSwitch.on = on;
    [cellSwitch addTarget:self action:action forControlEvents:UIControlEventValueChanged];
    return cell;
}

@end
