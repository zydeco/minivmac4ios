//
//  InsertDiskViewController.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 07/05/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import "InsertDiskViewController.h"
#import "AppDelegate.h"
#import "UIImage+DiskImageIcon.h"

@interface InsertDiskViewController () <UITextFieldDelegate>

@end

@interface FileTableViewCell : UITableViewCell

@property (nonatomic, weak) InsertDiskViewController *controller;
@property (nonatomic, strong) NSString *filePath;
@property (nonatomic, assign) BOOL showExtension;

- (void)share:(id)sender;
- (void)rename:(id)sender;
- (void)delete:(id)sender;

@end

@implementation InsertDiskViewController
{
    NSString *basePath;
    NSArray<NSString*> *diskImages, *otherFiles;
    UIAlertController *createDiskImageController;
    __block __weak UITextField *sizeTextField;
    __block __weak UITextField *nameTextField;
    NSString *fileToRename;
    BOOL importing;
}

+ (void)initialize {
    UIMenuController *menuController = [UIMenuController sharedMenuController];
    menuController.menuItems = @[[[UIMenuItem alloc] initWithTitle:NSLocalizedString(@"Rename", @"Rename Context Menu Item") action:@selector(rename:)],
                                 [[UIMenuItem alloc] initWithTitle:NSLocalizedString(@"Share", @"Share Context Menu Item") action:@selector(share:)]];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    basePath = [AppDelegate sharedInstance].documentsPath;
    [self loadDirectoryContents];
    self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    importing = [[AppDelegate sharedEmulator].currentApplication isEqualToString:@"ImportFl"];
    if (importing) {
        self.title = NSLocalizedString(@"Import File", nil);
    }
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(reloadData:) name:[AppDelegate sharedEmulator].insertDiskNotification object:nil];
    [nc addObserver:self selector:@selector(reloadData:) name:[AppDelegate sharedEmulator].ejectDiskNotification object:nil];
    [nc addObserver:self selector:@selector(reloadData:) name:DocumentsChangedNotification object:nil];
    if ([UIApplication instancesRespondToSelector:@selector(btcMouseSetRawMode:)]) {
        [[UIApplication sharedApplication] btcMouseSetRawMode:NO];
    }
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc removeObserver:self name:[AppDelegate sharedEmulator].insertDiskNotification object:nil];
    [nc removeObserver:self name:[AppDelegate sharedEmulator].ejectDiskNotification object:nil];
    [nc removeObserver:self name:DocumentsChangedNotification object:nil];
}

- (void)loadDirectoryContents {
    NSArray *allFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:basePath error:NULL];
    diskImages = [allFiles filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"%@ containsObject: pathExtension.lowercaseString", [AppDelegate sharedInstance].diskImageExtensions]];
    otherFiles = [allFiles filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSString* _Nonnull name, NSDictionary<NSString *,id> * _Nullable bindings) {
        BOOL isDiskImage = [self->diskImages containsObject:name];
        BOOL isDirectory;
        BOOL isHidden = [name hasPrefix:@"."];
        [[NSFileManager defaultManager] fileExistsAtPath:[self->basePath stringByAppendingPathComponent:name] isDirectory:&isDirectory];
        return !(isDirectory || isDiskImage || isHidden);
    }]];
}

- (void)reloadData:(id)sender {
    [self loadDirectoryContents];
    [self.tableView reloadData];
}

- (IBAction)refresh:(id)sender {
    [self reloadData:sender];
    [self.refreshControl endRefreshing];
}

#pragma mark - Button Actions

- (void)showSettings:(id)sender {
    [[AppDelegate sharedInstance] showSettings:sender];
}

- (void)dismiss:(id)sender {
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)macInterrupt:(id)sender {
    [self dismiss:sender];
    [[AppDelegate sharedEmulator] interrupt];
}

- (void)macReset:(id)sender {
    [[AppDelegate sharedEmulator] reset];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return self.editing || importing ? 2 : 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    switch (section) {
        case 0: return diskImages.count + (self.editing ? 1 : 0);
        case 1: return otherFiles.count;
        default: return 0;
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return section == 0 ? NSLocalizedString(@"Disk Images", nil) : NSLocalizedString(@"Other Files", nil);
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return self.editing ? UITableViewAutomaticDimension : 0.0;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    [super setEditing:editing animated:animated];
    [self.tableView beginUpdates];
    [self loadDirectoryContents];
    self.tableView.tableHeaderView = nil;
    [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationFade];
    if (importing) {
        [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:1] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editing) {
        [self.tableView insertSections:[NSIndexSet indexSetWithIndex:1] withRowAnimation:UITableViewRowAnimationFade];
    } else {
        [self.tableView deleteSections:[NSIndexSet indexSetWithIndex:1] withRowAnimation:UITableViewRowAnimationFade];
    }
    [self.tableView endUpdates];
}

- (NSString *)fileAtIndexPath:(NSIndexPath *)indexPath {
    NSArray<NSString*> *files = indexPath.section == 0 ? diskImages : otherFiles;
    if (indexPath.row < files.count) {
        NSString *fileName = files[indexPath.row];
        return [basePath stringByAppendingPathComponent:fileName];
    } else {
        return nil;
    }
}

- (NSIndexPath *)indexPathForFile:(NSString *)filePath {
    NSString *fileName = filePath.lastPathComponent;
    if ([diskImages containsObject:fileName]) {
        return [NSIndexPath indexPathForRow:[diskImages indexOfObject:fileName] inSection:0];
    } else if ([otherFiles containsObject:fileName]) {
        return [NSIndexPath indexPathForRow:[otherFiles indexOfObject:fileName] inSection:1];
    } else {
        return nil;
    }
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    FileTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"cell" forIndexPath:indexPath];
    NSString *filePath = [self fileAtIndexPath:indexPath];
    if (filePath) {
        cell.showExtension = self.editing || importing;
        cell.filePath = filePath;
        if ([[AppDelegate sharedEmulator] isDiskInserted:filePath]) {
            cell.userInteractionEnabled = NO;
            cell.textLabel.enabled = NO;
            cell.imageView.alpha = 0.5;
        }
    } else {
        cell.textLabel.text = NSLocalizedString(@"Create Disk Image…", nil);
        cell.detailTextLabel.text = nil;
    }
    cell.controller = self;
    return cell;
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (self.editing == NO) {
        return UITableViewCellEditingStyleNone;
    } else if (indexPath.section == 0) {
        if (indexPath.row == diskImages.count) {
            return UITableViewCellEditingStyleInsert;
        }
        NSString *filePath = [self fileAtIndexPath:indexPath];
        BOOL isInserted = [[AppDelegate sharedEmulator] isDiskInserted:filePath];
        return isInserted ? UITableViewCellEditingStyleNone : UITableViewCellEditingStyleDelete;
    } else {
        return UITableViewCellEditingStyleDelete;
    }
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        NSString *filePath = [self fileAtIndexPath:indexPath];
        if ([UIAlertController class]) {
            [self askDeleteFile:filePath sourceView:[tableView cellForRowAtIndexPath:indexPath]];
        } else {
            [self deleteFile:filePath];
        }
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        [self createDiskImage];
    }
}

- (BOOL)tableView:(UITableView *)tableView shouldShowMenuForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    if (@available(iOS 13, *)) {
        return NO;
    }
    return [self fileAtIndexPath:indexPath] != nil;
}

- (BOOL)tableView:(UITableView *)tableView canPerformAction:(SEL)action forRowAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender {
    return (action == @selector(share:) || (action == @selector(rename:) || ([UIAlertController class] && action == @selector(delete:))));
}

- (void)tableView:(UITableView *)tableView performAction:(SEL)action forRowAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender {
    // menu will not be shown if this method doesn't exist
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    NSString *filePath = [self fileAtIndexPath:indexPath];
    BOOL hadAnyDisksInserted = [AppDelegate sharedEmulator].anyDiskInserted;
    if ([self insertDisk:filePath]) {
        [self dismissViewControllerAnimated:YES completion:nil];
        if (!hadAnyDisksInserted) {
            // Add to quick actions if it was booted from (no disks inserted previously and still inserted after 10s)
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                id<Emulator> emulator = [AppDelegate sharedEmulator];
                if (emulator.isRunning && [emulator isDiskInserted:filePath]) {
                    [self updateApplicationShortcutsWithDisk:filePath];
                }
            });
        }
    }
}

#pragma mark - Quick Actions

- (void)updateApplicationShortcutsWithDisk:(NSString*)filePath {
    // Update user defaults
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    NSString *fileName = filePath.lastPathComponent;
    NSMutableArray *recentDisks = [userDefaults arrayForKey:@"recentDisks"].mutableCopy;
    if ([recentDisks containsObject:fileName]) {
        return;
    }
    [recentDisks insertObject:fileName atIndex:0];
    if (recentDisks.count > 4) {
        [recentDisks removeObjectsInRange:NSMakeRange(4, recentDisks.count - 4)];
    }
    [userDefaults setObject:recentDisks forKey:@"recentDisks"];
    
    // Update quick actions
    NSMutableArray *shortcutItems = [NSMutableArray arrayWithCapacity:4];
    for (NSString *diskName in recentDisks) {
        UIApplicationShortcutItem *shortcutItem = [self shortcutItemForDisk:diskName];
        if (shortcutItem) {
            [shortcutItems addObject:shortcutItem];
        }
    }
    [UIApplication sharedApplication].shortcutItems = shortcutItems;
}

- (nullable UIApplicationShortcutItem*)shortcutItemForDisk:(NSString*)fileName {
    BOOL isDiskImage = [[AppDelegate sharedInstance].diskImageExtensions containsObject:fileName.pathExtension.lowercaseString];
    if (!isDiskImage) {
        return nil;
    }
    NSString *filePath = [basePath stringByAppendingPathComponent:fileName];
    NSString *title = [fileName stringByDeletingPathExtension];
    UIApplicationShortcutIcon *icon = nil;
    NSDictionary *attributes = [[NSURL fileURLWithPath:filePath] resourceValuesForKeys:@[NSURLTotalFileSizeKey, NSURLFileSizeKey] error:NULL];
    if (attributes && attributes[NSURLTotalFileSizeKey]) {
        NSInteger fileSize = [attributes[NSURLTotalFileSizeKey] integerValue];
        NSInteger numBlocks = fileSize / 512;
        icon = [UIApplicationShortcutIcon iconWithTemplateImageName:numBlocks == 800 || numBlocks == 1600 ? @"floppy" : @"floppyV"];
    } else {
        return nil;
    }
    return [[UIApplicationShortcutItem alloc] initWithType:@"disk" localizedTitle:title localizedSubtitle:nil icon:icon userInfo:@{@"disk": fileName}];
}

#pragma mark - File Actions

- (BOOL)insertDisk:(NSString*)filePath {
    id<Emulator> emulator = [AppDelegate sharedEmulator];
    if (filePath && ![emulator isDiskInserted:filePath]) {
        return [emulator insertDisk:filePath];
    }
    return NO;
}

- (void)deleteFile:(NSString*)filePath {
    NSError *error = nil;
    if ([[NSFileManager defaultManager] removeItemAtPath:filePath error:&error]) {
        NSIndexPath *indexPath = [self indexPathForFile:filePath];
        [self loadDirectoryContents];
        [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else {
        [[AppDelegate sharedInstance] showAlertWithTitle:NSLocalizedString(@"Could not delete file", nil) message:error.localizedDescription];
    }
}

- (void)askDeleteFile:(NSString*)filePath sourceView:(UIView*)sourceView {
    NSString *fileName = filePath.lastPathComponent;
    NSString *message = [NSString stringWithFormat:NSLocalizedString(@"Are you sure you want to delete %@? This operation cannot be undone.", nil), fileName];
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Delete File", nil) message:message preferredStyle:UIAlertControllerStyleActionSheet];
    [alertController addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil) style:UIAlertActionStyleCancel handler:nil]];
    [alertController addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Delete", nil) style:UIAlertActionStyleDestructive handler:^(UIAlertAction * _Nonnull action) {
        [self deleteFile:filePath];
    }]];
    alertController.popoverPresentationController.sourceView = sourceView;
    [self presentViewController:alertController animated:YES completion:nil];
}

- (void)askRenameFile:(NSString*)filePath {
    NSString *fileName = filePath.lastPathComponent;
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:fileName message:NSLocalizedString(@"Enter new name", nil) preferredStyle:UIAlertControllerStyleAlert];
    [alertController addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
        self->nameTextField = textField;
        textField.delegate = self;
        textField.placeholder = fileName;
        textField.text = fileName;
    }];
    [alertController addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil) style:UIAlertActionStyleCancel handler:nil]];
    [alertController addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Rename", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        NSString *newName = alertController.textFields.firstObject.text;
        [self renameFile:filePath toName:newName];
    }]];
    [self presentViewController:alertController animated:YES completion:nil];
}

- (void)renameFile:(NSString*)filePath toName:(NSString*)newName {
    NSError *error = nil;
    NSString *newPath = [filePath.stringByDeletingLastPathComponent stringByAppendingPathComponent:newName];
    if ([[NSFileManager defaultManager] moveItemAtPath:filePath toPath:newPath error:&error]) {
        NSIndexPath *oldIndexPath = [self indexPathForFile:filePath];
        [self loadDirectoryContents];
        NSIndexPath *newIndexPath = [self indexPathForFile:newPath];
        if (newIndexPath == nil || newIndexPath.section >= self.tableView.numberOfSections) {
            [self.tableView deleteRowsAtIndexPaths:@[oldIndexPath] withRowAnimation:UITableViewRowAnimationFade];
        } else {
            [self.tableView moveRowAtIndexPath:oldIndexPath toIndexPath:newIndexPath];
            [self.tableView reloadRowsAtIndexPaths:@[newIndexPath] withRowAnimation:UITableViewRowAnimationNone];
        }
    } else {
        [[AppDelegate sharedInstance] showAlertWithTitle:NSLocalizedString(@"Could not rename file", nil) message:error.localizedDescription];
    }
}

#pragma mark - Disk Image Creation

- (void)createDiskImage {
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Create Disk Image", nil) message:nil preferredStyle:UIAlertControllerStyleAlert];
    [alertController addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
        textField.placeholder = NSLocalizedString(@"name", nil);
        [textField addTarget:self action:@selector(validateCreateDiskImageInput:) forControlEvents:UIControlEventAllEditingEvents];
    }];
    
    [alertController addTextFieldWithConfigurationHandler:^(UITextField * _Nonnull textField) {
        [self _configureNewDiskSizeField:textField];
    }];
    
    [alertController addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil) style:UIAlertActionStyleCancel handler:nil]];
    UIAlertAction *createAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Create", nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
        NSString *name = [self _newDiskImageName];
        off_t size = [self _newDiskImageSize];
        self->createDiskImageController = nil;
        [self createDiskImageWithName:name size:size];
    }];
    [alertController addAction:createAction];
    createAction.enabled = NO;
    [self presentViewController:alertController animated:YES completion:nil];
    createDiskImageController = alertController;
}

- (void)_configureNewDiskSizeField:(UITextField*)textField {
    textField.secureTextEntry = NO;
    textField.placeholder = NSLocalizedString(@"size", nil);
    textField.keyboardType = UIKeyboardTypeDecimalPad;
    UILabel *unitLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 60.0, 32.0)];
    textField.rightViewMode = UITextFieldViewModeAlways;
    textField.rightView = unitLabel;
    unitLabel.textAlignment = NSTextAlignmentRight;
    UISegmentedControl *unitsControl = [[UISegmentedControl alloc] initWithFrame:CGRectMake(0, 0, 80.0, 16.0)];
    NSArray *units = @[NSLocalizedString(@"KB", nil), NSLocalizedString(@"MB", nil)];
    [units enumerateObjectsUsingBlock:^(NSString *title, NSUInteger idx, BOOL * _Nonnull stop) {
        [unitsControl insertSegmentWithTitle:title atIndex:idx animated:NO];
    }];
    unitsControl.selectedSegmentIndex = 0;
    textField.rightView = unitsControl;
    sizeTextField = textField;
    textField.delegate = self;
    [textField addTarget:self action:@selector(validateCreateDiskImageInput:) forControlEvents:UIControlEventAllEditingEvents];
    [unitsControl addTarget:self action:@selector(validateCreateDiskImageInput:) forControlEvents:UIControlEventValueChanged];
    unitLabel.text = [unitsControl titleForSegmentAtIndex:unitsControl.selectedSegmentIndex];
}

- (BOOL)validateCreateDiskImageInput:(id)sender {
    BOOL valid = NO;
    if (self.presentedViewController == createDiskImageController) {
        NSString *name = [self _newDiskImageName];
        BOOL nameIsValid = (name.length > 0) && ![name hasPrefix:@"."] && ![name containsString:@"/"] && ![name containsString:@"*"];
        
        off_t size = [self _newDiskImageSize];
        BOOL sizeIsValid = (size >= 400 * 1024) && (size <= 2LL * 1024 * 1024 * 1024);
        
        valid = nameIsValid && sizeIsValid;
        if (createDiskImageController != nil) {
            UIAlertAction *createAction = createDiskImageController.actions[1];
            createAction.enabled = valid;
        } else if (sender == sizeTextField.rightView) {
            // fake edit event to call alertViewShouldEnableFirstOtherButton
            [sizeTextField sendActionsForControlEvents:UIControlEventEditingChanged];
        }
    }
    return valid;
}

- (NSString*)_newDiskImageName {
    if (createDiskImageController != nil) {
        return createDiskImageController.textFields[0].text;
    } else {
        return nil;
    }
}

- (off_t)_newDiskImageSize {
    if (createDiskImageController == nil) {
        return 0;
    }
    UISegmentedControl *unitsControl = (UISegmentedControl*)sizeTextField.rightView;
    off_t unitsMultiplier = (unitsControl.selectedSegmentIndex == 0) ? 1024 : 1024 * 1024;
    off_t size = sizeTextField.text.floatValue * unitsMultiplier;
    return size;
}

- (void)createDiskImageWithName:(NSString*)name size:(off_t)size {
    NSString *imageFileName = [[AppDelegate sharedInstance].diskImageExtensions containsObject:name.pathExtension.lowercaseString] ? name : [name stringByAppendingPathExtension:@"img"];
    NSString *imagePath = [basePath stringByAppendingPathComponent:imageFileName];
    
    int fd = open(imagePath.fileSystemRepresentation, O_CREAT | O_TRUNC | O_EXCL | O_WRONLY, 0666);
    if (fd == -1) {
        [[AppDelegate sharedInstance] showAlertWithTitle:NSLocalizedString(@"Could not create disk image", nil) message:[[NSString alloc] initWithUTF8String:strerror(errno)]];
        return;
    }
    
    [self showFileActivityIndicatorWithTitle:NSLocalizedString(@"Creating Disk Image", nil) completion:^(UIActivityIndicatorView *activityIndicatorView) {
        [self _writeNewDiskImage:fd size:size activityIndicator:activityIndicatorView];
    }];
}

- (void)showFileActivityIndicatorWithTitle:(NSString*)title completion:(void (^_Nonnull)(UIActivityIndicatorView* activityIndicatorView))completionBlock {
    UIActivityIndicatorView *activityIndicatorView = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleWhiteLarge];
    if (@available(iOS 13, *)) {
        activityIndicatorView.color = [UIColor labelColor];
    } else {
        activityIndicatorView.color = [UIColor blackColor];
    }
    UIAlertController *alertController = [UIAlertController alertControllerWithTitle:title message:@"\n\n\n" preferredStyle:UIAlertControllerStyleAlert];
    [self presentViewController:alertController animated:true completion:^{
        UIView *alertView = alertController.view;
        activityIndicatorView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin | UIViewAutoresizingFlexibleBottomMargin;
        activityIndicatorView.center = CGPointMake(alertView.bounds.size.width / 2.0, alertView.bounds.size.height / 2.0 + 32.0);
        [alertView addSubview:activityIndicatorView];
        [activityIndicatorView startAnimating];
        completionBlock(activityIndicatorView);
    }];
}

- (void)dismissFileActivityIndicator:(UIActivityIndicatorView*)activityIndicatorView withErrorTitle:(NSString*)errorTitle message:(NSString*)errorMessage {
    dispatch_async(dispatch_get_main_queue(), ^{
        [activityIndicatorView stopAnimating];
        [self dismissViewControllerAnimated:YES completion:^{
            if (errorTitle) {
                [[AppDelegate sharedInstance] showAlertWithTitle:errorTitle message:errorMessage];
            }
        }];
        [self.tableView beginUpdates];
        [self loadDirectoryContents];
        [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationFade];
        [self.tableView endUpdates];
    });
}

- (void)_writeNewDiskImage:(int)fd size:(off_t)size activityIndicator:(UIActivityIndicatorView*)activityIndicatorView {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        int error = 0;
        if (ftruncate(fd, size)) {
            error = errno;
        }
        close(fd);
        [self dismissFileActivityIndicator:activityIndicatorView
                            withErrorTitle:error ? NSLocalizedString(@"Could not create disk image", nil) : nil
                                   message:error ? @(strerror(error)) : nil];
    });
}

- (NSString*)nameForDuplicatingPath:(NSString*)filePath {
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *fileName = filePath.lastPathComponent;
    NSString *copyPath = [NSString stringWithFormat:@"%@/%@ copy.%@", filePath.stringByDeletingLastPathComponent, fileName.lastPathComponent.stringByDeletingPathExtension, fileName.pathExtension];
    for (int copyNumber = 2; [fileManager fileExistsAtPath:copyPath]; copyNumber++) {
        copyPath = [NSString stringWithFormat:@"%@/%@ copy %d.%@", filePath.stringByDeletingLastPathComponent, fileName.lastPathComponent.stringByDeletingPathExtension, copyNumber, fileName.pathExtension];
    }
    return copyPath;
}

- (void)duplicateDiskImage:(NSString*)filePath {
    [self showFileActivityIndicatorWithTitle:NSLocalizedString(@"Duplicating disk image", nil) completion:^(UIActivityIndicatorView *activityIndicatorView) {
        [self _duplicateDiskImage:filePath activityIndicator:activityIndicatorView];
    }];
}

- (void)_duplicateDiskImage:(NSString*)filePath activityIndicator:(UIActivityIndicatorView*)activityIndicatorView {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSError *error = nil;
        [[NSFileManager defaultManager] copyItemAtPath:filePath toPath:[self nameForDuplicatingPath:filePath] error:&error];
        dispatch_async(dispatch_get_main_queue(), ^{
            [activityIndicatorView stopAnimating];
            [self dismissViewControllerAnimated:YES completion:^{
                if (error) {
                    [[AppDelegate sharedInstance] showAlertWithTitle:NSLocalizedString(@"Could not duplicate disk image", nil) message:error.localizedDescription];
                }
            }];
            [self.tableView beginUpdates];
            [self loadDirectoryContents];
            [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationFade];
            [self.tableView endUpdates];
        });
    });
}

#pragma mark - Text Field Delegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    if (textField == sizeTextField) {
        UISegmentedControl *unitsControl = (UISegmentedControl*)textField.rightView;
        NSArray *unitShortcuts = @[@"k", @"m"];
        if (string.length == 0) {
            return YES;
        } else if (string.length == 1 && [unitShortcuts indexOfObject:string.lowercaseString] != NSNotFound) {
            unitsControl.selectedSegmentIndex = [unitShortcuts indexOfObject:string.lowercaseString];
            [unitsControl sendActionsForControlEvents:UIControlEventValueChanged];
            return NO;
        } else {
            NSString *newString = [textField.text stringByReplacingCharactersInRange:range withString:string];
            NSScanner *scanner = [NSScanner scannerWithString:newString];
            double value;
            return [scanner scanDouble:&value] && scanner.isAtEnd && value >= 0;
        }
    } else {
        return YES;
    }
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
    if (textField == nameTextField && textField.text.pathExtension.length > 0) {
        UITextPosition *beforeExtensionPosition = [textField positionFromPosition:textField.endOfDocument offset:-(textField.text.pathExtension.length + 1)];
        UITextRange *nameWithoutExtensionRange = [textField textRangeFromPosition:textField.beginningOfDocument toPosition:beforeExtensionPosition];
        [textField performSelector:@selector(setSelectedTextRange:) withObject:nameWithoutExtensionRange afterDelay:0.1];
    }
}

#pragma mark - Context Menu

- (UIContextMenuConfiguration *)tableView:(UITableView *)tableView contextMenuConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath point:(CGPoint)point API_AVAILABLE(ios(13.0)) {
    FileTableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
    if (cell.filePath == nil) {
        return nil;
    }
    
    UIAction *insertAction = [UIAction actionWithTitle:@"Insert" image:[UIImage systemImageNamed:@"play.fill"] identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
        [self insertDisk:cell.filePath];
    }];
    UIAction *deleteAction = [UIAction actionWithTitle:@"Delete" image:[UIImage systemImageNamed:@"trash"] identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
        [cell delete:self];
    }];
    
    deleteAction.attributes = UIMenuElementAttributesDestructive;
    if ([[AppDelegate sharedEmulator] isDiskInserted:cell.filePath]) {
        insertAction.attributes = UIMenuElementAttributesDisabled;
        deleteAction.attributes = UIMenuElementAttributesDisabled;
    }
    
    NSArray<UIAction*> *actions = @[
        insertAction,
        [UIAction actionWithTitle:@"Rename" image:[UIImage systemImageNamed:@"pencil"] identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            [cell rename:self];
        }],
        [UIAction actionWithTitle:@"Duplicate" image:[UIImage systemImageNamed:@"plus.square.on.square"] identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            [self duplicateDiskImage:cell.filePath];
        }],
        [UIAction actionWithTitle:@"Share" image:[UIImage systemImageNamed:@"square.and.arrow.up"] identifier:nil handler:^(__kindof UIAction * _Nonnull action) {
            [cell share:self];
        }],
        deleteAction
    ];
    return [UIContextMenuConfiguration configurationWithIdentifier:nil previewProvider:nil actionProvider:^UIMenu * _Nullable(NSArray<UIMenuElement *> * _Nonnull suggestedActions) {
        return [UIMenu menuWithTitle:@"" children:actions];
    }];
}

@end

@implementation FileTableViewCell
{
    UIImage *defaultIcon;
}

- (void)prepareForReuse {
    [super prepareForReuse];
    self.userInteractionEnabled = YES;
    self.textLabel.text = nil;
    self.textLabel.enabled = YES;
    self.imageView.image = nil;
    self.imageView.alpha = 1.0;
    self.detailTextLabel.text = nil;
    if (_filePath) {
        [[NSNotificationCenter defaultCenter] removeObserver:self name:DidUpdateIconForDiskImageNotification object:_filePath];
        _filePath = nil;
    }
}

- (void)setFilePath:(NSString *)filePath {
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    if (_filePath) {
        [notificationCenter removeObserver:self name:DidUpdateIconForDiskImageNotification object:_filePath];
    }
    _filePath = filePath;
    if (_filePath) {
        [notificationCenter addObserver:self selector:@selector(didUpdateDiskIcon:) name:DidUpdateIconForDiskImageNotification object:_filePath];
    }
    NSString *fileName = filePath.lastPathComponent;
    self.textLabel.text = self.showExtension ? fileName : fileName.stringByDeletingPathExtension;
    NSDictionary *attributes = [[NSURL fileURLWithPath:filePath] resourceValuesForKeys:@[NSURLTotalFileSizeKey, NSURLFileSizeKey] error:NULL];
    if (attributes && attributes[NSURLTotalFileSizeKey]) {
        BOOL isDiskImage = [[AppDelegate sharedInstance].diskImageExtensions containsObject:fileName.pathExtension.lowercaseString];
        if (isDiskImage) {
            NSInteger fileSize = [attributes[NSURLTotalFileSizeKey] integerValue];
            NSInteger numBlocks = fileSize / 512;
            defaultIcon = [UIImage imageNamed:numBlocks == 800 || numBlocks == 1600 ? @"floppy" : @"floppyV"];
            self.imageView.image = [UIImage imageWithIconForDiskImage:filePath] ?: defaultIcon;
        } else {
            defaultIcon = [UIImage imageNamed:@"document"];
            self.imageView.image = defaultIcon;
        }
        NSString *sizeString = [NSByteCountFormatter stringFromByteCount:[attributes[NSURLTotalFileSizeKey] longLongValue] countStyle:NSByteCountFormatterCountStyleBinary];
        self.detailTextLabel.text = sizeString;
    } else {
        self.imageView.image = nil;
        self.detailTextLabel.text = nil;
    }
}

- (void)didUpdateDiskIcon:(NSNotification*)notification {
    if ([_filePath isEqual:notification.object]) {
        UIImage *icon = notification.userInfo[@"icon"];
        self.imageView.image = icon ?: defaultIcon;
    }
}

- (void)share:(id)sender {
    UIActivityViewController *avc = [[UIActivityViewController alloc] initWithActivityItems:@[[NSURL fileURLWithPath:self.filePath]] applicationActivities:nil];
    if ([avc respondsToSelector:@selector(popoverPresentationController)]) {
        avc.modalPresentationStyle = UIModalPresentationPopover;
        avc.popoverPresentationController.sourceView = self.imageView;
        avc.popoverPresentationController.sourceRect = self.imageView.bounds;
    }
    [self.controller presentViewController:avc animated:YES completion:nil];
}

- (void)rename:(id)sender {
    [self.controller askRenameFile:self.filePath];
}

- (void)delete:(id)sender {
    [self.controller askDeleteFile:self.filePath sourceView:self];
}

@end
