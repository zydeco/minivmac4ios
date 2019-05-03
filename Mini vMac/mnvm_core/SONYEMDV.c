/*
	SONYEMDV.c

	Copyright (C) 2009 Philip Cummins, Jesus A. Alvarez, Paul C. Pratt

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
	SONY floppy disk EMulated DeVice

	The Sony hardware is not actually emulated. Instead the
	ROM is patched to replace the Sony disk driver with
	code that calls Mini vMac extensions implemented in
	the file.

	Information neeeded to better support the Disk Copy 4.2
	format was found in libdc42.c of the Lisa Emulator Project
	by Ray A. Arachelian, and adapted to Mini vMac
	by Jesus A. Alvarez.
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"
#include "ENDIANAC.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#include "MINEM68K.h"
#endif

#include "SONYEMDV.h"

/*
	ReportAbnormalID unused 0x090B - 0x09FF
*/


LOCALVAR ui5b vSonyMountedMask = 0;

#define vSonyIsLocked(Drive_No) \
	((vSonyWritableMask & ((ui5b)1 << (Drive_No))) == 0)
#define vSonyIsMounted(Drive_No) \
	((vSonyMountedMask & ((ui5b)1 << (Drive_No))) != 0)

LOCALFUNC blnr vSonyNextPendingInsert0(tDrive *Drive_No)
{
	/* find next drive to Mount */
	ui5b MountPending = vSonyInsertedMask & (~ vSonyMountedMask);
	if (MountPending != 0) {
		tDrive i;
		for (i = 0; i < NumDrives; ++i) {
			if ((MountPending & ((ui5b)1 << i)) != 0) {
				*Drive_No = i;
				return trueblnr; /* only one disk at a time */
			}
		}
	}

	return falseblnr;
}

LOCALFUNC tMacErr CheckReadableDrive(tDrive Drive_No)
{
	tMacErr result;

	if (Drive_No >= NumDrives) {
		result = mnvm_nsDrvErr;
	} else if (! vSonyIsMounted(Drive_No)) {
		result = mnvm_offLinErr;
	} else {
		result = mnvm_noErr;
	}

	return result;
}

LOCALFUNC tMacErr vSonyTransferVM(blnr IsWrite,
	CPTR Buffera, tDrive Drive_No,
	ui5r Sony_Start, ui5r Sony_Count, ui5r *Sony_ActCount)
{
	/*
		Transfer data between emulated disk and emulated memory. Taking
		into account that the emulated memory may not be contiguous in
		real memory. (Though it generally is for macintosh emulation.)
	*/
	tMacErr result;
	ui5b contig;
	ui5r actual;
	ui3p Buffer;
	ui5r offset = Sony_Start;
	ui5r n = Sony_Count;

label_1:
	if (0 == n) {
		result = mnvm_noErr;
	} else {
		Buffer = get_real_address0(n, ! IsWrite, Buffera, &contig);
		if (0 == contig) {
			result = mnvm_miscErr;
		} else {
			result = vSonyTransfer(IsWrite, Buffer, Drive_No,
				offset, contig, &actual);
			offset += actual;
			Buffera += actual;
			n -= actual;
			if (mnvm_noErr == result) {
				goto label_1;
			}
		}
	}

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = Sony_Count - n;
	}
	return result;
}

LOCALPROC MyMoveBytesVM(CPTR srcPtr, CPTR dstPtr, si5b byteCount)
{
	ui3p src;
	ui3p dst;
	ui5b contigSrc;
	ui5b contigDst;
	ui5r contig;

label_1:
	if (0 != byteCount) {
		src = get_real_address0(byteCount, falseblnr, srcPtr,
			&contigSrc);
		dst = get_real_address0(byteCount, trueblnr,  dstPtr,
			&contigDst);
		if ((0 == contigSrc) || (0 == contigDst)) {
			ReportAbnormalID(0x0901, "MyMoveBytesVM fails");
		} else {
			contig = (contigSrc < contigDst) ? contigSrc : contigDst;
			MyMoveBytes(src, dst, contig);
			srcPtr += contig;
			dstPtr += contig;
			byteCount -= contig;
			goto label_1;
		}
	}
}

LOCALVAR ui5r ImageDataOffset[NumDrives];
	/* size of any header in disk image file */
LOCALVAR ui5r ImageDataSize[NumDrives];
	/* size of disk image file contents */

#if Sony_SupportTags
LOCALVAR ui5r ImageTagOffset[NumDrives];
	/* offset to disk image file tags */
#endif

#if Sony_SupportDC42
#define kDC42offset_diskName      0
#define kDC42offset_dataSize     64
#define kDC42offset_tagSize      68
#define kDC42offset_dataChecksum 72
#define kDC42offset_tagChecksum  76
#define kDC42offset_diskFormat   80
#define kDC42offset_formatByte   81
#define kDC42offset_private      82
#define kDC42offset_userData     84
#endif

#define ChecksumBlockSize 1024

#if Sony_SupportDC42 && Sony_WantChecksumsUpdated
LOCALFUNC tMacErr DC42BlockChecksum(tDrive Drive_No,
	ui5r Sony_Start, ui5r Sony_Count, ui5r *r)
{
	tMacErr result;
	ui5r n;
	ui3b Buffer[ChecksumBlockSize];
	ui3b *p;
	ui5b sum = 0;
	ui5r offset = Sony_Start;
	ui5r remaining = Sony_Count;

	while (0 != remaining) {
		/* read a block */
		if (remaining > ChecksumBlockSize) {
			n = ChecksumBlockSize;
		} else {
			n = remaining;
		}

		result = vSonyTransfer(falseblnr, Buffer, Drive_No, offset,
			n, nullpr);
		if (mnvm_noErr != result) {
			return result;
		}

		offset += n;
		remaining -= n;

		/* add to Checksum */
		p = Buffer;
		n >>= 1; /* n = number of words */
		while (0 != n) {
			--n;
			/* ROR.l sum+word */
			sum += do_get_mem_word(p);
			p += 2;
			sum = (sum >> 1) | ((sum & 1) << 31);
		}
	}

	*r = sum;
	return mnvm_noErr;
}
#endif

#if Sony_SupportDC42 && Sony_WantChecksumsUpdated
#if Sony_SupportTags
#define SizeCheckSumsToUpdate 8
#else
#define SizeCheckSumsToUpdate 4
#endif
#endif

#if Sony_WantChecksumsUpdated
LOCALPROC Drive_UpdateChecksums(tDrive Drive_No)
{
	if (! vSonyIsLocked(Drive_No)) {
		ui5r DataOffset = ImageDataOffset[Drive_No];
#if Sony_SupportDC42
		if (kDC42offset_userData == DataOffset) {
			/* a disk copy 4.2 image */
			tMacErr result;
			ui5r dataChecksum;
			ui3b Buffer[SizeCheckSumsToUpdate];
			ui5r Sony_Count = SizeCheckSumsToUpdate;
			ui5r DataSize = ImageDataSize[Drive_No];

			/* Checksum image data */
			result = DC42BlockChecksum(Drive_No,
				DataOffset, DataSize, &dataChecksum);
			if (mnvm_noErr != result) {
				ReportAbnormalID(0x0902, "Failed to find dataChecksum");
				dataChecksum = 0;
			}
			do_put_mem_long(Buffer, dataChecksum);
#if Sony_SupportTags
			{
				ui5r tagChecksum;
				ui5r TagOffset = ImageTagOffset[Drive_No];
				ui5r TagSize =
					(0 == TagOffset) ? 0 : ((DataSize >> 9) * 12);
				if (TagSize < 12) {
					tagChecksum = 0;
				} else {
					/*
						Checksum of tags doesn't include first block.
						presumably because of bug in original disk
						copy program.
					*/
					result = DC42BlockChecksum(Drive_No,
						TagOffset + 12, TagSize - 12, &tagChecksum);
					if (mnvm_noErr != result) {
						ReportAbnormalID(0x0903,
							"Failed to find tagChecksum");
						tagChecksum = 0;
					}
				}
				do_put_mem_long(Buffer + 4, tagChecksum);
			}
#endif

			/* write Checksums */
			vSonyTransfer(trueblnr, Buffer, Drive_No,
				kDC42offset_dataChecksum, Sony_Count, nullpr);
		}
#endif
	}
}
#endif

#define checkheaderoffset 0
#define checkheadersize 128

#define Sony_SupportOtherFormats Sony_SupportDC42

LOCALFUNC tMacErr vSonyNextPendingInsert(tDrive *Drive_No)
{
	tDrive i;
	tMacErr result;
	ui5r L;

	if (! vSonyNextPendingInsert0(&i)) {
		result = mnvm_nsDrvErr;
	} else {
		result = vSonyGetSize(i, &L);
		if (mnvm_noErr == result) {
			/* first, set up for default format */
			ui5r DataOffset = 0;
			ui5r DataSize = L;
#if Sony_SupportTags
			ui5r TagOffset = 0;
#endif

#if Sony_SupportOtherFormats
#if IncludeSonyRawMode
			if (! vSonyRawMode)
#endif
			{
				ui5r DataOffset0;
				ui5r DataSize0;
				ui5r TagOffset0;
				ui5r TagSize0;
				ui3b Temp[checkheadersize];
				ui5r Sony_Count = checkheadersize;
				blnr gotFormat = falseblnr;

				result = vSonyTransfer(falseblnr, Temp, i,
					checkheaderoffset, Sony_Count, nullpr);
				if (mnvm_noErr == result) {
#if Sony_SupportDC42
					/* Detect Disk Copy 4.2 image */
					if (0x0100 == do_get_mem_word(
						&Temp[kDC42offset_private]))
					{
						/* DC42 signature found, check sizes */
						DataSize0 = do_get_mem_long(
							&Temp[kDC42offset_dataSize]);
						TagSize0 = do_get_mem_long(
							&Temp[kDC42offset_tagSize]);
						DataOffset0 = kDC42offset_userData;
						TagOffset0 = DataOffset0 + DataSize0;
						if (L >= (TagOffset0 + TagSize0))
						if (0 == (DataSize0 & 0x01FF))
						if ((DataSize0 >> 9) >= 4)
						if (Temp[kDC42offset_diskName] < 64)
							/* length of pascal string */
						{
							if (0 == TagSize0) {
								/* no tags */
								gotFormat = trueblnr;
							} else if ((DataSize0 >> 9) * 12
								== TagSize0)
							{
								/* 12 byte tags */
								gotFormat = trueblnr;
							}
							if (gotFormat) {
#if Sony_VerifyChecksums /* mostly useful to check the Checksum code */
								ui5r dataChecksum;
								ui5r tagChecksum;
								ui5r dataChecksum0 = do_get_mem_long(
									&Temp[kDC42offset_dataChecksum]);
								ui5r tagChecksum0 = do_get_mem_long(
									&Temp[kDC42offset_tagChecksum]);
								result = DC42BlockChecksum(i,
									DataOffset0, DataSize0,
									&dataChecksum);
								if (TagSize0 >= 12) {
									result = DC42BlockChecksum(i,
										TagOffset0 + 12, TagSize0 - 12,
										&tagChecksum);
								} else {
									tagChecksum = 0;
								}
								if (dataChecksum != dataChecksum0) {
									ReportAbnormalID(0x0904,
										"bad dataChecksum");
								}
								if (tagChecksum != tagChecksum0) {
									ReportAbnormalID(0x0905,
										"bad tagChecksum");
								}
#endif
								DataOffset = DataOffset0;
								DataSize = DataSize0;
#if Sony_SupportTags
								TagOffset =
									(0 == TagSize0) ? 0 : TagOffset0;
#endif

#if (! Sony_SupportTags) || (! Sony_WantChecksumsUpdated)
								if (! vSonyIsLocked(i)) {
#if ! Sony_WantChecksumsUpdated
									/* unconditionally revoke */
#else
									if (0 != TagSize0)
#endif
									{
										DiskRevokeWritable(i);
									}
								}
#endif
							}
						}
					}
#endif /* Sony_SupportDC42 */
				}
			}
			if (mnvm_noErr == result)
#endif /* Sony_SupportOtherFormats */
			{
				vSonyMountedMask |= ((ui5b)1 << i);

				ImageDataOffset[i] = DataOffset;
				ImageDataSize[i] = DataSize;
#if Sony_SupportTags
				ImageTagOffset[i] = TagOffset;
#endif

				*Drive_No = i;
			}
		}

		if (mnvm_noErr != result) {
			(void) vSonyEject(i);
		}
	}

	return result;
}

#define MinTicksBetweenInsert 240
	/*
		if call PostEvent too frequently, insert events seem to get lost
	*/

LOCALVAR ui4r DelayUntilNextInsert;

LOCALVAR CPTR MountCallBack = 0;

/* This checks to see if a disk (image) has been inserted */
GLOBALPROC Sony_Update (void)
{
	if (DelayUntilNextInsert != 0) {
		--DelayUntilNextInsert;
	} else {
		if (MountCallBack != 0) {
			tDrive i;

			if (mnvm_noErr == vSonyNextPendingInsert(&i)) {
				ui5b data = i;

				if (vSonyIsLocked(i)) {
					data |= ((ui5b)0x00FF) << 16;
				}

				DiskInsertedPsuedoException(MountCallBack, data);

#if IncludeSonyRawMode
				if (! vSonyRawMode)
#endif
				{
					DelayUntilNextInsert = MinTicksBetweenInsert;
					/*
						but usually will reach kDriveStatus first,
						where shorten delay.
					*/
				}
			}
		}
	}
}

LOCALFUNC tMacErr Drive_Transfer(blnr IsWrite, CPTR Buffera,
	tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count,
	ui5r *Sony_ActCount)
{
	tMacErr result;

	QuietEnds();

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = 0;
	}

	result = CheckReadableDrive(Drive_No);
	if (mnvm_noErr == result) {
		if (IsWrite && vSonyIsLocked(Drive_No)) {
			result = mnvm_vLckdErr;
		} else {
			ui5r DataSize = ImageDataSize[Drive_No];
			if (Sony_Start > DataSize) {
				result = mnvm_eofErr;
			} else {
				blnr hit_eof = falseblnr;
				ui5r L = DataSize - Sony_Start;
				if (L >= Sony_Count) {
					L = Sony_Count;
				} else {
					hit_eof = trueblnr;
				}
				result = vSonyTransferVM(IsWrite, Buffera, Drive_No,
					ImageDataOffset[Drive_No] + Sony_Start, L,
					Sony_ActCount);
				if ((mnvm_noErr == result) && hit_eof) {
					result = mnvm_eofErr;
				}
			}
		}
	}

	return result;
}

LOCALVAR blnr QuitOnEject = falseblnr;

GLOBALPROC Sony_SetQuitOnEject(void)
{
	QuitOnEject = trueblnr;
}

LOCALFUNC tMacErr Drive_Eject(tDrive Drive_No)
{
	tMacErr result;

	result = CheckReadableDrive(Drive_No);
	if (mnvm_noErr == result) {
		vSonyMountedMask &= ~ ((ui5b)1 << Drive_No);
#if Sony_WantChecksumsUpdated
		Drive_UpdateChecksums(Drive_No);
#endif
		result = vSonyEject(Drive_No);
		if (QuitOnEject != 0) {
			if (! AnyDiskInserted()) {
				ForceMacOff = trueblnr;
			}
		}
	}

	return result;
}

#if IncludeSonyNew
LOCALFUNC tMacErr Drive_EjectDelete(tDrive Drive_No)
{
	tMacErr result;

	result = CheckReadableDrive(Drive_No);
	if (mnvm_noErr == result) {
		if (vSonyIsLocked(Drive_No)) {
			result = mnvm_vLckdErr;
		} else {
			vSonyMountedMask &= ~ ((ui5b)1 << Drive_No);
			result = vSonyEjectDelete(Drive_No);
		}
	}

	return result;
}
#endif

GLOBALPROC Sony_EjectAllDisks(void)
{
	tDrive i;

	vSonyMountedMask = 0;
	for (i = 0; i < NumDrives; ++i) {
		if (vSonyIsInserted(i)) {
#if Sony_WantChecksumsUpdated
			Drive_UpdateChecksums(i);
#endif
			(void) vSonyEject(i);
		}
	}
}

GLOBALPROC Sony_Reset(void)
{
	DelayUntilNextInsert = 0;
	QuitOnEject = falseblnr;
	MountCallBack = 0;
}

/*
	Mini vMac extension for low level access to disk operations.
*/

#define kCmndDiskNDrives 1
#define kCmndDiskRead 2
#define kCmndDiskWrite 3
#define kCmndDiskEject 4
#define kCmndDiskGetSize 5
#define kCmndDiskGetCallBack 6
#define kCmndDiskSetCallBack 7
#define kCmndDiskQuitOnEject 8
#define kCmndDiskFeatures 9
#define kCmndDiskNextPendingInsert 10
#if IncludeSonyRawMode
#define kCmndDiskGetRawMode 11
#define kCmndDiskSetRawMode 12
#endif
#if IncludeSonyNew
#define kCmndDiskNew 13
#define kCmndDiskGetNewWanted 14
#define kCmndDiskEjectDelete 15
#endif
#if IncludeSonyGetName
#define kCmndDiskGetName 16
#endif

#define kFeatureCmndDisk_RawMode 0
#define kFeatureCmndDisk_New 1
#define kFeatureCmndDisk_NewName 2
#define kFeatureCmndDisk_GetName 3

#define kParamDiskNumDrives 8
#define kParamDiskStart 8
#define kParamDiskCount 12
#define kParamDiskBuffer 16
#define kParamDiskDrive_No 20

GLOBALPROC ExtnDisk_Access(CPTR p)
{
	tMacErr result = mnvm_controlErr;

	switch (get_vm_word(p + ExtnDat_commnd)) {
		case kCmndVersion:
			put_vm_word(p + ExtnDat_version, 2);
			result = mnvm_noErr;
			break;
		case kCmndDiskNDrives: /* count drives */
			put_vm_word(p + kParamDiskNumDrives, NumDrives);
			result = mnvm_noErr;
			break;
		case kCmndDiskRead:
			{
				ui5r Sony_ActCount;
				CPTR Buffera = get_vm_long(p + kParamDiskBuffer);
				tDrive Drive_No = get_vm_word(p + kParamDiskDrive_No);
				ui5r Sony_Start = get_vm_long(p + kParamDiskStart);
				ui5r Sony_Count = get_vm_long(p + kParamDiskCount);

				result = Drive_Transfer(falseblnr, Buffera, Drive_No,
					Sony_Start, Sony_Count, &Sony_ActCount);

				put_vm_long(p + kParamDiskCount, Sony_ActCount);
			}
			break;
		case kCmndDiskWrite:
			{
				ui5r Sony_ActCount;
				CPTR Buffera = get_vm_long(p + kParamDiskBuffer);
				tDrive Drive_No = get_vm_word(p + kParamDiskDrive_No);
				ui5r Sony_Start = get_vm_long(p + kParamDiskStart);
				ui5r Sony_Count = get_vm_long(p + kParamDiskCount);

				result = Drive_Transfer(trueblnr, Buffera, Drive_No,
					Sony_Start, Sony_Count, &Sony_ActCount);

				put_vm_long(p + kParamDiskCount, Sony_ActCount);
			}
			break;
		case kCmndDiskEject:
			{
				tDrive Drive_No = get_vm_word(p + kParamDiskDrive_No);
				result = Drive_Eject(Drive_No);
			}
			break;
		case kCmndDiskGetSize:
			{
				tDrive Drive_No = get_vm_word(p + kParamDiskDrive_No);

				result = CheckReadableDrive(Drive_No);
				if (mnvm_noErr == result) {
					put_vm_long(p + kParamDiskCount,
						ImageDataSize[Drive_No]);
					result = mnvm_noErr;
				}
			}
			break;
		case kCmndDiskGetCallBack:
			put_vm_long(p + kParamDiskBuffer, MountCallBack);
			result = mnvm_noErr;
			break;
		case kCmndDiskSetCallBack:
			MountCallBack = get_vm_long(p + kParamDiskBuffer);
			result = mnvm_noErr;
			break;
		case kCmndDiskQuitOnEject:
			QuitOnEject = trueblnr;
			result = mnvm_noErr;
			break;
		case kCmndDiskFeatures:
			{
				ui5r v = (0
#if IncludeSonyRawMode
					| ((ui5b)1 << kFeatureCmndDisk_RawMode)
#endif
#if IncludeSonyNew
					| ((ui5b)1 << kFeatureCmndDisk_New)
#endif
#if IncludeSonyNameNew
					| ((ui5b)1 << kFeatureCmndDisk_NewName)
#endif
#if IncludeSonyGetName
					| ((ui5b)1 << kFeatureCmndDisk_GetName)
#endif
					);

				put_vm_long(p + ExtnDat_params + 0, v);
				result = mnvm_noErr;
			}
			break;
		case kCmndDiskNextPendingInsert:
			{
				tDrive i;

				result = vSonyNextPendingInsert(&i);
				if (mnvm_noErr == result) {
					put_vm_word(p + kParamDiskDrive_No, i);
				}
			}
			break;
#if IncludeSonyRawMode
		case kCmndDiskGetRawMode:
			put_vm_word(p + kParamDiskBuffer, vSonyRawMode);
			result = mnvm_noErr;
			break;
		case kCmndDiskSetRawMode:
			vSonyRawMode = get_vm_word(p + kParamDiskBuffer);
			result = mnvm_noErr;
			break;
#endif
#if IncludeSonyNew
		case kCmndDiskNew:
			{
				ui5b count = get_vm_long(p + ExtnDat_params + 0);
				tPbuf Pbuf_No = get_vm_word(p + ExtnDat_params + 4);
				/* reserved word at offset 6, should be zero */

				result = mnvm_noErr;

#if IncludePbufs
				if (Pbuf_No != NotAPbuf) {
					result = CheckPbuf(Pbuf_No);
					if (mnvm_noErr == result) {
						vSonyNewDiskWanted = trueblnr;
						vSonyNewDiskSize = count;
#if IncludeSonyNameNew
						if (vSonyNewDiskName != NotAPbuf) {
							PbufDispose(vSonyNewDiskName);
						}
						vSonyNewDiskName = Pbuf_No;
#else
						PbufDispose(Pbuf_No);
#endif
					}
				} else
#endif
				{
					vSonyNewDiskWanted = trueblnr;
					vSonyNewDiskSize = count;
				}
			}
			break;
		case kCmndDiskGetNewWanted:
			put_vm_word(p + kParamDiskBuffer, vSonyNewDiskWanted);
			result = mnvm_noErr;
			break;
		case kCmndDiskEjectDelete:
			{
				tDrive Drive_No = get_vm_word(p + kParamDiskDrive_No);
				result = Drive_EjectDelete(Drive_No);
			}
			break;
#endif
#if IncludeSonyGetName
		case kCmndDiskGetName:
			{
				tDrive Drive_No = get_vm_word(p + ExtnDat_params + 0);
				/* reserved word at offset 2, should be zero */
				result = CheckReadableDrive(Drive_No);
				if (mnvm_noErr == result) {
					tPbuf Pbuf_No;
					result = vSonyGetName(Drive_No, &Pbuf_No);
					put_vm_word(p + ExtnDat_params + 4, Pbuf_No);
				}
			}
			break;
#endif
	}

	put_vm_word(p + ExtnDat_result, result);
}


/*
	Mini vMac extension that implements most of the logic
	of the replacement disk driver patched into the emulated ROM.
	(sony_driver in ROMEMDEV.c)

	This logic used to be completely contained in the 68k code
	of the replacement driver, using only the low level
	disk access extension.
*/

/* Sony Variable Drive Setting Offsets */

#define kTrack       0 /* Current Track */
#define kWriteProt   2 /* FF if Write Protected, 00 if readable */
#define kDiskInPlace 3
	/*
		00 = No Disk, 01 = Disk In,
		2 = MacOS Read, FC-FF = Just Ejected
	*/
#define kInstalled   4
	/* 00 = Unknown, 01 = Installed, FF = Not Installed */
#define kSides       5
	/* 00 if Single Sided Drive, FF if Doubled Sided Drive */
#define kQLink       6 /* Link to Next Drive */
#define kQType      10 /* Drive Type (0 = Size Saved, 1 = Very Large) */
#define kQDriveNo   12 /* Drive Number (1 = Internal, 2 = External) */
#define kQRefNum    14
	/* Driver Reference Number (-5 for .Sony, FFFB) */
#define kQFSID      16 /* File System ID (0 = MacOS) */
#define kQDrvSz     18 /* size, low-order word */
#define kQDrvSz2    20 /* size, hi-order word */

#define kTwoSideFmt 18
	/* FF if double-sided format, 00 if single-sided format */
#define kNewIntf    19
	/* FF if new 800K interface or 00 if old 400K interface */
#define kDriveErrs  20 /* Drive Soft Errors */

/* Sony Driver Control Call csCodes */

#define kKillIO             1
#define kVerifyDisk         5
#define kFormatDisk         6
#define kEjectDisk          7
#define kSetTagBuffer       8
#define kTrackCacheControl  9
#define kGetIconID         20
#define kDriveIcon         21
#define kMediaIcon         22
#define kDriveInfo         23
#define kFormatCopy     21315

/* Sony Driver Status Call csCodes */

#define kReturnFormatList  6
#define kDriveStatus       8
#define kMFMStatus        10
#define kDuplicatorVersionSupport  17494

/* Parameter Block Offsets */

#define kqLink         0
#define kqType         4
#define kioTrap        6
#define kioCmdAddr     8
#define kioCompletion 12
#define kioResult     16
#define kioNamePtr    18
#define kioVRefNum    22
#define kioRefNum     24
#define kcsCode       26
#define kcsParam      28
#define kioBuffer     32 /* Buffer to store data into */
#define kioReqCount   36 /* Requested Number of Bytes */
#define kioActCount   40 /* Actual Number of Bytes obtained */
#define kioPosMode    44 /* Positioning Mode */
#define kioPosOffset  46 /* Position Offset */

/* Positioning Modes */

#define kfsAtMark    0 /* At Mark (Ignore PosOffset) */
#define kfsFromStart 1 /* At Start (PosOffset is absolute) */
#define kfsFromLEOF  2 /* From Logical End of File - PosOffset */
#define kfsFromMark  3 /* At Mark + PosOffset */

/* Device Control Entry Offsets */

#define kdCtlPosition 16

#if 0
struct MyDriverDat_R {
	ui5b zeroes[4];  /*  0 */
	ui5b checkval;   /* 16 */
	ui5b pokeaddr;   /* 20 */
	ui4b NumDrives;  /* 24 */
	ui4b DiskExtn;   /* 26 */
	TMTask NullTask; /* 28 */
	/* total size must be <= FirstDriveVarsOffset */
};

typedef struct MyDriverDat_R MyDriverDat_R;
#endif


#if CurEmMd <= kEmMd_Twiggy

#define SonyVarsPtr 0x0128 /* TwiggyVars, actually */

#if CurEmMd <= kEmMd_Twig43
#define MinSonVarsSize 0x000000FA
#define FirstDriveVarsOffset 0x004A
#define EachDriveVarsSize 0x0042
#else
#define MinSonVarsSize 0x000000E6
#define FirstDriveVarsOffset 0x004C
#define EachDriveVarsSize 0x002E
#endif

#else

#define SonyVarsPtr 0x0134

#define FirstDriveVarsOffset 0x004A
#define EachDriveVarsSize 0x0042
#if CurEmMd <= kEmMd_128K
#define MinSonVarsSize 0x000000FA
#else
#define MinSonVarsSize 0x00000310
#endif

#endif

#define kcom_checkval 0x841339E2

#define Sony_dolog (dbglog_HAVE && 0)

#if Sony_SupportTags
LOCALVAR CPTR TheTagBuffer;
#endif

LOCALFUNC ui5b DriveVarsLocation(tDrive Drive_No)
{
	CPTR SonyVars = get_vm_long(SonyVarsPtr);

	if (Drive_No < NumDrives) {
		return SonyVars + FirstDriveVarsOffset
			+ EachDriveVarsSize * Drive_No;
	} else {
		return 0;
	}
}

LOCALFUNC tMacErr Sony_Mount(CPTR p)
{
	ui5b data = get_vm_long(p + ExtnDat_params + 0);
	tMacErr result = mnvm_miscErr;
	tDrive i = data & 0x0000FFFF;
	CPTR dvl = DriveVarsLocation(i);

	if (0 == dvl) {
#if Sony_dolog
		dbglog_WriteNote("Sony : Mount : no dvl");
#endif

		result = mnvm_nsDrvErr;
	} else if (get_vm_byte(dvl + kDiskInPlace) == 0x00) {
		ui5b L = ImageDataSize[i] >> 9; /* block count */

#if Sony_dolog
		dbglog_StartLine();
		dbglog_writeCStr("Sony : Mount : Drive=");
		dbglog_writeHex(i);
		dbglog_writeCStr(", L=");
		dbglog_writeHex(L);
		dbglog_writeReturn();
#endif

#if CurEmMd <= kEmMd_Twiggy
		if (L == 1702) {
			put_vm_byte(dvl + kTwoSideFmt, 0xFF);
				/* Drive i Single Format */
			put_vm_byte(dvl + kNewIntf, 0x00);
				/* Drive i doesn't use new interface */
			put_vm_word(dvl + kQType, 0x00); /* Drive Type */
			put_vm_word(dvl + kDriveErrs, 0x0000);
				/* Drive i has no errors */
		} else
#else
		if ((L == 800)
#if CurEmMd > kEmMd_128K
			|| (L == 1600)
#endif
		)
		{
#if CurEmMd <= kEmMd_128K
			put_vm_byte(dvl + kTwoSideFmt, 0x00);
				/* Drive i Single Format */
			put_vm_byte(dvl + kNewIntf, 0x00);
				/* Drive i doesn't use new interface */
#else
			if (L == 800) {
				put_vm_byte(dvl + kTwoSideFmt, 0x00);
					/* Drive i Single Format */
			} else {
				put_vm_byte(dvl + kTwoSideFmt, 0xFF);
					/* Drive Double Format */
			}
			put_vm_byte(dvl + kNewIntf, 0xFF);
				/* Drive i uses new interface */
#endif
			put_vm_word(dvl + kQType, 0x00); /* Drive Type */
			put_vm_word(dvl + kDriveErrs, 0x0000);
				/* Drive i has no errors */
		} else
#endif
		{
			put_vm_word(dvl + kQRefNum, 0xFFFE);  /* Driver */
			put_vm_word(dvl + kQType, 0x01); /* Drive Type */
			put_vm_word(dvl + kQDrvSz , L);
			put_vm_word(dvl + kQDrvSz2, L >> 16);
		}

#if CurEmMd <= kEmMd_Twiggy
		put_vm_word(dvl + kQFSID, 0x00); /* kQFSID must be 0 for 4.3T */
#endif

		put_vm_byte(dvl + kWriteProt, data >> 16);
		put_vm_byte(dvl + kDiskInPlace, 0x01); /* Drive Disk Inserted */

		put_vm_long(p + ExtnDat_params + 4, i + 1);
			/* PostEvent Disk Inserted eventMsg */
		result = mnvm_noErr;
	} else {
		/* disk already in place, a mistake has been made */
#if Sony_dolog
		dbglog_WriteNote("Sony : Mount : already in place");
#endif
	}

	return result;
}

#if Sony_SupportTags
LOCALFUNC tMacErr Sony_PrimeTags(tDrive Drive_No,
	ui5r Sony_Start, ui5r Sony_Count, blnr IsWrite)
{
	tMacErr result = mnvm_noErr;
	ui5r TagOffset = ImageTagOffset[Drive_No];

	if ((0 != TagOffset) && (Sony_Count > 0)) {
		ui5r block = Sony_Start >> 9;
		ui5r n = Sony_Count >> 9; /* is >= 1 if get here */

		TagOffset += block * 12;

		if (0 != TheTagBuffer) {
			ui5r count = 12 * n;
			result = vSonyTransferVM(IsWrite, TheTagBuffer, Drive_No,
				TagOffset, count, nullpr);
			if (mnvm_noErr == result) {
				MyMoveBytesVM(TheTagBuffer + count - 12, 0x02FC, 12);
			}
		} else {
			if (! IsWrite) {
				/* only need to read the last block tags */
				ui5r count = 12;
				TagOffset += 12 * (n - 1);
				result = vSonyTransferVM(falseblnr, 0x02FC, Drive_No,
					TagOffset, count, nullpr);
			} else {
				ui5r count = 12;
				ui4r BufTgFBkNum = get_vm_word(0x0302);
				do {
					put_vm_word(0x0302, BufTgFBkNum);
					result = vSonyTransferVM(trueblnr, 0x02FC, Drive_No,
						TagOffset, count, nullpr);
					if (mnvm_noErr != result) {
						goto label_fail;
					}
					BufTgFBkNum += 1;
					TagOffset += 12;
				} while (--n != 0);
			}
		}
	}

label_fail:
	return result;
}
#endif

/* Handles I/O to disks */
LOCALFUNC tMacErr Sony_Prime(CPTR p)
{
	tMacErr result;
	ui5r Sony_Count;
	ui5r Sony_Start;
	ui5r Sony_ActCount = 0;
	CPTR ParamBlk = get_vm_long(p + ExtnDat_params + 0);
	CPTR DeviceCtl = get_vm_long(p + ExtnDat_params + 4);
	tDrive Drive_No = get_vm_word(ParamBlk + kioVRefNum) - 1;
	ui4r IOTrap = get_vm_word(ParamBlk + kioTrap);
	CPTR dvl = DriveVarsLocation(Drive_No);

	if (0 == dvl) {
#if Sony_dolog
		dbglog_WriteNote("Sony : Prime : no dvl");
#endif

		result = mnvm_nsDrvErr;
	} else
#if CurEmMd >= kEmMd_Twiggy
	if (0xA002 != (IOTrap & 0xF0FE)) {
#if Sony_dolog
		dbglog_WriteNote("Sony : Prime : "
			"not read (0xA002) or write (0xA003)");
#endif

		result = mnvm_controlErr;
	} else
#endif
	{
		blnr IsWrite = (0 != (IOTrap & 0x0001));
		ui3b DiskInPlaceV = get_vm_byte(dvl + kDiskInPlace);

		if (DiskInPlaceV != 0x02) {
			if (DiskInPlaceV == 0x01) {
				put_vm_byte(dvl + kDiskInPlace, 0x02); /* Clamp Drive */
			} else {
				result = mnvm_offLinErr;
				goto label_fail;
				/*
					if don't check for this, will go right
					ahead and boot off a disk that hasn't
					been mounted yet by Sony_Update.
					(disks other than the boot disk aren't
					seen unless mounted by Sony_Update)
				*/
			}
		}

#if 0
		ui4r PosMode = get_vm_word(ParamBlk + kioPosMode);

		if (0 != (PosMode & 64)) {
#if ExtraAbnormalReports
			/*
				This is used when copy to floppy
				disk with Finder. But not implemented
				yet.
			*/
			ReportAbnormalID(0x0906, "read verify mode requested");
#endif
			PosMode &= ~ 64;
		}

		/*
			Don't use the following code, because
			according to Apple's Technical Note FL24
			the Device Manager takes care of this,
			and puts the result in dCtlPosition.
			(The RAMDisk example in Apple's sample
			code serves to confirm this. Further
			evidence found in Basilisk II emulator,
			and disassembly of Mac Plus disk driver.)
		*/
		ui5r PosOffset = get_vm_long(ParamBlk + kioPosOffset);
		switch (PosMode) {
			case kfsAtMark:
				Sony_Start = get_vm_long(DeviceCtl + kdCtlPosition);
				break;
			case kfsFromStart:
				Sony_Start = PosOffset;
				break;
#if 0
			/*
				not valid for device driver.
				actually only kfsFromStart seems to be used.
			*/
			case kfsFromLEOF:
				Sony_Start = ImageDataSize[Drive_No]
					+ PosOffset;
				break;
#endif
			case kfsFromMark:
				Sony_Start = PosOffset
					+ get_vm_long(DeviceCtl + kdCtlPosition);
				break;
			default:
				ReportAbnormalID(0x0907, "unknown PosMode");
				result = mnvm_paramErr;
				goto label_fail;
				break;
		}
#endif
		Sony_Start = get_vm_long(DeviceCtl + kdCtlPosition);

		Sony_Count = get_vm_long(ParamBlk + kioReqCount);

#if Sony_dolog
		dbglog_StartLine();
		dbglog_writeCStr("Sony : Prime : Drive=");
		dbglog_writeHex(Drive_No);
		dbglog_writeCStr(", IsWrite=");
		dbglog_writeHex(IsWrite);
		dbglog_writeCStr(", Start=");
		dbglog_writeHex(Sony_Start);
		dbglog_writeCStr(", Count=");
		dbglog_writeHex(Sony_Count);
		dbglog_writeReturn();
#endif

		if ((0 != (Sony_Start & 0x1FF))
			|| (0 != (Sony_Count & 0x1FF)))
		{
			/* only whole blocks allowed */
#if ExtraAbnormalReports
			ReportAbnormalID(0x0908, "not blockwise in Sony_Prime");
#endif
			result = mnvm_paramErr;
		} else if (IsWrite && (get_vm_byte(dvl + kWriteProt) != 0)) {
			result = mnvm_wPrErr;
		} else {
			CPTR Buffera = get_vm_long(ParamBlk + kioBuffer);
			result = Drive_Transfer(IsWrite, Buffera, Drive_No,
					Sony_Start, Sony_Count, &Sony_ActCount);
#if Sony_SupportTags
			if (mnvm_noErr == result) {
				result = Sony_PrimeTags(Drive_No,
					Sony_Start, Sony_Count, IsWrite);
			}
#endif
			put_vm_long(DeviceCtl + kdCtlPosition,
				Sony_Start + Sony_ActCount);
		}
	}

label_fail:
	put_vm_word(ParamBlk + kioResult, result);
	put_vm_long(ParamBlk + kioActCount, Sony_ActCount);

	if (mnvm_noErr != result) {
		put_vm_word(0x0142 /* DskErr */, result);
	}
	return result;
}

/* Implements control csCodes for the Sony driver */
LOCALFUNC tMacErr Sony_Control(CPTR p)
{
	tMacErr result;
	CPTR ParamBlk = get_vm_long(p + ExtnDat_params + 0);
	/* CPTR DeviceCtl = get_vm_long(p + ExtnDat_params + 4); */
	ui4r OpCode = get_vm_word(ParamBlk + kcsCode);

	if (kKillIO == OpCode) {
#if Sony_dolog
		dbglog_WriteNote("Sony : Control : kKillIO");
#endif

		result = mnvm_miscErr;
	} else if (kSetTagBuffer == OpCode) {
#if Sony_dolog
		dbglog_WriteNote("Sony : Control : kSetTagBuffer");
#endif

#if Sony_SupportTags
		TheTagBuffer = get_vm_long(ParamBlk + kcsParam);
		result = mnvm_noErr;
#else
		result = mnvm_controlErr;
#endif
	} else if (kTrackCacheControl == OpCode) {
#if Sony_dolog
		dbglog_WriteNote("Sony : Control : kTrackCacheControl");
#endif

#if CurEmMd <= kEmMd_128K
		result = mnvm_controlErr;
#else
#if 0
		ui3r Arg1 = get_vm_byte(ParamBlk + kcsParam);
		ui3r Arg2 = get_vm_byte(ParamBlk + kcsParam + 1);
		if (0 == Arg1) {
			/* disable track cache */
		} else {
			/* enable track cache */
		}
		if (Arg2 < 0) {
			/* remove track cache */
		} else if (Arg2 > 0) {
			/* install track cache */
		}
#endif
		result = mnvm_noErr;
			/* not implemented, but pretend we did it */
#endif
	} else {
		tDrive Drive_No = get_vm_word(ParamBlk + kioVRefNum) - 1;
		CPTR dvl = DriveVarsLocation(Drive_No);

		if (0 == dvl) {
#if Sony_dolog
			dbglog_WriteNote("Sony : Control : no dvl");
#endif

			result = mnvm_nsDrvErr;
		} else if (get_vm_byte(dvl + kDiskInPlace) == 0) {
#if Sony_dolog
			dbglog_WriteNote("Sony : Control : not DiskInPlace");
#endif

			result = mnvm_offLinErr;
		} else {
			switch (OpCode) {
				case kVerifyDisk :
#if Sony_dolog
					dbglog_WriteNote("Sony : Control : kVerifyDisk");
#endif

					result = mnvm_noErr;
					break;
				case kEjectDisk :
#if Sony_dolog
					dbglog_StartLine();
					dbglog_writeCStr("Sony : Control : kEjectDisk : ");
					dbglog_writeHex(Drive_No);
					dbglog_writeReturn();
#endif

					put_vm_byte(dvl + kWriteProt, 0x00);
						/* Drive Writeable */
					put_vm_byte(dvl + kDiskInPlace, 0x00);
						/* Drive No Disk */
#if 0
					put_vm_byte(dvl + kTwoSideFmt, 0x00);
						/* Drive Single Format (Initially) */
#endif
					put_vm_word(dvl + kQRefNum, 0xFFFB);
						/* Drive i uses .Sony */

					result = Drive_Eject(Drive_No);
					break;
				case kFormatDisk :
#if Sony_dolog
					dbglog_StartLine();
					dbglog_writeCStr("Sony : Control : kFormatDisk : ");
					dbglog_writeHex(Drive_No);
					dbglog_writeReturn();
#endif

					result = mnvm_noErr;
					break;
				case kDriveIcon :
#if Sony_dolog
					dbglog_StartLine();
					dbglog_writeCStr("Sony : Control : kDriveIcon : ");
					dbglog_writeHex(Drive_No);
					dbglog_writeReturn();
#endif

					if (get_vm_word(dvl + kQType) != 0) {
						put_vm_long(ParamBlk + kcsParam,
							my_disk_icon_addr);
						result = mnvm_noErr;
					} else {
						result = mnvm_controlErr;
							/*
								Driver can't respond to
								this Control call (-17)
							*/
					}
					break;
#if CurEmMd >= kEmMd_SE
				case kDriveInfo :
					{
						ui5b v;

#if Sony_dolog
						dbglog_StartLine();
						dbglog_writeCStr(
							"Sony : Control : kDriveInfo : ");
						dbglog_writeHex(kDriveIcon);
						dbglog_writeReturn();
#endif

						if (get_vm_word(dvl + kQType) != 0) {
							v = 0x00000001; /* unspecified drive */
						} else {
#if CurEmMd <= kEmMd_128K
							v = 0x00000002; /* 400K Drive */
#else
							v = 0x00000003; /* 800K Drive */
#endif
						}
						if (Drive_No != 0) {
							v += 0x00000900;
								/* Secondary External Drive */
						}
						put_vm_long(ParamBlk + kcsParam, v);
						result = mnvm_noErr; /* No error (0) */
					}
					break;
#endif
				default :
#if Sony_dolog
					dbglog_StartLine();
					dbglog_writeCStr("Sony : Control : OpCode : ");
					dbglog_writeHex(OpCode);
					dbglog_writeReturn();
#endif
#if ExtraAbnormalReports
					if ((kGetIconID != OpCode)
						&& (kMediaIcon != OpCode)
						&& (kDriveInfo != OpCode))
					{
						ReportAbnormalID(0x0909,
							"unexpected OpCode in Sony_Control");
					}
#endif
					result = mnvm_controlErr;
						/*
							Driver can't respond to
							this Control call (-17)
						*/
					break;
			}
		}
	}

	if (mnvm_noErr != result) {
		put_vm_word(0x0142 /* DskErr */, result);
	}
	return result;
}

/* Handles the DriveStatus call */
LOCALFUNC tMacErr Sony_Status(CPTR p)
{
	tMacErr result;
	CPTR ParamBlk = get_vm_long(p + ExtnDat_params + 0);
	/* CPTR DeviceCtl = get_vm_long(p + ExtnDat_params + 4); */
	ui4r OpCode = get_vm_word(ParamBlk + kcsCode);

#if Sony_dolog
	dbglog_StartLine();
	dbglog_writeCStr("Sony : Sony_Status OpCode = ");
	dbglog_writeHex(OpCode);
	dbglog_writeReturn();
#endif

	if (kDriveStatus == OpCode) {
		tDrive Drive_No = get_vm_word(ParamBlk + kioVRefNum) - 1;
		CPTR Src = DriveVarsLocation(Drive_No);
		if (Src == 0) {
			result = mnvm_nsDrvErr;
		} else {
			if (DelayUntilNextInsert > 4) {
				DelayUntilNextInsert = 4;
			}
			MyMoveBytesVM(Src, ParamBlk + kcsParam, 22);
			result = mnvm_noErr;
		}
	} else {
#if ExtraAbnormalReports
		if ((kReturnFormatList != OpCode)
			&& (kDuplicatorVersionSupport != OpCode))
		{
			ReportAbnormalID(0x090A,
				"unexpected OpCode in Sony_Control");
		}
#endif
		result = mnvm_statusErr;
	}

	if (mnvm_noErr != result) {
		put_vm_word(0x0142 /* DskErr */, result);
	}
	return result;
}

LOCALFUNC tMacErr Sony_Close(CPTR p)
{
#if 0
	CPTR ParamBlk = get_vm_long(p + ExtnDat_params + 0);
	CPTR DeviceCtl = get_vm_long(p + ExtnDat_params + 4);
#endif
	UnusedParam(p);
	return mnvm_closErr; /* Can't Close Driver */
}

LOCALFUNC tMacErr Sony_OpenA(CPTR p)
{
#if Sony_dolog
	dbglog_WriteNote("Sony : OpenA");
#endif

	if (MountCallBack != 0) {
		return mnvm_opWrErr; /* driver already open */
	} else {
		ui5b L = FirstDriveVarsOffset + EachDriveVarsSize * NumDrives;

		if (L < MinSonVarsSize) {
			L = MinSonVarsSize;
		}

		put_vm_long(p + ExtnDat_params + 0, L);

		return mnvm_noErr;
	}
}

LOCALFUNC tMacErr Sony_OpenB(CPTR p)
{
	si4b i;
	CPTR dvl;

#if Sony_dolog
	dbglog_WriteNote("Sony : OpenB");
#endif

	CPTR SonyVars = get_vm_long(p + ExtnDat_params + 4);
	/* CPTR ParamBlk = get_vm_long(p + ExtnDat_params + 24); (unused) */
#if CurEmMd > kEmMd_128K
	CPTR DeviceCtl = get_vm_long(p + ExtnDat_params + 28);
#endif

	put_vm_long(SonyVars + 16 /* checkval */, kcom_checkval);
	put_vm_long(SonyVars + 20 /* pokeaddr */, kExtn_Block_Base);
	put_vm_word(SonyVars + 24 /* NumDrives */, NumDrives);
	put_vm_word(SonyVars + 26 /* DiskExtn */, kExtnDisk);

	put_vm_long(SonyVarsPtr, SonyVars);

	for (i = 0; (dvl = DriveVarsLocation(i)) != 0; ++i) {
		put_vm_byte(dvl + kDiskInPlace, 0x00); /* Drive i No Disk */
		put_vm_byte(dvl + kInstalled, 0x01);   /* Drive i Installed */
#if CurEmMd <= kEmMd_128K
		put_vm_byte(dvl + kSides, 0x00);
			/* Drive i Single Sided */
#else
		put_vm_byte(dvl + kSides, 0xFF);
			/* Drive i Double Sided */
#endif
		put_vm_word(dvl + kQDriveNo, i + 1);   /* Drive i is Drive 1 */
		put_vm_word(dvl + kQRefNum, 0xFFFB);   /* Drive i uses .Sony */
	}

	{
		CPTR UTableBase = get_vm_long(0x011C);

		put_vm_long(UTableBase + 4 * 1,
			get_vm_long(UTableBase + 4 * 4));
			/* use same drive for hard disk as used for sony floppies */
	}

#if CurEmMd > kEmMd_128K
	/* driver version in driver i/o queue header */
	put_vm_byte(DeviceCtl + 7, 1);
#endif

#if CurEmMd <= kEmMd_128K
	/* init Drive Queue */
	put_vm_word(0x308, 0);
	put_vm_long(0x308 + 2, 0);
	put_vm_long(0x308 + 6, 0);
#endif

	put_vm_long(p + ExtnDat_params + 8,
		SonyVars + FirstDriveVarsOffset + kQLink);
	put_vm_word(p + ExtnDat_params + 12, EachDriveVarsSize);
	put_vm_word(p + ExtnDat_params + 14, NumDrives);
	put_vm_word(p + ExtnDat_params + 16, 1);
	put_vm_word(p + ExtnDat_params + 18, 0xFFFB);
#if CurEmMd <= kEmMd_128K
	put_vm_long(p + ExtnDat_params + 20, 0);
#else
	put_vm_long(p + ExtnDat_params + 20, SonyVars + 28 /* NullTask */);
#endif

#if Sony_SupportTags
	TheTagBuffer = 0;
#endif

	return mnvm_noErr;
}

LOCALFUNC tMacErr Sony_OpenC(CPTR p)
{
#if Sony_dolog
	dbglog_WriteNote("Sony : OpenC");
#endif

	MountCallBack = get_vm_long(p + ExtnDat_params + 0)
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
		| 0x40000000
#endif
		;
	return mnvm_noErr;
}

#define kCmndSonyPrime 1
#define kCmndSonyControl 2
#define kCmndSonyStatus 3
#define kCmndSonyClose 4
#define kCmndSonyOpenA 5
#define kCmndSonyOpenB 6
#define kCmndSonyOpenC 7
#define kCmndSonyMount 8

GLOBALPROC ExtnSony_Access(CPTR p)
{
	tMacErr result;

	switch (get_vm_word(p + ExtnDat_commnd)) {
		case kCmndVersion:
			put_vm_word(p + ExtnDat_version, 0);
			result = mnvm_noErr;
			break;
		case kCmndSonyPrime:
			result = Sony_Prime(p);
			break;
		case kCmndSonyControl:
			result = Sony_Control(p);
			break;
		case kCmndSonyStatus:
			result = Sony_Status(p);
			break;
		case kCmndSonyClose:
			result = Sony_Close(p);
			break;
		case kCmndSonyOpenA:
			result = Sony_OpenA(p);
			break;
		case kCmndSonyOpenB:
			result = Sony_OpenB(p);
			break;
		case kCmndSonyOpenC:
			result = Sony_OpenC(p);
			break;
		case kCmndSonyMount:
			result = Sony_Mount(p);
			break;
		default:
			result = mnvm_controlErr;
			break;
	}

	put_vm_word(p + ExtnDat_result, result);
}
