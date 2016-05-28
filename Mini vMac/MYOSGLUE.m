/*
	MYOSGLUE.m
 
	Copyright (C) 2012 Paul C. Pratt, SDL by Sam Lantinga and others
 
	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.
 
	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
 */

/*
	MY Operating System GLUE. (for iOS)
 
	All operating system dependent code for the
	iOS should go here.
 
	Originally derived from Cocoa port of SDL Library
	by Sam Lantinga (but little trace of that remains).
 */

@import UIKit;
@import AudioUnit;
@import AudioToolbox;
#include "CNFGRAPI.h"
#include "SYSDEPNS.h"
#include "ENDIANAC.h"
#include "MYOSGLUE.h"
#include "STRCONST.h"
#import "Emulator.h"

#pragma mark - some simple utilities

GLOBALPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount) {
    (void)memcpy((char *)destPtr, (char *)srcPtr, byteCount);
}

#pragma mark - control mode and internationalization

#define NeedCell2UnicodeMap 1

#include "INTLCHAR.h"

#pragma mark - sending debugging info to file

#if dbglog_HAVE

#define dbglog_ToStdErr 0

#if !dbglog_ToStdErr
LOCALVAR FILE *dbglog_File = NULL;
#endif

LOCALFUNC blnr dbglog_open0(void) {
#if dbglog_ToStdErr
    return trueblnr;
#else
    NSString *myLogPath = [MyDataPath stringByAppendingPathComponent:@"dbglog.txt"];
    const char *path = [myLogPath fileSystemRepresentation];

    dbglog_File = fopen(path, "w");
    return (NULL != dbglog_File);
#endif
}

LOCALPROC dbglog_write0(char *s, uimr L) {
#if dbglog_ToStdErr
    (void)fwrite(s, 1, L, stderr);
#else
    if (NULL != dbglog_File) {
        (void)fwrite(s, 1, L, dbglog_File);
    }
#endif
}

LOCALPROC dbglog_close0(void) {
#if !dbglog_ToStdErr
    if (NULL != dbglog_File) {
        fclose(dbglog_File);
        dbglog_File = NULL;
    }
#endif
}

#endif

#pragma mark - information about the environment

#define WantColorTransValid 1

#include "COMOSGLU.h"

#pragma mark - Cocoa Stuff

LOCALFUNC blnr FindNamedChildFilePath(NSString *parentPath, char *ChildName, NSString **childPath) {
    NSFileManager *fm = [NSFileManager defaultManager];
    BOOL isDirectory;
    if ([fm fileExistsAtPath:parentPath isDirectory:&isDirectory] && isDirectory) {
        NSString *searchString = @(ChildName).lowercaseString;
        __block NSString *foundName = nil;
        [[fm contentsOfDirectoryAtPath:parentPath error:NULL] enumerateObjectsUsingBlock:^(NSString * _Nonnull obj, NSUInteger idx, BOOL * _Nonnull stop) {
            if ([obj.lowercaseString isEqualToString:searchString]) {
                *stop = YES;
                foundName = obj;
            }
        }];
        if (foundName) {
            *childPath = [parentPath stringByAppendingPathComponent:foundName];
        }
        return foundName != nil;
    } else {
        return falseblnr;
    }
}

LOCALVAR CGDataProviderRef screenDataProvider = NULL;
LOCALVAR CGColorSpaceRef bwColorSpace = NULL;
LOCALVAR CGColorSpaceRef colorColorSpace = NULL;

LOCALFUNC blnr Screen_Init(void) {
    screenDataProvider = CGDataProviderCreateWithData(NULL, screencomparebuff, vMacScreenNumBytes, NULL);
    CGColorSpaceRef baseColorSpace = CGColorSpaceCreateDeviceRGB();
    uint8_t clut[] = {255, 255, 255, 0, 0, 0};
    bwColorSpace = CGColorSpaceCreateIndexed(baseColorSpace, 1, clut);
#if 0 != vMacScreenDepth
    ColorModeWorks = trueblnr;
#endif
#if vMacScreenDepth >= 4
    colorColorSpace = baseColorSpace;
#else
    CGColorSpaceRelease(baseColorSpace);
#endif
    return trueblnr;
}

LOCALPROC Screen_UnInit(void) {
    if (screenDataProvider) {
        CGDataProviderRelease(screenDataProvider);
        screenDataProvider = NULL;
    }
    if (bwColorSpace) {
        CGColorSpaceRelease(bwColorSpace);
        bwColorSpace = NULL;
    }
    if (colorColorSpace) {
        CGColorSpaceRelease(colorColorSpace);
        colorColorSpace = NULL;
    }
}

LOCALVAR NSString *MyDataPath = nil;

LOCALFUNC blnr InitCocoaStuff(void) {
    MyDataPath = [Emulator sharedEmulator].dataPath;
    if (MyDataPath) {
        [MyDataPath retain];
    }

    Screen_Init();
    return trueblnr;
}

LOCALPROC UnInitCocoaStuff(void) {
    [MyDataPath release];
    MyDataPath = nil;
    Screen_UnInit();
}

#pragma mark - Parameter Buffers

#if IncludePbufs
LOCALVAR void *PbufDat[NumPbufs];
#endif

#if IncludePbufs
LOCALFUNC tMacErr PbufNewFromPtr(void *p, ui5b count, tPbuf *r) {
    tPbuf i;
    tMacErr err;

    if (!FirstFreePbuf(&i)) {
        free(p);
        err = mnvm_miscErr;
    } else {
        *r = i;
        PbufDat[i] = p;
        PbufNewNotify(i, count);
        err = mnvm_noErr;
    }

    return err;
}
#endif

#if IncludePbufs
GLOBALFUNC tMacErr PbufNew(ui5b count, tPbuf *r) {
    tMacErr err = mnvm_miscErr;

    void *p = calloc(1, count);
    if (NULL != p) {
        err = PbufNewFromPtr(p, count, r);
    }

    return err;
}
#endif

#if IncludePbufs
GLOBALPROC PbufDispose(tPbuf i) {
    free(PbufDat[i]);
    PbufDisposeNotify(i);
}
#endif

#if IncludePbufs
LOCALPROC UnInitPbufs(void) {
    tPbuf i;

    for (i = 0; i < NumPbufs; ++i) {
        if (PbufIsAllocated(i)) {
            PbufDispose(i);
        }
    }
}
#endif

#if IncludePbufs
GLOBALPROC PbufTransfer(ui3p Buffer,
                        tPbuf i,
                        ui5r offset,
                        ui5r count,
                        blnr IsWrite) {
    void *p = ((ui3p)PbufDat[i]) + offset;
    if (IsWrite) {
        (void)memcpy(p, Buffer, count);
    } else {
        (void)memcpy(Buffer, p, count);
    }
}
#endif

#pragma mark - Text Translation

LOCALPROC UniCharStrFromSubstCStr(int *L, unichar *x, char *s, blnr AddEllipsis) {
    int i;
    int L0;
    ui3b ps[ClStrMaxLength];

    ClStrFromSubstCStr(&L0, ps, s);
    if (AddEllipsis) {
        ClStrAppendChar(&L0, ps, kCellEllipsis);
    }

    for (i = 0; i < L0; ++i) {
        x[i] = Cell2UnicodeMap[ps[i]];
    }

    *L = L0;
}

LOCALFUNC NSString *NSStringCreateFromSubstCStr(char *s,
                                                blnr AddEllipsis) {
    int L;
    unichar x[ClStrMaxLength];

    UniCharStrFromSubstCStr(&L, x, s, AddEllipsis);

    return [NSString stringWithCharacters:x length:L];
}

#if IncludeSonyNameNew
LOCALFUNC blnr MacRomanFileNameToNSString(tPbuf i,
                                          NSString **r) {
    ui3p p;
    void *Buffer = PbufDat[i];
    ui5b L = PbufSize[i];

    p = (ui3p)malloc(L /* + 1 */);
    if (p != NULL) {
        NSData *d;
        ui3b *p0 = (ui3b *)Buffer;
        ui3b *p1 = (ui3b *)p;

        if (L > 0) {
            ui5b j = L;

            do {
                ui3b x = *p0++;
                if (x < 32) {
                    x = '-';
                } else if (x >= 128) {
                } else {
                    switch (x) {
                        case '/':
                        case '<':
                        case '>':
                        case '|':
                        case ':':
                            x = '-';
                        default:
                            break;
                    }
                }
                *p1++ = x;
            } while (--j > 0);

            if ('.' == p[0]) {
                p[0] = '-';
            }
        }

        d = [[NSData alloc] initWithBytesNoCopy:p length:L];

        *r = [[[NSString alloc]
            initWithData:d
                encoding:NSMacOSRomanStringEncoding]
            autorelease];

        [d release];

        return trueblnr;
    }

    return falseblnr;
}
#endif

#if IncludeSonyGetName || IncludeHostTextClipExchange
LOCALFUNC tMacErr CopyBytesToPbuf(const char *x, ui5r L, tPbuf *r) {
    if (NULL == x) {
        return mnvm_miscErr;
    } else {
        ui3p p = (ui3p)malloc(L);

        if (NULL == p) {
            return mnvm_miscErr;
        } else {
            memcpy((char *)p, x, L);

            return PbufNewFromPtr(p, L, r);
        }
    }
}
#endif

#if IncludeSonyGetName || IncludeHostTextClipExchange
LOCALFUNC tMacErr NSStringToRomanPbuf(NSString *string, tPbuf *r) {
    tMacErr v = mnvm_miscErr;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSData *d0 = [string dataUsingEncoding:NSMacOSRomanStringEncoding];
    const void *s = [d0 bytes];
    NSUInteger L = [d0 length];

    v = CopyBytesToPbuf(s, (ui5r)L, r);

    [pool release];

    return v;
}
#endif

#pragma mark - Drives

#define NotAfileRef NULL
LOCALVAR FILE *Drives[NumDrives]; /* open disk image files */
#if IncludeSonyGetName || IncludeSonyNew
LOCALVAR NSString *DriveNames[NumDrives];
#endif

LOCALPROC InitDrives(void) {
    /*
     This isn't really needed, Drives[i] and DriveNames[i]
     need not have valid values when not vSonyIsInserted[i].
     */
    tDrive i;

    for (i = 0; i < NumDrives; ++i) {
        Drives[i] = NotAfileRef;
#if IncludeSonyGetName || IncludeSonyNew
        DriveNames[i] = nil;
#endif
    }
}

GLOBALFUNC tMacErr vSonyTransfer(blnr IsWrite, ui3p Buffer, tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count, ui5r *Sony_ActCount) {
    tMacErr err = mnvm_miscErr;
    FILE *refnum = Drives[Drive_No];
    ui5r NewSony_Count = 0;

    if (0 == fseek(refnum, Sony_Start, SEEK_SET)) {
        if (IsWrite) {
            NewSony_Count = (ui5r)fwrite(Buffer, 1, Sony_Count, refnum);
        } else {
            NewSony_Count = (ui5r)fread(Buffer, 1, Sony_Count, refnum);
        }

        if (NewSony_Count == Sony_Count) {
            err = mnvm_noErr;
        }
    }

    if (nullpr != Sony_ActCount) {
        *Sony_ActCount = NewSony_Count;
    }

    return err; /*& figure out what really to return &*/
}

GLOBALFUNC tMacErr vSonyGetSize(tDrive Drive_No, ui5r *Sony_Count) {
    tMacErr err = mnvm_miscErr;
    FILE *refnum = Drives[Drive_No];
    long v;

    if (0 == fseek(refnum, 0, SEEK_END)) {
        v = ftell(refnum);
        if (v >= 0) {
            *Sony_Count = (ui5r)v;
            err = mnvm_noErr;
        }
    }

    return err; /*& figure out what really to return &*/
}

#ifndef HaveAdvisoryLocks
#define HaveAdvisoryLocks 1
#endif

/*
	What is the difference between fcntl(fd, F_SETLK ...
	and flock(fd ... ?
 */

#if HaveAdvisoryLocks
LOCALFUNC blnr MyLockFile(FILE *refnum) {
    blnr IsOk = falseblnr;

    int fd = fileno(refnum);

    if (-1 == flock(fd, LOCK_EX | LOCK_NB)) {
        if (EWOULDBLOCK == errno) {
            /* already locked */
            MacMsg(kStrImageInUseTitle, kStrImageInUseMessage,
                   falseblnr);
        } else {
            /*
             Failed for other reasons, such as unsupported
             for this volume.
             Don't prevent opening.
             */
            IsOk = trueblnr;
        }
    } else {
        IsOk = trueblnr;
    }

    return IsOk;
}
#endif

#if HaveAdvisoryLocks
LOCALPROC MyUnlockFile(FILE *refnum) {
    int fd = fileno(refnum);
    flock(fd, LOCK_UN);
}
#endif

LOCALFUNC tMacErr vSonyEject0(tDrive Drive_No, blnr deleteit) {
    FILE *refnum = Drives[Drive_No];

    NSDictionary *userInfo = @{@"path": DriveNames[Drive_No],
                               @"drive": @(Drive_No),
                               @"delete": @(deleteit)};
    DiskEjectedNotify(Drive_No);

#if HaveAdvisoryLocks
    MyUnlockFile(refnum);
#endif

    fclose(refnum);
    Drives[Drive_No] = NotAfileRef; /* not really needed */

#if IncludeSonyGetName || IncludeSonyNew
    {
        NSString *filePath = DriveNames[Drive_No];
        if (NULL != filePath) {
            if (deleteit) {
                NSAutoreleasePool *pool =
                    [[NSAutoreleasePool alloc] init];
                const char *s = [filePath fileSystemRepresentation];
                remove(s);
                [pool release];
            }
            [filePath release];
            DriveNames[Drive_No] = NULL; /* not really needed */
        }
    }
#endif
    
    [[NSNotificationCenter defaultCenter] postNotificationName:[Emulator sharedEmulator].ejectDiskNotification object:[Emulator sharedEmulator] userInfo:userInfo];

    return mnvm_noErr;
}

GLOBALFUNC tMacErr vSonyEject(tDrive Drive_No) {
    return vSonyEject0(Drive_No, falseblnr);
}

#if IncludeSonyNew
GLOBALFUNC tMacErr vSonyEjectDelete(tDrive Drive_No) {
    return vSonyEject0(Drive_No, trueblnr);
}
#endif

LOCALPROC UnInitDrives(void) {
    tDrive i;

    for (i = 0; i < NumDrives; ++i) {
        if (vSonyIsInserted(i)) {
            (void)vSonyEject(i);
        }
    }
}

#if IncludeSonyGetName
GLOBALFUNC tMacErr vSonyGetName(tDrive Drive_No, tPbuf *r) {
    tMacErr v = mnvm_miscErr;
    NSString *filePath = DriveNames[Drive_No];
    if (NULL != filePath) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        NSString *s0 = [filePath lastPathComponent];
        v = NSStringToRomanPbuf(s0, r);

        [pool release];
    }

    return v;
}
#endif

LOCALFUNC blnr Sony_Insert0(FILE *refnum, blnr locked, NSString *filePath) {
    tDrive Drive_No;
    blnr IsOk = falseblnr;

    if (!FirstFreeDisk(&Drive_No)) {
        MacMsg(kStrTooManyImagesTitle, kStrTooManyImagesMessage,
               falseblnr);
    } else {
        NSDictionary *userInfo = @{@"path": filePath,
                                   @"drive": @(Drive_No)};
        [[NSNotificationCenter defaultCenter] postNotificationName:[Emulator sharedEmulator].insertDiskNotification object:[Emulator sharedEmulator] userInfo:userInfo];
        
/* printf("Sony_Insert0 %d\n", (int)Drive_No); */

#if HaveAdvisoryLocks
        if (locked || MyLockFile(refnum))
#endif
        {
            Drives[Drive_No] = refnum;
            DiskInsertNotify(Drive_No, locked);

#if IncludeSonyGetName || IncludeSonyNew
            DriveNames[Drive_No] = [filePath retain];
#endif

            IsOk = trueblnr;
        }
    }

    if (!IsOk) {
        fclose(refnum);
    }

    return IsOk;
}

GLOBALFUNC blnr Sony_IsInserted(NSString *filePath) {
#if IncludeSonyGetName
    for (int i=0; i < NumDrives; i++) {
        if (vSonyIsInserted(i) && [DriveNames[i] isEqualToString:filePath]) {
            return trueblnr;
        }
    }
#endif
    return falseblnr;
}

GLOBALFUNC blnr Sony_Insert1(NSString *filePath, blnr silentfail) {
    /* const char *drivepath = [filePath UTF8String]; */
    const char *drivepath = [filePath fileSystemRepresentation];
    blnr locked = falseblnr;
    /* printf("Sony_Insert1 %s\n", drivepath); */
    FILE *refnum = fopen(drivepath, "rb+");
    if (NULL == refnum) {
        locked = trueblnr;
        refnum = fopen(drivepath, "rb");
    }
    if (NULL == refnum) {
        if (!silentfail) {
            MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
        }
    } else {
        return Sony_Insert0(refnum, locked, filePath);
    }
    return falseblnr;
}

LOCALFUNC blnr Sony_Insert2(char *s) {
    NSString *sPath;

    if (!FindNamedChildFilePath(MyDataPath, s, &sPath)) {
        return falseblnr;
    } else {
        return Sony_Insert1(sPath, trueblnr);
    }
}

LOCALFUNC blnr LoadInitialImages(void) {
    if (!AnyDiskInserted()) {
        int n = NumDrives > 9 ? 9 : NumDrives;
        int i;
        char s[] = "disk?.dsk";

        for (i = 1; i <= n; ++i) {
            s[4] = '0' + i;
            if (!Sony_Insert2(s)) {
                /* stop on first error (including file not found) */
                return trueblnr;
            }
        }
    }

    return trueblnr;
}

#if IncludeSonyNew
LOCALFUNC blnr WriteZero(FILE *refnum, ui5b L) {
#define ZeroBufferSize 2048
    ui5b i;
    ui3b buffer[ZeroBufferSize];

    memset(&buffer, 0, ZeroBufferSize);

    while (L > 0) {
        i = (L > ZeroBufferSize) ? ZeroBufferSize : L;
        if (fwrite(buffer, 1, i, refnum) != i) {
            return falseblnr;
        }
        L -= i;
    }
    return trueblnr;
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk0(ui5b L, NSString *sPath) {
    blnr IsOk = falseblnr;
    const char *drivepath = [sPath fileSystemRepresentation];
    FILE *refnum = fopen(drivepath, "wb+");
    if (NULL == refnum) {
        MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
    } else {
        if (WriteZero(refnum, L)) {
            IsOk = Sony_Insert0(refnum, falseblnr, sPath);
            refnum = NULL;
        }
        if (refnum != NULL) {
            fclose(refnum);
        }
        if (!IsOk) {
            (void)remove(drivepath);
        }
    }
}
#endif

#pragma mark - ROM

/* --- ROM --- */

LOCALFUNC tMacErr LoadMacRomFrom(NSString *parentPath) {
    FILE *ROM_File;
    size_t File_Size;
    NSString *RomPath;
    tMacErr err = mnvm_fnfErr;

    if (FindNamedChildFilePath(parentPath, RomFileName, &RomPath)) {
        const char *path = [RomPath fileSystemRepresentation];

        ROM_File = fopen(path, "rb");
        if (NULL != ROM_File) {
            File_Size = fread(ROM, 1, kROM_Size, ROM_File);
            if (kROM_Size != File_Size) {
                if (feof(ROM_File)) {
                    err = mnvm_eofErr;
                } else {
                    err = mnvm_miscErr;
                }
            } else {
                err = mnvm_noErr;
            }
            fclose(ROM_File);
        }
    }

    return err;
}

LOCALFUNC blnr LoadMacRom(void) {
    tMacErr err;

    if (mnvm_fnfErr == (err = LoadMacRomFrom(MyDataPath))) {
    }

    if (mnvm_noErr != err) {
        if (mnvm_fnfErr == err) {
            MacMsg(kStrNoROMTitle, kStrNoROMMessage, trueblnr);
        } else if (mnvm_eofErr == err) {
            MacMsg(kStrShortROMTitle, kStrShortROMMessage,
                   trueblnr);
        } else {
            MacMsg(kStrNoReadROMTitle, kStrNoReadROMMessage,
                   trueblnr);
        }

        SpeedStopped = trueblnr;
    }

    return trueblnr; /* keep launching Mini vMac, regardless */
}

#if IncludeHostTextClipExchange
GLOBALFUNC tMacErr HTCEexport(tPbuf i) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSData *d = [NSData dataWithBytes:PbufDat[i] length:PbufSize[i]];
    NSString *ss = [[[NSString alloc]
        initWithData:d
            encoding:NSMacOSRomanStringEncoding]
        autorelease];
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    pasteboard.string = ss;

    PbufDispose(i);

    [pool release];

    return mnvm_noErr;
}
#endif

#if IncludeHostTextClipExchange
GLOBALFUNC tMacErr HTCEimport(tPbuf *r) {
    tMacErr err = mnvm_miscErr;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
    if (pasteboard.string != nil) {
        err = NSStringToRomanPbuf(pasteboard.string, r);
    }
    [pool release];

    return err;
}
#endif

#pragma mark - time, date, location

#define dbglog_TimeStuff (0 && dbglog_HAVE)

LOCALVAR ui5b TrueEmulatedTime = 0;

LOCALVAR NSTimeInterval LatestTime;
LOCALVAR NSTimeInterval NextTickChangeTime;

#define MyTickDuration (1.0 / 60.14742)

LOCALVAR ui5b NewMacDateInSeconds;

LOCALPROC UpdateTrueEmulatedTime(void) {
    NSTimeInterval TimeDiff;

    LatestTime = [NSDate timeIntervalSinceReferenceDate];
    TimeDiff = LatestTime - NextTickChangeTime;

    if (TimeDiff >= 0.0) {
        if (TimeDiff > 16 * MyTickDuration) {
            /* emulation interrupted, forget it */
            ++TrueEmulatedTime;
            NextTickChangeTime = LatestTime + MyTickDuration;

#if dbglog_TimeStuff
            dbglog_writelnNum("emulation interrupted",
                              TrueEmulatedTime);
#endif
        } else {
            do {
                ++TrueEmulatedTime;
                TimeDiff -= MyTickDuration;
                NextTickChangeTime += MyTickDuration;
            } while (TimeDiff >= 0.0);
        }
    } else if (TimeDiff < (-16 * MyTickDuration)) {
/* clock set back, reset */
#if dbglog_TimeStuff
        dbglog_writeln("clock set back");
#endif

        NextTickChangeTime = LatestTime + MyTickDuration;
    }
}

LOCALVAR ui5b MyDateDelta;

LOCALFUNC blnr CheckDateTime(void) {
    NewMacDateInSeconds = ((ui5b)LatestTime) + MyDateDelta;
    if (CurMacDateInSeconds != NewMacDateInSeconds) {
        CurMacDateInSeconds = NewMacDateInSeconds;
        return trueblnr;
    } else {
        return falseblnr;
    }
}

LOCALPROC StartUpTimeAdjust(void) {
    LatestTime = [NSDate timeIntervalSinceReferenceDate];
    NextTickChangeTime = LatestTime;
}

LOCALFUNC blnr InitLocationDat(void) {
    NSTimeZone *MyZone = [NSTimeZone localTimeZone];
    ui5b TzOffSet = (ui5b)[MyZone secondsFromGMT];
    BOOL isdst = [MyZone isDaylightSavingTime];

    MyDateDelta = TzOffSet - 1233815296;
    LatestTime = [NSDate timeIntervalSinceReferenceDate];
    NewMacDateInSeconds = ((ui5b)LatestTime) + MyDateDelta;
    CurMacDateInSeconds = NewMacDateInSeconds;
    CurMacDelta = (TzOffSet & 0x00FFFFFF) | ((isdst ? 0x80 : 0) << 24);

    return trueblnr;
}

#pragma mark - Mouse

GLOBALPROC SetMouseButton(blnr down) {
    MyMouseButtonSet(down);
}

GLOBALPROC SetMouseLoc(ui4r h, ui4r v) {
    MyMousePositionSet(h, v);
}

GLOBALPROC SetMouseDelta(ui4r dh, ui4r dv) {
    MyMousePositionSetDelta(dh, dv);
}

#pragma mark - Keyboard

GLOBALPROC SetKeyState(int key, blnr down) {
    Keyboard_UpdateKeyMap(key, down);
}

#pragma mark - Video Out

#if 0 != vMacScreenDepth && vMacScreenDepth < 4
LOCALPROC UpdateColorTable() {
    unsigned char *colorTable = malloc(3 * CLUT_size);
    for (int i=0; i < CLUT_size; i++) {
        colorTable[3*i + 0] = CLUT_reds[i] >> 8;
        colorTable[3*i + 1] = CLUT_greens[i] >> 8;
        colorTable[3*i + 2] = CLUT_blues[i] >> 8;
    }
    CGColorSpaceRef baseColorSpace = CGColorSpaceCreateDeviceRGB();
    if (colorColorSpace != NULL) {
        CGColorSpaceRelease(colorColorSpace);
    }
    colorColorSpace = CGColorSpaceCreateIndexed(baseColorSpace, CLUT_size-1, colorTable);
    CGColorSpaceRelease(baseColorSpace);
    free(colorTable);
}
#endif

LOCALPROC HaveChangedScreenBuff(ui4r top, ui4r left, ui4r bottom, ui4r right) {
    size_t bitsPerPixel = 1;
    size_t bitsPerComponent = 1;
    size_t bytesPerRow = vMacScreenMonoByteWidth;
    CGBitmapInfo options = 0;
    CGColorSpaceRef colorSpace = bwColorSpace;
#if vMacScreenDepth != 0
    if (UseColorMode) {
        bitsPerPixel = 1 << vMacScreenDepth;
        bytesPerRow = vMacScreenByteWidth;
#if vMacScreenDepth < 4
        bitsPerComponent = 1 << vMacScreenDepth;
        if (!ColorTransValid) {
            UpdateColorTable();
            ColorTransValid = trueblnr;
        }
#elif vMacScreenDepth == 4
        bitsPerComponent = 5;
        options = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder16Big;
#elif vMacScreenDepth == 5
        bitsPerComponent = 8;
        options = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Big;
#endif
        colorSpace = colorColorSpace;
    }
#endif
    
    if (colorSpace) {
        CGImageRef screenImage = CGImageCreate(vMacScreenWidth, vMacScreenHeight, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpace, options, screenDataProvider, NULL, false, kCGRenderingIntentDefault);
        [[Emulator sharedEmulator] updateScreen:screenImage];
        CGImageRelease(screenImage);
    }
}

LOCALPROC MyDrawChangesAndClear(void) {
    if (ScreenChangedBottom > ScreenChangedTop) {
        HaveChangedScreenBuff(ScreenChangedTop, ScreenChangedLeft,
                              ScreenChangedBottom, ScreenChangedRight);
        ScreenClearChanges();
    }
}

GLOBALPROC DoneWithDrawingForTick(void) {
#if EnableMouseMotion && MayFullScreen
    if (HaveMouseMotion) {
        AutoScrollScreen();
    }
#endif
    MyDrawChangesAndClear();
}

#pragma mark - Sound

#if MySoundEnabled

#define kLn2SoundBuffers 4 /* kSoundBuffers must be a power of two */
#define kSoundBuffers (1 << kLn2SoundBuffers)
#define kSoundBuffMask (kSoundBuffers - 1)

#define DesiredMinFilledSoundBuffs 3
/*
 if too big then sound lags behind emulation.
 if too small then sound will have pauses.
	*/

#define kLnOneBuffLen 9
#define kLnAllBuffLen (kLn2SoundBuffers + kLnOneBuffLen)
#define kOneBuffLen (1UL << kLnOneBuffLen)
#define kAllBuffLen (1UL << kLnAllBuffLen)
#define kLnOneBuffSz (kLnOneBuffLen + kLn2SoundSampSz - 3)
#define kLnAllBuffSz (kLnAllBuffLen + kLn2SoundSampSz - 3)
#define kOneBuffSz (1UL << kLnOneBuffSz)
#define kAllBuffSz (1UL << kLnAllBuffSz)
#define kOneBuffMask (kOneBuffLen - 1)
#define kAllBuffMask (kAllBuffLen - 1)
#define dbhBufferSize (kAllBuffSz + kOneBuffSz)

#define dbglog_SoundStuff (0 && dbglog_HAVE)
#define dbglog_SoundBuffStats (0 && dbglog_HAVE)

LOCALVAR tpSoundSamp TheSoundBuffer = nullpr;
volatile static ui4b ThePlayOffset;
volatile static ui4b TheFillOffset;
volatile static ui4b MinFilledSoundBuffs;
#if dbglog_SoundBuffStats
LOCALVAR ui4b MaxFilledSoundBuffs;
#endif
LOCALVAR ui4b TheWriteOffset;

LOCALPROC MySound_Start0(void) {
    /* Reset variables */
    ThePlayOffset = 0;
    TheFillOffset = 0;
    TheWriteOffset = 0;
    MinFilledSoundBuffs = kSoundBuffers + 1;
#if dbglog_SoundBuffStats
    MaxFilledSoundBuffs = 0;
#endif
}

GLOBALFUNC tpSoundSamp MySound_BeginWrite(ui4r n, ui4r *actL) {
    ui4b ToFillLen = kAllBuffLen - (TheWriteOffset - ThePlayOffset);
    ui4b WriteBuffContig =
        kOneBuffLen - (TheWriteOffset & kOneBuffMask);

    if (WriteBuffContig < n) {
        n = WriteBuffContig;
    }
    if (ToFillLen < n) {
/* overwrite previous buffer */
#if dbglog_SoundStuff
        dbglog_writeln("sound buffer over flow");
#endif
        TheWriteOffset -= kOneBuffLen;
    }

    *actL = n;
    return TheSoundBuffer + (TheWriteOffset & kAllBuffMask);
}

#if 4 == kLn2SoundSampSz
LOCALPROC ConvertSoundBlockToNative(tpSoundSamp p) {
    int i;

    for (i = kOneBuffLen; --i >= 0;) {
        *p++ -= 0x8000;
    }
}
#else
#define ConvertSoundBlockToNative(p)
#endif

LOCALPROC MySound_WroteABlock(void) {
#if (4 == kLn2SoundSampSz)
    ui4b PrevWriteOffset = TheWriteOffset - kOneBuffLen;
    tpSoundSamp p = TheSoundBuffer + (PrevWriteOffset & kAllBuffMask);
#endif

#if dbglog_SoundStuff
    dbglog_writeln("enter MySound_WroteABlock");
#endif

    ConvertSoundBlockToNative(p);

    TheFillOffset = TheWriteOffset;

#if dbglog_SoundBuffStats
    {
        ui4b ToPlayLen = TheFillOffset - ThePlayOffset;
        ui4b ToPlayBuffs = ToPlayLen >> kLnOneBuffLen;

        if (ToPlayBuffs > MaxFilledSoundBuffs) {
            MaxFilledSoundBuffs = ToPlayBuffs;
        }
    }
#endif
}

LOCALFUNC blnr MySound_EndWrite0(ui4r actL) {
    blnr v;

    TheWriteOffset += actL;

    if (0 != (TheWriteOffset & kOneBuffMask)) {
        v = falseblnr;
    } else {
        /* just finished a block */

        MySound_WroteABlock();

        v = trueblnr;
    }

    return v;
}

LOCALPROC MySound_SecondNotify0(void) {
    if (MinFilledSoundBuffs <= kSoundBuffers) {
        if (MinFilledSoundBuffs > DesiredMinFilledSoundBuffs) {
#if dbglog_SoundStuff
            dbglog_writeln("MinFilledSoundBuffs too high");
#endif
            NextTickChangeTime += MyTickDuration;
        } else if (MinFilledSoundBuffs < DesiredMinFilledSoundBuffs) {
#if dbglog_SoundStuff
            dbglog_writeln("MinFilledSoundBuffs too low");
#endif
            ++TrueEmulatedTime;
        }
#if dbglog_SoundBuffStats
        dbglog_writelnNum("MinFilledSoundBuffs",
                          MinFilledSoundBuffs);
        dbglog_writelnNum("MaxFilledSoundBuffs",
                          MaxFilledSoundBuffs);
        MaxFilledSoundBuffs = 0;
#endif
        MinFilledSoundBuffs = kSoundBuffers + 1;
    }
}

typedef ui4r trSoundTemp;

#define kCenterTempSound 0x8000

#define AudioStepVal 0x0040

#if 3 == kLn2SoundSampSz
#define ConvertTempSoundSampleFromNative(v) ((v) << 8)
#elif 4 == kLn2SoundSampSz
#define ConvertTempSoundSampleFromNative(v) ((v) + kCenterSound)
#else
#error "unsupported kLn2SoundSampSz"
#endif

#if 3 == kLn2SoundSampSz
#define ConvertTempSoundSampleToNative(v) ((v) >> 8)
#elif 4 == kLn2SoundSampSz
#define ConvertTempSoundSampleToNative(v) ((v)-kCenterSound)
#else
#error "unsupported kLn2SoundSampSz"
#endif

LOCALPROC SoundRampTo(trSoundTemp *last_val, trSoundTemp dst_val, tpSoundSamp *stream, int *len) {
    trSoundTemp diff;
    tpSoundSamp p = *stream;
    int n = *len;
    trSoundTemp v1 = *last_val;

    while ((v1 != dst_val) && (0 != n)) {
        if (v1 > dst_val) {
            diff = v1 - dst_val;
            if (diff > AudioStepVal) {
                v1 -= AudioStepVal;
            } else {
                v1 = dst_val;
            }
        } else {
            diff = dst_val - v1;
            if (diff > AudioStepVal) {
                v1 += AudioStepVal;
            } else {
                v1 = dst_val;
            }
        }

        --n;
        *p++ = ConvertTempSoundSampleToNative(v1);
    }

    *stream = p;
    *len = n;
    *last_val = v1;
}

struct MySoundR {
    tpSoundSamp fTheSoundBuffer;
    volatile ui4b(*fPlayOffset);
    volatile ui4b(*fFillOffset);
    volatile ui4b(*fMinFilledSoundBuffs);

    volatile trSoundTemp lastv;

    blnr enabled;
    blnr wantplaying;
    blnr HaveStartedPlaying;

    AudioUnit outputAudioUnit;
};
typedef struct MySoundR MySoundR;

LOCALPROC my_audio_callback(void *udata, void *stream, int len) {
    ui4b ToPlayLen;
    ui4b FilledSoundBuffs;
    int i;
    MySoundR *datp = (MySoundR *)udata;
    tpSoundSamp CurSoundBuffer = datp->fTheSoundBuffer;
    ui4b CurPlayOffset = *datp->fPlayOffset;
    trSoundTemp v0 = datp->lastv;
    trSoundTemp v1 = v0;
    tpSoundSamp dst = (tpSoundSamp)stream;

#if kLn2SoundSampSz > 3
    len >>= (kLn2SoundSampSz - 3);
#endif

#if dbglog_SoundStuff
    dbglog_writeln("Enter my_audio_callback");
    dbglog_writelnNum("len", len);
#endif

label_retry:
    ToPlayLen = *datp->fFillOffset - CurPlayOffset;
    FilledSoundBuffs = ToPlayLen >> kLnOneBuffLen;

    if (!datp->wantplaying) {
#if dbglog_SoundStuff
        dbglog_writeln("playing end transistion");
#endif

        SoundRampTo(&v1, kCenterTempSound, &dst, &len);

        ToPlayLen = 0;
    } else if (!datp->HaveStartedPlaying) {
#if dbglog_SoundStuff
        dbglog_writeln("playing start block");
#endif

        if ((ToPlayLen >> kLnOneBuffLen) < 8) {
            ToPlayLen = 0;
        } else {
            tpSoundSamp p = datp->fTheSoundBuffer + (CurPlayOffset & kAllBuffMask);
            trSoundTemp v2 = ConvertTempSoundSampleFromNative(*p);

#if dbglog_SoundStuff
            dbglog_writeln("have enough samples to start");
#endif

            SoundRampTo(&v1, v2, &dst, &len);

            if (v1 == v2) {
#if dbglog_SoundStuff
                dbglog_writeln("finished start transition");
#endif

                datp->HaveStartedPlaying = trueblnr;
            }
        }
    }

    if (0 == len) {
        /* done */

        if (FilledSoundBuffs < *datp->fMinFilledSoundBuffs) {
            *datp->fMinFilledSoundBuffs = FilledSoundBuffs;
        }
    } else if (0 == ToPlayLen) {
#if dbglog_SoundStuff
        dbglog_writeln("under run");
#endif

        for (i = 0; i < len; ++i) {
            *dst++ = ConvertTempSoundSampleToNative(v1);
        }
        *datp->fMinFilledSoundBuffs = 0;
    } else {
        ui4b PlayBuffContig = kAllBuffLen - (CurPlayOffset & kAllBuffMask);
        tpSoundSamp p = CurSoundBuffer + (CurPlayOffset & kAllBuffMask);

        if (ToPlayLen > PlayBuffContig) {
            ToPlayLen = PlayBuffContig;
        }
        if (ToPlayLen > len) {
            ToPlayLen = len;
        }

        for (i = 0; i < ToPlayLen; ++i) {
            *dst++ = *p++;
        }
        v1 = ConvertTempSoundSampleFromNative(p[-1]);

        CurPlayOffset += ToPlayLen;
        len -= ToPlayLen;

        *datp->fPlayOffset = CurPlayOffset;

        goto label_retry;
    }

    datp->lastv = v1;
}

LOCALFUNC OSStatus audioCallback(void *inRefCon,
                                 AudioUnitRenderActionFlags *ioActionFlags,
                                 const AudioTimeStamp *inTimeStamp,
                                 UInt32 inBusNumber,
                                 UInt32 inNumberFrames,
                                 AudioBufferList *ioData) {
    AudioBuffer *abuf;
    UInt32 i;
    UInt32 n = ioData->mNumberBuffers;

#if dbglog_SoundStuff
    dbglog_writeln("Enter audioCallback");
    dbglog_writelnNum("mNumberBuffers", n);
#endif

    for (i = 0; i < n; i++) {
        abuf = &ioData->mBuffers[i];
        my_audio_callback(inRefCon,
                          abuf->mData, abuf->mDataByteSize);
    }

    return 0;
}

LOCALVAR MySoundR cur_audio;

LOCALPROC ZapAudioVars(void) {
    memset(&cur_audio, 0, sizeof(MySoundR));
}

LOCALPROC MySound_Stop(void) {
#if dbglog_SoundStuff
    dbglog_writeln("enter MySound_Stop");
#endif

    if (cur_audio.wantplaying) {
        OSStatus result;
        ui4r retry_limit = 50; /* half of a second */

        cur_audio.wantplaying = falseblnr;

    label_retry:
        if (kCenterTempSound == cur_audio.lastv) {
#if dbglog_SoundStuff
            dbglog_writeln("reached kCenterTempSound");
#endif

            /* done */
        } else if (0 == --retry_limit) {
#if dbglog_SoundStuff
            dbglog_writeln("retry limit reached");
#endif
            /* done */
        } else {
            /*
             give time back, particularly important
             if got here on a suspend event.
             */
            struct timespec rqt;
            struct timespec rmt;

#if dbglog_SoundStuff
            dbglog_writeln("busy, so sleep");
#endif

            rqt.tv_sec = 0;
            rqt.tv_nsec = 10000000;
            (void)nanosleep(&rqt, &rmt);

            goto label_retry;
        }

        if (noErr != (result = AudioOutputUnitStop(cur_audio.outputAudioUnit))) {
#if dbglog_HAVE
            dbglog_writeln("AudioOutputUnitStop fails");
#endif
        }
    }

#if dbglog_SoundStuff
    dbglog_writeln("leave MySound_Stop");
#endif
}

LOCALPROC MySound_Start(void) {
    OSStatus result;

    if ((!cur_audio.wantplaying) && cur_audio.enabled) {
#if dbglog_SoundStuff
        dbglog_writeln("enter MySound_Start");
#endif

        MySound_Start0();
        cur_audio.lastv = kCenterTempSound;
        cur_audio.HaveStartedPlaying = falseblnr;
        cur_audio.wantplaying = trueblnr;

        if (noErr != (result = AudioOutputUnitStart(cur_audio.outputAudioUnit))) {
#if dbglog_HAVE
            dbglog_writeln("AudioOutputUnitStart fails");
#endif
            cur_audio.wantplaying = falseblnr;
        }

#if dbglog_SoundStuff
        dbglog_writeln("leave MySound_Start");
#endif
    }
}

LOCALPROC MySound_UnInit(void) {
    if (cur_audio.enabled) {
        OSStatus result;
        struct AURenderCallbackStruct callback;

        cur_audio.enabled = falseblnr;

        /* Remove the input callback */
        callback.inputProc = 0;
        callback.inputProcRefCon = 0;

        if (noErr != (result = AudioUnitSetProperty(cur_audio.outputAudioUnit,
                                                    kAudioUnitProperty_SetRenderCallback,
                                                    kAudioUnitScope_Input,
                                                    0,
                                                    &callback,
                                                    sizeof(callback)))) {
#if dbglog_HAVE
            dbglog_writeln(
                "AudioUnitSetProperty fails"
                "(kAudioUnitProperty_SetRenderCallback)");
#endif
        }

        if (noErr != (result = AudioComponentInstanceDispose(cur_audio.outputAudioUnit))) {
#if dbglog_HAVE
            dbglog_writeln("AudioComponentInstanceDispose fails in MySound_UnInit");
#endif
        }
    }
}

#define SOUND_SAMPLERATE 22255 /* = round(7833600 * 2 / 704) */

LOCALFUNC blnr MySound_Init(void) {
    OSStatus result = noErr;
    AudioComponent comp;
    AudioComponentDescription desc;
    struct AURenderCallbackStruct callback;
    AudioStreamBasicDescription requestedDesc;

    cur_audio.fTheSoundBuffer = TheSoundBuffer;
    cur_audio.fPlayOffset = &ThePlayOffset;
    cur_audio.fFillOffset = &TheFillOffset;
    cur_audio.fMinFilledSoundBuffs = &MinFilledSoundBuffs;
    cur_audio.wantplaying = falseblnr;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    requestedDesc.mFormatID = kAudioFormatLinearPCM;
    requestedDesc.mFormatFlags = kLinearPCMFormatFlagIsPacked
#if 3 != kLn2SoundSampSz
                                 | kLinearPCMFormatFlagIsSignedInteger
#endif
        ;
    requestedDesc.mChannelsPerFrame = 1;
    requestedDesc.mSampleRate = SOUND_SAMPLERATE;
    requestedDesc.mBitsPerChannel = (1 << kLn2SoundSampSz);
    requestedDesc.mFramesPerPacket = 1;
    requestedDesc.mBytesPerFrame = (requestedDesc.mBitsPerChannel * requestedDesc.mChannelsPerFrame) >> 3;
    requestedDesc.mBytesPerPacket = requestedDesc.mBytesPerFrame * requestedDesc.mFramesPerPacket;

    callback.inputProc = audioCallback;
    callback.inputProcRefCon = &cur_audio;

    if (NULL == (comp = AudioComponentFindNext(NULL, &desc))) {
#if dbglog_HAVE
        dbglog_writeln(
            "Failed to start CoreAudio: "
            "FindNextComponent returned NULL");
#endif
    } else

        if (noErr != (result = AudioComponentInstanceNew(comp, &cur_audio.outputAudioUnit))) {
#if dbglog_HAVE
        dbglog_writeln("Failed to start CoreAudio: AudioComponentInstanceNew");
#endif
    } else

        if (noErr != (result = AudioUnitInitialize(cur_audio.outputAudioUnit))) {
#if dbglog_HAVE
        dbglog_writeln("Failed to start CoreAudio: AudioUnitInitialize");
#endif
    } else

        if (noErr != (result = AudioUnitSetProperty(cur_audio.outputAudioUnit,
                                                    kAudioUnitProperty_StreamFormat,
                                                    kAudioUnitScope_Input,
                                                    0,
                                                    &requestedDesc,
                                                    sizeof(requestedDesc)))) {
#if dbglog_HAVE
        dbglog_writeln(
            "Failed to start CoreAudio: "
            "AudioUnitSetProperty(kAudioUnitProperty_StreamFormat)");
#endif
    } else

        if (noErr != (result = AudioUnitSetProperty(cur_audio.outputAudioUnit,
                                                    kAudioUnitProperty_SetRenderCallback,
                                                    kAudioUnitScope_Input,
                                                    0,
                                                    &callback,
                                                    sizeof(callback)))) {
#if dbglog_HAVE
        dbglog_writeln(
            "Failed to start CoreAudio: "
            "AudioUnitSetProperty(kAudioUnitProperty_SetInputCallback)");
#endif
    } else

    {
        cur_audio.enabled = trueblnr;

        MySound_Start();
        /*
                         This should be taken care of by LeaveSpeedStopped,
                         but since takes a while to get going properly,
                         start early.
                         */
    }

    return trueblnr; /* keep going, even if no sound */
}

GLOBALPROC MySound_EndWrite(ui4r actL) {
    if (MySound_EndWrite0(actL)) {
    }
}

LOCALPROC MySound_SecondNotify(void) {
    if (cur_audio.enabled) {
        MySound_SecondNotify0();
    }
}

#endif

#pragma mark - platform independent code can be thought of as going here

#include "PROGMAIN.h"

LOCALPROC ZapOSGLUVars(void) {
    InitDrives();
#if MySoundEnabled
    ZapAudioVars();
#endif
}

LOCALPROC ReserveAllocAll(void) {
#if dbglog_HAVE
    dbglog_ReserveAlloc();
#endif
    ReserveAllocOneBlock(&ROM, kROM_Size, 5, falseblnr);

    ReserveAllocOneBlock(&screencomparebuff,
                         vMacScreenNumBytes, 5, trueblnr);

#if UseControlKeys
    ReserveAllocOneBlock(&CntrlDisplayBuff,
                         vMacScreenNumBytes, 5, falseblnr);
#endif

#if MySoundEnabled
    ReserveAllocOneBlock((ui3p *)&TheSoundBuffer,
                         dbhBufferSize, 5, falseblnr);
#endif

    EmulationReserveAlloc();
}

LOCALFUNC blnr AllocMyMemory(void) {
#if 0 /* for testing start up error reporting */
    MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
    return falseblnr;
#else
    uimr n;
    blnr IsOk = falseblnr;

    ReserveAllocOffset = 0;
    ReserveAllocBigBlock = nullpr;
    ReserveAllocAll();
    n = ReserveAllocOffset;
    ReserveAllocBigBlock = (ui3p)calloc(1, n);
    if (NULL == ReserveAllocBigBlock) {
        MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
    } else {
        ReserveAllocOffset = 0;
        ReserveAllocAll();
        if (n != ReserveAllocOffset) {
            /* oops, program error */
        } else {
            IsOk = trueblnr;
        }
    }

    return IsOk;
#endif
}

LOCALPROC UnallocMyMemory(void) {
    if (nullpr != ReserveAllocBigBlock) {
        free((char *)ReserveAllocBigBlock);
    }
}

LOCALVAR blnr CurSpeedStopped = trueblnr;

LOCALPROC LeaveSpeedStopped(void) {
#if MySoundEnabled
    MySound_Start();
#endif
    
    StartUpTimeAdjust();
}

LOCALPROC EnterSpeedStopped(void) {
#if MySoundEnabled
    MySound_Stop();
#endif
}

GLOBALFUNC blnr GetSpeedStopped(void) {
    return CurSpeedStopped;
}

GLOBALPROC SetSpeedStopped(blnr stopped) {
    CurSpeedStopped = stopped;
}

LOCALPROC MacMsgDisplayOn() {
    if (SavedBriefMsg != nullpr) {
        NSString *title = NSStringCreateFromSubstCStr(SavedBriefMsg, falseblnr);
        NSString *message = NSStringCreateFromSubstCStr(SavedLongMsg, falseblnr);
        if ([UIAlertController class]) {
            UIAlertController *alertController = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
            blnr wasStopped = CurSpeedStopped;
            [alertController addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                SetSpeedStopped(wasStopped);
            }]];
            SetSpeedStopped(trueblnr);
            [[UIApplication sharedApplication].keyWindow.rootViewController presentViewController:alertController animated:YES completion:nil];
        } else {
            // fallback for iOS 7
            UIAlertView *alertView = [[UIAlertView alloc] initWithTitle:title message:message delegate:nil cancelButtonTitle:@"OK" otherButtonTitles: nil];
            [alertView show];
        }
        SavedBriefMsg = nullpr;
        SavedLongMsg = nullpr;
    }
}

LOCALFUNC blnr InitOSGLU(void) {
    blnr IsOk = falseblnr;
    @autoreleasepool {
        if (AllocMyMemory())
            if (InitCocoaStuff())
#if dbglog_HAVE
                if (dbglog_open())
#endif
#if MySoundEnabled
                    if (MySound_Init())
/* takes a while to stabilize, do as soon as possible */
#endif
                        if (LoadInitialImages())
                            if (LoadMacRom())
#if UseActvCode
                                if (ActvCodeInit())
#endif
#if EmLocalTalk
                                    if (InitLocalTalk())
#endif
                                        if (InitLocationDat()) {
                                            InitKeyCodes();
                                            IsOk = trueblnr;
                                        }
    }

    return IsOk;
}

LOCALPROC CheckSavedMacMsg(void) {
    if (nullpr != SavedBriefMsg) {
        MacMsgDisplayOn();
    }
}

LOCALPROC UnInitOSGLU(void) {
#if MySoundEnabled
    MySound_Stop();
#endif
#if MySoundEnabled
    MySound_UnInit();
#endif
#if IncludePbufs
    UnInitPbufs();
#endif
    UnInitDrives();

#if dbglog_HAVE
    dbglog_close();
#endif

    CheckSavedMacMsg();
    UnInitCocoaStuff();

    UnallocMyMemory();
}

LOCALPROC CheckForSavedTasks(void) {
    if (MyEvtQNeedRecover) {
        MyEvtQNeedRecover = falseblnr;

        /* attempt cleanup, MyEvtQNeedRecover may get set again */
        MyEvtQTryRecoverFromFull();
    }

    if (RequestMacOff) {
        RequestMacOff = falseblnr;
        ForceMacOff = trueblnr;
    }

    if (ForceMacOff) {
        return;
    }

    if (CurSpeedStopped != SpeedStopped) {
        CurSpeedStopped = !CurSpeedStopped;
        if (CurSpeedStopped) {
            EnterSpeedStopped();
        } else {
            LeaveSpeedStopped();
        }
    }

    if ((nullpr != SavedBriefMsg)) {
        MacMsgDisplayOn();
    }
}

GLOBALFUNC blnr ExtraTimeNotOver(void) {
    UpdateTrueEmulatedTime();
    return TrueEmulatedTime == OnTrueTime;
}

GLOBALPROC WaitForNextTick(void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSRunLoop *mainRunLoop = [NSRunLoop mainRunLoop];
    while (ExtraTimeNotOver()) {
        [mainRunLoop runMode:NSDefaultRunLoopMode beforeDate:[NSDate dateWithTimeIntervalSinceReferenceDate:NextTickChangeTime]];
    }

    CheckForSavedTasks();

    if (ForceMacOff) {
        goto label_exit;
    }

    if (CurSpeedStopped) {
        DoneWithDrawingForTick();
    }

    if (CheckDateTime()) {
#if MySoundEnabled
        MySound_SecondNotify();
#endif
#if EnableDemoMsg
        DemoModeSecondNotify();
#endif
    }

    OnTrueTime = TrueEmulatedTime;

#if dbglog_TimeStuff
    dbglog_writelnNum("WaitForNextTick, OnTrueTime", OnTrueTime);
#endif

label_exit:
    [pool release];
}

GLOBALPROC RunEmulator(void) {
    ZapOSGLUVars();

    if (InitOSGLU()) {
        ProgramMain();
    }
    UnInitOSGLU();
}
