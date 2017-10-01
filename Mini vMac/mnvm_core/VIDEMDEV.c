/*
	VIDEMDEV.c

	Copyright (C) 2008 Paul C. Pratt

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
	VIDeo card EMulated DEVice

	Emulation of video card for Macintosh II.

	Written referring to:
		Sample firmware code in "Designing Cards and Drivers
		for Macintosh II and Macintosh SE", Apple Computer,
		page 8-20.

		Basilisk II source code, especially slot_rom.cpp
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"
#include "ENDIANAC.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#include "MINEM68K.h"
#include "SONYEMDV.h"
#endif

#include "VIDEMDEV.h"

/*
	ReportAbnormalID unused 0x0A08 - 0x0AFF
*/

#define VID_dolog (dbglog_HAVE && 0)

LOCALVAR const ui3b VidDrvr_contents[] = {
0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x2A, 0x00, 0x00, 0x00, 0xE2, 0x00, 0xEC,
0x00, 0xB6, 0x15, 0x2E, 0x44, 0x69, 0x73, 0x70,
0x6C, 0x61, 0x79, 0x5F, 0x56, 0x69, 0x64, 0x65,
0x6F, 0x5F, 0x53, 0x61, 0x6D, 0x70, 0x6C, 0x65,
0x00, 0x00, 0x24, 0x48, 0x26, 0x49, 0x70, 0x04,
0xA4, 0x40, 0x70, 0x04, 0xA7, 0x22, 0x66, 0x00,
0x00, 0x50, 0x27, 0x48, 0x00, 0x14, 0xA0, 0x29,
0x49, 0xFA, 0x00, 0x4A, 0x70, 0x10, 0xA7, 0x1E,
0x66, 0x00, 0x00, 0x3E, 0x31, 0x7C, 0x00, 0x06,
0x00, 0x04, 0x21, 0x4C, 0x00, 0x08, 0x21, 0x4B,
0x00, 0x0C, 0x70, 0x00, 0x10, 0x2B, 0x00, 0x28,
0xA0, 0x75, 0x66, 0x24, 0x22, 0x6B, 0x00, 0x14,
0x22, 0x51, 0x22, 0x88, 0x3F, 0x3C, 0x00, 0x01,
0x55, 0x4F, 0x3F, 0x3C, 0x00, 0x03, 0x41, 0xFA,
0x00, 0x9C, 0x2F, 0x18, 0x20, 0x50, 0x20, 0x8F,
0xDE, 0xFC, 0x00, 0x0A, 0x70, 0x00, 0x60, 0x02,
0x70, 0xE9, 0x4E, 0x75, 0x2F, 0x08, 0x55, 0x4F,
0x3F, 0x3C, 0x00, 0x04, 0x41, 0xFA, 0x00, 0x7E,
0x2F, 0x18, 0x20, 0x50, 0x20, 0x8F, 0x50, 0x4F,
0x20, 0x29, 0x00, 0x2A, 0xE1, 0x98, 0x02, 0x40,
0x00, 0x0F, 0x20, 0x78, 0x0D, 0x28, 0x4E, 0x90,
0x20, 0x5F, 0x70, 0x01, 0x4E, 0x75, 0x2F, 0x0B,
0x26, 0x69, 0x00, 0x14, 0x42, 0x67, 0x55, 0x4F,
0x3F, 0x3C, 0x00, 0x03, 0x41, 0xFA, 0x00, 0x4E,
0x2F, 0x18, 0x20, 0x50, 0x20, 0x8F, 0xDE, 0xFC,
0x00, 0x0A, 0x20, 0x53, 0x20, 0x50, 0xA0, 0x76,
0x20, 0x4B, 0xA0, 0x23, 0x70, 0x00, 0x26, 0x5F,
0x4E, 0x75, 0x2F, 0x08, 0x55, 0x4F, 0x3F, 0x3C,
0x00, 0x06, 0x60, 0x08, 0x2F, 0x08, 0x55, 0x4F,
0x3F, 0x3C, 0x00, 0x05, 0x41, 0xFA, 0x00, 0x1E,
0x2F, 0x18, 0x20, 0x50, 0x20, 0x8F, 0x5C, 0x4F,
0x30, 0x1F, 0x20, 0x5F, 0x08, 0x28, 0x00, 0x09,
0x00, 0x06, 0x67, 0x02, 0x4E, 0x75, 0x20, 0x78,
0x08, 0xFC, 0x4E, 0xD0
};

LOCALPROC ChecksumSlotROM(void)
{
	/* Calculate CRC */
	/* assuming check sum field initialized to zero */
	int i;
	ui3p p = VidROM;
	ui5b crc = 0;

	for (i = kVidROM_Size; --i >= 0; ) {
		crc = ((crc << 1) | (crc >> 31)) + *p++;
	}
	do_put_mem_long(p - 12, crc);
}

LOCALVAR ui3p pPatch;

LOCALPROC PatchAByte(ui3b v)
{
	*pPatch++ = v;
}

LOCALPROC PatchAWord(ui4r v)
{
	PatchAByte(v >> 8);
	PatchAByte(v & 0x00FF);
}

LOCALPROC PatchALong(ui5r v)
{
	PatchAWord(v >> 16);
	PatchAWord(v & 0x0000FFFF);
}

#if 0
LOCALPROC PatchAOSLstEntry0(ui3r Id, ui5r Offset)
{
	PatchALong((Id << 24) | (Offset & 0x00FFFFFF));
}
#endif

LOCALPROC PatchAOSLstEntry(ui3r Id, ui3p Offset)
{
	PatchALong((Id << 24) | ((Offset - pPatch) & 0x00FFFFFF));
}

LOCALFUNC ui3p ReservePatchOSLstEntry(void)
{
	ui3p v = pPatch;
	pPatch += 4;
	return v;
}

LOCALPROC PatchAReservedOSLstEntry(ui3p p, ui3r Id)
{
	ui3p pPatchSave = pPatch;
	pPatch = p;
	PatchAOSLstEntry(Id, pPatchSave);
	pPatch = pPatchSave;
}

LOCALPROC PatchADatLstEntry(ui3r Id, ui5r Data)
{
	PatchALong((Id << 24) | (Data & 0x00FFFFFF));
}

LOCALPROC PatchAnEndOfLst(void)
{
	PatchADatLstEntry(0xFF /* endOfList */, 0x00000000);
}

GLOBALFUNC blnr Vid_Init(void)
{
	int i;
	ui5r UsedSoFar;

	ui3p pAt_sRsrcDir;
	ui3p pTo_sRsrc_Board;
	ui3p pTo_BoardType;
	ui3p pTo_BoardName;
	ui3p pTo_VenderInfo;
	ui3p pTo_VendorID;
	ui3p pTo_RevLevel;
	ui3p pTo_PartNum;
	ui3p pTo_sRsrc_Video;
	ui3p pTo_VideoType;
	ui3p pTo_VideoName;
	ui3p pTo_MinorBase;
	ui3p pTo_MinorLength;
#if 0
	ui3p pTo_MajorBase;
	ui3p pTo_MajorLength;
#endif
	ui3p pTo_VidDrvrDir;
	ui3p pTo_sMacOS68020;
	ui3p pTo_OneBitMode;
	ui3p pTo_OneVidParams;
#if 0 != vMacScreenDepth
	ui3p pTo_ColorBitMode = nullpr;
	ui3p pTo_ColorVidParams;
#endif

	pPatch = VidROM;

	pAt_sRsrcDir = pPatch;
	pTo_sRsrc_Board = ReservePatchOSLstEntry();
	pTo_sRsrc_Video = ReservePatchOSLstEntry();
	PatchAnEndOfLst();

	PatchAReservedOSLstEntry(pTo_sRsrc_Board, 0x01 /* sRsrc_Board */);
	pTo_BoardType = ReservePatchOSLstEntry();
	pTo_BoardName = ReservePatchOSLstEntry();
	PatchADatLstEntry(0x20 /* BoardId */, 0x0000764D);
		/* 'vM', for Mini vMac */
	pTo_VenderInfo = ReservePatchOSLstEntry();
	PatchAnEndOfLst();

	PatchAReservedOSLstEntry(pTo_BoardType, 0x01 /* sRsrcType */);
	PatchAWord(0x0001);
	PatchAWord(0x0000);
	PatchAWord(0x0000);
	PatchAWord(0x0000);

	PatchAReservedOSLstEntry(pTo_BoardName, 0x02 /* sRsrcName */);
	/*
		'Mini vMac video card' as ascii c string
		(null terminated), and
		zero padded to end aligned long.
	*/
	PatchALong(0x4D696E69);
	PatchALong(0x20764D61);
	PatchALong(0x63207669);
	PatchALong(0x64656F20);
	PatchALong(0x63617264);
	PatchALong(0x00000000);

	PatchAReservedOSLstEntry(pTo_VenderInfo, 0x24 /* vendorInfo */);

	pTo_VendorID = ReservePatchOSLstEntry();
	pTo_RevLevel = ReservePatchOSLstEntry();
	pTo_PartNum = ReservePatchOSLstEntry();
	PatchAnEndOfLst();

	PatchAReservedOSLstEntry(pTo_VendorID, 0x01 /* vendorId */);
	/*
		'Paul C. Pratt' as ascii c string
		(null terminated), and
		zero padded to end aligned long.
	*/
	PatchALong(0x5061756C);
	PatchALong(0x20432E20);
	PatchALong(0x50726174);
	PatchALong(0x74000000);

	PatchAReservedOSLstEntry(pTo_RevLevel, 0x03 /* revLevel */);
	/*
		'1.0' as ascii c string
		(null terminated), and
		zero padded to end aligned long.
	*/
	PatchALong(0x312E3000);

	PatchAReservedOSLstEntry(pTo_PartNum, 0x04 /* partNum */);
	/*
		'TFB-1' as ascii c string
		(null terminated), and
		zero padded to end aligned long.
	*/
	PatchALong(0x5446422D);
	PatchALong(0x31000000);

	PatchAReservedOSLstEntry(pTo_sRsrc_Video, 0x80 /* sRsrc_Video */);

	pTo_VideoType = ReservePatchOSLstEntry();
	pTo_VideoName = ReservePatchOSLstEntry();
	pTo_VidDrvrDir = ReservePatchOSLstEntry();
	PatchADatLstEntry(0x08 /* sRsrcHWDevId */, 0x00000001);
	pTo_MinorBase = ReservePatchOSLstEntry();
	pTo_MinorLength = ReservePatchOSLstEntry();
#if 0
	pTo_MajorBase = ReservePatchOSLstEntry();
	pTo_MajorLength = ReservePatchOSLstEntry();
#endif
	pTo_OneBitMode = ReservePatchOSLstEntry();
#if 0 != vMacScreenDepth
	if (ColorModeWorks) {
		pTo_ColorBitMode = ReservePatchOSLstEntry();
	}
#endif
	PatchAnEndOfLst();

	PatchAReservedOSLstEntry(pTo_VideoType, 0x01 /* sRsrcType */);

	PatchAWord(0x0003); /* catDisplay */
	PatchAWord(0x0001); /* typVideo */
	PatchAWord(0x0001); /* drSwApple */
	PatchAWord(0x0001); /* drHwTFB */

	PatchAReservedOSLstEntry(pTo_VideoName, 0x02 /* sRsrcName */);
	/*
		'Display_Video_Apple_TFB' as ascii c string
		(null terminated), and
		zero padded to end aligned long.
	*/
	PatchALong(0x44697370);
	PatchALong(0x6C61795F);
	PatchALong(0x56696465);
	PatchALong(0x6F5F4170);
	PatchALong(0x706C655F);
	PatchALong(0x54464200);

	PatchAReservedOSLstEntry(pTo_MinorBase, 0x0A /* MinorBaseOS */);
	PatchALong(0x00000000);

	PatchAReservedOSLstEntry(pTo_MinorLength, 0x0B /* MinorLength */);
	PatchALong(kVidMemRAM_Size);

#if 0
	PatchAReservedOSLstEntry(pTo_MajorBase, 0x0C /* MinorBaseOS */);
	PatchALong(0x00000000);

	PatchAReservedOSLstEntry(pTo_MajorLength, 0x0D /* MinorLength */);
	PatchALong(kVidMemRAM_Size);
#endif

	PatchAReservedOSLstEntry(pTo_VidDrvrDir, 0x04 /* sRsrcDrvrDir */);
	pTo_sMacOS68020 = ReservePatchOSLstEntry();
	PatchAnEndOfLst();

	PatchAReservedOSLstEntry(pTo_sMacOS68020, 0x02 /* sMacOS68020 */);

	PatchALong(4 + sizeof(VidDrvr_contents) + 8);
	MyMoveBytes((ui3p)VidDrvr_contents,
		pPatch, sizeof(VidDrvr_contents));
	pPatch += sizeof(VidDrvr_contents);
	PatchAWord(kcom_callcheck);
	PatchAWord(kExtnVideo);
	PatchALong(kExtn_Block_Base);

	PatchAReservedOSLstEntry(pTo_OneBitMode, 0x80 /* oneBitMode */);
	pTo_OneVidParams = ReservePatchOSLstEntry();
	PatchADatLstEntry(0x03 /* mVidParams */, 0x00000001);
	PatchADatLstEntry(0x04 /* mDevType */, 0x00000000);
	PatchAnEndOfLst();

	PatchAReservedOSLstEntry(pTo_OneVidParams, 0x01 /* mVidParams */);
	PatchALong(0x0000002E); /* physical Block Size */
	PatchALong(0x00000000); /* defmBaseOffset */
	PatchAWord(vMacScreenWidth / 8);
		/* (Bounds.R-Bounds.L)*PixelSize/8 */
	PatchAWord(0x0000); /* Bounds.T */
	PatchAWord(0x0000); /* Bounds.L */
	PatchAWord(vMacScreenHeight); /* Bounds.B */
	PatchAWord(vMacScreenWidth); /* Bounds.R */
	PatchAWord(0x0000); /* bmVersion */
	PatchAWord(0x0000); /* packType not used */
	PatchALong(0x00000000); /* packSize not used */
	PatchALong(0x00480000); /* bmHRes */
	PatchALong(0x00480000); /* bmVRes */
	PatchAWord(0x0000); /* bmPixelType */
	PatchAWord(0x0001); /* bmPixelSize */
	PatchAWord(0x0001); /* bmCmpCount */
	PatchAWord(0x0001); /* bmCmpSize */
	PatchALong(0x00000000); /* bmPlaneBytes */

#if 0 != vMacScreenDepth
	if (ColorModeWorks) {

		PatchAReservedOSLstEntry(pTo_ColorBitMode, 0x81);
		pTo_ColorVidParams = ReservePatchOSLstEntry();
		PatchADatLstEntry(0x03 /* mVidParams */, 0x00000001);
		PatchADatLstEntry(0x04 /* mDevType */,
			(vMacScreenDepth < 4) ? 0x00000000 : 0x00000002);
			/* 2 for direct devices, according to Basilisk II */
		PatchAnEndOfLst();

		PatchAReservedOSLstEntry(pTo_ColorVidParams, 0x01);
		PatchALong(0x0000002E); /* physical Block Size */
		PatchALong(0x00000000); /* defmBaseOffset */
		PatchAWord(vMacScreenByteWidth);
		PatchAWord(0x0000); /* Bounds.T */
		PatchAWord(0x0000); /* Bounds.L */
		PatchAWord(vMacScreenHeight); /* Bounds.B */
		PatchAWord(vMacScreenWidth); /* Bounds.R */
		PatchAWord(0x0000); /* bmVersion */
		PatchAWord(0x0000); /* packType not used */
		PatchALong(0x00000000); /* packSize not used */
		PatchALong(0x00480000); /* bmHRes */
		PatchALong(0x00480000); /* bmVRes */
		PatchAWord((vMacScreenDepth < 4) ? 0x0000 : 0x0010);
			/* bmPixelType */
		PatchAWord(1 << vMacScreenDepth); /* bmPixelSize */
		PatchAWord((vMacScreenDepth < 4) ? 0x0001 : 0x0003);
			/* bmCmpCount */
#if 4 == vMacScreenDepth
		PatchAWord(0x0005); /* bmCmpSize */
#elif 5 == vMacScreenDepth
		PatchAWord(0x0008); /* bmCmpSize */
#else
		PatchAWord(1 << vMacScreenDepth); /* bmCmpSize */
#endif
		PatchALong(0x00000000); /* bmPlaneBytes */

	}
#endif

	UsedSoFar = (pPatch - VidROM) + 20;
	if (UsedSoFar > kVidROM_Size) {
		ReportAbnormalID(0x0A01, "kVidROM_Size to small");
		return falseblnr;
	}

	for (i = kVidROM_Size - UsedSoFar; --i >= 0; ) {
		PatchAByte(0);
	}

	pPatch = (kVidROM_Size - 20) + VidROM;
	PatchALong((pAt_sRsrcDir - pPatch) & 0x00FFFFFF);
	PatchALong(/* 0x0000041E */ kVidROM_Size);
	PatchALong(0x00000000);
	PatchAByte(0x01);
	PatchAByte(0x01);
	PatchALong(0x5A932BC7);
	PatchAByte(0x00);
	PatchAByte(0x0F);

	ChecksumSlotROM();

#if (0 != vMacScreenDepth) && (vMacScreenDepth < 4)
	CLUT_reds[0] = 0xFFFF;
	CLUT_greens[0] = 0xFFFF;
	CLUT_blues[0] = 0xFFFF;
	CLUT_reds[CLUT_size - 1] = 0;
	CLUT_greens[CLUT_size - 1] = 0;
	CLUT_blues[CLUT_size - 1] = 0;
#endif

	return trueblnr;
}

IMPORTPROC Vid_VBLinterrupt_PulseNotify(void);

GLOBALPROC Vid_Update(void)
{
	if (! Vid_VBLintunenbl) {
		Vid_VBLinterrupt = 0;
		Vid_VBLinterrupt_PulseNotify();
	}
}

LOCALFUNC ui4r Vid_GetMode(void)
{
	return
#if 0 != vMacScreenDepth
		UseColorMode ? 129 :
#endif
		128;
}

LOCALFUNC tMacErr Vid_SetMode(ui4r v)
{
#if 0 == vMacScreenDepth
	UnusedParam(v);
#else
	if (UseColorMode != ((v != 128) && ColorModeWorks)) {
		UseColorMode = ! UseColorMode;
		ColorMappingChanged = trueblnr;
	}
#endif
	return mnvm_noErr;
}

GLOBALFUNC ui4r Vid_Reset(void)
{
#if 0 != vMacScreenDepth
	UseColorMode = falseblnr;
#endif
	return 128;
}

#define kCmndVideoFeatures 1
#define kCmndVideoGetIntEnbl 2
#define kCmndVideoSetIntEnbl 3
#define kCmndVideoClearInt 4
#define kCmndVideoStatus 5
#define kCmndVideoControl 6

#define CntrlParam_csCode 0x1A /* control/status code [word] */
#define CntrlParam_csParam 0x1C /* operation-defined parameters */

#define VDPageInfo_csMode 0
#define VDPageInfo_csData 2
#define VDPageInfo_csPage 6
#define VDPageInfo_csBaseAddr 8

#define VDSetEntryRecord_csTable 0
#define VDSetEntryRecord_csStart 4
#define VDSetEntryRecord_csCount 6

#define VDGammaRecord_csGTable 0

#define VidBaseAddr 0xF9900000
	/* appears to be completely ignored */

LOCALVAR blnr UseGrayTones = falseblnr;

LOCALPROC FillScreenWithGrayPattern(void)
{
	int i;
	int j;
	ui5b *p1 = (ui5b *)VidMem;

#if 0 != vMacScreenDepth
	if (UseColorMode) {
#if 1 == vMacScreenDepth
		ui5b pat = 0xCCCCCCCC;
#elif 2 == vMacScreenDepth
		ui5b pat = 0xF0F0F0F0;
#elif 3 == vMacScreenDepth
		ui5b pat = 0xFF00FF00;
#elif 4 == vMacScreenDepth
		ui5b pat = 0x00007FFF;
#elif 5 == vMacScreenDepth
		ui5b pat = 0x00000000;
#endif
		for (i = vMacScreenHeight; --i >= 0; ) {
			for (j = vMacScreenByteWidth >> 2; --j >= 0; ) {
				*p1++ = pat;
#if 5 == vMacScreenDepth
				pat = (~ pat) & 0x00FFFFFF;
#endif
			}
			pat = (~ pat)
#if 4 == vMacScreenDepth
				& 0x7FFF7FFF
#elif 5 == vMacScreenDepth
				& 0x00FFFFFF
#endif
				;
		}
	} else
#endif
	{
		ui5b pat = 0xAAAAAAAA;

		for (i = vMacScreenHeight; --i >= 0; ) {
			for (j = vMacScreenMonoByteWidth >> 2; --j >= 0; ) {
				*p1++ = pat;
			}
			pat = ~ pat;
		}
	}
}

GLOBALPROC ExtnVideo_Access(CPTR p)
{
	tMacErr result = mnvm_controlErr;

	switch (get_vm_word(p + ExtnDat_commnd)) {
		case kCmndVersion:
#if VID_dolog
			dbglog_WriteNote("Video_Access kCmndVersion");
#endif
			put_vm_word(p + ExtnDat_version, 1);
			result = mnvm_noErr;
			break;
		case kCmndVideoGetIntEnbl:
#if VID_dolog
			dbglog_WriteNote("Video_Access kCmndVideoGetIntEnbl");
#endif
			put_vm_word(p + 8,
				Vid_VBLintunenbl ? 0 : 1);
			result = mnvm_noErr;
			break;
		case kCmndVideoSetIntEnbl:
#if VID_dolog
			dbglog_WriteNote("Video_Access kCmndVideoSetIntEnbl");
#endif
			Vid_VBLintunenbl =
				(0 == get_vm_word(p + 8))
					? 1 : 0;
			result = mnvm_noErr;
			break;
		case kCmndVideoClearInt:
#if VID_dolog && 0 /* frequent */
			dbglog_WriteNote("Video_Access kCmndVideoClearInt");
#endif
			Vid_VBLinterrupt = 1;
			result = mnvm_noErr;
			break;
		case kCmndVideoControl:
			{
				CPTR CntrlParams = get_vm_long(p + 8);
				CPTR csParam =
					get_vm_long(CntrlParams + CntrlParam_csParam);
				ui4r csCode =
					get_vm_word(CntrlParams + CntrlParam_csCode);

				switch (csCode) {
					case 0: /* VidReset */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, VidReset");
#endif
						put_vm_word(csParam + VDPageInfo_csMode,
							Vid_GetMode());
						put_vm_word(csParam + VDPageInfo_csPage, 0);
							/* page is always 0 */
						put_vm_long(csParam + VDPageInfo_csBaseAddr,
							VidBaseAddr);

						result = mnvm_noErr;
						break;
					case 1: /* KillIO */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, KillIO");
#endif
						result = mnvm_noErr;
						break;
					case 2: /* SetVidMode */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, "
							"SetVidMode");
#endif
						if (0 != get_vm_word(
							csParam + VDPageInfo_csPage))
						{
							/* return mnvm_controlErr, page must be 0 */
							ReportAbnormalID(0x0A02,
								"SetVidMode not page 0");
						} else {
							result = Vid_SetMode(get_vm_word(
								csParam + VDPageInfo_csMode));
							put_vm_long(csParam + VDPageInfo_csBaseAddr,
								VidBaseAddr);
						}
						break;
					case 3: /* SetEntries */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, "
							"SetEntries");
#endif
#if (0 != vMacScreenDepth) && (vMacScreenDepth < 4)
						if (UseColorMode) {
							CPTR csTable = get_vm_long(
								csParam + VDSetEntryRecord_csTable);
							ui4r csStart = get_vm_word(
								csParam + VDSetEntryRecord_csStart);
							ui4r csCount = 1 + get_vm_word(
								csParam + VDSetEntryRecord_csCount);

							if (((ui4r) 0xFFFF) == csStart) {
								int i;

								result = mnvm_noErr;
								for (i = 0; i < csCount; ++i) {
									ui4r j = get_vm_word(csTable + 0);
									if (j == 0) {
										/* ignore input, leave white */
									} else
									if (j == CLUT_size - 1) {
										/* ignore input, leave black */
									} else
									if (j >= CLUT_size) {
										/* out of range */
										result = mnvm_paramErr;
									} else
									{
										ui4r r =
											get_vm_word(csTable + 2);
										ui4r g =
											get_vm_word(csTable + 4);
										ui4r b =
											get_vm_word(csTable + 6);
										CLUT_reds[j] = r;
										CLUT_greens[j] = g;
										CLUT_blues[j] = b;
									}
									csTable += 8;
								}
								ColorMappingChanged = trueblnr;
							} else
							if (csStart + csCount < csStart) {
								/* overflow */
								result = mnvm_paramErr;
							} else
							if (csStart + csCount > CLUT_size) {
								result = mnvm_paramErr;
							} else
							{
								int i;

								for (i = 0; i < csCount; ++i) {
									int j = i + csStart;

									if (j == 0) {
										/* ignore input, leave white */
									} else
									if (j == CLUT_size - 1) {
										/* ignore input, leave black */
									} else
									{
										ui4r r =
											get_vm_word(csTable + 2);
										ui4r g =
											get_vm_word(csTable + 4);
										ui4r b =
											get_vm_word(csTable + 6);
										CLUT_reds[j] = r;
										CLUT_greens[j] = g;
										CLUT_blues[j] = b;
									}
									csTable += 8;
								}
								ColorMappingChanged = trueblnr;
								result = mnvm_noErr;
							}
						} else
#endif
						{
							/* not implemented */
						}
						break;
					case 4: /* SetGamma */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, SetGamma");
#endif
						{
#if 0
							CPTR csTable = get_vm_long(
								csParam + VDGammaRecord_csGTable);
							/* not implemented */
#endif
						}
#if 0
						ReportAbnormalID(0x0A03,
							"Video_Access SetGamma not implemented");
#else
						result = mnvm_noErr;
#endif
						break;
					case 5: /* GrayScreen */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, "
							"GrayScreen");
#endif
						{
#if 0
							ui4r csPage = get_vm_word(
								csParam + VDPageInfo_csPage);
							/* not implemented */
#endif
							FillScreenWithGrayPattern();
							result = mnvm_noErr;
						}
						break;
					case 6: /* SetGray */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, SetGray");
#endif
						{
							ui3r csMode = get_vm_byte(
								csParam + VDPageInfo_csMode);
								/*
									"Designing Cards and Drivers" book
									says this is a word, but it seems
									to be a byte.
								*/

							UseGrayTones = (csMode != 0);
							result = mnvm_noErr;
						}
						break;
					case 9: /* SetDefaultMode */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, "
							"SetDefaultMode");
#endif
						/* not handled yet */
						/*
							seen when close Monitors control panel
							in system 7.5.5
						*/
						break;
					case 16: /* SavePreferredConfiguration */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoControl, "
							"SavePreferredConfiguration");
#endif
						/* not handled yet */
						/*
							seen when close Monitors control panel
							in system 7.5.5
						*/
						break;
					default:
						ReportAbnormalID(0x0A04,
							"kCmndVideoControl, unknown csCode");
#if dbglog_HAVE
						dbglog_writelnNum("csCode", csCode);
#endif
						break;
				}
			}
			break;
		case kCmndVideoStatus:
			{
				CPTR CntrlParams = get_vm_long(p + 8);
				CPTR csParam = get_vm_long(
					CntrlParams + CntrlParam_csParam);
				ui4r csCode = get_vm_word(
					CntrlParams + CntrlParam_csCode);

				result = mnvm_statusErr;
				switch (csCode) {
					case 2: /* GetMode */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, GetMode");
#endif
						put_vm_word(csParam + VDPageInfo_csMode,
							Vid_GetMode());
						put_vm_word(csParam + VDPageInfo_csPage, 0);
							/* page is always 0 */
						put_vm_long(csParam + VDPageInfo_csBaseAddr,
							VidBaseAddr);
						result = mnvm_noErr;
						break;
					case 3: /* GetEntries */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetEntries");
#endif
						{
#if 0
							CPTR csTable = get_vm_long(
								csParam + VDSetEntryRecord_csTable);
							put_vm_word(
								csParam + VDSetEntryRecord_csStart,
								csStart);
							put_vm_word(
								csParam + VDSetEntryRecord_csCount,
								csCount);
#endif
							ReportAbnormalID(0x0A05,
								"GetEntries not implemented");
						}
						break;
					case 4: /* GetPages */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, GetPages");
#endif
						put_vm_word(csParam + VDPageInfo_csPage, 1);
							/* always 1 page */
						result = mnvm_noErr;
						break;
					case 5: /* GetPageAddr */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus,"
							" GetPageAddr");
#endif
						{
							ui4r csPage = get_vm_word(
								csParam + VDPageInfo_csPage);
							if (0 != csPage) {
								/*
									return mnvm_statusErr,
									page must be 0
								*/
							} else {
								put_vm_long(
									csParam + VDPageInfo_csBaseAddr,
									VidBaseAddr);
								result = mnvm_noErr;
							}
						}
						break;
					case 6: /* GetGray */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, GetGray");
#endif
						put_vm_word(csParam + VDPageInfo_csMode,
							UseGrayTones ? 0x0100 : 0);
							/*
								"Designing Cards and Drivers" book
								says this is a word, but it seems
								to be a byte.
							*/
						result = mnvm_noErr;
						break;
					case 8: /* GetGamma */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetGamma");
#endif
						/* not handled yet */
						/*
							seen when close Monitors control panel
							in system 7.5.5
						*/
						break;
					case 9: /* GetDefaultMode */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetDefaultMode");
#endif
						/* not handled yet */
						/* seen in System 7.5.5 boot */
						break;
					case 10: /* GetCurrentMode */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetCurrentMode");
#endif
#if 0
						put_vm_word(csParam + VDPageInfo_csMode,
							Vid_GetMode());
						put_vm_long(csParam + VDPageInfo_csData, 0);
							/* what is this ? */
						put_vm_word(csParam + VDPageInfo_csPage, 0);
							/* page is always 0 */
						put_vm_long(csParam + VDPageInfo_csBaseAddr,
							VidBaseAddr);

						result = mnvm_noErr;
#endif
						break;
					case 12: /* GetConnection */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetConnection");
#endif
						/* not handled yet */
						/* seen in System 7.5.5 boot */
						break;
					case 13: /* GetCurrentMode */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetCurrentMode");
#endif
						/* not handled yet */
						/* seen in System 7.5.5 boot */
						break;
					case 14: /* GetModeBaseAddress */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetModeBaseAddress");
#endif
						/* not handled yet */
						/*
							seen in System 7.5.5 Monitors control panel
						*/
						break;
					case 16: /* GetPreferredConfiguration */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetPreferredConfiguration");
#endif
						/* not handled yet */
						/* seen in System 7.5.5 boot */
						break;
					case 17: /* GetNextResolution */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetNextResolution");
#endif
						/* not handled yet */
						/*
							seen in System 7.5.5 monitors control panel
							option button
						*/
						break;
					case 18: /* GetVideoParameters */
#if VID_dolog
						dbglog_WriteNote(
							"Video_Access kCmndVideoStatus, "
							"GetVideoParameters");
#endif
						/* not handled yet */
						/* seen in System 7.5.5 boot */
						break;
					default:
						ReportAbnormalID(0x0A06,
							"Video_Access kCmndVideoStatus, "
								"unknown csCode");
#if dbglog_HAVE
						dbglog_writelnNum("csCode", csCode);
#endif
						break;
				}
			}
			break;
		default:
			ReportAbnormalID(0x0A07,
				"Video_Access, unknown commnd");
			break;
	}

	put_vm_word(p + ExtnDat_result, result);
}
