/*
	M68KITAB.c

	Copyright (C) 2007 Paul C. Pratt

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
	Motorola 68K Instructions TABle
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#endif

#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"

#include "M68KITAB.h"

struct WorkR {
	/* expected size : 8 bytes */
	ui5b opcode;
	ui5b opsize;
	ui4r MainClass;
#if WantCycByPriOp
	ui4r Cycles;
#endif
	DecOpR DecOp;
};
typedef struct WorkR WorkR;

#define b76(p) ((p->opcode >> 6) & 3)
#define b8(p) ((p->opcode >> 8) & 1)
#define mode(p) ((p->opcode >> 3) & 7)
#define reg(p) (p->opcode & 7)
#define md6(p) ((p->opcode >> 6) & 7)
#define rg9(p) ((p->opcode >> 9) & 7)

enum {
	kAddrValidAny,
	kAddrValidData,
	kAddrValidDataAlt,
	kAddrValidControl,
	kAddrValidControlAlt,
	kAddrValidAltMem,

	kNumAddrValids
};

#define kAddrValidMaskAny        (1 << kAddrValidAny)
#define kAddrValidMaskData       (1 << kAddrValidData)
#define kAddrValidMaskDataAlt    (1 << kAddrValidDataAlt)
#define kAddrValidMaskControl    (1 << kAddrValidControl)
#define kAddrValidMaskControlAlt (1 << kAddrValidControlAlt)
#define kAddrValidMaskAltMem     (1 << kAddrValidAltMem)

#define CheckInSet(v, m) (0 != ((1 << (v)) & (m)))

#define kMyAvgCycPerInstr (10 * kCycleScale + (40 * kCycleScale / 64))

LOCALFUNC MayNotInline ui3r GetArgkRegSz(WorkR *p)
{
	ui3r CurArgk;

	switch (p->opsize) {
		case 1:
			CurArgk = kArgkRegB;
			break;
		case 2:
		default: /* keep compiler happy */
			CurArgk = kArgkRegW;
			break;
		case 4:
			CurArgk = kArgkRegL;
			break;
	}

	return CurArgk;
}

LOCALFUNC MayNotInline ui3r GetArgkMemSz(WorkR *p)
{
	ui3r CurArgk;

	switch (p->opsize) {
		case 1:
			CurArgk = kArgkMemB;
			break;
		case 2:
		default: /* keep compiler happy */
			CurArgk = kArgkMemW;
			break;
		case 4:
			CurArgk = kArgkMemL;
			break;
	}

	return CurArgk;
}

#if WantCycByPriOp
LOCALFUNC MayNotInline ui4r OpEACalcCyc(WorkR *p, ui3r m, ui3r r)
{
	ui4r v;

	switch (m) {
		case 0:
		case 1:
			v = 0;
			break;
		case 2:
			v = ((4 == p->opsize)
				? (8 * kCycleScale + 2 * RdAvgXtraCyc)
				: (4 * kCycleScale + RdAvgXtraCyc));
			break;
		case 3:
			v = ((4 == p->opsize)
				? (8 * kCycleScale + 2 * RdAvgXtraCyc)
				: (4 * kCycleScale + RdAvgXtraCyc));
			break;
		case 4:
			v = ((4 == p->opsize)
				? (10 * kCycleScale + 2 * RdAvgXtraCyc)
				: (6 * kCycleScale + RdAvgXtraCyc));
			break;
		case 5:
			v = ((4 == p->opsize)
				? (12 * kCycleScale + 3 * RdAvgXtraCyc)
				: (8 * kCycleScale + 2 * RdAvgXtraCyc));
			break;
		case 6:
			v = ((4 == p->opsize)
				? (14 * kCycleScale + 3 * RdAvgXtraCyc)
				: (10 * kCycleScale + 2 * RdAvgXtraCyc));
			break;
		case 7:
			switch (r) {
				case 0:
					v = ((4 == p->opsize)
						? (12 * kCycleScale + 3 * RdAvgXtraCyc)
						: (8 * kCycleScale + 2 * RdAvgXtraCyc));
					break;
				case 1:
					v = ((4 == p->opsize)
						? (16 * kCycleScale + 4 * RdAvgXtraCyc)
						: (12 * kCycleScale + 3 * RdAvgXtraCyc));
					break;
				case 2:
					v = ((4 == p->opsize)
						? (12 * kCycleScale + 3 * RdAvgXtraCyc)
						: (8 * kCycleScale + 2 * RdAvgXtraCyc));
					break;
				case 3:
					v = ((4 == p->opsize)
						? (14 * kCycleScale + 3 * RdAvgXtraCyc)
						: (10 * kCycleScale + 2 * RdAvgXtraCyc));
					break;
				case 4:
					v = ((4 == p->opsize)
						? (8 * kCycleScale + 2 * RdAvgXtraCyc)
						: (4 * kCycleScale + RdAvgXtraCyc));
					break;
				default:
					v = 0;
					break;
			}
			break;
		default: /* keep compiler happy */
			v = 0;
			break;
	}

	return v;
}
#endif

#if WantCycByPriOp
LOCALFUNC MayNotInline ui4r OpEADestCalcCyc(WorkR *p, ui3r m, ui3r r)
{
	ui4r v;

	switch (m) {
		case 0:
		case 1:
			v = 0;
			break;
		case 2:
			v = ((4 == p->opsize)
				? (8 * kCycleScale + 2 * WrAvgXtraCyc)
				: (4 * kCycleScale + WrAvgXtraCyc));
			break;
		case 3:
			v = ((4 == p->opsize)
				? (8 * kCycleScale + 2 * WrAvgXtraCyc)
				: (4 * kCycleScale + WrAvgXtraCyc));
			break;
		case 4:
			v = ((4 == p->opsize)
				? (8 * kCycleScale + 2 * WrAvgXtraCyc)
				: (4 * kCycleScale + WrAvgXtraCyc));
			break;
		case 5:
			v = ((4 == p->opsize)
				? (12 * kCycleScale + RdAvgXtraCyc + 2 * WrAvgXtraCyc)
				: (8 * kCycleScale + RdAvgXtraCyc + WrAvgXtraCyc));
			break;
		case 6:
			v = ((4 == p->opsize)
				? (14 * kCycleScale + RdAvgXtraCyc + 2 * WrAvgXtraCyc)
				: (10 * kCycleScale + RdAvgXtraCyc + WrAvgXtraCyc));
			break;
		case 7:
			switch (r) {
				case 0:
					v = ((4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc));
					break;
				case 1:
					v = ((4 == p->opsize)
						? (16 * kCycleScale
							+ 2 * RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (12 * kCycleScale
							+ 2 * RdAvgXtraCyc + WrAvgXtraCyc));
					break;
				default:
					v = 0;
					break;
			}
			break;
		default: /* keep compiler happy */
			v = 0;
			break;
	}

	return v;
}
#endif

LOCALPROC SetDcoArgFields(WorkR *p, blnr src,
	ui3r CurAMd, ui3r CurArgk, ui3r CurArgDat)
{
	ui5b *pv = src ? (&p->DecOp.B) : (&p->DecOp.A);
	ui5r v = *pv;

	SetDcoFldArgDat(v, CurArgDat);
	SetDcoFldAMd(v, CurAMd);
	SetDcoFldArgk(v, CurArgk);

	*pv = v;
}

LOCALFUNC MayNotInline blnr CheckValidAddrMode(WorkR *p,
	ui3r m, ui3r r, ui3r v, blnr src)
{
	ui3r CurAMd = 0; /* init to keep compiler happy */
	ui3r CurArgk = 0; /* init to keep compiler happy */
	ui3r CurArgDat = 0;
	blnr IsOk;

	switch (m) {
		case 0:
			CurAMd = kAMdReg;
			CurArgk = GetArgkRegSz(p);
			CurArgDat = r;
			IsOk = CheckInSet(v,
				kAddrValidMaskAny | kAddrValidMaskData
					| kAddrValidMaskDataAlt);
			break;
		case 1:
			CurAMd = kAMdReg;
			CurArgk = GetArgkRegSz(p);
			CurArgDat = r + 8;
			IsOk = CheckInSet(v, kAddrValidMaskAny);
			break;
		case 2:
			CurAMd = kAMdIndirect;
			CurArgk = GetArgkMemSz(p);
			CurArgDat = r + 8;
			IsOk = CheckInSet(v,
				kAddrValidMaskAny | kAddrValidMaskData
					| kAddrValidMaskDataAlt | kAddrValidMaskControl
					| kAddrValidMaskControlAlt | kAddrValidMaskAltMem);
			break;
		case 3:
			switch (p->opsize) {
				case 1:
					if (7 == r) {
						CurAMd = kAMdAPosIncW;
					} else {
						CurAMd = kAMdAPosIncB;
					}
					break;
				case 2:
				default: /* keep compiler happy */
					CurAMd = kAMdAPosIncW;
					break;
				case 4:
					CurAMd = kAMdAPosIncL;
					break;
			}
			CurArgk = GetArgkMemSz(p);
			CurArgDat = r + 8;
			IsOk = CheckInSet(v,
				kAddrValidMaskAny | kAddrValidMaskData
					| kAddrValidMaskDataAlt | kAddrValidMaskAltMem);
			break;
		case 4:
			switch (p->opsize) {
				case 1:
					if (7 == r) {
						CurAMd = kAMdAPreDecW;
					} else {
						CurAMd = kAMdAPreDecB;
					}
					break;
				case 2:
				default: /* keep compiler happy */
					CurAMd = kAMdAPreDecW;
					break;
				case 4:
					CurAMd = kAMdAPreDecL;
					break;
			}
			CurArgk = GetArgkMemSz(p);
			CurArgDat = r + 8;
			IsOk = CheckInSet(v,
				kAddrValidMaskAny | kAddrValidMaskData
					| kAddrValidMaskDataAlt | kAddrValidMaskAltMem);
			break;
		case 5:
			CurAMd = kAMdADisp;
			CurArgk = GetArgkMemSz(p);
			CurArgDat = r + 8;
			IsOk = CheckInSet(v,
				kAddrValidMaskAny | kAddrValidMaskData
					| kAddrValidMaskDataAlt | kAddrValidMaskControl
					| kAddrValidMaskControlAlt | kAddrValidMaskAltMem);
			break;
		case 6:
			CurAMd = kAMdAIndex;
			CurArgk = GetArgkMemSz(p);
			CurArgDat = r + 8;
			IsOk = CheckInSet(v,
				kAddrValidMaskAny | kAddrValidMaskData
					| kAddrValidMaskDataAlt | kAddrValidMaskControl
					| kAddrValidMaskControlAlt | kAddrValidMaskAltMem);
			break;
		case 7:
			switch (r) {
				case 0:
					CurAMd = kAMdAbsW;
					CurArgk = GetArgkMemSz(p);
					IsOk = CheckInSet(v,
						kAddrValidMaskAny | kAddrValidMaskData
							| kAddrValidMaskDataAlt
							| kAddrValidMaskControl
							| kAddrValidMaskControlAlt
							| kAddrValidMaskAltMem);
					break;
				case 1:
					CurAMd = kAMdAbsL;
					CurArgk = GetArgkMemSz(p);
					IsOk = CheckInSet(v,
						kAddrValidMaskAny | kAddrValidMaskData
							| kAddrValidMaskDataAlt
							| kAddrValidMaskControl
							| kAddrValidMaskControlAlt
							| kAddrValidMaskAltMem);
					break;
				case 2:
					CurAMd = kAMdPCDisp;
					CurArgk = GetArgkMemSz(p);
					IsOk = CheckInSet(v,
						kAddrValidMaskAny | kAddrValidMaskData
							| kAddrValidMaskControl);
					break;
				case 3:
					CurAMd = kAMdPCIndex;
					CurArgk = GetArgkMemSz(p);
					IsOk = CheckInSet(v,
						kAddrValidMaskAny | kAddrValidMaskData
							| kAddrValidMaskControl);
					break;
				case 4:
					switch (p->opsize) {
						case 1:
							CurAMd = kAMdImmedB;
							break;
						case 2:
						default: /* keep compiler happy */
							CurAMd = kAMdImmedW;
							break;
						case 4:
							CurAMd = kAMdImmedL;
							break;
					}
					CurArgk = kArgkCnst;
					IsOk = CheckInSet(v,
						kAddrValidMaskAny | kAddrValidMaskData);
					break;
				default:
					IsOk = falseblnr;
					break;
			}
			break;
		default: /* keep compiler happy */
			IsOk = falseblnr;
			break;
	}

	if (IsOk) {
		SetDcoArgFields(p, src, CurAMd, CurArgk, CurArgDat);
	}

	return IsOk;
}

#if WantCycByPriOp
LOCALFUNC MayNotInline blnr LeaPeaEACalcCyc(WorkR *p, ui3r m, ui3r r)
{
	ui4r v;

	UnusedParam(p);
	switch (m) {
		case 2:
			v = 0;
			break;
		case 5:
			v = (4 * kCycleScale + RdAvgXtraCyc);
			break;
		case 6:
			v = (8 * kCycleScale + RdAvgXtraCyc);
			break;
		case 7:
			switch (r) {
				case 0:
					v = (4 * kCycleScale + RdAvgXtraCyc);
					break;
				case 1:
					v = (8 * kCycleScale + 2 * RdAvgXtraCyc);
					break;
				case 2:
					v = (4 * kCycleScale + RdAvgXtraCyc);
					break;
				case 3:
					v = (8 * kCycleScale + RdAvgXtraCyc);
					break;
				default:
					v = 0;
					break;
			}
			break;
		default: /* keep compiler happy */
			v = 0;
			break;
	}

	return v;
}
#endif

LOCALFUNC blnr IsValidAddrMode(WorkR *p)
{
	return CheckValidAddrMode(p,
		mode(p), reg(p), kAddrValidAny, falseblnr);
}

LOCALFUNC MayNotInline blnr CheckDataAltAddrMode(WorkR *p)
{
	return CheckValidAddrMode(p,
		mode(p), reg(p), kAddrValidDataAlt, falseblnr);
}

LOCALFUNC MayNotInline blnr CheckDataAddrMode(WorkR *p)
{
	return CheckValidAddrMode(p,
		mode(p), reg(p), kAddrValidData, falseblnr);
}

LOCALFUNC MayNotInline blnr CheckControlAddrMode(WorkR *p)
{
	return CheckValidAddrMode(p,
		mode(p), reg(p), kAddrValidControl, falseblnr);
}

LOCALFUNC MayNotInline blnr CheckControlAltAddrMode(WorkR *p)
{
	return CheckValidAddrMode(p,
		mode(p), reg(p), kAddrValidControlAlt, falseblnr);
}

LOCALFUNC MayNotInline blnr CheckAltMemAddrMode(WorkR *p)
{
	return CheckValidAddrMode(p,
		mode(p), reg(p), kAddrValidAltMem, falseblnr);
}

LOCALPROC FindOpSizeFromb76(WorkR *p)
{
	p->opsize = 1 << b76(p);
#if 0
	switch (b76(p)) {
		case 0 :
			p->opsize = 1;
			break;
		case 1 :
			p->opsize = 2;
			break;
		case 2 :
			p->opsize = 4;
			break;
	}
#endif
}

LOCALFUNC ui3r OpSizeOffset(WorkR *p)
{
	ui3r v;

	switch (p->opsize) {
		case 1 :
			v = 0;
			break;
		case 2 :
			v = 1;
			break;
		case 4 :
		default :
			v = 2;
			break;
	}

	return v;
}


LOCALFUNC ui5r octdat(ui5r x)
{
	if (x == 0) {
		return 8;
	} else {
		return x;
	}
}

LOCALPROC MayInline DeCode0(WorkR *p)
{
	if (b8(p) == 1) {
		if (mode(p) == 1) {
			/* MoveP 0000ddd1mm001aaa */
#if WantCycByPriOp
			switch (b76(p)) {
				case 0:
					p->Cycles = (16 * kCycleScale + 4 * RdAvgXtraCyc);
					break;
				case 1:
					p->Cycles = (24 * kCycleScale + 6 * RdAvgXtraCyc);
					break;
				case 2:
					p->Cycles = (16 * kCycleScale
						+ 2 * RdAvgXtraCyc + 2 * WrAvgXtraCyc);
					break;
				case 3:
				default: /* keep compiler happy */
					p->Cycles = (24 * kCycleScale
						+ 2 * RdAvgXtraCyc + 4 * WrAvgXtraCyc);
					break;
			}
#endif
			p->MainClass = kIKindMoveP;
		} else {
			/* dynamic bit, Opcode = 0000ddd1ttmmmrrr */
			if (mode(p) == 0) {
#if WantCycByPriOp
				switch (b76(p)) {
					case 0: /* BTst */
						p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
						break;
					case 1: /* BChg */
						p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
						break;
					case 2: /* BClr */
						p->Cycles = (10 * kCycleScale + RdAvgXtraCyc);
						break;
					case 3: /* BSet */
						p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
						break;
				}
#endif
				p->MainClass = kIKindBitOpDD;
			} else {
				if (b76(p) == 0) { /* BTst */
					if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindBitOpDM;
					}
				} else {
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles = (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindBitOpDM;
					}
				}
			}
		}
	} else {
		if (rg9(p) == 4) {
			/* static bit 00001010ssmmmrrr */
			if (mode(p) == 0) {
#if WantCycByPriOp
				switch (b76(p)) {
					case 0: /* BTst */
						p->Cycles =
							(10 * kCycleScale + 2 * RdAvgXtraCyc);
						break;
					case 1: /* BChg */
						p->Cycles =
							(12 * kCycleScale + 2 * RdAvgXtraCyc);
						break;
					case 2: /* BClr */
						p->Cycles =
							(14 * kCycleScale + 2 * RdAvgXtraCyc);
						break;
					case 3: /* BSet */
						p->Cycles =
							(12 * kCycleScale + 2 * RdAvgXtraCyc);
						break;
				}
#endif
				p->MainClass = kIKindBitOpND;
			} else {
				if (b76(p) == 0) { /* BTst */
					if ((mode(p) == 7) && (reg(p) == 4)) {
						p->MainClass = kIKindIllegal;
					} else {
						if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
							p->Cycles =
								(8 * kCycleScale + 2 * RdAvgXtraCyc);
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindBitOpNM;
						}
					}
				} else {
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles = (12 * kCycleScale
							+ 2 * RdAvgXtraCyc + WrAvgXtraCyc);
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindBitOpNM;
					}
				}
			}
		} else
		if (b76(p) == 3) {
#if Use68020
			if (rg9(p) < 3) {
				/* CHK2 or CMP2 00000ss011mmmrrr */
				if (CheckControlAddrMode(p)) {
#if WantCycByPriOp
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindCHK2orCMP2;
				}
			} else
			if (rg9(p) >= 5) {
				if ((mode(p) == 7) && (reg(p) == 4)) {
					/* CAS2 00001ss011111100 */
					p->MainClass = kIKindCAS2;
				} else {
					/* CAS 00001ss011mmmrrr */
					p->MainClass = kIKindCAS;
				}
			} else
			if (rg9(p) == 3) {
				/* CALLM or RTM 0000011011mmmrrr */
				p->MainClass = kIKindCallMorRtm;
			} else
#endif
			{
				p->MainClass = kIKindIllegal;
			}
		} else
		if (rg9(p) == 6) {
			/* CMPI 00001100ssmmmrrr */
#if 0
			if (CheckDataAltAddrMode(p)) {
				p->MainClass = kIKindCmpI;
			}
#endif
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, 7, 4, kAddrValidAny, trueblnr))
			if (CheckValidAddrMode(p,
				mode(p), reg(p), kAddrValidDataAlt, falseblnr))
			{
#if WantCycByPriOp
				if (0 == mode(p)) {
					p->Cycles = (4 == p->opsize)
						? (14 * kCycleScale + 3 * RdAvgXtraCyc)
						: (8 * kCycleScale + 2 * RdAvgXtraCyc);
				} else {
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale + 3 * RdAvgXtraCyc)
						: (8 * kCycleScale + 2 * RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindCmpB + OpSizeOffset(p);
			}
		} else if (rg9(p) == 7) {
#if Use68020
			/* MoveS 00001110ssmmmrrr */
			if (CheckAltMemAddrMode(p)) {
#if WantCycByPriOp
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindMoveS;
			}
#else
			p->MainClass = kIKindIllegal;
#endif
		} else {
			if ((mode(p) == 7) && (reg(p) == 4)) {
				switch (rg9(p)) {
					case 0:
					case 1:
					case 5:
#if WantCycByPriOp
						p->Cycles =
							(20 * kCycleScale + 3 * RdAvgXtraCyc);
#endif
						p->MainClass = kIKindBinOpStatusCCR;
						break;
					default:
						p->MainClass = kIKindIllegal;
						break;
				}
			} else {
				switch (rg9(p)) {
					case 0:
#if 0
						if (CheckDataAltAddrMode(p)) {
							p->MainClass = kIKindOrI;
						}
#endif
						FindOpSizeFromb76(p);
						if (CheckValidAddrMode(p, 7, 4,
							kAddrValidAny, trueblnr))
						if (CheckValidAddrMode(p, mode(p), reg(p),
							kAddrValidDataAlt, falseblnr))
						{
#if WantCycByPriOp
							if (0 != mode(p)) {
								p->Cycles = (4 == p->opsize)
									? (20 * kCycleScale
										+ 3 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc)
									: (12 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ WrAvgXtraCyc);
							} else {
								p->Cycles = (4 == p->opsize)
									? (16 * kCycleScale
										+ 3 * RdAvgXtraCyc)
									: (8 * kCycleScale
										+ 2 * RdAvgXtraCyc);
							}
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindOrI;
						}
						break;
					case 1:
#if 0
						if (CheckDataAltAddrMode(p)) {
							p->MainClass = kIKindAndI;
						}
#endif
						FindOpSizeFromb76(p);
						if (CheckValidAddrMode(p, 7, 4,
							kAddrValidAny, trueblnr))
						if (CheckValidAddrMode(p, mode(p), reg(p),
							kAddrValidDataAlt, falseblnr))
						{
#if WantCycByPriOp
							if (0 != mode(p)) {
								p->Cycles = (4 == p->opsize)
									? (20 * kCycleScale
										+ 3 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc)
									: (12 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ WrAvgXtraCyc);
							} else {
								p->Cycles = (4 == p->opsize)
									? (14 * kCycleScale
										+ 3 * RdAvgXtraCyc)
									: (8 * kCycleScale
										+ 2 * RdAvgXtraCyc);
							}
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindAndI;
						}
						break;
					case 2:
#if 0
						if (CheckDataAltAddrMode(p)) {
							p->MainClass = kIKindSubI;
						}
#endif
						FindOpSizeFromb76(p);
						if (CheckValidAddrMode(p, 7, 4,
							kAddrValidAny, trueblnr))
						if (CheckValidAddrMode(p, mode(p), reg(p),
							kAddrValidDataAlt, falseblnr))
						{
#if WantCycByPriOp
							if (0 != mode(p)) {
								p->Cycles = (4 == p->opsize)
									? (20 * kCycleScale
										+ 3 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc)
									: (12 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ WrAvgXtraCyc);
							} else {
								p->Cycles = (4 == p->opsize)
									? (16 * kCycleScale
										+ 3 * RdAvgXtraCyc)
									: (8 * kCycleScale
										+ 2 * RdAvgXtraCyc);
							}
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindSubB + OpSizeOffset(p);
						}
						break;
					case 3:
#if 0
						if (CheckDataAltAddrMode(p)) {
							p->MainClass = kIKindAddI;
						}
#endif
						FindOpSizeFromb76(p);
						if (CheckValidAddrMode(p, 7, 4,
							kAddrValidAny, trueblnr))
						if (CheckValidAddrMode(p, mode(p), reg(p),
							kAddrValidDataAlt, falseblnr))
						{
#if WantCycByPriOp
							if (0 != mode(p)) {
								p->Cycles = (4 == p->opsize)
									? (20 * kCycleScale
										+ 3 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc)
									: (12 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ WrAvgXtraCyc);
							} else {
								p->Cycles = (4 == p->opsize)
									? (16 * kCycleScale
										+ 3 * RdAvgXtraCyc)
									: (8 * kCycleScale
										+ 2 * RdAvgXtraCyc);
							}
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindAddB + OpSizeOffset(p);
						}
						break;
					case 5:
#if 0
						if (CheckDataAltAddrMode(p)) {
							p->MainClass = kIKindEorI;
						}
#endif
						FindOpSizeFromb76(p);
						if (CheckValidAddrMode(p, 7, 4,
							kAddrValidAny, trueblnr))
						if (CheckValidAddrMode(p, mode(p), reg(p),
							kAddrValidDataAlt, falseblnr))
						{
#if WantCycByPriOp
							if (0 != mode(p)) {
								p->Cycles = (4 == p->opsize)
									? (20 * kCycleScale
										+ 3 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc)
									: (12 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ WrAvgXtraCyc);
							} else {
								p->Cycles = (4 == p->opsize)
									? (16 * kCycleScale
										+ 3 * RdAvgXtraCyc)
									: (8 * kCycleScale
										+ 2 * RdAvgXtraCyc);
							}
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindEorI;
						}
						break;
					default:
						/* for compiler. should be 0, 1, 2, 3, or 5 */
						p->MainClass = kIKindIllegal;
						break;
				}
			}
		}
	}
}

LOCALPROC MayInline DeCode1(WorkR *p)
{
	p->opsize = 1;
	if (md6(p) == 1) { /* MOVEA */
		p->MainClass = kIKindIllegal;
	} else if (mode(p) == 1) {
		/* not allowed for byte sized move */
		p->MainClass = kIKindIllegal;
	} else {
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		if (CheckValidAddrMode(p, md6(p), rg9(p),
			kAddrValidDataAlt, falseblnr))
		{
#if WantCycByPriOp
			p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
			p->Cycles += OpEADestCalcCyc(p, md6(p), rg9(p));
#endif
			p->MainClass = kIKindMoveB;
		}
	}
}

LOCALPROC MayInline DeCode2(WorkR *p)
{
	p->opsize = 4;
	if (md6(p) == 1) { /* MOVEA */
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		if (CheckValidAddrMode(p, 1, rg9(p),
			kAddrValidAny, falseblnr))
		{
#if WantCycByPriOp
			p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			p->MainClass = kIKindMoveAL;
		}
	} else {
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		if (CheckValidAddrMode(p, md6(p), rg9(p),
			kAddrValidDataAlt, falseblnr))
		{
#if WantCycByPriOp
			p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
			p->Cycles += OpEADestCalcCyc(p, md6(p), rg9(p));
#endif
			p->MainClass = kIKindMoveL;
		}
	}
}

LOCALPROC MayInline DeCode3(WorkR *p)
{
	p->opsize = 2;
	if (md6(p) == 1) { /* MOVEA */
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		if (CheckValidAddrMode(p, 1, rg9(p),
			kAddrValidAny, falseblnr))
		{
#if WantCycByPriOp
			p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			p->MainClass = kIKindMoveAW;
		}
	} else {
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		if (CheckValidAddrMode(p, md6(p), rg9(p),
			kAddrValidDataAlt, falseblnr))
		{
#if WantCycByPriOp
			p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
			p->Cycles += OpEADestCalcCyc(p, md6(p), rg9(p));
#endif
			p->MainClass = kIKindMoveW;
		}
	}
}

#if WantCycByPriOp

#if WantCloserCyc
#define MoveAvgN 0
#else
#define MoveAvgN 3
#endif

LOCALFUNC MayNotInline ui4r MoveMEACalcCyc(WorkR *p, ui3r m, ui3r r)
{
	ui4r v;

	UnusedParam(p);
	switch (m) {
		case 2:
		case 3:
		case 4:
			v = (8 * kCycleScale + 2 * RdAvgXtraCyc);
			break;
		case 5:
			v = (12 * kCycleScale + 3 * RdAvgXtraCyc);
			break;
		case 6:
			v = (14 * kCycleScale + 3 * RdAvgXtraCyc);
			break;
		case 7:
			switch (r) {
				case 0:
					v = (12 * kCycleScale + 3 * RdAvgXtraCyc);
					break;
				case 1:
					v = (16 * kCycleScale + 4 * RdAvgXtraCyc);
					break;
				default:
					v = 0;
					break;
			}
			break;
		default: /* keep compiler happy */
			v = 0;
			break;
	}

	return v;
}

#endif

LOCALPROC MayInline DeCode4(WorkR *p)
{
	if (b8(p) != 0) {
		switch (b76(p)) {
			case 0:
#if Use68020
				/* Chk.L 0100ddd100mmmrrr */
				if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindChkL;
				}
#else
				p->MainClass = kIKindIllegal;
#endif
				break;
			case 1:
				p->MainClass = kIKindIllegal;
				break;
			case 2:
				/* Chk.W 0100ddd110mmmrrr */
				if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
					p->Cycles = (10 * kCycleScale + RdAvgXtraCyc);
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindChkW;
				}
				break;
			case 3:
			default: /* keep compiler happy */
#if Use68020
				if ((0 == mode(p)) && (4 == rg9(p))) {
					p->MainClass = kIKindEXTBL;
				} else
#endif
				{
					/* Lea 0100aaa111mmmrrr */
					if (CheckControlAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
						p->Cycles +=
							LeaPeaEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindLea;
					}
				}
				break;
		}
	} else {
		switch (rg9(p)) {
			case 0:
				if (b76(p) != 3) {
					/* NegX 01000000ssmmmrrr */
					FindOpSizeFromb76(p);
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (4 == p->opsize)
								? (12 * kCycleScale
									+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
								: (8 * kCycleScale
									+ RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles = (4 == p->opsize)
								? (6 * kCycleScale + RdAvgXtraCyc)
								: (4 * kCycleScale + RdAvgXtraCyc);
						}
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindNegXB + OpSizeOffset(p);
					}
				} else {
#if Use68020
/* reference seems incorrect to say not for 68000 */
#endif
					/* Move from SR 0100000011mmmrrr */
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles =
							(12 * kCycleScale + 2 * RdAvgXtraCyc);
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindMoveSREa;
					}
				}
				break;
			case 1:
				if (b76(p) != 3) {
					/* Clr 01000010ssmmmrrr */
					FindOpSizeFromb76(p);
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (4 == p->opsize)
								? (12 * kCycleScale
									+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
								: (8 * kCycleScale
									+ RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles = (4 == p->opsize)
								? (6 * kCycleScale + RdAvgXtraCyc)
								: (4 * kCycleScale + RdAvgXtraCyc);
						}
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindClr;
					}
				} else {
#if Use68020
					/* Move from CCR 0100001011mmmrrr */
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindMoveCCREa;
					}
#else
					p->MainClass = kIKindIllegal;
#endif
				}
				break;
			case 2:
				if (b76(p) != 3) {
					/* Neg 01000100ssmmmrrr */
					FindOpSizeFromb76(p);
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (4 == p->opsize)
								? (12 * kCycleScale
									+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
								: (8 * kCycleScale
									+ RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles = (4 == p->opsize)
								? (6 * kCycleScale + RdAvgXtraCyc)
								: (4 * kCycleScale + RdAvgXtraCyc);
						}
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindNegB + OpSizeOffset(p);
					}
				} else {
					/* Move to CCR 0100010011mmmrrr */
					if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
						p->Cycles = (12 * kCycleScale + RdAvgXtraCyc);
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindMoveEaCCR;
					}
				}
				break;
			case 3:
				if (b76(p) != 3) {
					/* Not 01000110ssmmmrrr */
					FindOpSizeFromb76(p);
					if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (4 == p->opsize)
								? (12 * kCycleScale
									+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
								: (8 * kCycleScale
									+ RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles = (4 == p->opsize)
								? (6 * kCycleScale + RdAvgXtraCyc)
								: (4 * kCycleScale + RdAvgXtraCyc);
						}
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindNot;
					}
				} else {
					/* Move from SR 0100011011mmmrrr */
					if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (8 * kCycleScale
								+ RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles =
								(6 * kCycleScale + RdAvgXtraCyc);
						}
						p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
						p->MainClass = kIKindMoveEaSR;
					}
				}
				break;
			case 4:
				switch (b76(p)) {
					case 0:
#if Use68020
						if (mode(p) == 1) {
							/* Link.L 0100100000001rrr */
							p->MainClass = kIKindLinkL;
						} else
#endif
						{
							/* Nbcd 0100100000mmmrrr */
							if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
								if (0 != mode(p)) {
									p->Cycles = (8 * kCycleScale
										+ RdAvgXtraCyc + WrAvgXtraCyc);
								} else {
									p->Cycles = (6 * kCycleScale
										+ RdAvgXtraCyc);
								}
								p->Cycles +=
									OpEACalcCyc(p, mode(p), reg(p));
#endif
								p->MainClass = kIKindNbcd;
							}
						}
						break;
					case 1:
						if (mode(p) == 0) {
							/* Swap 0100100001000rrr */
#if WantCycByPriOp
							p->Cycles =
								(4 * kCycleScale + RdAvgXtraCyc);
#endif
							p->MainClass = kIKindSwap;
						} else
#if Use68020
						if (mode(p) == 1) {
							p->MainClass = kIKindBkpt;
						} else
#endif
						{
							/* PEA 0100100001mmmrrr */
							if (CheckControlAddrMode(p)) {
#if WantCycByPriOp
								p->Cycles = (12 * kCycleScale
										+ RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc);
								p->Cycles +=
									LeaPeaEACalcCyc(p, mode(p), reg(p));
#endif
								p->MainClass = kIKindPEA;
							}
						}
						break;
					case 2:
						if (mode(p) == 0) {
							/* EXT.W */
#if WantCycByPriOp
							p->Cycles =
								(4 * kCycleScale + RdAvgXtraCyc);
#endif
							p->MainClass = kIKindEXTW;
						} else {
							/* MOVEM reg to mem 01001d001ssmmmrrr */
							if (mode(p) == 4) {
#if WantCycByPriOp
								p->Cycles =
									MoveMEACalcCyc(p, mode(p), reg(p));
								p->Cycles += MoveAvgN * 4 * kCycleScale
									+ MoveAvgN * WrAvgXtraCyc;
#endif
								p->MainClass = kIKindMOVEMRmMW;
							} else {
								if (CheckControlAltAddrMode(p)) {
#if WantCycByPriOp
									p->Cycles = MoveMEACalcCyc(p,
										mode(p), reg(p));
									p->Cycles +=
										MoveAvgN * 4 * kCycleScale
											+ MoveAvgN * WrAvgXtraCyc;
#endif
									p->MainClass = kIKindMOVEMrm;
								}
							}
						}
						break;
					case 3:
					default: /* keep compiler happy */
						if (mode(p) == 0) {
							/* EXT.L */
#if WantCycByPriOp
							p->Cycles =
								(4 * kCycleScale + RdAvgXtraCyc);
#endif
							p->MainClass = kIKindEXTL;
						} else {
							/* MOVEM reg to mem 01001d001ssmmmrrr */
							if (mode(p) == 4) {
#if WantCycByPriOp
								p->Cycles = MoveMEACalcCyc(p,
									mode(p), reg(p));
								p->Cycles += MoveAvgN * 8 * kCycleScale
									+ MoveAvgN * 2 * WrAvgXtraCyc;
#endif
								p->MainClass = kIKindMOVEMRmML;
							} else {
								if (CheckControlAltAddrMode(p)) {
#if WantCycByPriOp
									p->Cycles = MoveMEACalcCyc(p,
										mode(p), reg(p));
									p->Cycles +=
										MoveAvgN * 8 * kCycleScale
										+ MoveAvgN * 2 * WrAvgXtraCyc;
#endif
									p->MainClass = kIKindMOVEMrm;
								}
							}
						}
						break;
				}
				break;
			case 5:
				if (b76(p) == 3) {
					if ((mode(p) == 7) && (reg(p) == 4)) {
						/* the ILLEGAL instruction */
						p->MainClass = kIKindIllegal;
					} else {
						/* Tas 0100101011mmmrrr */
						if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
							if (0 != mode(p)) {
								p->Cycles = (14 * kCycleScale
									+ 2 * RdAvgXtraCyc + WrAvgXtraCyc);
							} else {
								p->Cycles = (4 * kCycleScale
									+ RdAvgXtraCyc);
							}
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindTas;
						}
					}
				} else {
					/* Tst 01001010ssmmmrrr */
					FindOpSizeFromb76(p);
					if (b76(p) == 0) {
						if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
							p->Cycles =
								(4 * kCycleScale + RdAvgXtraCyc);
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindTst;
						}
					} else {
						if (IsValidAddrMode(p)) {
#if WantCycByPriOp
							p->Cycles =
								(4 * kCycleScale + RdAvgXtraCyc);
							p->Cycles +=
								OpEACalcCyc(p, mode(p), reg(p));
#endif
							p->MainClass = kIKindTst;
						}
					}
				}
				break;
			case 6:
				if (((p->opcode >> 7) & 1) == 1) {
					/* MOVEM mem to reg 0100110011smmmrrr */
					FindOpSizeFromb76(p);
					if (mode(p) == 3) {
#if WantCycByPriOp
						p->Cycles = 4 * kCycleScale + RdAvgXtraCyc;
						p->Cycles += MoveMEACalcCyc(p, mode(p), reg(p));
						if (4 == p->opsize) {
							p->Cycles += MoveAvgN * 8 * kCycleScale
								+ 2 * MoveAvgN * RdAvgXtraCyc;
						} else {
							p->Cycles += MoveAvgN * 4 * kCycleScale
								+ MoveAvgN * RdAvgXtraCyc;
						}
#endif
						if (b76(p) == 2) {
							p->MainClass = kIKindMOVEMApRW;
						} else {
							p->MainClass = kIKindMOVEMApRL;
						}
					} else {
						if (CheckControlAddrMode(p)) {
#if WantCycByPriOp
							p->Cycles = 4 * kCycleScale + RdAvgXtraCyc;
							p->Cycles += MoveMEACalcCyc(p,
								mode(p), reg(p));
							if (4 == p->opsize) {
								p->Cycles += MoveAvgN * 8 * kCycleScale
									+ 2 * MoveAvgN * RdAvgXtraCyc;
							} else {
								p->Cycles += MoveAvgN * 4 * kCycleScale
									+ MoveAvgN * RdAvgXtraCyc;
							}
#endif
							p->MainClass = kIKindMOVEMmr;
						}
					}
				} else {
#if Use68020
					if (((p->opcode >> 6) & 1) == 1) {
						/* DIVU 0100110001mmmrrr 0rrr0s0000000rrr */
						/* DIVS 0100110001mmmrrr 0rrr1s0000000rrr */
						p->MainClass = kIKindDivL;
					} else {
						/* MULU 0100110000mmmrrr 0rrr0s0000000rrr */
						/* MULS 0100110000mmmrrr 0rrr1s0000000rrr */
						p->MainClass = kIKindMulL;
					}
#else
					p->MainClass = kIKindIllegal;
#endif
				}
				break;
			case 7:
			default: /* keep compiler happy */
				switch (b76(p)) {
					case 0:
						p->MainClass = kIKindIllegal;
						break;
					case 1:
						switch (mode(p)) {
							case 0:
							case 1:
								/* Trap 010011100100vvvv */
#if WantCycByPriOp
								p->Cycles = (34 * kCycleScale
									+ 4 * RdAvgXtraCyc
									+ 3 * WrAvgXtraCyc);
#endif
								p->MainClass = kIKindTrap;
								break;
							case 2:
								/* Link */
#if WantCycByPriOp
								p->Cycles = (16 * kCycleScale
									+ 2 * RdAvgXtraCyc
									+ 2 * WrAvgXtraCyc);
#endif
								if (reg(p) == 6) {
									p->MainClass = kIKindLinkA6;
								} else {
									p->MainClass = kIKindLink;
								}
								break;
							case 3:
								/* Unlk */
#if WantCycByPriOp
								p->Cycles = (12 * kCycleScale
									+ 3 * RdAvgXtraCyc);
#endif
								if (reg(p) == 6) {
									p->MainClass = kIKindUnlkA6;
								} else {
									p->MainClass = kIKindUnlk;
								}
								break;
							case 4:
								/* MOVE USP 0100111001100aaa */
#if WantCycByPriOp
								p->Cycles =
									(4 * kCycleScale + RdAvgXtraCyc);
#endif
								p->MainClass = kIKindMoveRUSP;
								break;
							case 5:
								/* MOVE USP 0100111001101aaa */
#if WantCycByPriOp
								p->Cycles =
									(4 * kCycleScale + RdAvgXtraCyc);
#endif
								p->MainClass = kIKindMoveUSPR;
								break;
							case 6:
								switch (reg(p)) {
									case 0:
										/* Reset 0100111001100000 */
#if WantCycByPriOp
										p->Cycles = (132 * kCycleScale
											+ RdAvgXtraCyc);
#endif
										p->MainClass = kIKindReset;
										break;
									case 1:
										/* Nop = 0100111001110001 */
#if WantCycByPriOp
										p->Cycles = (4 * kCycleScale
											+ RdAvgXtraCyc);
#endif
										p->MainClass = kIKindNop;
										break;
									case 2:
										/* Stop 0100111001110010 */
#if WantCycByPriOp
										p->Cycles = (4 * kCycleScale);
#endif
										p->MainClass = kIKindStop;
										break;
									case 3:
										/* Rte 0100111001110011 */
#if WantCycByPriOp
										p->Cycles = (20 * kCycleScale
											+ 5 * RdAvgXtraCyc);
#endif
										p->MainClass = kIKindRte;
										break;
									case 4:
										/* Rtd 0100111001110100 */
#if Use68020
										p->MainClass = kIKindRtd;
#else
										p->MainClass = kIKindIllegal;
#endif
										break;
									case 5:
										/* Rts 0100111001110101 */
#if WantCycByPriOp
										p->Cycles = (16 * kCycleScale
											+ 4 * RdAvgXtraCyc);
#endif
										p->MainClass = kIKindRts;
										break;
									case 6:
										/* TrapV 0100111001110110 */
#if WantCycByPriOp
										p->Cycles = (4 * kCycleScale
											+ RdAvgXtraCyc);
#endif
										p->MainClass = kIKindTrapV;
										break;
									case 7:
									default: /* keep compiler happy */
										/* Rtr 0100111001110111 */
#if WantCycByPriOp
										p->Cycles = (20 * kCycleScale
											+ 2 * RdAvgXtraCyc);
#endif
										p->MainClass = kIKindRtr;
										break;
								}
								break;
							case 7:
							default: /* keep compiler happy */
#if Use68020
								/* MOVEC 010011100111101m */
								p->MainClass = kIKindMoveC;
#else
								p->MainClass = kIKindIllegal;
#endif
								break;
						}
						break;
					case 2:
						/* Jsr 0100111010mmmrrr */
						if (CheckControlAddrMode(p)) {
#if WantCycByPriOp
							switch (mode(p)) {
								case 2:
									p->Cycles = (16 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc);
									break;
								case 5:
									p->Cycles = (18 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc);
									break;
								case 6:
									p->Cycles = (22 * kCycleScale
										+ 2 * RdAvgXtraCyc
										+ 2 * WrAvgXtraCyc);
									break;
								case 7:
								default: /* keep compiler happy */
									switch (reg(p)) {
										case 0:
											p->Cycles =
												(18 * kCycleScale
													+ 2 * RdAvgXtraCyc
													+ 2 * WrAvgXtraCyc);
											break;
										case 1:
											p->Cycles =
												(20 * kCycleScale
													+ 3 * RdAvgXtraCyc
													+ 2 * WrAvgXtraCyc);
											break;
										case 2:
											p->Cycles =
												(18 * kCycleScale
													+ 2 * RdAvgXtraCyc
													+ 2 * WrAvgXtraCyc);
											break;
										case 3:
										default:
											/* keep compiler happy */
											p->Cycles =
												(22 * kCycleScale
													+ 2 * RdAvgXtraCyc
													+ 2 * WrAvgXtraCyc);
											break;
									}
									break;
							}
#endif
							p->MainClass = kIKindJsr;
						}
						break;
					case 3:
					default: /* keep compiler happy */
						/* JMP 0100111011mmmrrr */
						if (CheckControlAddrMode(p)) {
#if WantCycByPriOp
							switch (mode(p)) {
								case 2:
									p->Cycles = (8 * kCycleScale
										+ 2 * RdAvgXtraCyc);
									break;
								case 5:
									p->Cycles = (10 * kCycleScale
										+ 2 * RdAvgXtraCyc);
									break;
								case 6:
									p->Cycles = (14 * kCycleScale
										+ 2 * RdAvgXtraCyc);
									break;
								case 7:
								default: /* keep compiler happy */
									switch (reg(p)) {
										case 0:
											p->Cycles =
												(10 * kCycleScale
													+ 2 * RdAvgXtraCyc);
											break;
										case 1:
											p->Cycles =
												(12 * kCycleScale
													+ 3 * RdAvgXtraCyc);
											break;
										case 2:
											p->Cycles =
												(10 * kCycleScale
													+ 2 * RdAvgXtraCyc);
											break;
										case 3:
										default:
											/* keep compiler happy */
											p->Cycles =
												(14 * kCycleScale
													+ 3 * RdAvgXtraCyc);
											break;
									}
									break;
							}
#endif
							p->MainClass = kIKindJmp;
						}
						break;
				}
				break;
		}
	}
}

LOCALPROC MayInline DeCode5(WorkR *p)
{
	if (b76(p) == 3) {
		if (mode(p) == 1) {
			/* DBcc 0101cccc11001ddd */
#if WantCycByPriOp
#if WantCloserCyc
			p->Cycles = 0;
#else
			p->Cycles = (11 * kCycleScale + 2 * RdAvgXtraCyc);
				/*
					average of cc true 12(2/0),
					cc false taken 10(2/0),
					and not 14(3/0)
				*/
#endif
#endif
			if (1 == ((p->opcode >> 8) & 15)) {
				p->MainClass = kIKindDBF;
			} else {
				p->MainClass = kIKindDBcc;
			}
		} else {
#if Use68020
			if ((mode(p) == 7) && (reg(p) >= 2)) {
				/* TRAPcc 0101cccc11111sss */
				p->MainClass = kIKindTRAPcc;
			} else
#endif
			{
				p->opsize = 1;
				/* Scc 0101cccc11mmmrrr */
				if (CheckDataAltAddrMode(p)) {
#if WantCycByPriOp
					if (0 != mode(p)) {
						p->Cycles = (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
					} else {
#if WantCloserCyc
						p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
#else
						p->Cycles = (5 * kCycleScale + RdAvgXtraCyc);
							/* 4 when false, 6 when true */
#endif
					}
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindScc;
				}
			}
		}
	} else {
		if (mode(p) == 1) {
			p->opsize = b8(p) * 2 + 2;
			SetDcoArgFields(p, trueblnr, kAMdDat4,
				kArgkCnst, octdat(rg9(p)));
			SetDcoArgFields(p, falseblnr, kAMdReg,
				kArgkRegL, reg(p) + 8);
				/* always long, regardless of opsize */
			if (b8(p) == 0) {
#if WantCycByPriOp
				p->Cycles = (4 == p->opsize)
					? (8 * kCycleScale + RdAvgXtraCyc)
					: (4 * kCycleScale + RdAvgXtraCyc);
#endif
				p->MainClass = kIKindAddQA; /* AddQA 0101nnn0ss001rrr */
			} else {
#if WantCycByPriOp
				p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
#endif
				p->MainClass = kIKindSubQA; /* SubQA 0101nnn1ss001rrr */
			}
		} else {
			FindOpSizeFromb76(p);
			SetDcoArgFields(p, trueblnr, kAMdDat4,
				kArgkCnst, octdat(rg9(p)));
			if (CheckValidAddrMode(p,
				mode(p), reg(p), kAddrValidDataAlt, falseblnr))
			{
#if WantCycByPriOp
				if (0 != mode(p)) {
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
				} else {
					p->Cycles = (4 == p->opsize)
						? (8 * kCycleScale + RdAvgXtraCyc)
						: (4 * kCycleScale + RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				if (b8(p) == 0) {
					/* AddQ 0101nnn0ssmmmrrr */
#if 0
					if (CheckDataAltAddrMode(p)) {
						p->MainClass = kIKindAddQ;
					}
#endif
					p->MainClass = kIKindAddB + OpSizeOffset(p);
				} else {
					/* SubQ 0101nnn1ssmmmrrr */
#if 0
					if (CheckDataAltAddrMode(p)) {
						p->MainClass = kIKindSubQ;
					}
#endif
					p->MainClass = kIKindSubB + OpSizeOffset(p);
				}
			}
		}
	}
}

LOCALPROC MayInline DeCode6(WorkR *p)
{
	ui5b cond = (p->opcode >> 8) & 15;

	if (cond == 1) {
		/* Bsr 01100001nnnnnnnn */
#if WantCycByPriOp
		p->Cycles = (18 * kCycleScale
			+ 2 * RdAvgXtraCyc + 2 * WrAvgXtraCyc);
#endif
		if (0 == (p->opcode & 255)) {
			p->MainClass = kIKindBsrW;
		} else
#if Use68020
		if (255 == (p->opcode & 255)) {
			p->MainClass = kIKindBsrL;
		} else
#endif
		{
			p->MainClass = kIKindBsrB;
		}
	} else if (cond == 0) {
		/* Bra 01100000nnnnnnnn */
#if WantCycByPriOp
		p->Cycles = (10 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
		if (0 == (p->opcode & 255)) {
			p->MainClass = kIKindBraW;
		} else
#if Use68020
		if (255 == (p->opcode & 255)) {
			p->MainClass = kIKindBraL;
		} else
#endif
		{
			p->MainClass = kIKindBraB;
		}
	} else {
		/* Bcc 0110ccccnnnnnnnn */
		if (0 == (p->opcode & 255)) {
#if WantCycByPriOp
#if WantCloserCyc
			p->Cycles = 0;
#else
			p->Cycles = (11 * kCycleScale + 2 * RdAvgXtraCyc);
				/* average of branch taken 10(2/0) and not 12(2/0) */
#endif
#endif
			p->MainClass = kIKindBccW;
		} else
#if Use68020
		if (255 == (p->opcode & 255)) {
			p->MainClass = kIKindBccL;
		} else
#endif
		{
#if WantCycByPriOp
#if WantCloserCyc
			p->Cycles = 0;
#else
			p->Cycles = (9 * kCycleScale
				+ RdAvgXtraCyc + (RdAvgXtraCyc / 2));
				/* average of branch taken 10(2/0) and not 8(1/0) */
#endif
#endif
			p->MainClass = kIKindBccB;
		}
	}
}

LOCALPROC MayInline DeCode7(WorkR *p)
{
	if (b8(p) == 0) {
		p->opsize = 4;
#if WantCycByPriOp
		p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
#endif
		p->MainClass = kIKindMoveQ;
	} else {
		p->MainClass = kIKindIllegal;
	}
}

LOCALPROC MayInline DeCode8(WorkR *p)
{
	if (b76(p) == 3) {
		p->opsize = 2;
		if (CheckDataAddrMode(p)) {
			if (b8(p) == 0) {
				/* DivU 1000ddd011mmmrrr */
#if WantCycByPriOp
				p->Cycles = RdAvgXtraCyc;
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#if ! WantCloserCyc
				p->Cycles += 133 * kCycleScale;
					/*
						worse case 140, less than ten percent
						different from best case
					*/
#endif
#endif
				p->MainClass = kIKindDivU;
			} else {
				/* DivS 1000ddd111mmmrrr */
#if WantCycByPriOp
				p->Cycles = RdAvgXtraCyc;
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#if ! WantCloserCyc
				p->Cycles += 150 * kCycleScale;
					/*
						worse case 158, less than ten percent different
						from best case
					*/
#endif
#endif
				p->MainClass = kIKindDivS;
			}
		}
	} else {
		if (b8(p) == 0) {
			/* OR 1000ddd0ssmmmrrr */
#if 0
			if (CheckDataAddrMode(p)) {
				p->MainClass = kIKindOrEaD;
			}
#endif
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, mode(p), reg(p),
				kAddrValidData, trueblnr))
			if (CheckValidAddrMode(p, 0, rg9(p),
				kAddrValidAny, falseblnr))
			{
#if WantCycByPriOp
				if (4 == p->opsize) {
					if ((mode(p) < 2)
						|| ((7 == mode(p)) && (reg(p) == 4)))
					{
						p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
					} else {
						p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
					}
				} else {
					p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindOrEaD;
			}
		} else {
			if (mode(p) < 2) {
				switch (b76(p)) {
					case 0:
						/* SBCD 1000xxx10000mxxx */
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (18 * kCycleScale
								+ 3 * RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles = (6 * kCycleScale
								+ RdAvgXtraCyc);
						}
#endif
						if (mode(p) == 0) {
							p->MainClass = kIKindSbcdr;
						} else {
							p->MainClass = kIKindSbcdm;
						}
						break;
#if Use68020
					case 1:
						/* PACK 1000rrr10100mrrr */
						p->MainClass = kIKindPack;
						break;
					case 2:
						/* UNPK 1000rrr11000mrrr */
						p->MainClass = kIKindUnpk;
						break;
#endif
					default:
						p->MainClass = kIKindIllegal;
						break;
				}
			} else {
				/* OR 1000ddd1ssmmmrrr */
#if 0
				if (CheckDataAltAddrMode(p)) {
					p->MainClass = kIKindOrDEa;
				}
#endif
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 0, rg9(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, mode(p), reg(p),
					kAddrValidAltMem, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindOrDEa;
				}
			}
		}
	}
}

LOCALPROC MayInline DeCode9(WorkR *p)
{
	if (b76(p) == 3) {
		/* SUBA 1001dddm11mmmrrr */
#if 0
		if (IsValidAddrMode(p)) {
			p->MainClass = kIKindSubA;
		}
#endif
		p->opsize = b8(p) * 2 + 2;
		SetDcoArgFields(p, falseblnr, kAMdReg, kArgkRegL, rg9(p) + 8);
			/* always long, regardless of opsize */
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		{
#if WantCycByPriOp
			if (4 == p->opsize) {
				if ((mode(p) < 2) || ((7 == mode(p)) && (reg(p) == 4)))
				{
					p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
				} else {
					p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
				}
			} else {
				p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
			}
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			p->MainClass = kIKindSubA;
		}
	} else {
		if (b8(p) == 0) {
			/* SUB 1001ddd0ssmmmrrr */
#if 0
			if (IsValidAddrMode(p)) {
				p->MainClass = kIKindSubEaR;
			}
#endif
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, mode(p), reg(p),
				kAddrValidAny, trueblnr))
			if (CheckValidAddrMode(p, 0, rg9(p),
				kAddrValidAny, falseblnr))
			{
#if WantCycByPriOp
				if (4 == p->opsize) {
					if ((mode(p) < 2)
						|| ((7 == mode(p)) && (reg(p) == 4)))
					{
						p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
					} else {
						p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
					}
				} else {
					p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindSubB + OpSizeOffset(p);
			}
		} else {
			if (mode(p) == 0) {
				/* SUBX 1001ddd1ss000rrr */
				/* p->MainClass = kIKindSubXd; */
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 0, reg(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, 0, rg9(p),
					kAddrValidAny, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (8 * kCycleScale + RdAvgXtraCyc)
						: (4 * kCycleScale + RdAvgXtraCyc);
#endif
					p->MainClass = kIKindSubXB + OpSizeOffset(p);
				}
			} else if (mode(p) == 1) {
				/* SUBX 1001ddd1ss001rrr */
				/* p->MainClass = kIKindSubXm; */
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 4, reg(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, 4, rg9(p),
					kAddrValidAny, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (30 * kCycleScale
							+ 5 * RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (18 * kCycleScale
							+ 3 * RdAvgXtraCyc + 1 * WrAvgXtraCyc);
#endif
					p->MainClass = kIKindSubXB + OpSizeOffset(p);
				}
			} else {
				/* SUB 1001ddd1ssmmmrrr */
#if 0
				if (CheckAltMemAddrMode(p)) {
					p->MainClass = kIKindSubREa;
				}
#endif
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 0, rg9(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, mode(p),
					reg(p), kAddrValidAltMem, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindSubB + OpSizeOffset(p);
				}
			}
		}
	}
}

LOCALPROC MayInline DeCodeA(WorkR *p)
{
#if WantCycByPriOp
	p->Cycles = (34 * kCycleScale
		+ 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
	p->MainClass = kIKindA;
}

LOCALPROC MayInline DeCodeB(WorkR *p)
{
	if (b76(p) == 3) {
		/* CMPA 1011ddds11mmmrrr */
#if 0
		if (IsValidAddrMode(p)) {
			p->MainClass = kIKindCmpA;
		}
#endif
		p->opsize = b8(p) * 2 + 2;
		SetDcoArgFields(p, falseblnr, kAMdReg, kArgkRegL, rg9(p) + 8);
			/* always long, regardless of opsize */
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		{
#if WantCycByPriOp
			p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			p->MainClass = kIKindCmpA;
		}
	} else if (b8(p) == 1) {
		if (mode(p) == 1) {
			/* CmpM 1011ddd1ss001rrr */
#if 0
			p->MainClass = kIKindCmpM;
#endif
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, 3, reg(p),
				kAddrValidAny, trueblnr))
			if (CheckValidAddrMode(p, 3, rg9(p),
				kAddrValidAny, falseblnr))
			{
#if WantCycByPriOp
				p->Cycles = (4 == p->opsize)
					? (20 * kCycleScale + 5 * RdAvgXtraCyc)
					: (12 * kCycleScale + 3 * RdAvgXtraCyc);
#endif
				p->MainClass = kIKindCmpB + OpSizeOffset(p);
			}
		} else {
#if 0
			/* Eor 1011ddd1ssmmmrrr */
			if (CheckDataAltAddrMode(p)) {
				p->MainClass = kIKindEor;
			}
#endif
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, 0, rg9(p),
				kAddrValidAny, trueblnr))
			if (CheckValidAddrMode(p, mode(p), reg(p),
				kAddrValidDataAlt, falseblnr))
			{
#if WantCycByPriOp
				if (0 != mode(p)) {
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
				} else {
					p->Cycles = (4 == p->opsize)
						? (8 * kCycleScale + RdAvgXtraCyc)
						: (4 * kCycleScale + RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindEor;
			}
		}
	} else {
		/* Cmp 1011ddd0ssmmmrrr */
#if 0
		if (IsValidAddrMode(p)) {
			p->MainClass = kIKindCmp;
		}
#endif
		FindOpSizeFromb76(p);
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		if (CheckValidAddrMode(p, 0, rg9(p),
			kAddrValidAny, falseblnr))
		{
#if WantCycByPriOp
			p->Cycles = (4 == p->opsize)
				? (6 * kCycleScale + RdAvgXtraCyc)
				: (4 * kCycleScale + RdAvgXtraCyc);
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			p->MainClass = kIKindCmpB + OpSizeOffset(p);
		}
	}
}

LOCALPROC MayInline DeCodeC(WorkR *p)
{
	if (b76(p) == 3) {
		p->opsize = 2;
		if (CheckDataAddrMode(p)) {
#if WantCycByPriOp
#if WantCloserCyc
			p->Cycles = (38 * kCycleScale + RdAvgXtraCyc);
#else
			p->Cycles = (54 * kCycleScale + RdAvgXtraCyc);
				/* worst case 70, best case 38 */
#endif
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			if (b8(p) == 0) {
				/* MulU 1100ddd011mmmrrr */
				p->MainClass = kIKindMulU;
			} else {
				/* MulS 1100ddd111mmmrrr */
				p->MainClass = kIKindMulS;
			}
		}
	} else {
		if (b8(p) == 0) {
			/* And 1100ddd0ssmmmrrr */
#if 0
			if (CheckDataAddrMode(p)) {
				p->MainClass = kIKindAndEaD;
			}
#endif
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, mode(p), reg(p),
				kAddrValidData, trueblnr))
			if (CheckValidAddrMode(p, 0, rg9(p),
				kAddrValidAny, falseblnr))
			{
#if WantCycByPriOp
				if (4 == p->opsize) {
					if ((mode(p) < 2)
						|| ((7 == mode(p)) && (reg(p) == 4)))
					{
						p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
					} else {
						p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
					}
				} else {
					p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindAndEaD;
			}
		} else {
			if (mode(p) < 2) {
				switch (b76(p)) {
					case 0:
						/* ABCD 1100ddd10000mrrr */
#if WantCycByPriOp
						if (0 != mode(p)) {
							p->Cycles = (18 * kCycleScale
								+ 3 * RdAvgXtraCyc + WrAvgXtraCyc);
						} else {
							p->Cycles = (6 * kCycleScale
								+ RdAvgXtraCyc);
						}
#endif
						if (mode(p) == 0) {
							p->MainClass = kIKindAbcdr;
						} else {
							p->MainClass = kIKindAbcdm;
						}
						break;
					case 1:
						/* Exg 1100ddd10100trrr */
#if WantCycByPriOp
						p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
#endif
						if (mode(p) == 0) {
							p->MainClass = kIKindExgdd;
						} else {
							p->MainClass = kIKindExgaa;
						}
						break;
					case 2:
					default: /* keep compiler happy */
						if (mode(p) == 0) {
							p->MainClass = kIKindIllegal;
						} else {
							/* Exg 1100ddd110001rrr */
#if WantCycByPriOp
							p->Cycles = (6 * kCycleScale
								+ RdAvgXtraCyc);
#endif
							p->MainClass = kIKindExgda;
						}
						break;
				}
			} else {
				/* And 1100ddd1ssmmmrrr */
#if 0
				if (CheckAltMemAddrMode(p)) {
					p->MainClass = kIKindAndDEa;
				}
#endif
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 0, rg9(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, mode(p), reg(p),
					kAddrValidAltMem, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindAndDEa;
				}
			}
		}
	}
}

LOCALPROC MayInline DeCodeD(WorkR *p)
{
	if (b76(p) == 3) {
		/* ADDA 1101dddm11mmmrrr */
		p->opsize = b8(p) * 2 + 2;
		SetDcoArgFields(p, falseblnr, kAMdReg, kArgkRegL, rg9(p) + 8);
			/* always long, regardless of opsize */
		if (CheckValidAddrMode(p, mode(p), reg(p),
			kAddrValidAny, trueblnr))
		{
#if WantCycByPriOp
			if (4 == p->opsize) {
				if ((mode(p) < 2) || ((7 == mode(p)) && (reg(p) == 4)))
				{
					p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
				} else {
					p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
				}
			} else {
				p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
			}
			p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
			p->MainClass = kIKindAddA;
		}
	} else {
		if (b8(p) == 0) {
			/* ADD 1101ddd0ssmmmrrr */
			FindOpSizeFromb76(p);
			if (CheckValidAddrMode(p, mode(p), reg(p),
				kAddrValidAny, trueblnr))
			if (CheckValidAddrMode(p, 0, rg9(p),
				kAddrValidAny, falseblnr))
			{
#if WantCycByPriOp
				if (4 == p->opsize) {
					if ((mode(p) < 2)
						|| ((7 == mode(p)) && (reg(p) == 4)))
					{
						p->Cycles = (8 * kCycleScale + RdAvgXtraCyc);
					} else {
						p->Cycles = (6 * kCycleScale + RdAvgXtraCyc);
					}
				} else {
					p->Cycles = (4 * kCycleScale + RdAvgXtraCyc);
				}
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindAddB + OpSizeOffset(p);
			}
		} else {
			if (mode(p) == 0) {
				/* ADDX 1101ddd1ss000rrr */
				/* p->MainClass = kIKindAddXd; */
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 0, reg(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, 0, rg9(p),
					kAddrValidAny, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (8 * kCycleScale + RdAvgXtraCyc)
						: (4 * kCycleScale + RdAvgXtraCyc);
#endif
					p->MainClass = kIKindAddXB + OpSizeOffset(p);
				}
			} else if (mode(p) == 1) {
				/* p->MainClass = kIKindAddXm; */
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 4, reg(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, 4, rg9(p),
					kAddrValidAny, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (30 * kCycleScale
							+ 5 * RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (18 * kCycleScale
							+ 3 * RdAvgXtraCyc + 1 * WrAvgXtraCyc);
#endif
					p->MainClass = kIKindAddXB + OpSizeOffset(p);
				}
			} else {
				/* ADD 1101ddd1ssmmmrrr */
				FindOpSizeFromb76(p);
				if (CheckValidAddrMode(p, 0, rg9(p),
					kAddrValidAny, trueblnr))
				if (CheckValidAddrMode(p, mode(p), reg(p),
					kAddrValidAltMem, falseblnr))
				{
#if WantCycByPriOp
					p->Cycles = (4 == p->opsize)
						? (12 * kCycleScale
							+ RdAvgXtraCyc + 2 * WrAvgXtraCyc)
						: (8 * kCycleScale
							+ RdAvgXtraCyc + WrAvgXtraCyc);
					p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
					p->MainClass = kIKindAddB + OpSizeOffset(p);
				}
			}
		}
	}
}

LOCALPROC MayInline DeCodeE(WorkR *p)
{
	if (b76(p) == 3) {
		if ((p->opcode & 0x0800) != 0) {
#if Use68020
			/* 11101???11mmmrrr */
			switch (mode(p)) {
				case 1:
				case 3:
				case 4:
				default: /* keep compiler happy */
					p->MainClass = kIKindIllegal;
					break;
				case 0:
				case 2:
				case 5:
				case 6:
					p->MainClass = kIKindBitField;
					break;
				case 7:
					switch (reg(p)) {
						case 0:
						case 1:
							p->MainClass = kIKindBitField;
							break;
						case 2:
						case 3:
							switch ((p->opcode >> 8) & 7) {
								case 0: /* BFTST */
								case 1: /* BFEXTU */
								case 3: /* BFEXTS */
								case 5: /* BFFFO */
									p->MainClass = kIKindBitField;
									break;
								default:
									p->MainClass = kIKindIllegal;
									break;
							}
							break;
						default:
							p->MainClass = kIKindIllegal;
							break;
					}
					break;
			}
#else
			p->MainClass = kIKindIllegal;
#endif
		} else {
			/* 11100ttd11mmmddd */
			if (CheckAltMemAddrMode(p)) {
#if WantCycByPriOp
				p->Cycles = (8 * kCycleScale
					+ RdAvgXtraCyc + WrAvgXtraCyc);
				p->Cycles += OpEACalcCyc(p, mode(p), reg(p));
#endif
				p->MainClass = kIKindRolopNM;
			}
		}
	} else {
		if (mode(p) < 4) {
			/* 1110cccdss0ttddd */
#if WantCycByPriOp
			p->Cycles = (octdat(rg9(p)) * (2 * kCycleScale))
				+ ((4 == p->opsize)
					? (8 * kCycleScale + RdAvgXtraCyc)
					: (6 * kCycleScale + RdAvgXtraCyc));
#endif
			p->MainClass = kIKindRolopND;
		} else {
			/* 1110rrrdss1ttddd */
#if WantCycByPriOp
#if WantCloserCyc
			p->Cycles = (4 == p->opsize)
				? (8 * kCycleScale + RdAvgXtraCyc)
				: (6 * kCycleScale + RdAvgXtraCyc);
#else
			p->Cycles = (4 == p->opsize)
				? ((8 * kCycleScale)
					+ RdAvgXtraCyc + (8 * (2 * kCycleScale)))
					/* say average shift count of 8 */
				: ((6 * kCycleScale)
					+ RdAvgXtraCyc + (4 * (2 * kCycleScale)));
					/* say average shift count of 4 */
#endif
#endif
			p->MainClass = kIKindRolopDD;
		}
	}
}

LOCALPROC MayInline DeCodeF(WorkR *p)
{
#if WantCycByPriOp
	p->Cycles =
		(34 * kCycleScale + 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
	p->MainClass = kIKindF;
}

LOCALPROC DeCodeOneOp(WorkR *p)
{
	switch (p->opcode >> 12) {
		case 0x0:
			DeCode0(p);
			break;
		case 0x1:
			DeCode1(p);
			break;
		case 0x2:
			DeCode2(p);
			break;
		case 0x3:
			DeCode3(p);
			break;
		case 0x4:
			DeCode4(p);
			break;
		case 0x5:
			DeCode5(p);
			break;
		case 0x6:
			DeCode6(p);
			break;
		case 0x7:
			DeCode7(p);
			break;
		case 0x8:
			DeCode8(p);
			break;
		case 0x9:
			DeCode9(p);
			break;
		case 0xA:
			DeCodeA(p);
			break;
		case 0xB:
			DeCodeB(p);
			break;
		case 0xC:
			DeCodeC(p);
			break;
		case 0xD:
			DeCodeD(p);
			break;
		case 0xE:
			DeCodeE(p);
			break;
		case 0xF:
		default: /* keep compiler happy */
			DeCodeF(p);
			break;
	}

	if (kIKindIllegal == p->MainClass) {
#if WantCycByPriOp
		p->Cycles = (34 * kCycleScale
			+ 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		SetDcoSrcAMd(&p->DecOp, 0);
		SetDcoSrcArgk(&p->DecOp, 0);
		SetDcoSrcArgDat(&p->DecOp, 0);
		SetDcoDstAMd(&p->DecOp, 0);
		SetDcoDstArgk(&p->DecOp, 0);
		SetDcoDstArgDat(&p->DecOp, 0);
	}

	SetDcoMainClas(&p->DecOp, p->MainClass);
#if WantCycByPriOp
	SetDcoCycles(&p->DecOp, p->Cycles);
#else
	SetDcoCycles(&p->DecOp, kMyAvgCycPerInstr);
#endif
}

GLOBALPROC M68KITAB_setup(DecOpR *p)
{
	ui5b i;
	WorkR r;

	for (i = 0; i < (ui5b)256 * 256; ++i) {
		r.opcode = i;
		r.MainClass = kIKindIllegal;

		r.DecOp.A = 0;
		r.DecOp.B = 0;
#if WantCycByPriOp
		r.Cycles = kMyAvgCycPerInstr;
#endif

		DeCodeOneOp(&r);

		p[i].A = r.DecOp.A;
		p[i].B = r.DecOp.B;
	}
}
