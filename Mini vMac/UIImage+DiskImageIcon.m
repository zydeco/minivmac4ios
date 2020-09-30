//
//  UIImage+DiskImageIcon.m
//  Mini vMac
//
//  Created by Jesús A. Álvarez on 21/05/2016.
//  Copyright © 2016-2018 namedfork. All rights reserved.
//

#import "UIImage+DiskImageIcon.h"
#import "libhfs.h"
#import "res.h"
#import "mfs.h"
#import "AppDelegate.h"
#import <sys/xattr.h>

NSString *DidUpdateIconForDiskImageNotification = @"didUpdateIconForDiskImage";
static const char kDiskImageIconAttributeName[] = "net.namedfork.DiskImageIcon";

#define kDiskImageHasDC42Header 1 << 0
#define RSHORT(base, offset) ntohs(*((short *)((base) + (offset))))
#define RLONG(base, offset) ntohl(*((long *)((base) + (offset))))
#define RCSTR(base, offset) ((char *)((base) + (offset)))

@interface DiskImageIconReader : NSObject

- (UIImage *)iconForDiskImage:(NSString *)path;

@end

@implementation UIImage (DiskImageIcon)

+ (void)_didEjectDisk:(NSNotification*)notification {
    NSString *path = [notification.userInfo[@"path"] stringByStandardizingPath];
    [self loadIconForDiskImageAndNotify:path];
}

+ (UIImage *)imageWithIconForDiskImage:(NSString *)path {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didEjectDisk:) name:[AppDelegate sharedEmulator].ejectDiskNotification object:nil];
    });
    
    // check attribute
    ssize_t attrLen = getxattr(path.fileSystemRepresentation, kDiskImageIconAttributeName, NULL, 0, 0, 0);
    if (attrLen == -1) {
        [self loadIconForDiskImageAndNotify:path];
        return nil;
    }
    
    // read data
    void *attrData = malloc(attrLen);
    getxattr(path.fileSystemRepresentation, kDiskImageIconAttributeName, attrData, attrLen, 0, 0);
    return [UIImage imageWithData:[NSData dataWithBytesNoCopy:attrData length:attrLen freeWhenDone:YES]];
}

+ (void)loadIconForDiskImageAndNotify:(NSString *)path {
    if ([NSThread isMainThread]) {
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            [self loadIconForDiskImageAndNotify:path];
        });
        return;
    }
    
    // get current value
    ssize_t attrLen = getxattr(path.fileSystemRepresentation, kDiskImageIconAttributeName, NULL, 0, 0, 0);
    NSData *previousData = nil;
    if (attrLen != -1) {
        void *attrData = malloc(attrLen);
        getxattr(path.fileSystemRepresentation, kDiskImageIconAttributeName, attrData, attrLen, 0, 0);
        previousData = [NSData dataWithBytesNoCopy:attrData length:attrLen freeWhenDone:YES];
    }
    
    // load new icon
    UIImage *icon = [[DiskImageIconReader new] iconForDiskImage:path];
    NSData *newData = UIImagePNGRepresentation(icon);
    if (![newData isEqualToData:previousData]) {
        // save new icon and notify
        setxattr(path.fileSystemRepresentation, kDiskImageIconAttributeName, newData.bytes, newData.length, 0, 0);
        NSNotification *newIconNotification = [[NSNotification alloc] initWithName:DidUpdateIconForDiskImageNotification object:path userInfo:icon ? @{@"icon": icon} : nil];
        [[NSNotificationCenter defaultCenter] performSelectorOnMainThread:@selector(postNotification:) withObject:newIconNotification waitUntilDone:NO];
    }
}

@end

// Mac OS 1 bit palette
static uint32_t ctb1[2] = {0xFFFFFF, 0x000000};

// Mac OS 4 bit palette
static uint32_t ctb4[16] = {
    0xFFFFFF, 0xFFFF00, 0xFF6600, 0xDD0000, 0xFF0099, 0x330099, 0x0000DD, 0x0099FF,
    0x00BB00, 0x006600, 0x663300, 0x996633, 0xCCCCCC, 0x888888, 0x444444, 0x000000};
// Mac OS 8 bit palette
static uint32_t ctb8[256] = {
    0xFFFFFF, 0xFFFFCC, 0xFFFF99, 0xFFFF66, 0xFFFF33, 0xFFFF00, 0xFFCCFF, 0xFFCCCC,
    0xFFCC99, 0xFFCC66, 0xFFCC33, 0xFFCC00, 0xFF99FF, 0xFF99CC, 0xFF9999, 0xFF9966,
    0xFF9933, 0xFF9900, 0xFF66FF, 0xFF66CC, 0xFF6699, 0xFF6666, 0xFF6633, 0xFF6600,
    0xFF33FF, 0xFF33CC, 0xFF3399, 0xFF3366, 0xFF3333, 0xFF3300, 0xFF00FF, 0xFF00CC,
    0xFF0099, 0xFF0066, 0xFF0033, 0xFF0000, 0xCCFFFF, 0xCCFFCC, 0xCCFF99, 0xCCFF66,
    0xCCFF33, 0xCCFF00, 0xCCCCFF, 0xCCCCCC, 0xCCCC99, 0xCCCC66, 0xCCCC33, 0xCCCC00,
    0xCC99FF, 0xCC99CC, 0xCC9999, 0xCC9966, 0xCC9933, 0xCC9900, 0xCC66FF, 0xCC66CC,
    0xCC6699, 0xCC6666, 0xCC6633, 0xCC6600, 0xCC33FF, 0xCC33CC, 0xCC3399, 0xCC3366,
    0xCC3333, 0xCC3300, 0xCC00FF, 0xCC00CC, 0xCC0099, 0xCC0066, 0xCC0033, 0xCC0000,
    0x99FFFF, 0x99FFCC, 0x99FF99, 0x99FF66, 0x99FF33, 0x99FF00, 0x99CCFF, 0x99CCCC,
    0x99CC99, 0x99CC66, 0x99CC33, 0x99CC00, 0x9999FF, 0x9999CC, 0x999999, 0x999966,
    0x999933, 0x999900, 0x9966FF, 0x9966CC, 0x996699, 0x996666, 0x996633, 0x996600,
    0x9933FF, 0x9933CC, 0x993399, 0x993366, 0x993333, 0x993300, 0x9900FF, 0x9900CC,
    0x990099, 0x990066, 0x990033, 0x990000, 0x66FFFF, 0x66FFCC, 0x66FF99, 0x66FF66,
    0x66FF33, 0x66FF00, 0x66CCFF, 0x66CCCC, 0x66CC99, 0x66CC66, 0x66CC33, 0x66CC00,
    0x6699FF, 0x6699CC, 0x669999, 0x669966, 0x669933, 0x669900, 0x6666FF, 0x6666CC,
    0x666699, 0x666666, 0x666633, 0x666600, 0x6633FF, 0x6633CC, 0x663399, 0x663366,
    0x663333, 0x663300, 0x6600FF, 0x6600CC, 0x660099, 0x660066, 0x660033, 0x660000,
    0x33FFFF, 0x33FFCC, 0x33FF99, 0x33FF66, 0x33FF33, 0x33FF00, 0x33CCFF, 0x33CCCC,
    0x33CC99, 0x33CC66, 0x33CC33, 0x33CC00, 0x3399FF, 0x3399CC, 0x339999, 0x339966,
    0x339933, 0x339900, 0x3366FF, 0x3366CC, 0x336699, 0x336666, 0x336633, 0x336600,
    0x3333FF, 0x3333CC, 0x333399, 0x333366, 0x333333, 0x333300, 0x3300FF, 0x3300CC,
    0x330099, 0x330066, 0x330033, 0x330000, 0x00FFFF, 0x00FFCC, 0x00FF99, 0x00FF66,
    0x00FF33, 0x00FF00, 0x00CCFF, 0x00CCCC, 0x00CC99, 0x00CC66, 0x00CC33, 0x00CC00,
    0x0099FF, 0x0099CC, 0x009999, 0x009966, 0x009933, 0x009900, 0x0066FF, 0x0066CC,
    0x006699, 0x006666, 0x006633, 0x006600, 0x0033FF, 0x0033CC, 0x003399, 0x003366,
    0x003333, 0x003300, 0x0000FF, 0x0000CC, 0x000099, 0x000066, 0x000033, 0xEE0000,
    0xDD0000, 0xBB0000, 0xAA0000, 0x880000, 0x770000, 0x550000, 0x440000, 0x220000,
    0x110000, 0x00EE00, 0x00DD00, 0x00BB00, 0x00AA00, 0x008800, 0x007700, 0x005500,
    0x004400, 0x002200, 0x001100, 0x0000EE, 0x0000DD, 0x0000BB, 0x0000AA, 0x000088,
    0x000077, 0x000055, 0x000044, 0x000022, 0x000011, 0xEEEEEE, 0xDDDDDD, 0xBBBBBB,
    0xAAAAAA, 0x888888, 0x777777, 0x555555, 0x444444, 0x222222, 0x111111, 0x000000};

static uint8_t maskReplacement[][128] = {
    {   0x1D, 0xC7, 0xFC, 0x00, 0x3F, 0xE7, 0xFC, 0x00, 0x7F, 0xF6, 0xAC, 0x00,
        0x3F, 0xF7, 0x5C, 0x00, 0x17, 0xC6, 0x0C, 0x00, 0x00, 0x02, 0xAB, 0xFE,
        0x0F, 0xFF, 0x5B, 0xFE, 0x0F, 0xF9, 0xFB, 0xAA, 0x0F, 0x5D, 0xFE, 0x02,
        0x0A, 0xA8, 0x01, 0xAC, 0x04, 0x00, 0x00, 0x5C, 0x02, 0xBA, 0xAA, 0x2C,
        0x03, 0xD5, 0x54, 0x5C, 0x02, 0xBA, 0xAA, 0x2C, 0x7E, 0x17, 0x5C, 0x5C,
        0x6A, 0xBA, 0xAA, 0x2C, 0x75, 0x55, 0x54, 0x3C, 0x7A, 0x6A, 0xEA, 0x20,
        0x70, 0x95, 0xB5, 0x60, 0x1A, 0x2A, 0xAA, 0xE0, 0x1D, 0x95, 0x15, 0x60,
        0x1A, 0xAB, 0x1A, 0xE0, 0x1D, 0x95, 0x15, 0x60, 0x1A, 0xAB, 0x1A, 0xE0,
        0x1D, 0x95, 0x15, 0xE0, 0x3E, 0xAB, 0x1B, 0x70, 0x30, 0x45, 0x10, 0x38,
        0x20, 0x08, 0x08, 0x3C, 0x68, 0x03, 0xF8, 0x16, 0xC0, 0x00, 0x00, 0x02,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE
    }, {
        0x1D, 0xC7, 0xFC, 0x00, 0x3F, 0xE7, 0xFC, 0x00, 0x7F, 0xF7, 0xFC, 0x00,
        0x3F, 0xF7, 0xFC, 0x00, 0x17, 0xC7, 0xFC, 0x00, 0x00, 0x03, 0xFB, 0xFE,
        0x0F, 0xFF, 0xFF, 0xFE, 0x0F, 0xFF, 0xFF, 0xFE, 0x0F, 0xFF, 0xFF, 0xFE,
        0x0F, 0xFF, 0xFF, 0xFC, 0x07, 0xFF, 0xFF, 0xFC, 0x03, 0xFF, 0xFF, 0xFC,
        0x03, 0xFF, 0xFF, 0xFC, 0x03, 0xFF, 0xFF, 0xFC, 0x7F, 0xFF, 0xFF, 0xFC,
        0x7F, 0xFF, 0xFF, 0xFC, 0x7F, 0xFF, 0xFF, 0xFC, 0x7F, 0xFF, 0xFF, 0xE0,
        0x7F, 0xFF, 0xFF, 0xE0, 0x1F, 0xFF, 0xFF, 0xE0, 0x1F, 0xFF, 0xFF, 0xE0,
        0x1F, 0xFF, 0xFF, 0xE0, 0x1F, 0xFF, 0xFF, 0xE0, 0x1F, 0xFF, 0xFF, 0xE0,
        0x1F, 0xFF, 0xFF, 0xE0, 0x3F, 0xFF, 0xFF, 0xF0, 0x3F, 0xFF, 0xFF, 0xF8,
        0x3F, 0xFF, 0xFF, 0xFC, 0x7F, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE,
        0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE
    }
};

@implementation DiskImageIconReader

- (UIImage *)iconForDiskImage:(NSString *)path {
    // determine format and offset of disk image
    NSFileHandle *fh = [NSFileHandle fileHandleForReadingAtPath:path];
    if (fh == nil) {
        return nil;
    }
    [fh seekToFileOffset:1024];
    NSData *checkHeader = [fh readDataOfLength:128];
    [fh closeFile];
    if (checkHeader == nil || checkHeader.length != 128) {
        return nil;
    }
    const unsigned char *chb = [checkHeader bytes];

    // determine type from header
    if ((chb[0] == 0x42) && (chb[1] == 0x44)) {
        /* hfs */
        return [self iconForHFSDiskImage:path options:0];
    } else if ((chb[0] == 0xD2) && (chb[1] == 0xD7)) {
        /* mfs */
        return [self iconForMFSDiskImage:path options:0];
    } else if ((chb[84] == 0x42) && (chb[85] == 0x44)) {
        /* hfs, dc42 header */
        return [self iconForHFSDiskImage:path options:kDiskImageHasDC42Header];
    } else if ((chb[84] == 0xD2) && (chb[85] == 0xD7)) {
        /* mfs, dc42  header */
        return [self iconForMFSDiskImage:path options:kDiskImageHasDC42Header];
    }

    return nil;
}

#pragma mark - MFS

- (UIImage *)iconForMFSDiskImage:(NSString *)path options:(int)options {
    // open disk image
    size_t offset = (options & kDiskImageHasDC42Header) ? 84 : 0;
    MFSVolume *vol = mfs_vopen([path fileSystemRepresentation], (size_t)offset, 0);
    if (vol == NULL) {
        NSLog(@"Can't open MFS volume at %@", path);
        return nil;
    }
    NSString *volName = [NSString stringWithCString:vol->name encoding:NSMacOSRomanStringEncoding];
    NSString *volComment;
    char *const volCommentBytes = mfs_comment(vol, NULL);
    if (volCommentBytes) {
        volComment = [NSString stringWithCString:volCommentBytes encoding:NSMacOSRomanStringEncoding];
        free(volCommentBytes);
    }

    // find applications
    MFSDirectoryRecord *rec;
    NSMutableArray *apps = [NSMutableArray arrayWithCapacity:5];
    for (int i = 0; vol->directory[i]; i++) {
        rec = vol->directory[i];
        if (ntohl(rec->flUsrWds.type) != 'APPL') {
            continue;
        }
        [apps addObject:[NSNumber numberWithInt:i]];
    }

    // if there's more than one app, find one that looks matching
    if ([apps count] == 0) {
        return nil;
    } else if ([apps count] > 1) {
        rec = NULL;
        for (NSNumber *num in apps) {
            rec = vol->directory[[num intValue]];
            NSString *appName = [[NSString alloc] initWithCString:rec->flCName encoding:NSMacOSRomanStringEncoding];
            if (![self chooseApp:appName inVolume:volName hint:volComment]) {
                rec = NULL;
            }
            if (rec) {
                break;
            }
        }
        if (rec == NULL) {
            return nil;
        }
    } else {
        rec = vol->directory[[[apps objectAtIndex:0] intValue]];
    }

    // open resource fork
    MFSFork *rsrcFork = mfs_fkopen(vol, rec, kMFSForkRsrc, 0);
    RFILE *rfile = res_open_funcs(rsrcFork, mfs_fkseek, mfs_fkread);

    // get icon
    UIImage *icon = [self appIconForResourceFile:rfile creator:ntohl(rec->flUsrWds.creator)];
    
    // close stuff
    res_close(rfile);
    mfs_fkclose(rsrcFork);
    mfs_vclose(vol);

    return icon;
}

#pragma mark - HFS

- (UIImage *)iconForHFSDiskImage:(NSString *)path options:(int)options {
    // open disk image
    int mountFlags = HFS_MODE_RDONLY;
    if (options & kDiskImageHasDC42Header) {
        mountFlags |= HFS_OPT_DC42HEADER;
    }
    hfsvol *vol = hfs_mount([path fileSystemRepresentation], 0, mountFlags);
    if (vol == NULL) {
        NSLog(@"Can't open HFS volume at %@ with flags %x", path, mountFlags);
        return nil;
    }

    // try volume icon
    UIImage *volumeIcon = [self iconFromHFSVolumeIcon:vol];
    if (volumeIcon) {
        hfs_umount(vol);
        return volumeIcon;
    }

    // find best application
    UIImage *icon = nil;
    NSString *appPath = [self findAppInHFSVolume:vol];
    hfsfile *hfile = NULL;
    RFILE *rfile = NULL;
    if (appPath == nil) {
        hfs_umount(vol);
        return nil;
    }

    // open resource fork
    hfile = hfs_open(vol, [appPath cStringUsingEncoding:NSMacOSRomanStringEncoding]);
    if (hfile == NULL) {
        hfs_umount(vol);
        return nil;
    }
    hfs_setfork(hfile, 1);
    rfile = res_open_funcs(hfile, (res_seek_func)hfs_seek, (res_read_func)hfs_read);
    if (rfile == NULL) {
        hfs_close(hfile);
        hfs_umount(vol);
        return nil;
    }

    // get icon
    hfsdirent ent;
    if (hfs_stat(vol, [appPath cStringUsingEncoding:NSMacOSRomanStringEncoding], &ent)) {
        res_close(rfile);
        hfs_close(hfile);
        hfs_umount(vol);
        return nil;
    }
    icon = [self appIconForResourceFile:rfile creator:ntohl(*(uint32_t *)ent.u.file.creator)];
    
    // close stuff
    res_close(rfile);
    hfs_close(hfile);
    hfs_umount(vol);
    return icon;
}

- (NSString *)findAppInHFSVolume:(hfsvol *)vol {
    // get disk name
    hfsvolent volEnt;
    hfs_vstat(vol, &volEnt);
    NSString *volName = [NSString stringWithCString:volEnt.name encoding:NSMacOSRomanStringEncoding];
    NSString *volComment = [self commentForHFSVolume:vol];

    // find apps
    NSMutableArray *apps = [[NSMutableArray alloc] initWithCapacity:5];
    [self findApps:apps inDirectory:HFS_CNID_ROOTDIR ofHFSVolume:vol skipFolder:volEnt.blessed];

    // decide which one to use
    NSString *myApp = nil;
    NSString *appName = nil;
    if ([apps count] == 1) {
        myApp = [apps objectAtIndex:0];
    } else if ([apps count] > 1) {
        for (NSString *appPath in apps) {
            // choose an app
            appName = [appPath componentsSeparatedByString:@":"].lastObject;
            if (![self chooseApp:appName inVolume:volName hint:volComment]) {
                continue;
            }
            myApp = appPath;
        }
    }
    
    return myApp;
}

- (void)findApps:(NSMutableArray *)apps inDirectory:(unsigned long)cnid ofHFSVolume:(hfsvol *)vol skipFolder:(unsigned long)skipCNID {
    if (hfs_setcwd(vol, cnid)) {
        return;
    }
    hfsdir *dir = hfs_opendir(vol, ":");
    if (dir == NULL) {
        return;
    }
    hfsdirent ent;
    while (hfs_readdir(dir, &ent) == 0) {
        if (ent.flags & HFS_ISDIR && ent.cnid != skipCNID) {
            [self findApps:apps inDirectory:ent.cnid ofHFSVolume:vol skipFolder:skipCNID];
        } else if (ntohl(*(uint32_t *)ent.u.file.type) == 'APPL') {
            // Found an app
            [apps addObject:[self pathToDirEntry:&ent ofHFSVolume:vol]];
        }
    }
    hfs_closedir(dir);
}

- (NSString *)pathToDirEntry:(const hfsdirent *)ent ofHFSVolume:(hfsvol *)vol {
    NSMutableString *path = [NSMutableString stringWithCString:ent->name encoding:NSMacOSRomanStringEncoding];
    NSString *entName;
    char name[HFS_MAX_FLEN + 1];
    unsigned long cnid = ent->parid;
    while (cnid != HFS_CNID_ROOTPAR) {
        if (hfs_dirinfo(vol, &cnid, name)) {
            return nil;
        }
        entName = [[NSString alloc] initWithCString:name encoding:NSMacOSRomanStringEncoding];
        [path insertString:@":" atIndex:0];
        [path insertString:entName atIndex:0];
    }
    return path;
}

- (NSString *)commentForHFSVolume:(hfsvol *)vol {
    hfsvolent vent;
    hfsdirent dent;
    NSString *comment = nil;

    // get comment ID
    if (hfs_vstat(vol, &vent) || hfs_stat(vol, ":", &dent)) {
        return nil;
    }
    unsigned short cmtID = dent.fdcomment;

    // open desktop
    hfsfile *hfile = NULL;
    RFILE *rfile = NULL;
    hfs_chdir(vol, vent.name);
    hfile = hfs_open(vol, "Desktop");
    if (hfile == NULL) {
        return nil;
    }
    hfs_setfork(hfile, 1);
    rfile = res_open_funcs(hfile, (res_seek_func)hfs_seek, (res_read_func)hfs_read);
    if (rfile == NULL) {
        hfs_close(hfile);
        return nil;
    }

    // read resource
    unsigned char cmtLen;
    size_t readBytes;
    res_read(rfile, 'FCMT', cmtID, &cmtLen, 0, 1, &readBytes, NULL);
    if (readBytes == 0) {
        res_close(rfile);
        hfs_close(hfile);
        return nil;
    }
    char cmtBytes[256];
    res_read(rfile, 'FCMT', cmtID, cmtBytes, 1, cmtLen, &readBytes, NULL);
    cmtBytes[cmtLen] = '\0';
    comment = [NSString stringWithCString:cmtBytes encoding:NSMacOSRomanStringEncoding];

    // close
    res_close(rfile);
    hfs_close(hfile);
    return comment;
}

- (UIImage *)iconFromHFSVolumeIcon:(hfsvol *)vol {
    UIImage *icon = nil;
    hfsvolent vent;
    if (hfs_vstat(vol, &vent)) {
        return nil;
    }

    // open icon file
    hfs_chdir(vol, vent.name);
    hfsfile *hfile = NULL;
    RFILE *rfile = NULL;
    hfile = hfs_open(vol, "Icon\x0D");
    if (hfile == NULL) {
        res_close(rfile);
        return nil;
    }
    hfs_setfork(hfile, 1);
    rfile = res_open_funcs(hfile, (res_seek_func)hfs_seek, (res_read_func)hfs_read);
    if (rfile == NULL) {
        if (hfile) {
            hfs_close(hfile);
        }
        return nil;
    }
    
    // read icon family
    NSDictionary *iconFamily = [self iconFamilyID:-16455 inResourceFile:rfile];
    icon = [self iconImageFromFamily:iconFamily];

    res_close(rfile);
    if (hfile) {
        hfs_close(hfile);
    }
    return icon;
}

#pragma mark - App Selection

- (BOOL)chooseApp:(NSString *)appName inVolume:(NSString *)volName hint:(NSString *)hint {
    return ([appName hasPrefix:volName] ||
            [volName hasPrefix:appName] ||
            [volName isEqualToString:appName] ||
            [appName isEqualToString:hint]);
}

#pragma mark - Resource Access

- (UIImage*)appIconForResourceFile:(RFILE *)rfile creator:(OSType)creator {
    // load bundle
    size_t numBundles;
    ResAttr *bundles = res_list(rfile, 'BNDL', NULL, 0, 0, &numBundles, NULL);
    void *bundle = NULL;
    if (numBundles == 0 || bundles == NULL) {
        return nil;
    }
    for (int i = 0; i < numBundles; i++) {
        bundle = res_read(rfile, 'BNDL', bundles[i].ID, NULL, 0, 0, NULL, NULL);
        if (bundle == NULL || ntohl(*(OSType *)bundle) == creator) {
            break;
        }
        free(bundle);
        bundle = NULL;
    }
    free(bundles);
    if (bundle == NULL) {
        return nil;
    }

    // read bundle
    NSInteger iconID = [self iconFamilyIDForType:'APPL' inBundle:bundle inResourceFile:rfile];
    free(bundle);
    if (iconID == NSNotFound) {
        return nil;
    }

    // read icon family
    NSDictionary *iconFamily = [self iconFamilyID:iconID inResourceFile:rfile];

    // create image
    return [self iconImageFromFamily:iconFamily];
}

- (NSDictionary *)iconFamilyID:(int16_t)famID inResourceFile:(RFILE *)rfile {
    NSMutableDictionary *iconFamily = [NSMutableDictionary dictionaryWithCapacity:6];
    NSData *iconData, *maskData;
    void *iconRsrc;
    size_t resSize;

    // separate resources
    const uint32_t iconResourceTypes[] = {'ICN#', 'icl4', 'icl8', 'ics#', 'ics4', 'ics8', 0};
    for (int i = 0; iconResourceTypes[i]; i++) {
        iconRsrc = res_read(rfile, iconResourceTypes[i], famID, NULL, 0, 0, &resSize, NULL);
        if (iconRsrc == NULL) {
            continue;
        }
        [iconFamily setObject:[NSData dataWithBytes:iconRsrc length:resSize] forKey:[NSString stringWithFormat:@"%c%c%c%c", TYPECHARS(iconResourceTypes[i])]];
        free(iconRsrc);
    }

    // mask pseudo-resources
    if ((iconData = [iconFamily objectForKey:@"ICN#"])) {
        maskData = [iconData subdataWithRange:NSMakeRange(0x80, 0x80)];
        [iconFamily setObject:maskData forKey:@"IMK#"];
    }
    if ((iconData = [iconFamily objectForKey:@"ics#"])) {
        maskData = [iconData subdataWithRange:NSMakeRange(0x20, 0x20)];
        [iconFamily setObject:maskData forKey:@"imk#"];
    }

    return iconFamily;
}

- (NSInteger)iconFamilyIDForType:(OSType)type inBundle:(void *)bndl inResourceFile:(RFILE *)rfile {
    short numIconFamilies = RSHORT(bndl, 0x0C) + 1;
    short *iconFamily = (short *)(bndl + 0x0E);
    short numFileRefs = RSHORT(bndl, (numIconFamilies * 4) + 0x12) + 1;
    short *fileRef = (short *)(bndl + (numIconFamilies * 4) + 0x14);

    // find FREF for APPL type
    short localIconID;
    void *FREF = NULL;
    for (int i = 0; i < 2 * numFileRefs; i += 2) {
        FREF = res_read(rfile, 'FREF', (int)ntohs(fileRef[i + 1]), NULL, 0, 0, NULL, NULL);
        if (FREF == NULL || RLONG(FREF, 0) == 'APPL') {
            break;
        }
        free(FREF);
        FREF = NULL;
    }
    if (FREF == NULL) {
        return NSNotFound;
    }

    // read FREF
    localIconID = RSHORT(FREF, 4);
    free(FREF);

    // find resource ID for local ID
    for (int i = 0; i < 2 * numIconFamilies; i += 2) {
        if (ntohs(iconFamily[i]) == localIconID) {
            return (int)ntohs(iconFamily[i + 1]);
        }
    }

    return NSNotFound;
}

- (UIImage*)iconImageFromFamily:(NSDictionary *)iconFamily {
    NSData *iconData, *iconMask;
    if ((iconMask = [iconFamily objectForKey:@"IMK#"])) {
        // has large mask, find best large icon
        if ((iconData = [iconFamily objectForKey:@"icl8"])) {
            return [self iconImageWithData:iconData mask:iconMask size:32 depth:8];
        } else if ((iconData = [iconFamily objectForKey:@"icl4"])) {
            return [self iconImageWithData:iconData mask:iconMask size:32 depth:4];
        } else {
            iconData = [iconFamily objectForKey:@"ICN#"];
        }
        return [self iconImageWithData:iconData mask:iconMask size:32 depth:1];
    } else if ((iconMask = [iconFamily objectForKey:@"imk#"])) {
        // has small mask, find best small icon
        if ((iconData = [iconFamily objectForKey:@"ics8"])) {
            return [self iconImageWithData:iconData mask:iconMask size:32 depth:8];
        } else if ((iconData = [iconFamily objectForKey:@"ics4"])) {
            return [self iconImageWithData:iconData mask:iconMask size:32 depth:4];
        } else {
            iconData = [iconFamily objectForKey:@"ics#"];
        }
        return [self iconImageWithData:iconData mask:iconMask size:32 depth:1];
    }
    return NULL;
}

- (UIImage*)iconImageWithData:(NSData *)iconData mask:(NSData *)iconMask size:(int)size depth:(int)depth {
    if (iconData == nil || iconMask == nil) {
        return NULL;
    }

    // convert to ARGB
    #define _iSETPIXELRGB(px, py, sa, srgb)                     \
    data[(4 * (px + (py * size))) + 0] = sa;                    \
    data[(4 * (px + (py * size))) + 1] = ((srgb >> 16) & 0xFF); \
    data[(4 * (px + (py * size))) + 2] = ((srgb >> 8) & 0xFF);  \
    data[(4 * (px + (py * size))) + 3] = (srgb & 0xFF)

    CFMutableDataRef pixels = CFDataCreateMutable(kCFAllocatorDefault, 4 * size * size);
    CFDataSetLength(pixels, 4 * size * size);
    unsigned char *data = CFDataGetMutableBytePtr(pixels);
    const unsigned char *pixelData = [iconData bytes];
    const unsigned char *maskData = [iconMask bytes];
    
    if (size == 32 && iconMask.length == 128) {
        for (int i=0; i < sizeof(maskReplacement) / sizeof(maskReplacement[0]); i+=2) {
            if (memcmp(maskData, maskReplacement[i], 128) == 0) {
                maskData = maskReplacement[i+1];
                break;
            }
        }
    }
    
    int m, mxy, pxy, rgb;
    if (pixels == NULL) {
        return NULL;
    }
    switch (depth) {
        case 1:
            // 1-bit
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    mxy = pxy = (y * (size / 8)) + (x / 8);
                    m = ((maskData[mxy] >> (7 - (x % 8))) & 0x01) ? 0xFF : 0x00;
                    rgb = ctb1[((pixelData[pxy] >> (7 - (x % 8))) & 0x01)];
                    _iSETPIXELRGB(x, y, m, rgb);
                }
            }
            break;
        case 4:
            // 4-bit
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    mxy = (y * (size / 8)) + (x / 8);
                    pxy = (y * (size / 2)) + (x / 2);
                    m = ((maskData[mxy] >> (7 - (x % 8))) & 0x01) ? 0xFF : 0x00;
                    rgb = ctb4[(pixelData[pxy] >> 4 * (1 - x % 2)) & 0x0F];
                    _iSETPIXELRGB(x, y, m, rgb);
                }
            }
            break;
        case 8:
            // 8-bit
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    mxy = (y * (size / 8)) + (x / 8);
                    pxy = (y * size) + x;
                    m = ((maskData[mxy] >> (7 - (x % 8))) & 0x01) ? 0xFF : 0x00;
                    rgb = ctb8[pixelData[pxy]];
                    _iSETPIXELRGB(x, y, m, rgb);
                }
            }
            break;
    }

    // create image
    CGDataProviderRef provider = CGDataProviderCreateWithCFData(pixels);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGImageRef cgImage = CGImageCreate(size, size, 8, 32, size * 4, colorSpace, kCGImageAlphaFirst | kCGBitmapByteOrder32Big, provider, NULL, false, kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    CFRelease(pixels);
    UIImage *image = [UIImage imageWithCGImage:cgImage];
    CGImageRelease(cgImage);
    return image;
}

@end
