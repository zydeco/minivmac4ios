/*
	GLOBGLUE.c

	Copyright (C) 2003 Bernd Schmidt, Philip Cummins, Paul C. Pratt

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
	GLOBal GLUE (or GLOB of GLUE)

	Holds the program together.

	Some code here adapted from "custom.c" in vMac by Philip Cummins,
	in turn descended from code in the Un*x Amiga Emulator by
	Bernd Schmidt.
*/

#ifndef AllFiles
#include "SYSDEPNS.h"

#include "MYOSGLUE.h"
#include "ENDIANAC.h"
#include "EMCONFIG.h"
#endif

#include "GLOBGLUE.h"

/*
	ReportAbnormalID unused 0x111D - 0x11FF
*/

/*
	ReportAbnormalID ranges unused 0x12xx - 0xFFxx
*/

IMPORTPROC m68k_reset(void);
IMPORTPROC IWM_Reset(void);
IMPORTPROC SCC_Reset(void);
IMPORTPROC SCSI_Reset(void);
IMPORTPROC VIA1_Reset(void);
#if EmVIA2
IMPORTPROC VIA2_Reset(void);
#endif
IMPORTPROC Sony_Reset(void);

IMPORTPROC ExtnDisk_Access(CPTR p);
IMPORTPROC ExtnSony_Access(CPTR p);
#if EmVidCard
IMPORTPROC ExtnVideo_Access(CPTR p);
#endif

IMPORTPROC Sony_SetQuitOnEject(void);

IMPORTPROC m68k_IPLchangeNtfy(void);
IMPORTPROC MINEM68K_Init(
	ui3b *fIPL);

IMPORTFUNC ui5b GetCyclesRemaining(void);
IMPORTPROC SetCyclesRemaining(ui5b n);

IMPORTPROC SetHeadATTel(ATTep p);
IMPORTFUNC ATTep FindATTel(CPTR addr);

IMPORTFUNC ui5b SCSI_Access(ui5b Data, blnr WriteMem, CPTR addr);
IMPORTFUNC ui5b SCC_Access(ui5b Data, blnr WriteMem, CPTR addr);
IMPORTFUNC ui5b IWM_Access(ui5b Data, blnr WriteMem, CPTR addr);
IMPORTFUNC ui5b VIA1_Access(ui5b Data, blnr WriteMem, CPTR addr);
#if EmVIA2
IMPORTFUNC ui5b VIA2_Access(ui5b Data, blnr WriteMem, CPTR addr);
#endif
#if EmASC
IMPORTFUNC ui5b ASC_Access(ui5b Data, blnr WriteMem, CPTR addr);
#endif

IMPORTFUNC ui3r get_vm_byte(CPTR addr);
IMPORTFUNC ui4r get_vm_word(CPTR addr);
IMPORTFUNC ui5r get_vm_long(CPTR addr);

IMPORTPROC put_vm_byte(CPTR addr, ui3r b);
IMPORTPROC put_vm_word(CPTR addr, ui4r w);
IMPORTPROC put_vm_long(CPTR addr, ui5r l);

GLOBALVAR ui5r my_disk_icon_addr;

GLOBALPROC customreset(void)
{
	IWM_Reset();
	SCC_Reset();
	SCSI_Reset();
	VIA1_Reset();
#if EmVIA2
	VIA2_Reset();
#endif
	Sony_Reset();
	Extn_Reset();
#if CurEmMd <= kEmMd_Plus
	WantMacReset = trueblnr;
	/*
		kludge, code in Finder appears
		to do RESET and not expect
		to come back. Maybe asserting
		the RESET somehow causes
		other hardware compenents to
		later reset the 68000.
	*/
#endif
}

GLOBALVAR ui3p RAM = nullpr;

#if EmVidCard
GLOBALVAR ui3p VidROM = nullpr;
#endif

#if IncludeVidMem
GLOBALVAR ui3p VidMem = nullpr;
#endif

GLOBALVAR ui3b Wires[kNumWires];


#if WantDisasm
IMPORTPROC m68k_WantDisasmContext(void);
#endif

#if WantDisasm
GLOBALPROC dbglog_StartLine(void)
{
	m68k_WantDisasmContext();
	dbglog_writeCStr(" ");
}
#endif

#if dbglog_HAVE
GLOBALPROC dbglog_WriteMemArrow(blnr WriteMem)
{
	if (WriteMem) {
		dbglog_writeCStr(" <- ");
	} else {
		dbglog_writeCStr(" -> ");
	}
}
#endif

#if dbglog_HAVE
GLOBALPROC dbglog_AddrAccess(char *s, ui5r Data,
	blnr WriteMem, ui5r addr)
{
	dbglog_StartLine();
	dbglog_writeCStr(s);
	dbglog_writeCStr("[");
	dbglog_writeHex(addr);
	dbglog_writeCStr("]");
	dbglog_WriteMemArrow(WriteMem);
	dbglog_writeHex(Data);
	dbglog_writeReturn();
}
#endif

#if dbglog_HAVE
GLOBALPROC dbglog_Access(char *s, ui5r Data, blnr WriteMem)
{
	dbglog_StartLine();
	dbglog_writeCStr(s);
	dbglog_WriteMemArrow(WriteMem);
	dbglog_writeHex(Data);
	dbglog_writeReturn();
}
#endif

#if dbglog_HAVE
GLOBALPROC dbglog_WriteNote(char *s)
{
	dbglog_StartLine();
	dbglog_writeCStr(s);
	dbglog_writeReturn();
}
#endif

#if dbglog_HAVE
GLOBALPROC dbglog_WriteSetBool(char *s, blnr v)
{
	dbglog_StartLine();
	dbglog_writeCStr(s);
	dbglog_writeCStr(" <- ");
	if (v) {
		dbglog_writeCStr("1");
	} else {
		dbglog_writeCStr("0");
	}
	dbglog_writeReturn();
}
#endif

#if WantAbnormalReports
LOCALVAR blnr GotOneAbnormal = falseblnr;
#endif

#ifndef ReportAbnormalInterrupt
#define ReportAbnormalInterrupt 0
#endif

#if WantAbnormalReports
GLOBALPROC DoReportAbnormalID(ui4r id
#if dbglog_HAVE
	, char *s
#endif
	)
{
#if dbglog_HAVE
	dbglog_StartLine();
	dbglog_writeCStr("*** abnormal : ");
	dbglog_writeCStr(s);
	dbglog_writeReturn();
#endif

	if (! GotOneAbnormal) {
		WarnMsgAbnormalID(id);
#if ReportAbnormalInterrupt
		SetInterruptButton(trueblnr);
#endif
		GotOneAbnormal = trueblnr;
	}
}
#endif

/* map of address space */

#define kRAM_Base 0x00000000 /* when overlay off */
#if (CurEmMd == kEmMd_PB100)
#define kRAM_ln2Spc 23
#elif (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
#define kRAM_ln2Spc 23
#else
#define kRAM_ln2Spc 22
#endif

#if IncludeVidMem
#if CurEmMd == kEmMd_PB100
#define kVidMem_Base 0x00FA0000
#define kVidMem_ln2Spc 16
#else
#define kVidMem_Base 0x00540000
#define kVidMem_ln2Spc 18
#endif
#endif

#if CurEmMd == kEmMd_PB100
#define kSCSI_Block_Base 0x00F90000
#define kSCSI_ln2Spc 16
#else
#define kSCSI_Block_Base 0x00580000
#define kSCSI_ln2Spc 19
#endif

#define kRAM_Overlay_Base 0x00600000 /* when overlay on */
#define kRAM_Overlay_Top  0x00800000

#if CurEmMd == kEmMd_PB100
#define kSCCRd_Block_Base 0x00FD0000
#define kSCC_ln2Spc 16
#else
#define kSCCRd_Block_Base 0x00800000
#define kSCC_ln2Spc 22
#endif

#if CurEmMd != kEmMd_PB100
#define kSCCWr_Block_Base 0x00A00000
#define kSCCWr_Block_Top  0x00C00000
#endif

#if CurEmMd == kEmMd_PB100
#define kIWM_Block_Base 0x00F60000
#define kIWM_ln2Spc 16
#else
#define kIWM_Block_Base 0x00C00000
#define kIWM_ln2Spc 21
#endif

#if CurEmMd == kEmMd_PB100
#define kVIA1_Block_Base 0x00F70000
#define kVIA1_ln2Spc 16
#else
#define kVIA1_Block_Base 0x00E80000
#define kVIA1_ln2Spc 19
#endif

#if CurEmMd == kEmMd_PB100
#define kASC_Block_Base 0x00FB0000
#define kASC_ln2Spc 16
#endif
#define kASC_Mask 0x00000FFF


#if IncludeExtnPbufs
LOCALFUNC tMacErr PbufTransferVM(CPTR Buffera,
	tPbuf i, ui5r offset, ui5r count, blnr IsWrite)
{
	tMacErr result;
	ui5b contig;
	ui3p Buffer;

label_1:
	if (0 == count) {
		result = mnvm_noErr;
	} else {
		Buffer = get_real_address0(count, ! IsWrite, Buffera, &contig);
		if (0 == contig) {
			result = mnvm_miscErr;
		} else {
			PbufTransfer(Buffer, i, offset, contig, IsWrite);
			offset += contig;
			Buffera += contig;
			count -= contig;
			goto label_1;
		}
	}

	return result;
}
#endif

/* extension mechanism */

#if IncludeExtnPbufs
#define kCmndPbufFeatures 1
#define kCmndPbufNew 2
#define kCmndPbufDispose 3
#define kCmndPbufGetSize 4
#define kCmndPbufTransfer 5
#endif

#if IncludeExtnPbufs
LOCALPROC ExtnParamBuffers_Access(CPTR p)
{
	tMacErr result = mnvm_controlErr;

	switch (get_vm_word(p + ExtnDat_commnd)) {
		case kCmndVersion:
			put_vm_word(p + ExtnDat_version, 1);
			result = mnvm_noErr;
			break;
		case kCmndPbufFeatures:
			put_vm_long(p + ExtnDat_params + 0, 0);
			result = mnvm_noErr;
			break;
		case kCmndPbufNew:
			{
				tPbuf Pbuf_No;
				ui5b count = get_vm_long(p + ExtnDat_params + 4);
				/* reserved word at offset 2, should be zero */
				result = PbufNew(count, &Pbuf_No);
				put_vm_word(p + ExtnDat_params + 0, Pbuf_No);
			}
			break;
		case kCmndPbufDispose:
			{
				tPbuf Pbuf_No = get_vm_word(p + ExtnDat_params + 0);
				/* reserved word at offset 2, should be zero */
				result = CheckPbuf(Pbuf_No);
				if (mnvm_noErr == result) {
					PbufDispose(Pbuf_No);
				}
			}
			break;
		case kCmndPbufGetSize:
			{
				ui5r Count;
				tPbuf Pbuf_No = get_vm_word(p + ExtnDat_params + 0);
				/* reserved word at offset 2, should be zero */

				result = PbufGetSize(Pbuf_No, &Count);
				if (mnvm_noErr == result) {
					put_vm_long(p + ExtnDat_params + 4, Count);
				}
			}
			break;
		case kCmndPbufTransfer:
			{
				ui5r PbufCount;
				tPbuf Pbuf_No = get_vm_word(p + ExtnDat_params + 0);
				/* reserved word at offset 2, should be zero */
				ui5r offset = get_vm_long(p + ExtnDat_params + 4);
				ui5r count = get_vm_long(p + ExtnDat_params + 8);
				CPTR Buffera = get_vm_long(p + ExtnDat_params + 12);
				blnr IsWrite =
					(get_vm_word(p + ExtnDat_params + 16) != 0);
				result = PbufGetSize(Pbuf_No, &PbufCount);
				if (mnvm_noErr == result) {
					ui5r endoff = offset + count;
					if ((endoff < offset) /* overflow */
						|| (endoff > PbufCount))
					{
						result = mnvm_eofErr;
					} else {
						result = PbufTransferVM(Buffera,
							Pbuf_No, offset, count, IsWrite);
					}
				}
			}
			break;
	}

	put_vm_word(p + ExtnDat_result, result);
}
#endif

#if IncludeExtnHostTextClipExchange
#define kCmndHTCEFeatures 1
#define kCmndHTCEExport 2
#define kCmndHTCEImport 3
#endif

#if IncludeExtnHostTextClipExchange
LOCALPROC ExtnHostTextClipExchange_Access(CPTR p)
{
	tMacErr result = mnvm_controlErr;

	switch (get_vm_word(p + ExtnDat_commnd)) {
		case kCmndVersion:
			put_vm_word(p + ExtnDat_version, 1);
			result = mnvm_noErr;
			break;
		case kCmndHTCEFeatures:
			put_vm_long(p + ExtnDat_params + 0, 0);
			result = mnvm_noErr;
			break;
		case kCmndHTCEExport:
			{
				tPbuf Pbuf_No = get_vm_word(p + ExtnDat_params + 0);

				result = CheckPbuf(Pbuf_No);
				if (mnvm_noErr == result) {
					result = HTCEexport(Pbuf_No);
				}
			}
			break;
		case kCmndHTCEImport:
			{
				tPbuf Pbuf_No;
				result = HTCEimport(&Pbuf_No);
				put_vm_word(p + ExtnDat_params + 0, Pbuf_No);
			}
			break;
	}

	put_vm_word(p + ExtnDat_result, result);
}
#endif

#define kFindExtnExtension 0x64E1F58A
#define kDiskDriverExtension 0x4C9219E6
#if IncludeExtnPbufs
#define kHostParamBuffersExtension 0x314C87BF
#endif
#if IncludeExtnHostTextClipExchange
#define kHostClipExchangeExtension 0x27B130CA
#endif

#define kCmndFindExtnFind 1
#define kCmndFindExtnId2Code 2
#define kCmndFindExtnCount 3

#define kParamFindExtnTheExtn 8
#define kParamFindExtnTheId 12

LOCALPROC ExtnFind_Access(CPTR p)
{
	tMacErr result = mnvm_controlErr;

	switch (get_vm_word(p + ExtnDat_commnd)) {
		case kCmndVersion:
			put_vm_word(p + ExtnDat_version, 1);
			result = mnvm_noErr;
			break;
		case kCmndFindExtnFind:
			{
				ui5b extn = get_vm_long(p + kParamFindExtnTheExtn);

				if (extn == kDiskDriverExtension) {
					put_vm_word(p + kParamFindExtnTheId, kExtnDisk);
					result = mnvm_noErr;
				} else
#if IncludeExtnPbufs
				if (extn == kHostParamBuffersExtension) {
					put_vm_word(p + kParamFindExtnTheId,
						kExtnParamBuffers);
					result = mnvm_noErr;
				} else
#endif
#if IncludeExtnHostTextClipExchange
				if (extn == kHostClipExchangeExtension) {
					put_vm_word(p + kParamFindExtnTheId,
						kExtnHostTextClipExchange);
					result = mnvm_noErr;
				} else
#endif
				if (extn == kFindExtnExtension) {
					put_vm_word(p + kParamFindExtnTheId,
						kExtnFindExtn);
					result = mnvm_noErr;
				} else
				{
					/* not found */
				}
			}
			break;
		case kCmndFindExtnId2Code:
			{
				ui4r extn = get_vm_word(p + kParamFindExtnTheId);

				if (extn == kExtnDisk) {
					put_vm_long(p + kParamFindExtnTheExtn,
						kDiskDriverExtension);
					result = mnvm_noErr;
				} else
#if IncludeExtnPbufs
				if (extn == kExtnParamBuffers) {
					put_vm_long(p + kParamFindExtnTheExtn,
						kHostParamBuffersExtension);
					result = mnvm_noErr;
				} else
#endif
#if IncludeExtnHostTextClipExchange
				if (extn == kExtnHostTextClipExchange) {
					put_vm_long(p + kParamFindExtnTheExtn,
						kHostClipExchangeExtension);
					result = mnvm_noErr;
				} else
#endif
				if (extn == kExtnFindExtn) {
					put_vm_long(p + kParamFindExtnTheExtn,
						kFindExtnExtension);
					result = mnvm_noErr;
				} else
				{
					/* not found */
				}
			}
			break;
		case kCmndFindExtnCount:
			put_vm_word(p + kParamFindExtnTheId, kNumExtns);
			result = mnvm_noErr;
			break;
	}

	put_vm_word(p + ExtnDat_result, result);
}

#define kDSK_Params_Hi 0
#define kDSK_Params_Lo 1
#define kDSK_QuitOnEject 3 /* obsolete */

LOCALVAR ui4b ParamAddrHi;

LOCALPROC Extn_Access(ui5b Data, CPTR addr)
{
	switch (addr) {
		case kDSK_Params_Hi:
			ParamAddrHi = Data;
			break;
		case kDSK_Params_Lo:
			{
				CPTR p = ParamAddrHi << 16 | Data;

				ParamAddrHi = (ui4b) - 1;
				if (kcom_callcheck == get_vm_word(p + ExtnDat_checkval))
				{
					put_vm_word(p + ExtnDat_checkval, 0);

					switch (get_vm_word(p + ExtnDat_extension)) {
						case kExtnFindExtn:
							ExtnFind_Access(p);
							break;
#if EmVidCard
						case kExtnVideo:
							ExtnVideo_Access(p);
							break;
#endif
#if IncludeExtnPbufs
						case kExtnParamBuffers:
							ExtnParamBuffers_Access(p);
							break;
#endif
#if IncludeExtnHostTextClipExchange
						case kExtnHostTextClipExchange:
							ExtnHostTextClipExchange_Access(p);
							break;
#endif
						case kExtnDisk:
							ExtnDisk_Access(p);
							break;
						case kExtnSony:
							ExtnSony_Access(p);
							break;
						default:
							put_vm_word(p + ExtnDat_result,
								mnvm_controlErr);
							break;
					}
				}
			}
			break;
		case kDSK_QuitOnEject:
			/* obsolete, kept for compatibility */
			Sony_SetQuitOnEject();
			break;
	}
}

GLOBALPROC Extn_Reset(void)
{
	ParamAddrHi = (ui4b) - 1;
}

/* implementation of read/write for everything but RAM and ROM */

#define kSCC_Mask 0x03

#define kVIA1_Mask 0x00000F
#if EmVIA2
#define kVIA2_Mask 0x00000F
#endif

#define kIWM_Mask 0x00000F /* Allocated Memory Bandwidth for IWM */

#if CurEmMd <= kEmMd_512Ke
#define ROM_CmpZeroMask 0
#elif CurEmMd <= kEmMd_Plus
#if kROM_Size > 0x00020000
#define ROM_CmpZeroMask 0 /* For hacks like Mac ROM-inator */
#else
#define ROM_CmpZeroMask 0x00020000
#endif
#elif CurEmMd <= kEmMd_PB100
#define ROM_CmpZeroMask 0
#elif CurEmMd <= kEmMd_IIx
#define ROM_CmpZeroMask 0
#else
#error "ROM_CmpZeroMask not defined"
#endif

#define kROM_cmpmask (0x00F00000 | ROM_CmpZeroMask)

#if CurEmMd <= kEmMd_512Ke
#define Overlay_ROM_CmpZeroMask 0x00100000
#elif CurEmMd <= kEmMd_Plus
#define Overlay_ROM_CmpZeroMask 0x00020000
#elif CurEmMd <= kEmMd_Classic
#define Overlay_ROM_CmpZeroMask 0x00300000
#elif CurEmMd <= kEmMd_PB100
#define Overlay_ROM_CmpZeroMask 0
#elif CurEmMd <= kEmMd_IIx
#define Overlay_ROM_CmpZeroMask 0
#else
#error "Overlay_ROM_CmpZeroMask not defined"
#endif

enum {
	kMMDV_VIA1,
#if EmVIA2
	kMMDV_VIA2,
#endif
	kMMDV_SCC,
	kMMDV_Extn,
#if EmASC
	kMMDV_ASC,
#endif
	kMMDV_SCSI,
	kMMDV_IWM,

	kNumMMDVs
};

enum {
#if CurEmMd >= kEmMd_SE
	kMAN_OverlayOff,
#endif

	kNumMANs
};


LOCALVAR ATTer ATTListA[MaxATTListN];
LOCALVAR ui4r LastATTel;


LOCALPROC AddToATTList(ATTep p)
{
	ui4r NewLast = LastATTel + 1;
	if (NewLast >= MaxATTListN) {
		ReportAbnormalID(0x1101, "MaxATTListN not big enough");
	} else {
		ATTListA[LastATTel] = *p;
		LastATTel = NewLast;
	}
}

LOCALPROC InitATTList(void)
{
	LastATTel = 0;
}

LOCALPROC FinishATTList(void)
{
	{
		/* add guard */
		ATTer r;

		r.cmpmask = 0;
		r.cmpvalu = 0;
		r.usemask = 0;
		r.usebase = nullpr;
		r.Access = 0;
		AddToATTList(&r);
	}

	{
		ui4r i = LastATTel;
		ATTep p = &ATTListA[LastATTel];
		ATTep h = nullpr;

		while (0 != i) {
			--i;
			--p;
			p->Next = h;
			h = p;
		}

#if 0 /* verify list. not for final version */
		{
			ATTep q1;
			ATTep q2;
			for (q1 = h; nullpr != q1->Next; q1 = q1->Next) {
				if ((q1->cmpvalu & ~ q1->cmpmask) != 0) {
					ReportAbnormalID(0x1102, "ATTListA bad entry");
				}
				for (q2 = q1->Next; nullpr != q2->Next; q2 = q2->Next) {
					ui5r common_mask = (q1->cmpmask) & (q2->cmpmask);
					if ((q1->cmpvalu & common_mask) ==
						(q2->cmpvalu & common_mask))
					{
						ReportAbnormalID(0x1103, "ATTListA Conflict");
					}
				}
			}
		}
#endif

		SetHeadATTel(h);
	}
}

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
LOCALPROC SetUp_RAM24(void)
{
	ATTer r;
	ui5r bankbit = 0x00100000 << (((VIA2_iA7 << 1) | VIA2_iA6) << 1);

#if kRAMa_Size == kRAMb_Size
	if (kRAMa_Size == bankbit) {
		/* properly set up balanced RAM */
		r.cmpmask = 0x00FFFFFF & ~ ((1 << kRAM_ln2Spc) - 1);
		r.cmpvalu = 0;
		r.usemask = ((1 << kRAM_ln2Spc) - 1) & (kRAM_Size - 1);
		r.usebase = RAM;
		r.Access = kATTA_readwritereadymask;
		AddToATTList(&r);
	} else
#endif
	{
		bankbit &= 0x00FFFFFF; /* if too large, always use RAMa */

		if (0 != bankbit) {
#if kRAMb_Size != 0
			r.cmpmask = bankbit
				| (0x00FFFFFF & ~ ((1 << kRAM_ln2Spc) - 1));
			r.cmpvalu = bankbit;
			r.usemask = ((1 << kRAM_ln2Spc) - 1) & (kRAMb_Size - 1);
			r.usebase = kRAMa_Size + RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
#endif
		}

		{
			r.cmpmask = bankbit
				| (0x00FFFFFF & ~ ((1 << kRAM_ln2Spc) - 1));
			r.cmpvalu = 0;
			r.usemask = ((1 << kRAM_ln2Spc) - 1) & (kRAMa_Size - 1);
			r.usebase = RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
		}
	}
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
LOCALPROC SetUp_io(void)
{
	ATTer r;

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_VIA1;
	AddToATTList(&r);

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x2000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x2000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_VIA2;
	AddToATTList(&r);

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x4000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x4000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCC;
	AddToATTList(&r);

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x0C000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x0C000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_Extn;
	AddToATTList(&r);

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x10000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x10000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCSI;
	AddToATTList(&r);

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x14000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x14000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_ASC;
	AddToATTList(&r);

	if (Addr32) {
		r.cmpmask = 0xFF01E000;
		r.cmpvalu = 0x50000000 | 0x16000;
	} else {
		r.cmpmask = 0x00F1E000;
		r.cmpvalu = 0x00F00000 | 0x16000;
	}
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_IWM;
	AddToATTList(&r);

#if 0
		case 14:
			/*
				fail, nothing supposed to be here,
				but rom accesses it anyway
			*/
			{
				ui5r addr2 = addr & 0x1FFFF;

				if ((addr2 != 0x1DA00) && (addr2 != 0x1DC00)) {
					ReportAbnormalID(0x1104, "another unknown access");
				}
			}
			get_fail_realblock(p);
			break;
#endif
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
LOCALPROC SetUp_address24(void)
{
	ATTer r;

#if 0
	if (MemOverlay) {
		ReportAbnormalID(0x1105, "Overlay with 24 bit addressing");
	}
#endif

	if (MemOverlay) {
		r.cmpmask = Overlay_ROM_CmpZeroMask |
			(0x00FFFFFF & ~ ((1 << kRAM_ln2Spc) - 1));
		r.cmpvalu = kRAM_Base;
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTList(&r);
	} else {
		SetUp_RAM24();
	}

	r.cmpmask = kROM_cmpmask;
	r.cmpvalu = kROM_Base;
	r.usemask = kROM_Size - 1;
	r.usebase = ROM;
	r.Access = kATTA_readreadymask;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~ (0x100000 - 1);
	r.cmpvalu = 0x900000;
	r.usemask = (kVidMemRAM_Size - 1) & (0x100000 - 1);
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#if kVidMemRAM_Size >= 0x00200000
	r.cmpmask = 0x00FFFFFF & ~ (0x100000 - 1);
	r.cmpvalu = 0xA00000;
	r.usemask = (0x100000 - 1);
	r.usebase = VidMem + (1 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif
#if kVidMemRAM_Size >= 0x00400000
	r.cmpmask = 0x00FFFFFF & ~ (0x100000 - 1);
	r.cmpvalu = 0xB00000;
	r.usemask = (0x100000 - 1);
	r.usebase = VidMem + (2 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
	r.cmpmask = 0x00FFFFFF & ~ (0x100000 - 1);
	r.cmpvalu = 0xC00000;
	r.usemask = (0x100000 - 1);
	r.usebase = VidMem + (3 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	SetUp_io();
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
LOCALPROC SetUp_address32(void)
{
	ATTer r;

	if (MemOverlay) {
		r.cmpmask = ~ ((1 << 30) - 1);
		r.cmpvalu = 0;
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTList(&r);
	} else {
		ui5r bankbit =
			0x00100000 << (((VIA2_iA7 << 1) | VIA2_iA6) << 1);
#if kRAMa_Size == kRAMb_Size
		if (kRAMa_Size == bankbit) {
			/* properly set up balanced RAM */
			r.cmpmask = ~ ((1 << 30) - 1);
			r.cmpvalu = 0;
			r.usemask = kRAM_Size - 1;
			r.usebase = RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
		} else
#endif
		{
#if kRAMb_Size != 0
			r.cmpmask = bankbit | ~ ((1 << 30) - 1);
			r.cmpvalu = bankbit;
			r.usemask = kRAMb_Size - 1;
			r.usebase = kRAMa_Size + RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
#endif

			r.cmpmask = bankbit | ~ ((1 << 30) - 1);
			r.cmpvalu = 0;
			r.usemask = kRAMa_Size - 1;
			r.usebase = RAM;
			r.Access = kATTA_readwritereadymask;
			AddToATTList(&r);
		}
	}

	r.cmpmask = ~ ((1 << 28) - 1);
	r.cmpvalu = 0x40000000;
	r.usemask = kROM_Size - 1;
	r.usebase = ROM;
	r.Access = kATTA_readreadymask;
	AddToATTList(&r);

#if 0
	/* haven't persuaded emulated computer to look here yet. */
	/* NuBus super space */
	r.cmpmask = ~ ((1 << 28) - 1);
	r.cmpvalu = 0x90000000;
	r.usemask = kVidMemRAM_Size - 1;
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	/* Standard NuBus space */
	r.cmpmask = ~ ((1 << 20) - 1);
	r.cmpvalu = 0xF9F00000;
	r.usemask = kVidROM_Size - 1;
	r.usebase = VidROM;
	r.Access = kATTA_readreadymask;
	AddToATTList(&r);
#if 0
	r.cmpmask = ~ 0x007FFFFF;
	r.cmpvalu = 0xF9000000;
	r.usemask = 0x007FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	r.cmpmask = ~ 0x000FFFFF;
	r.cmpvalu = 0xF9900000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
/* kludge to allow more than 1M of Video Memory */
#if kVidMemRAM_Size >= 0x00200000
	r.cmpmask = ~ 0x000FFFFF;
	r.cmpvalu = 0xF9A00000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem + (1 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif
#if kVidMemRAM_Size >= 0x00400000
	r.cmpmask = ~ 0x000FFFFF;
	r.cmpvalu = 0xF9B00000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem + (2 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
	r.cmpmask = ~ 0x000FFFFF;
	r.cmpvalu = 0xF9C00000;
	r.usemask = 0x000FFFFF & (kVidMemRAM_Size - 1);
	r.usebase = VidMem + (3 << 20);
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	SetUp_io();

#if 0
	if ((addr >= 0x58000000) && (addr < 0x58000004)) {
		/* test hardware. fail */
	}
#endif
}
#endif

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
LOCALPROC SetUp_address(void)
{
	if (Addr32) {
		SetUp_address32();
	} else {
		SetUp_address24();
	}
}
#endif

/*
	unlike in the real Mac Plus, Mini vMac
	will allow misaligned memory access,
	since it is easier to allow it than
	it is to correctly simulate a bus error
	and back out of the current instruction.
*/

#ifndef ln2mtb
#define AddToATTListWithMTB AddToATTList
#else
LOCALPROC AddToATTListWithMTB(ATTep p)
{
	/*
		Test of memory mapping system.
	*/
	ATTer r;

	r.Access = p->Access;
	r.cmpmask = p->cmpmask | (1 << ln2mtb);
	r.usemask = p->usemask & ~ (1 << ln2mtb);

	r.cmpvalu = p->cmpvalu + (1 << ln2mtb);
	r.usebase = p->usebase;
	AddToATTList(&r);

	r.cmpvalu = p->cmpvalu;
	r.usebase = p->usebase + (1 << ln2mtb);
	AddToATTList(&r);
}
#endif

#if (CurEmMd != kEmMd_II) && (CurEmMd != kEmMd_IIx)
LOCALPROC SetUp_RAM24(void)
{
	ATTer r;

#if (0 == kRAMb_Size) || (kRAMa_Size == kRAMb_Size)
	r.cmpmask = 0x00FFFFFF & ~ ((1 << kRAM_ln2Spc) - 1);
	r.cmpvalu = kRAM_Base;
	r.usemask = kRAM_Size - 1;
	r.usebase = RAM;
	r.Access = kATTA_readwritereadymask;
	AddToATTListWithMTB(&r);
#else
	/* unbalanced memory */

#if 0 != (0x00FFFFFF & kRAMa_Size)
	/* condition should always be true if configuration file right */
	r.cmpmask = 0x00FFFFFF & (kRAMa_Size | ~ ((1 << kRAM_ln2Spc) - 1));
	r.cmpvalu = kRAM_Base + kRAMa_Size;
	r.usemask = kRAMb_Size - 1;
	r.usebase = kRAMa_Size + RAM;
	r.Access = kATTA_readwritereadymask;
	AddToATTListWithMTB(&r);
#endif

	r.cmpmask = 0x00FFFFFF & (kRAMa_Size | ~ ((1 << kRAM_ln2Spc) - 1));
	r.cmpvalu = kRAM_Base;
	r.usemask = kRAMa_Size - 1;
	r.usebase = RAM;
	r.Access = kATTA_readwritereadymask;
	AddToATTListWithMTB(&r);
#endif
}
#endif

#if (CurEmMd != kEmMd_II) && (CurEmMd != kEmMd_IIx)
LOCALPROC SetUp_address(void)
{
	ATTer r;

	if (MemOverlay) {
		r.cmpmask = Overlay_ROM_CmpZeroMask |
			(0x00FFFFFF & ~ ((1 << kRAM_ln2Spc) - 1));
		r.cmpvalu = kRAM_Base;
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTListWithMTB(&r);
	} else {
		SetUp_RAM24();
	}

	r.cmpmask = kROM_cmpmask;
	r.cmpvalu = kROM_Base;
#if (CurEmMd >= kEmMd_SE)
	if (MemOverlay) {
		r.usebase = nullpr;
		r.Access = kATTA_ntfymask;
		r.Ntfy = kMAN_OverlayOff;
		AddToATTList(&r);
	} else
#endif
	{
		r.usemask = kROM_Size - 1;
		r.usebase = ROM;
		r.Access = kATTA_readreadymask;
		AddToATTListWithMTB(&r);
	}

	if (MemOverlay) {
		r.cmpmask = 0x00E00000;
		r.cmpvalu = kRAM_Overlay_Base;
#if (0 == kRAMb_Size) || (kRAMa_Size == kRAMb_Size)
		r.usemask = kRAM_Size - 1;
			/* note that cmpmask and usemask overlap for 4M */
		r.usebase = RAM;
		r.Access = kATTA_readwritereadymask;
#else
		/* unbalanced memory */
		r.usemask = kRAMb_Size - 1;
		r.usebase = kRAMa_Size + RAM;
		r.Access = kATTA_readwritereadymask;
#endif
		AddToATTListWithMTB(&r);
	}

#if IncludeVidMem
	r.cmpmask = 0x00FFFFFF & ~ ((1 << kVidMem_ln2Spc) - 1);
	r.cmpvalu = kVidMem_Base;
	r.usemask = kVidMemRAM_Size - 1;
	r.usebase = VidMem;
	r.Access = kATTA_readwritereadymask;
	AddToATTList(&r);
#endif

	r.cmpmask = 0x00FFFFFF & ~ ((1 << kVIA1_ln2Spc) - 1);
	r.cmpvalu = kVIA1_Block_Base;
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_VIA1;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~ ((1 << kSCC_ln2Spc) - 1);
	r.cmpvalu = kSCCRd_Block_Base;
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCC;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~ ((1 << kExtn_ln2Spc) - 1);
	r.cmpvalu = kExtn_Block_Base;
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_Extn;
	AddToATTList(&r);

#if CurEmMd == kEmMd_PB100
	r.cmpmask = 0x00FFFFFF & ~ ((1 << kASC_ln2Spc) - 1);
	r.cmpvalu = kASC_Block_Base;
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_ASC;
	AddToATTList(&r);
#endif

	r.cmpmask = 0x00FFFFFF & ~ ((1 << kSCSI_ln2Spc) - 1);
	r.cmpvalu = kSCSI_Block_Base;
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_SCSI;
	AddToATTList(&r);

	r.cmpmask = 0x00FFFFFF & ~ ((1 << kIWM_ln2Spc) - 1);
	r.cmpvalu = kIWM_Block_Base;
	r.usebase = nullpr;
	r.Access = kATTA_mmdvmask;
	r.MMDV = kMMDV_IWM;
	AddToATTList(&r);
}
#endif

LOCALPROC SetUpMemBanks(void)
{
	InitATTList();

	SetUp_address();

	FinishATTList();
}

#if 0
LOCALPROC get_fail_realblock(ATTep p)
{
	p->cmpmask = 0;
	p->cmpvalu = 0xFFFFFFFF;
	p->usemask = 0;
	p->usebase = nullpr;
	p->Access = 0;
}
#endif

GLOBALFUNC ui5b MMDV_Access(ATTep p, ui5b Data,
	blnr WriteMem, blnr ByteSize, CPTR addr)
{
	switch (p->MMDV) {
		case kMMDV_VIA1:
			if (! ByteSize) {
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
				if (WriteMem && (addr == 0xF40006)) {
					/* for weirdness on shutdown in System 6 */
#if 0
					VIA1_Access((Data >> 8) & 0x00FF, WriteMem,
							(addr >> 9) & kVIA1_Mask);
					VIA1_Access((Data) & 0x00FF, WriteMem,
							(addr >> 9) & kVIA1_Mask);
#endif
				} else
#endif
				{
					ReportAbnormalID(0x1106, "access VIA1 word");
				}
			} else if ((addr & 1) != 0) {
				ReportAbnormalID(0x1107, "access VIA1 odd");
			} else {
#if CurEmMd != kEmMd_PB100
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
				if ((addr & 0x000001FE) != 0x00000000)
#else
				if ((addr & 0x000FE1FE) != 0x000FE1FE)
#endif
				{
					ReportAbnormalID(0x1108,
						"access VIA1 nonstandard address");
				}
#endif
				Data = VIA1_Access(Data, WriteMem,
					(addr >> 9) & kVIA1_Mask);
			}

			break;
#if EmVIA2
		case kMMDV_VIA2:
			if (! ByteSize) {
				if ((! WriteMem)
					&& ((0x3e00 == (addr & 0x1FFFF))
						|| (0x3e02 == (addr & 0x1FFFF))))
				{
					/* for weirdness at offset 0x71E in ROM */
					Data =
						(VIA2_Access(Data, WriteMem,
							(addr >> 9) & kVIA2_Mask) << 8)
						| VIA2_Access(Data, WriteMem,
							(addr >> 9) & kVIA2_Mask);

				} else {
					ReportAbnormalID(0x1109, "access VIA2 word");
				}
			} else if ((addr & 1) != 0) {
				if (0x3FFF == (addr & 0x1FFFF)) {
					/*
						for weirdness at offset 0x7C4 in ROM.
						looks like bug.
					*/
					Data = VIA2_Access(Data, WriteMem,
						(addr >> 9) & kVIA2_Mask);
				} else {
					ReportAbnormalID(0x110A, "access VIA2 odd");
				}
			} else {
				if ((addr & 0x000001FE) != 0x00000000) {
					ReportAbnormalID(0x110B,
						"access VIA2 nonstandard address");
				}
				Data = VIA2_Access(Data, WriteMem,
					(addr >> 9) & kVIA2_Mask);
			}
			break;
#endif
		case kMMDV_SCC:

#if (CurEmMd >= kEmMd_SE) \
	&& ! ((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

			if ((addr & 0x00100000) == 0) {
				ReportAbnormalID(0x110C,
					"access SCC unassigned address");
			} else
#endif
			if (! ByteSize) {
				ReportAbnormalID(0x110D, "Attemped Phase Adjust");
			} else
#if ! ((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))
			if (WriteMem != ((addr & 1) != 0)) {
				if (WriteMem) {
#if CurEmMd >= kEmMd_512Ke
#if CurEmMd != kEmMd_PB100
					ReportAbnormalID(0x110E, "access SCC even/odd");
					/*
						This happens on boot with 64k ROM.
					*/
#endif
#endif
				} else {
					SCC_Reset();
				}
			} else
#endif
#if (CurEmMd != kEmMd_PB100) \
	&& ! ((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

			if (WriteMem != (addr >= kSCCWr_Block_Base)) {
				ReportAbnormalID(0x110F, "access SCC wr/rd base wrong");
			} else
#endif
			{
#if CurEmMd != kEmMd_PB100
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
				if ((addr & 0x1FF9) != 0x00000000)
#else
				if ((addr & 0x001FFFF8) != 0x001FFFF8)
#endif
				{
					ReportAbnormalID(0x1110,
						"access SCC nonstandard address");
				}
#endif
				Data = SCC_Access(Data, WriteMem,
					(addr >> 1) & kSCC_Mask);
			}
			break;
		case kMMDV_Extn:
			if (ByteSize) {
				ReportAbnormalID(0x1111, "access Sony byte");
			} else if ((addr & 1) != 0) {
				ReportAbnormalID(0x1112, "access Sony odd");
			} else if (! WriteMem) {
				ReportAbnormalID(0x1113, "access Sony read");
			} else {
				Extn_Access(Data, (addr >> 1) & 0x0F);
			}
			break;
#if EmASC
		case kMMDV_ASC:
			if (! ByteSize) {
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
				if (WriteMem) {
					(void) ASC_Access((Data >> 8) & 0x00FF,
						WriteMem, addr & kASC_Mask);
					Data = ASC_Access((Data) & 0x00FF,
						WriteMem, (addr + 1) & kASC_Mask);
				} else {
					Data =
						(ASC_Access((Data >> 8) & 0x00FF,
							WriteMem, addr & kASC_Mask) << 8)
						| ASC_Access((Data) & 0x00FF,
							WriteMem, (addr + 1) & kASC_Mask);
				}
#else
				ReportAbnormalID(0x1114, "access ASC word");
#endif
			} else {
				Data = ASC_Access(Data, WriteMem, addr & kASC_Mask);
			}
			break;
#endif
		case kMMDV_SCSI:
			if (! ByteSize) {
				ReportAbnormalID(0x1115, "access SCSI word");
			} else
#if ! ((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))
			if (WriteMem != ((addr & 1) != 0)) {
				ReportAbnormalID(0x1116, "access SCSI even/odd");
			} else
#endif
			{
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
				if ((addr & 0x1F8F) != 0x00000000) {
					ReportAbnormalID(0x1117,
						"access SCSI nonstandard address");
				}
#endif
				Data = SCSI_Access(Data, WriteMem, (addr >> 4) & 0x07);
			}

			break;
		case kMMDV_IWM:
#if (CurEmMd >= kEmMd_SE) \
	&& ! ((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

			if ((addr & 0x00100000) == 0) {
				ReportAbnormalID(0x1118,
					"access IWM unassigned address");
			} else
#endif
			if (! ByteSize) {
#if ExtraAbnormalReports
				ReportAbnormalID(0x1119, "access IWM word");
				/*
					This happens when quitting 'Glider 3.1.2'.
					perhaps a bad handle is being disposed of.
				*/
#endif
			} else
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
			if ((addr & 1) != 0) {
				ReportAbnormalID(0x111A, "access IWM odd");
			} else
#else
			if ((addr & 1) == 0) {
				ReportAbnormalID(0x111B, "access IWM even");
			} else
#endif
			{
#if (CurEmMd != kEmMd_PB100) \
	&& ! ((CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx))

				if ((addr & 0x001FE1FF) != 0x001FE1FF) {
					ReportAbnormalID(0x111C,
						"access IWM nonstandard address");
				}
#endif
				Data = IWM_Access(Data, WriteMem,
					(addr >> 9) & kIWM_Mask);
			}

			break;
	}

	return Data;
}

GLOBALFUNC blnr MemAccessNtfy(ATTep pT)
{
	blnr v = falseblnr;

	switch (pT->Ntfy) {
#if CurEmMd >= kEmMd_SE
		case kMAN_OverlayOff:
			pT->Access = kATTA_readreadymask;

			MemOverlay = 0;
			SetUpMemBanks();

			v = trueblnr;

			break;
#endif
	}

	return v;
}

GLOBALPROC MemOverlay_ChangeNtfy(void)
{
#if CurEmMd <= kEmMd_Plus
	SetUpMemBanks();
#elif (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
	SetUpMemBanks();
#endif
}

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
GLOBALPROC Addr32_ChangeNtfy(void)
{
	SetUpMemBanks();
}
#endif

LOCALFUNC ATTep get_address_realblock1(blnr WriteMem, CPTR addr)
{
	ATTep p;

Label_Retry:
	p = FindATTel(addr);
	if (0 != (p->Access &
		(WriteMem ? kATTA_writereadymask : kATTA_readreadymask)))
	{
		/* ok */
	} else {
		if (0 != (p->Access & kATTA_ntfymask)) {
			if (MemAccessNtfy(p)) {
				goto Label_Retry;
			}
		}
		p = nullpr; /* fail */
	}

	return p;
}

GLOBALFUNC ui3p get_real_address0(ui5b L, blnr WritableMem, CPTR addr,
	ui5b *actL)
{
	ui5b bankleft;
	ui3p p;
	ATTep q;

	q = get_address_realblock1(WritableMem, addr);
	if (nullpr == q) {
		*actL = 0;
		p = nullpr;
	} else {
		ui5r m2 = q->usemask & ~ q->cmpmask;
		ui5r m3 = m2 & ~ (m2 + 1);
		p = q->usebase + (addr & q->usemask);
		bankleft = (m3 + 1) - (addr & m3);
		if (bankleft >= L) {
			/* this block is big enough (by far the most common case) */
			*actL = L;
		} else {
			*actL = bankleft;
		}
	}

	return p;
}

GLOBALVAR blnr InterruptButton = falseblnr;

GLOBALPROC SetInterruptButton(blnr v)
{
	if (InterruptButton != v) {
		InterruptButton = v;
		VIAorSCCinterruptChngNtfy();
	}
}

LOCALVAR ui3b CurIPL = 0;

GLOBALPROC VIAorSCCinterruptChngNtfy(void)
{
#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
	ui3b NewIPL;

	if (InterruptButton) {
		NewIPL = 7;
	} else if (SCCInterruptRequest) {
		NewIPL = 4;
	} else if (VIA2_InterruptRequest) {
		NewIPL = 2;
	} else if (VIA1_InterruptRequest) {
		NewIPL = 1;
	} else {
		NewIPL = 0;
	}
#else
	ui3b VIAandNotSCC = VIA1_InterruptRequest
		& ~ SCCInterruptRequest;
	ui3b NewIPL = VIAandNotSCC
		| (SCCInterruptRequest << 1)
		| (InterruptButton << 2);
#endif
	if (NewIPL != CurIPL) {
		CurIPL = NewIPL;
		m68k_IPLchangeNtfy();
	}
}

GLOBALFUNC blnr AddrSpac_Init(void)
{
	int i;

	for (i = 0; i < kNumWires; i++) {
		Wires[i] = 1;
	}

	MINEM68K_Init(
		&CurIPL);
	return trueblnr;
}

GLOBALPROC Memory_Reset(void)
{
	MemOverlay = 1;
	SetUpMemBanks();
}

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
EXPORTPROC PowerOff_ChangeNtfy(void);
GLOBALPROC PowerOff_ChangeNtfy(void)
{
	if (! VIA2_iB2) {
		ForceMacOff = trueblnr;
	}
}
#endif

/* user event queue utilities */

#if HaveMasterMyEvtQLock
GLOBALVAR ui4r MasterMyEvtQLock = 0;
	/*
		Takes a few ticks to process button event because
		of debounce code of Mac. So have this mechanism
		to prevent processing further events meanwhile.
	*/
#endif

GLOBALFUNC blnr FindKeyEvent(int *VirtualKey, blnr *KeyDown)
{
	MyEvtQEl *p;

	if (
#if HaveMasterMyEvtQLock
		(0 == MasterMyEvtQLock) &&
#endif
		(nullpr != (p = MyEvtQOutP())))
	{
		if (MyEvtQElKindKey == p->kind) {
			*VirtualKey = p->u.press.key;
			*KeyDown = p->u.press.down;
			MyEvtQOutDone();
			return trueblnr;
		}
	}

	return falseblnr;
}

/* task management */

#ifdef _VIA_Debug
#include <stdio.h>
#endif

GLOBALVAR uimr ICTactive;
GLOBALVAR iCountt ICTwhen[kNumICTs];

GLOBALPROC ICT_Zap(void)
{
	ICTactive = 0;
}

LOCALPROC InsertICT(int taskid, iCountt when)
{
	ICTwhen[taskid] = when;
	ICTactive |= (1 << taskid);
}

GLOBALVAR iCountt NextiCount = 0;

GLOBALFUNC iCountt GetCuriCount(void)
{
	return NextiCount - GetCyclesRemaining();
}

GLOBALPROC ICT_add(int taskid, ui5b n)
{
	/* n must be > 0 */
	si5r x = GetCyclesRemaining();
	ui5b when = NextiCount - x + n;

#ifdef _VIA_Debug
	fprintf(stderr, "ICT_add: %d, %d, %d\n", when, taskid, n);
#endif
	InsertICT(taskid, when);

	if (x > (si5r)n) {
		SetCyclesRemaining(n);
		NextiCount = when;
	}
}
