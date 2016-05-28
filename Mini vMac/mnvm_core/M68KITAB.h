/*
	M68KITAB.h

	Copyright (C) 2007, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#ifdef M68KITAB_H
#error "header already included"
#else
#define M68KITAB_H
#endif

enum {
	kIKindTst,
	kIKindCmpB,
	kIKindCmpW,
	kIKindCmpL,
	kIKindBccB,
	kIKindBccW,
	kIKindBraB,
	kIKindBraW,
	kIKindDBcc,
	kIKindDBF,
	kIKindSwap,
	kIKindMoveL,
	kIKindMoveW,
	kIKindMoveB,
	kIKindMoveAL,
	kIKindMoveAW,
	kIKindMoveQ,
	kIKindAddB,
	kIKindAddW,
	kIKindAddL,
	kIKindSubB,
	kIKindSubW,
	kIKindSubL,
	kIKindLea,
	kIKindPEA,
	kIKindA,
	kIKindBsrB,
	kIKindBsrW,
	kIKindJsr,
	kIKindLinkA6,
	kIKindMOVEMRmML,
	kIKindMOVEMApRL,
	kIKindUnlkA6,
	kIKindRts,
	kIKindJmp,
	kIKindClr,
	kIKindAddA,
	kIKindAddQA,
	kIKindSubA,
	kIKindSubQA,
	kIKindCmpA,
	kIKindAddXB,
	kIKindAddXW,
	kIKindAddXL,
	kIKindSubXB,
	kIKindSubXW,
	kIKindSubXL,
	kIKindRolopNM,
	kIKindRolopND,
	kIKindRolopDD,
	kIKindBitOpDD,
	kIKindBitOpDM,
	kIKindBitOpND,
	kIKindBitOpNM,
	kIKindAndI,
	kIKindAndEaD,
	kIKindAndDEa,
	kIKindOrI,
	kIKindOrDEa,
	kIKindOrEaD,
	kIKindEor,
	kIKindEorI,
	kIKindNot,
	kIKindScc,
	kIKindNegXB,
	kIKindNegXW,
	kIKindNegXL,
	kIKindNegB,
	kIKindNegW,
	kIKindNegL,
	kIKindEXTW,
	kIKindEXTL,
	kIKindMulU,
	kIKindMulS,
	kIKindDivU,
	kIKindDivS,
	kIKindExgdd,
	kIKindExgaa,
	kIKindExgda,
	kIKindMoveCCREa,
	kIKindMoveEaCCR,
	kIKindMoveSREa,
	kIKindMoveEaSR,
	kIKindBinOpStatusCCR,
	kIKindMOVEMApRW,
	kIKindMOVEMRmMW,
	kIKindMOVEMrm,
	kIKindMOVEMmr,
	kIKindAbcdr,
	kIKindAbcdm,
	kIKindSbcdr,
	kIKindSbcdm,
	kIKindNbcd,
	kIKindRte,
	kIKindNop,
	kIKindMoveP,
	kIKindIllegal,
	kIKindChkW,
	kIKindTrap,
	kIKindTrapV,
	kIKindRtr,
	kIKindLink,
	kIKindUnlk,
	kIKindMoveRUSP,
	kIKindMoveUSPR,
	kIKindTas,
	kIKindF,
	kIKindCallMorRtm,
	kIKindStop,
	kIKindReset,

#if Use68020
	kIKindBraL,
	kIKindBccL,
	kIKindBsrL,
	kIKindEXTBL,
	kIKindTRAPcc,
	kIKindChkL,
	kIKindBkpt,
	kIKindDivL,
	kIKindMulL,
	kIKindRtd,
	kIKindMoveC,
	kIKindLinkL,
	kIKindPack,
	kIKindUnpk,
	kIKindCHK2orCMP2,
	kIKindCAS2,
	kIKindCAS,
	kIKindMoveS,
	kIKindBitField,
#endif

	kNumIKinds
};

enum {
	kAMdReg,
	kAMdIndirect,
	kAMdAPosIncB,
	kAMdAPosIncW,
	kAMdAPosIncL,
	kAMdAPreDecB,
	kAMdAPreDecW,
	kAMdAPreDecL,
	kAMdADisp,
	kAMdAIndex,
	kAMdAbsW,
	kAMdAbsL,
	kAMdPCDisp,
	kAMdPCIndex,
	kAMdImmedB,
	kAMdImmedW,
	kAMdImmedL,
	kAMdDat4,

	kNumAMds
};

enum {
	kArgkRegB,
	kArgkRegW,
	kArgkRegL,
	kArgkMemB,
	kArgkMemW,
	kArgkMemL,
	kArgkCnst,

	kNumArgks
};

struct DecOpR {
	/* expected size : 8 bytes */
	ui5b A;
	ui5b B;
};
typedef struct DecOpR DecOpR;

#define GetUi5rField(v, shift, sz) \
	(((v) >> (shift)) & ((1UL << (sz)) - 1))
#define SetUi5rField(v, shift, sz, x) \
	(v) = (((v) & ~ (((1UL << (sz)) - 1) << (shift))) \
		| (((x) & ((1UL << (sz)) - 1)) << (shift)))

#define GetDcoFldAMd(f) (GetUi5rField((f), 16, 8))
#define GetDcoFldArgk(f) (GetUi5rField((f), 24, 4))
#define GetDcoFldArgDat(f) (GetUi5rField((f), 28, 4))

#define SetDcoFldAMd(f, x) SetUi5rField((f), 16, 8, x)
#define SetDcoFldArgk(f, x) SetUi5rField((f), 24, 4, x)
#define SetDcoFldArgDat(f, x) SetUi5rField((f), 28, 4, x)

#define GetDcoMainClas(p) (GetUi5rField((p)->A, 0, 16))
#define GetDcoDstAMd(p) (GetDcoFldAMd((p)->A))
#define GetDcoDstArgk(p) (GetDcoFldArgk((p)->A))
#define GetDcoDstArgDat(p) (GetDcoFldArgDat((p)->A))
#define GetDcoSrcAMd(p) (GetDcoFldAMd((p)->B))
#define GetDcoSrcArgk(p) (GetDcoFldArgk((p)->B))
#define GetDcoSrcArgDat(p) (GetDcoFldArgDat((p)->B))
#define GetDcoCycles(p) (GetUi5rField((p)->B, 0, 16))

#define SetDcoMainClas(p, x) SetUi5rField((p)->A, 0, 16, x)
#define SetDcoDstAMd(p, x) SetDcoFldAMd((p)->A, x)
#define SetDcoDstArgk(p, x) SetDcoFldArgk((p)->A, x)
#define SetDcoDstArgDat(p, x) SetDcoFldArgDat((p)->A, x)
#define SetDcoSrcAMd(p, x) SetDcoFldAMd((p)->B, x)
#define SetDcoSrcArgk(p, x) SetDcoFldArgk((p)->B, x)
#define SetDcoSrcArgDat(p, x) SetDcoFldArgDat((p)->B, x)
#define SetDcoCycles(p, x) SetUi5rField((p)->B, 0, 16, x)

EXPORTPROC M68KITAB_setup(DecOpR *p);
