//
//  SettingsViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 07/05/2016.
//  Copyright © 2016 namedfork. All rights reserved.
//

#import "SettingsViewController.h"
#import "AppDelegate.h"
#import "CNFGRAPI.h"

@interface SettingsViewController ()

@end

@implementation SettingsViewController
{
    NSArray *keyboardLayouts;
    NSString *aboutTitle;
    NSArray<NSDictionary<NSString*,NSString*>*> *aboutItems;
    UITextView *footerView;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    keyboardLayouts = [[NSBundle mainBundle] pathsForResourcesOfType:@"nfkeyboardlayout" inDirectory:@"Keyboard Layouts"];
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
    [AppDelegate sharedInstance].emulatorRunning = NO;
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [AppDelegate sharedInstance].emulatorRunning = YES;
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
    return 4;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 2) {
        return keyboardLayouts.count;
    } else if (section == 3) {
        return aboutItems.count;
    } else {
        return 1;
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    switch (section) {
        case 0: return NSLocalizedString(@"Speed", nil);
        case 1: return NSLocalizedString(@"Mouse Type", nil);
        case 2: return NSLocalizedString(@"Keyboard Layout", nil);
        case 3: return aboutTitle;
        default: return nil;
    }
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
    if (section == 3) {
        return footerView;
    } else {
        return nil;
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
    if (section == 3) {
        return footerView.bounds.size.height;
    } else {
        return 0.0;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = nil;
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSInteger section = indexPath.section;
    if (section == 0) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"speed" forIndexPath:indexPath];
        UISegmentedControl *speedControl = (UISegmentedControl*)[cell viewWithTag:128];
        speedControl.selectedSegmentIndex = [defaults integerForKey:@"speedValue"];
    } else if (section == 1) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"mouse" forIndexPath:indexPath];
        UISegmentedControl *mouseControl = (UISegmentedControl*)[cell viewWithTag:128];
        mouseControl.selectedSegmentIndex = [defaults boolForKey:@"trackpad"] ? 1 : 0;
    } else if (section == 2) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"keyboard" forIndexPath:indexPath];
        NSString *layout = keyboardLayouts[indexPath.row];
        cell.textLabel.text = layout.lastPathComponent.stringByDeletingPathExtension;
        BOOL selected = [[defaults stringForKey:@"keyboardLayout"] isEqualToString:layout.lastPathComponent];
        cell.accessoryType = selected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    } else if (section == 3) {
        cell = [tableView dequeueReusableCellWithIdentifier:@"about" forIndexPath:indexPath];
        NSDictionary<NSString*,NSString*> *item = aboutItems[indexPath.row];
        cell.textLabel.text = item[@"text"];
        NSString *detailText = item[@"detailText"];
        if ([detailText isEqualToString:@"$version"]) {
            NSString *versionString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
            NSString *commitString = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"GitVersion"];
            cell.detailTextLabel.text = commitString.length > 0 ? [NSString stringWithFormat:@"%@ (%@)", versionString, commitString] : versionString;
        } else if ([detailText isEqualToString:@"kAppVariationStr"]) {
            cell.detailTextLabel.text = @(kAppVariationStr);
        } else if ([detailText isEqualToString:@"kMaintainerName"]) {
            cell.detailTextLabel.text = @(kMaintainerName);
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
    if (indexPath.section == 2) {
        // selected keyboard layout
        NSString *layout = keyboardLayouts[indexPath.row];
        [defaults setValue:layout.lastPathComponent forKey:@"keyboardLayout"];
        [tableView reloadSections:[NSIndexSet indexSetWithIndex:indexPath.section] withRowAnimation:UITableViewRowAnimationAutomatic];
    } if (indexPath.section == 3) {
        // links in about
        NSString *linkURL = aboutItems[indexPath.row][@"link"];
        if (linkURL != nil) {
            [[UIApplication sharedApplication] openURL:[NSURL URLWithString:linkURL]];
        }
    }
}

@end
