/*
	MINEM68K.c

	Copyright (C) 2009 Bernd Schmidt, Paul C. Pratt

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
	EMulator of 68K cpu with GENeric c code (not assembly)

	This code descends from a simple 68000 emulator that I
	(Paul C. Pratt) wrote long ago. That emulator ran on a 680x0,
	and used the cpu it ran on to do some of the work. This
	descendent fills in those holes with code from the
	Un*x Amiga Emulator by Bernd Schmidt, as found being used in vMac.

	This emulator is about 10 times smaller than the UAE,
	at the cost of being 2 to 3 times slower.

	FPU Emulation added 9/12/2009 by Ross Martin
		(this code now located in "FPCPEMDV.h")
*/

#ifndef AllFiles
#include "SYSDEPNS.h"

#include "MYOSGLUE.h"
#include "ENDIANAC.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"

#include "M68KITAB.h"

#if WantDisasm
#include "DISAM68K.h"
#endif
#endif

#include "MINEM68K.h"

/*
	ReportAbnormalID unused 0x0123 - 0x01FF
*/

#ifndef DisableLazyFlagAll
#define DisableLazyFlagAll 0
#endif
	/*
		useful for debugging, to tell if an observed bug is
		being cause by lazy flag evaluation stuff.
		Can also disable parts of it individually:
	*/

#ifndef ForceFlagsEval
#if DisableLazyFlagAll
#define ForceFlagsEval 1
#else
#define ForceFlagsEval 0
#endif
#endif

#ifndef UseLazyZ
#if DisableLazyFlagAll || ForceFlagsEval
#define UseLazyZ 0
#else
#define UseLazyZ 1
#endif
#endif

#ifndef UseLazyCC
#if DisableLazyFlagAll
#define UseLazyCC 0
#else
#define UseLazyCC 1
#endif
#endif


typedef unsigned char flagtype; /* must be 0 or 1, not boolean */


/* Memory Address Translation Cache record */

struct MATCr {
	ui5r cmpmask;
	ui5r cmpvalu;
	ui5r usemask;
	ui3p usebase;
};
typedef struct MATCr MATCr;
typedef MATCr *MATCp;

#ifndef USE_PCLIMIT
#define USE_PCLIMIT 1
#endif

#define AKMemory 0
#define AKRegister 1

union ArgAddrT {
	ui5r mem;
	ui5r *rga;
};
typedef union ArgAddrT ArgAddrT;

enum {
	kLazyFlagsDefault,
	kLazyFlagsTstB,
	kLazyFlagsTstW,
	kLazyFlagsTstL,
	kLazyFlagsCmpB,
	kLazyFlagsCmpW,
	kLazyFlagsCmpL,
	kLazyFlagsSubB,
	kLazyFlagsSubW,
	kLazyFlagsSubL,
	kLazyFlagsAddB,
	kLazyFlagsAddW,
	kLazyFlagsAddL,
	kLazyFlagsNegB,
	kLazyFlagsNegW,
	kLazyFlagsNegL,
	kLazyFlagsAsrB,
	kLazyFlagsAsrW,
	kLazyFlagsAsrL,
	kLazyFlagsAslB,
	kLazyFlagsAslW,
	kLazyFlagsAslL,
#if UseLazyZ
	kLazyFlagsZSet,
#endif

	kNumLazyFlagsKinds
};

typedef void (my_reg_call *ArgSetDstP)(ui5r f);

#define FasterAlignedL 0
	/*
		If most long memory access is long aligned,
		this should be faster. But on the Mac, this
		doesn't seem to be the case, so an
		unpredictable branch slows it down.
	*/

#ifndef HaveGlbReg
#define HaveGlbReg 0
#endif

LOCALVAR struct regstruct
{
	ui5r regs[16]; /* Data and Address registers */

	ui3p pc_p;
	ui3p pc_pHi;
	si5rr MaxCyclesToGo;

#if WantCloserCyc
	DecOpR *CurDecOp;
#endif
	DecOpYR CurDecOpY;

	ui3r LazyFlagKind;
	ui3r LazyXFlagKind;
#if UseLazyZ
	ui3r LazyFlagZSavedKind;
#endif
	ui5r LazyFlagArgSrc;
	ui5r LazyFlagArgDst;
	ui5r LazyXFlagArgSrc;
	ui5r LazyXFlagArgDst;

	ArgAddrT ArgAddr;
	ArgSetDstP ArgSetDst;
	ui5b SrcVal;

	ui3p pc_pLo;
	ui5r pc; /* Program Counter */

	MATCr MATCrdB;
	MATCr MATCwrB;
	MATCr MATCrdW;
	MATCr MATCwrW;
#if FasterAlignedL
	MATCr MATCrdL;
	MATCr MATCwrL;
#endif
	ATTep HeadATTel;

	si5r MoreCyclesToGo;
	si5r ResidualCycles;
	ui3b fakeword[2];

	/* Status Register */
	ui5r intmask; /* bits 10-8 : interrupt priority mask */
	flagtype t1; /* bit 15: Trace mode 1 */
#if Use68020
	flagtype t0; /* bit 14: Trace mode 0 */
#endif
	flagtype s; /* bit 13: Supervisor or user privilege level */
#if Use68020
	flagtype m; /* bit 12: Master or interrupt mode */
#endif

	flagtype x; /* bit 4: eXtend */
	flagtype n; /* bit 3: Negative */
	flagtype z; /* bit 2: Zero */
	flagtype v; /* bit 1: oVerflow */
	flagtype c; /* bit 0: Carry */

#if EmMMU | EmFPU
	ui5b ArgKind;
#endif

	blnr TracePending;
	blnr ExternalInterruptPending;
#if 0
	blnr ResetPending;
#endif
	ui3b *fIPL;
#ifdef r_regs
	struct regstruct *save_regs;
#endif

	CPTR usp; /* User Stack Pointer */
	CPTR isp; /* Interrupt Stack Pointer */
#if Use68020
	CPTR msp; /* Master Stack Pointer */
	ui5b sfc; /* Source Function Code register */
	ui5b dfc; /* Destination Function Code register */
	ui5b vbr; /* Vector Base Register */
	ui5b cacr; /* Cache Control Register */
		/*
			bit 0 : Enable Cache
			bit 1 : Freeze Cache
			bit 2 : Clear Entry In Cache (write only)
			bit 3 : Clear Cache (write only)
		*/
	ui5b caar; /* Cache Address Register */
#endif

#define disp_table_sz (256 * 256)
#if SmallGlobals
	DecOpR *disp_table;
#else
	DecOpR disp_table[disp_table_sz];
#endif
} regs;

#define ui5r_MSBisSet(x) (((si5r)(x)) < 0)

#define Bool2Bit(x) ((x) ? 1 : 0)


#ifdef r_regs
register struct regstruct *g_regs asm (r_regs);
#define V_regs (*g_regs)
#else
#define V_regs regs
#endif

#ifdef r_pc_p
register ui3p g_pc_p asm (r_pc_p);
#define V_pc_p g_pc_p
#else
#define V_pc_p V_regs.pc_p
#endif

#ifdef r_MaxCyclesToGo
register si5rr g_MaxCyclesToGo asm (r_MaxCyclesToGo);
#define V_MaxCyclesToGo g_MaxCyclesToGo
#else
#define V_MaxCyclesToGo V_regs.MaxCyclesToGo
#endif

#ifdef r_pc_pHi
register ui3p g_pc_pHi asm (r_pc_pHi);
#define V_pc_pHi g_pc_pHi
#else
#define V_pc_pHi V_regs.pc_pHi
#endif

#define ZFLG V_regs.z
#define NFLG V_regs.n
#define CFLG V_regs.c
#define VFLG V_regs.v
#define XFLG V_regs.x

#define m68k_dreg(num) (V_regs.regs[(num)])
#define m68k_areg(num) (V_regs.regs[(num) + 8])


#ifndef WantDumpTable
#define WantDumpTable 0
#endif

#if WantDumpTable
LOCALVAR ui5b DumpTable[kNumIKinds];
#endif

#if USE_PCLIMIT
FORWARDPROC Recalc_PC_Block(void);
FORWARDFUNC ui5r my_reg_call Recalc_PC_BlockReturnUi5r(ui5r v);
#endif

LOCALINLINEFUNC ui4r nextiword(void)
/* NOT sign extended */
{
	ui4r r = do_get_mem_word(V_pc_p);
	V_pc_p += 2;

#if USE_PCLIMIT
	if (my_cond_rare(V_pc_p >= V_pc_pHi)) {
		Recalc_PC_Block();
	}
#endif

	return r;
}

LOCALINLINEFUNC ui5r nextiSByte(void)
{
	ui5r r = ui5r_FromSByte(do_get_mem_byte(V_pc_p + 1));
	V_pc_p += 2;

#if USE_PCLIMIT
	if (my_cond_rare(V_pc_p >= V_pc_pHi)) {
		return Recalc_PC_BlockReturnUi5r(r);
	}
#endif

	return r;
}

LOCALINLINEFUNC ui5r nextiSWord(void)
/* NOT sign extended */
{
	ui5r r = ui5r_FromSWord(do_get_mem_word(V_pc_p));
	V_pc_p += 2;

#if USE_PCLIMIT
	if (my_cond_rare(V_pc_p >= V_pc_pHi)) {
		return Recalc_PC_BlockReturnUi5r(r);
	}
#endif

	return r;
}

FORWARDFUNC ui5r nextilong_ext(void);

LOCALINLINEFUNC ui5r nextilong(void)
{
	ui5r r = do_get_mem_long(V_pc_p);
	V_pc_p += 4;

#if USE_PCLIMIT
	/* could be two words in different blocks */
	if (my_cond_rare(V_pc_p >= V_pc_pHi)) {
		r = nextilong_ext();
	}
#endif

	return r;
}

LOCALINLINEPROC BackupPC(void)
{
	V_pc_p -= 2;

#if USE_PCLIMIT
	if (my_cond_rare(V_pc_p < V_regs.pc_pLo)) {
		Recalc_PC_Block();
	}
#endif
}

LOCALINLINEFUNC CPTR m68k_getpc(void)
{
	return V_regs.pc + (V_pc_p - V_regs.pc_pLo);
}


FORWARDPROC DoCodeTst(void);
FORWARDPROC DoCodeCmpB(void);
FORWARDPROC DoCodeCmpW(void);
FORWARDPROC DoCodeCmpL(void);
FORWARDPROC DoCodeBccB(void);
FORWARDPROC DoCodeBccW(void);
FORWARDPROC DoCodeBraB(void);
FORWARDPROC DoCodeBraW(void);
FORWARDPROC DoCodeDBcc(void);
FORWARDPROC DoCodeDBF(void);
FORWARDPROC DoCodeSwap(void);
FORWARDPROC DoCodeMoveL(void);
FORWARDPROC DoCodeMoveW(void);
FORWARDPROC DoCodeMoveB(void);
FORWARDPROC DoCodeMoveA(void);
FORWARDPROC DoCodeMoveQ(void);
FORWARDPROC DoCodeAddB(void);
FORWARDPROC DoCodeAddW(void);
FORWARDPROC DoCodeAddL(void);
FORWARDPROC DoCodeSubB(void);
FORWARDPROC DoCodeSubW(void);
FORWARDPROC DoCodeSubL(void);
FORWARDPROC DoCodeLea(void);
FORWARDPROC DoCodePEA(void);
FORWARDPROC DoCodeA(void);
FORWARDPROC DoCodeBsrB(void);
FORWARDPROC DoCodeBsrW(void);
FORWARDPROC DoCodeJsr(void);
FORWARDPROC DoCodeLinkA6(void);
FORWARDPROC DoCodeMOVEMRmML(void);
FORWARDPROC DoCodeMOVEMApRL(void);
FORWARDPROC DoCodeUnlkA6(void);
FORWARDPROC DoCodeRts(void);
FORWARDPROC DoCodeJmp(void);
FORWARDPROC DoCodeClr(void);
FORWARDPROC DoCodeAddA(void);
FORWARDPROC DoCodeSubA(void);
FORWARDPROC DoCodeCmpA(void);
FORWARDPROC DoCodeAddXB(void);
FORWARDPROC DoCodeAddXW(void);
FORWARDPROC DoCodeAddXL(void);
FORWARDPROC DoCodeSubXB(void);
FORWARDPROC DoCodeSubXW(void);
FORWARDPROC DoCodeSubXL(void);
FORWARDPROC DoCodeAslB(void);
FORWARDPROC DoCodeAslW(void);
FORWARDPROC DoCodeAslL(void);
FORWARDPROC DoCodeAsrB(void);
FORWARDPROC DoCodeAsrW(void);
FORWARDPROC DoCodeAsrL(void);
FORWARDPROC DoCodeLslB(void);
FORWARDPROC DoCodeLslW(void);
FORWARDPROC DoCodeLslL(void);
FORWARDPROC DoCodeLsrB(void);
FORWARDPROC DoCodeLsrW(void);
FORWARDPROC DoCodeLsrL(void);
FORWARDPROC DoCodeRxlB(void);
FORWARDPROC DoCodeRxlW(void);
FORWARDPROC DoCodeRxlL(void);
FORWARDPROC DoCodeRxrB(void);
FORWARDPROC DoCodeRxrW(void);
FORWARDPROC DoCodeRxrL(void);
FORWARDPROC DoCodeRolB(void);
FORWARDPROC DoCodeRolW(void);
FORWARDPROC DoCodeRolL(void);
FORWARDPROC DoCodeRorB(void);
FORWARDPROC DoCodeRorW(void);
FORWARDPROC DoCodeRorL(void);
FORWARDPROC DoCodeBTstB(void);
FORWARDPROC DoCodeBChgB(void);
FORWARDPROC DoCodeBClrB(void);
FORWARDPROC DoCodeBSetB(void);
FORWARDPROC DoCodeBTstL(void);
FORWARDPROC DoCodeBChgL(void);
FORWARDPROC DoCodeBClrL(void);
FORWARDPROC DoCodeBSetL(void);
FORWARDPROC DoCodeAnd(void);
FORWARDPROC DoCodeOr(void);
FORWARDPROC DoCodeEor(void);
FORWARDPROC DoCodeNot(void);
FORWARDPROC DoCodeScc(void);
FORWARDPROC DoCodeNegXB(void);
FORWARDPROC DoCodeNegXW(void);
FORWARDPROC DoCodeNegXL(void);
FORWARDPROC DoCodeNegB(void);
FORWARDPROC DoCodeNegW(void);
FORWARDPROC DoCodeNegL(void);
FORWARDPROC DoCodeEXTW(void);
FORWARDPROC DoCodeEXTL(void);
FORWARDPROC DoCodeMulU(void);
FORWARDPROC DoCodeMulS(void);
FORWARDPROC DoCodeDivU(void);
FORWARDPROC DoCodeDivS(void);
FORWARDPROC DoCodeExg(void);
FORWARDPROC DoCodeMoveEaCR(void);
FORWARDPROC DoCodeMoveSREa(void);
FORWARDPROC DoCodeMoveEaSR(void);
FORWARDPROC DoCodeOrISR(void);
FORWARDPROC DoCodeAndISR(void);
FORWARDPROC DoCodeEorISR(void);
FORWARDPROC DoCodeOrICCR(void);
FORWARDPROC DoCodeAndICCR(void);
FORWARDPROC DoCodeEorICCR(void);
FORWARDPROC DoCodeMOVEMApRW(void);
FORWARDPROC DoCodeMOVEMRmMW(void);
FORWARDPROC DoCodeMOVEMrmW(void);
FORWARDPROC DoCodeMOVEMrmL(void);
FORWARDPROC DoCodeMOVEMmrW(void);
FORWARDPROC DoCodeMOVEMmrL(void);
FORWARDPROC DoCodeAbcd(void);
FORWARDPROC DoCodeSbcd(void);
FORWARDPROC DoCodeNbcd(void);
FORWARDPROC DoCodeRte(void);
FORWARDPROC DoCodeNop(void);
FORWARDPROC DoCodeMoveP0(void);
FORWARDPROC DoCodeMoveP1(void);
FORWARDPROC DoCodeMoveP2(void);
FORWARDPROC DoCodeMoveP3(void);
FORWARDPROC op_illg(void);
FORWARDPROC DoCodeChk(void);
FORWARDPROC DoCodeTrap(void);
FORWARDPROC DoCodeTrapV(void);
FORWARDPROC DoCodeRtr(void);
FORWARDPROC DoCodeLink(void);
FORWARDPROC DoCodeUnlk(void);
FORWARDPROC DoCodeMoveRUSP(void);
FORWARDPROC DoCodeMoveUSPR(void);
FORWARDPROC DoCodeTas(void);
FORWARDPROC DoCodeFdefault(void);
FORWARDPROC DoCodeStop(void);
FORWARDPROC DoCodeReset(void);

#if Use68020
FORWARDPROC DoCodeCallMorRtm(void);
FORWARDPROC DoCodeBraL(void);
FORWARDPROC DoCodeBccL(void);
FORWARDPROC DoCodeBsrL(void);
FORWARDPROC DoCodeEXTBL(void);
FORWARDPROC DoCodeTRAPcc(void);
FORWARDPROC DoCodeBkpt(void);
FORWARDPROC DoCodeDivL(void);
FORWARDPROC DoCodeMulL(void);
FORWARDPROC DoCodeRtd(void);
FORWARDPROC DoCodeMoveCCREa(void);
FORWARDPROC DoMoveFromControl(void);
FORWARDPROC DoMoveToControl(void);
FORWARDPROC DoCodeLinkL(void);
FORWARDPROC DoCodePack(void);
FORWARDPROC DoCodeUnpk(void);
FORWARDPROC DoCHK2orCMP2(void);
FORWARDPROC DoCAS2(void);
FORWARDPROC DoCAS(void);
FORWARDPROC DoMOVES(void);
FORWARDPROC DoBitField(void);
#endif

#if EmMMU
FORWARDPROC DoCodeMMU(void);
#endif

#if EmFPU
FORWARDPROC DoCodeFPU_md60(void);
FORWARDPROC DoCodeFPU_DBcc(void);
FORWARDPROC DoCodeFPU_Trapcc(void);
FORWARDPROC DoCodeFPU_Scc(void);
FORWARDPROC DoCodeFPU_FBccW(void);
FORWARDPROC DoCodeFPU_FBccL(void);
FORWARDPROC DoCodeFPU_Save(void);
FORWARDPROC DoCodeFPU_Restore(void);
FORWARDPROC DoCodeFPU_dflt(void);
#endif

typedef void (*func_pointer_t)(void);

LOCALVAR const func_pointer_t OpDispatch[kNumIKinds + 1] = {
	DoCodeTst /* kIKindTst */,
	DoCodeCmpB /* kIKindCmpB */,
	DoCodeCmpW /* kIKindCmpW */,
	DoCodeCmpL /* kIKindCmpL */,
	DoCodeBccB /* kIKindBccB */,
	DoCodeBccW /* kIKindBccW */,
	DoCodeBraB /* kIKindBraB */,
	DoCodeBraW /* kIKindBraW */,
	DoCodeDBcc /* kIKindDBcc */,
	DoCodeDBF /* kIKindDBF */,
	DoCodeSwap /* kIKindSwap */,
	DoCodeMoveL /* kIKindMoveL */,
	DoCodeMoveW /* kIKindMoveW */,
	DoCodeMoveB /* kIKindMoveB */,
	DoCodeMoveA /* kIKindMoveAL */,
	DoCodeMoveA /* kIKindMoveAW */,
	DoCodeMoveQ /* kIKindMoveQ */,
	DoCodeAddB /* kIKindAddB */,
	DoCodeAddW /* kIKindAddW */,
	DoCodeAddL /* kIKindAddL */,
	DoCodeSubB /* kIKindSubB */,
	DoCodeSubW /* kIKindSubW */,
	DoCodeSubL /* kIKindSubL */,
	DoCodeLea /* kIKindLea */,
	DoCodePEA /* kIKindPEA */,
	DoCodeA /* kIKindA */,
	DoCodeBsrB /* kIKindBsrB */,
	DoCodeBsrW /* kIKindBsrW */,
	DoCodeJsr /* kIKindJsr */,
	DoCodeLinkA6 /* kIKindLinkA6 */,
	DoCodeMOVEMRmML /* kIKindMOVEMRmML */,
	DoCodeMOVEMApRL /* kIKindMOVEMApRL */,
	DoCodeUnlkA6 /* kIKindUnlkA6 */,
	DoCodeRts /* kIKindRts */,
	DoCodeJmp /* kIKindJmp */,
	DoCodeClr /* kIKindClr */,
	DoCodeAddA /* kIKindAddA */,
	DoCodeAddA /* kIKindAddQA */,
	DoCodeSubA /* kIKindSubA */,
	DoCodeSubA /* kIKindSubQA */,
	DoCodeCmpA /* kIKindCmpA */,
	DoCodeAddXB /* kIKindAddXB */,
	DoCodeAddXW /* kIKindAddXW */,
	DoCodeAddXL /* kIKindAddXL */,
	DoCodeSubXB /* kIKindSubXB */,
	DoCodeSubXW /* kIKindSubXW */,
	DoCodeSubXL /* kIKindSubXL */,
	DoCodeAslB /* kIKindAslB */,
	DoCodeAslW /* kIKindAslW */,
	DoCodeAslL /* kIKindAslL */,
	DoCodeAsrB /* kIKindAsrB */,
	DoCodeAsrW /* kIKindAsrW */,
	DoCodeAsrL /* kIKindAsrL */,
	DoCodeLslB /* kIKindLslB */,
	DoCodeLslW /* kIKindLslW */,
	DoCodeLslL /* kIKindLslL */,
	DoCodeLsrB /* kIKindLsrB */,
	DoCodeLsrW /* kIKindLsrW */,
	DoCodeLsrL /* kIKindLsrL */,
	DoCodeRxlB /* kIKindRxlB */,
	DoCodeRxlW /* kIKindRxlW */,
	DoCodeRxlL /* kIKindRxlL */,
	DoCodeRxrB /* kIKindRxrB */,
	DoCodeRxrW /* kIKindRxrW */,
	DoCodeRxrL /* kIKindRxrL */,
	DoCodeRolB /* kIKindRolB */,
	DoCodeRolW /* kIKindRolW */,
	DoCodeRolL /* kIKindRolL */,
	DoCodeRorB /* kIKindRorB */,
	DoCodeRorW /* kIKindRorW */,
	DoCodeRorL /* kIKindRorL */,
	DoCodeBTstB /* kIKindBTstB */,
	DoCodeBChgB /* kIKindBChgB */,
	DoCodeBClrB /* kIKindBClrB */,
	DoCodeBSetB /* kIKindBSetB */,
	DoCodeBTstL /* kIKindBTstL */,
	DoCodeBChgL /* kIKindBChgL */,
	DoCodeBClrL /* kIKindBClrL */,
	DoCodeBSetL /* kIKindBSetL */,
	DoCodeAnd /* kIKindAndI */,
	DoCodeAnd /* kIKindAndEaD */,
	DoCodeAnd /* kIKindAndDEa */,
	DoCodeOr /* kIKindOrI */,
	DoCodeOr /* kIKindOrDEa */,
	DoCodeOr /* kIKindOrEaD */,
	DoCodeEor /* kIKindEor */,
	DoCodeEor /* kIKindEorI */,
	DoCodeNot /* kIKindNot */,
	DoCodeScc /* kIKindScc */,
	DoCodeNegXB /* kIKindNegXB */,
	DoCodeNegXW /* kIKindNegXW */,
	DoCodeNegXL /* kIKindNegXL */,
	DoCodeNegB /* kIKindNegB */,
	DoCodeNegW /* kIKindNegW */,
	DoCodeNegL /* kIKindNegL */,
	DoCodeEXTW /* kIKindEXTW */,
	DoCodeEXTL /* kIKindEXTL */,
	DoCodeMulU /* kIKindMulU */,
	DoCodeMulS /* kIKindMulS */,
	DoCodeDivU /* kIKindDivU */,
	DoCodeDivS /* kIKindDivS */,
	DoCodeExg /* kIKindExg */,
	DoCodeMoveEaCR /* kIKindMoveEaCCR */,
	DoCodeMoveSREa /* kIKindMoveSREa */,
	DoCodeMoveEaSR /* kIKindMoveEaSR */,
	DoCodeOrISR /* kIKindOrISR */,
	DoCodeAndISR /* kIKindAndISR */,
	DoCodeEorISR /* kIKindEorISR */,
	DoCodeOrICCR /* kIKindOrICCR */,
	DoCodeAndICCR /* kIKindAndICCR */,
	DoCodeEorICCR /* kIKindEorICCR */,
	DoCodeMOVEMApRW /* kIKindMOVEMApRW */,
	DoCodeMOVEMRmMW /* kIKindMOVEMRmMW */,
	DoCodeMOVEMrmW /* kIKindMOVEMrmW */,
	DoCodeMOVEMrmL /* kIKindMOVEMrmL */,
	DoCodeMOVEMmrW /* kIKindMOVEMmrW */,
	DoCodeMOVEMmrL /* kIKindMOVEMmrL */,
	DoCodeAbcd /* kIKindAbcd */,
	DoCodeSbcd /* kIKindSbcd */,
	DoCodeNbcd /* kIKindNbcd */,
	DoCodeRte /* kIKindRte */,
	DoCodeNop /* kIKindNop */,
	DoCodeMoveP0 /* kIKindMoveP0 */,
	DoCodeMoveP1 /* kIKindMoveP1 */,
	DoCodeMoveP2 /* kIKindMoveP2 */,
	DoCodeMoveP3 /* kIKindMoveP3 */,
	op_illg /* kIKindIllegal */,
	DoCodeChk /* kIKindChkW */,
	DoCodeTrap /* kIKindTrap */,
	DoCodeTrapV /* kIKindTrapV */,
	DoCodeRtr /* kIKindRtr */,
	DoCodeLink /* kIKindLink */,
	DoCodeUnlk /* kIKindUnlk */,
	DoCodeMoveRUSP /* kIKindMoveRUSP */,
	DoCodeMoveUSPR /* kIKindMoveUSPR */,
	DoCodeTas /* kIKindTas */,
	DoCodeFdefault /* kIKindFdflt */,
	DoCodeStop /* kIKindStop */,
	DoCodeReset /* kIKindReset */,

#if Use68020
	DoCodeCallMorRtm /* kIKindCallMorRtm */,
	DoCodeBraL /* kIKindBraL */,
	DoCodeBccL /* kIKindBccL */,
	DoCodeBsrL /* kIKindBsrL */,
	DoCodeEXTBL /* kIKindEXTBL */,
	DoCodeTRAPcc /* kIKindTRAPcc */,
	DoCodeChk /* kIKindChkL */,
	DoCodeBkpt /* kIKindBkpt */,
	DoCodeDivL /* kIKindDivL */,
	DoCodeMulL /* kIKindMulL */,
	DoCodeRtd /* kIKindRtd */,
	DoCodeMoveCCREa /* kIKindMoveCCREa */,
	DoMoveFromControl /* kIKindMoveCEa */,
	DoMoveToControl /* kIKindMoveEaC */,
	DoCodeLinkL /* kIKindLinkL */,
	DoCodePack /* kIKindPack */,
	DoCodeUnpk /* kIKindUnpk */,
	DoCHK2orCMP2 /* kIKindCHK2orCMP2 */,
	DoCAS2 /* kIKindCAS2 */,
	DoCAS /* kIKindCAS */,
	DoMOVES /* kIKindMoveS */,
	DoBitField /* kIKindBitField */,
#endif
#if EmMMU
	DoCodeMMU /* kIKindMMU */,
#endif
#if EmFPU
	DoCodeFPU_md60 /* kIKindFPUmd60 */,
	DoCodeFPU_DBcc /* kIKindFPUDBcc */,
	DoCodeFPU_Trapcc /* kIKindFPUTrapcc */,
	DoCodeFPU_Scc /* kIKindFPUScc */,
	DoCodeFPU_FBccW /* kIKindFPUFBccW */,
	DoCodeFPU_FBccL /* kIKindFPUFBccL */,
	DoCodeFPU_Save /* kIKindFPUSave */,
	DoCodeFPU_Restore /* kIKindFPURestore */,
	DoCodeFPU_dflt /* kIKindFPUdflt */,
#endif

	0
};

#ifndef WantBreakPoint
#define WantBreakPoint 0
#endif

#if WantBreakPoint

#define BreakPointAddress 0xD198

LOCALPROC BreakPointAction(void)
{
	dbglog_StartLine();
	dbglog_writeCStr("breakpoint A0=");
	dbglog_writeHex(m68k_areg(0));
	dbglog_writeCStr(" A1=");
	dbglog_writeHex(m68k_areg(1));
	dbglog_writeReturn();
}

#endif

LOCALINLINEPROC DecodeNextInstruction(func_pointer_t *d, ui4rr *Cycles,
	DecOpYR *y)
{
	ui5r opcode;
	DecOpR *p;
	ui4rr MainClas;

	opcode = nextiword();

	p = &V_regs.disp_table[opcode];

#if WantCloserCyc
	V_regs.CurDecOp = p;
#endif
	MainClas = p->x.MainClas;
	*Cycles = p->x.Cycles;
	*y = p->y;
#if WantDumpTable
	DumpTable[MainClas] ++;
#endif
	*d = OpDispatch[MainClas];
}

LOCALINLINEPROC UnDecodeNextInstruction(ui4rr Cycles)
{
	V_MaxCyclesToGo += Cycles;

	BackupPC();

#if WantDumpTable
	{
		ui5r opcode = do_get_mem_word(V_pc_p);
		DecOpR *p = &V_regs.disp_table[opcode];
		ui4rr MainClas = p->x.MainClas;

		DumpTable[MainClas] --;
	}
#endif
}

LOCALPROC m68k_go_MaxCycles(void)
{
	ui4rr Cycles;
	DecOpYR y;
	func_pointer_t d;

	/*
		Main loop of emulator.

		Always execute at least one instruction,
		even if V_regs.MaxInstructionsToGo == 0.
		Needed for trace flag to work.
	*/

	DecodeNextInstruction(&d, &Cycles, &y);

	V_MaxCyclesToGo -= Cycles;

	do {
		V_regs.CurDecOpY = y;

#if WantDisasm || WantBreakPoint
		{
			CPTR pc = m68k_getpc() - 2;
#if WantDisasm
			DisasmOneOrSave(pc);
#endif
#if WantBreakPoint
			if (BreakPointAddress == pc) {
				BreakPointAction();
			}
#endif
		}
#endif

		d();

		DecodeNextInstruction(&d, &Cycles, &y);

	} while (((si5rr)(V_MaxCyclesToGo -= Cycles)) > 0);

	/* abort instruction that have started to decode */

	UnDecodeNextInstruction(Cycles);
}

FORWARDFUNC ui5r my_reg_call get_byte_ext(CPTR addr);

LOCALFUNC ui5r my_reg_call get_byte(CPTR addr)
{
	ui3p m = (addr & V_regs.MATCrdB.usemask) + V_regs.MATCrdB.usebase;

	if ((addr & V_regs.MATCrdB.cmpmask) == V_regs.MATCrdB.cmpvalu) {
		return ui5r_FromSByte(*m);
	} else {
		return get_byte_ext(addr);
	}
}

FORWARDPROC my_reg_call put_byte_ext(CPTR addr, ui5r b);

LOCALPROC my_reg_call put_byte(CPTR addr, ui5r b)
{
	ui3p m = (addr & V_regs.MATCwrB.usemask) + V_regs.MATCwrB.usebase;
	if ((addr & V_regs.MATCwrB.cmpmask) == V_regs.MATCwrB.cmpvalu) {
		*m = b;
	} else {
		put_byte_ext(addr, b);
	}
}

FORWARDFUNC ui5r my_reg_call get_word_ext(CPTR addr);

LOCALFUNC ui5r my_reg_call get_word(CPTR addr)
{
	ui3p m = (addr & V_regs.MATCrdW.usemask) + V_regs.MATCrdW.usebase;
	if ((addr & V_regs.MATCrdW.cmpmask) == V_regs.MATCrdW.cmpvalu) {
		return ui5r_FromSWord(do_get_mem_word(m));
	} else {
		return get_word_ext(addr);
	}
}

FORWARDPROC my_reg_call put_word_ext(CPTR addr, ui5r w);

LOCALPROC my_reg_call put_word(CPTR addr, ui5r w)
{
	ui3p m = (addr & V_regs.MATCwrW.usemask) + V_regs.MATCwrW.usebase;
	if ((addr & V_regs.MATCwrW.cmpmask) == V_regs.MATCwrW.cmpvalu) {
		do_put_mem_word(m, w);
	} else {
		put_word_ext(addr, w);
	}
}

FORWARDFUNC ui5r my_reg_call get_long_misaligned_ext(CPTR addr);

LOCALFUNC ui5r my_reg_call get_long_misaligned(CPTR addr)
{
	CPTR addr2 = addr + 2;
	ui3p m = (addr & V_regs.MATCrdW.usemask) + V_regs.MATCrdW.usebase;
	ui3p m2 = (addr2 & V_regs.MATCrdW.usemask) + V_regs.MATCrdW.usebase;
	if (((addr & V_regs.MATCrdW.cmpmask) == V_regs.MATCrdW.cmpvalu)
		&& ((addr2 & V_regs.MATCrdW.cmpmask) == V_regs.MATCrdW.cmpvalu))
	{
		ui5r hi = do_get_mem_word(m);
		ui5r lo = do_get_mem_word(m2);
		ui5r Data = ((hi << 16) & 0xFFFF0000)
			| (lo & 0x0000FFFF);

		return ui5r_FromSLong(Data);
	} else {
		return get_long_misaligned_ext(addr);
	}
}

#if FasterAlignedL
FORWARDFUNC ui5r my_reg_call get_long_ext(CPTR addr);
#endif

#if FasterAlignedL
LOCALFUNC ui5r my_reg_call get_long(CPTR addr)
{
	if (0 == (addr & 0x03)) {
		ui3p m = (addr & V_regs.MATCrdL.usemask)
			+ V_regs.MATCrdL.usebase;
		if ((addr & V_regs.MATCrdL.cmpmask) == V_regs.MATCrdL.cmpvalu) {
			return ui5r_FromSLong(do_get_mem_long(m));
		} else {
			return get_long_ext(addr);
		}
	} else {
		return get_long_misaligned(addr);
	}
}
#else
#define get_long get_long_misaligned
#endif

FORWARDPROC my_reg_call put_long_misaligned_ext(CPTR addr, ui5r l);

LOCALPROC my_reg_call put_long_misaligned(CPTR addr, ui5r l)
{
	CPTR addr2 = addr + 2;
	ui3p m = (addr & V_regs.MATCwrW.usemask) + V_regs.MATCwrW.usebase;
	ui3p m2 = (addr2 & V_regs.MATCwrW.usemask) + V_regs.MATCwrW.usebase;
	if (((addr & V_regs.MATCwrW.cmpmask) == V_regs.MATCwrW.cmpvalu)
		&& ((addr2 & V_regs.MATCwrW.cmpmask) == V_regs.MATCwrW.cmpvalu))
	{
		do_put_mem_word(m, l >> 16);
		do_put_mem_word(m2, l);
	} else {
		put_long_misaligned_ext(addr, l);
	}
}

#if FasterAlignedL
FORWARDPROC my_reg_call put_long_ext(CPTR addr, ui5r l);
#endif

#if FasterAlignedL
LOCALPROC my_reg_call put_long(CPTR addr, ui5r l)
{
	if (0 == (addr & 0x03)) {
		ui3p m = (addr & V_regs.MATCwrL.usemask)
			+ V_regs.MATCwrL.usebase;
		if ((addr & V_regs.MATCwrL.cmpmask) == V_regs.MATCwrL.cmpvalu) {
			do_put_mem_long(m, l);
		} else {
			put_long_ext(addr, l);
		}
	} else {
		put_long_misaligned(addr, l);
	}
}
#else
#define put_long put_long_misaligned
#endif

LOCALFUNC ui5b my_reg_call get_disp_ea(ui5b base)
{
	ui4b dp = nextiword();
	int regno = (dp >> 12) & 0x0F;
	si5b regd = V_regs.regs[regno];
	if ((dp & 0x0800) == 0) {
		regd = (si5b)(si4b)regd;
	}
#if Use68020
	regd <<= (dp >> 9) & 3;
#if ExtraAbnormalReports
	if (((dp >> 9) & 3) != 0) {
		/* ReportAbnormal("Have scale in Extension Word"); */
		/* apparently can happen in Sys 7.5.5 boot on 68000 */
	}
#endif
	if (dp & 0x0100) {
		if ((dp & 0x80) != 0) {
			base = 0;
			/* ReportAbnormal("Extension Word: suppress base"); */
			/* used by Sys 7.5.5 boot */
		}
		if ((dp & 0x40) != 0) {
			regd = 0;
			/* ReportAbnormal("Extension Word: suppress regd"); */
			/* used by Mac II boot */
		}

		switch ((dp >> 4) & 0x03) {
			case 0:
				/* reserved */
				ReportAbnormalID(0x0101, "Extension Word: dp reserved");
				break;
			case 1:
				/* no displacement */
				/* ReportAbnormal("Extension Word: no displacement"); */
				/* used by Sys 7.5.5 boot */
				break;
			case 2:
				base += nextiSWord();
				/*
					ReportAbnormal("Extension Word: word displacement");
				*/
				/* used by Sys 7.5.5 boot */
				break;
			case 3:
				base += nextilong();
				/*
					ReportAbnormal("Extension Word: long displacement");
				*/
				/* used by Mac II boot from system 6.0.8? */
				break;
		}

		if ((dp & 0x03) == 0) {
			base += regd;
			if ((dp & 0x04) != 0) {
				ReportAbnormalID(0x0102,
					"Extension Word: reserved dp form");
			}
			/* ReportAbnormal("Extension Word: noindex"); */
			/* used by Sys 7.5.5 boot */
		} else {
			if ((dp & 0x04) != 0) {
				base = get_long(base);
				base += regd;
				/* ReportAbnormal("Extension Word: postindex"); */
				/* used by Sys 7.5.5 boot */
			} else {
				base += regd;
				base = get_long(base);
				/* ReportAbnormal("Extension Word: preindex"); */
				/* used by Sys 7.5.5 boot */
			}
			switch (dp & 0x03) {
				case 1:
					/* null outer displacement */
					/*
						ReportAbnormal(
							"Extension Word: null outer displacement");
					*/
					/* used by Sys 7.5.5 boot */
					break;
				case 2:
					base += nextiSWord();
					/*
						ReportAbnormal(
							"Extension Word: word outer displacement");
					*/
					/* used by Mac II boot from system 6.0.8? */
					break;
				case 3:
					base += nextilong();
					/*
						ReportAbnormal(
							"Extension Word: long outer displacement");
					*/
					/* used by Mac II boot from system 6.0.8? */
					break;
			}
		}

		return base;
	} else
#endif
	{
		return base + (si3b)(dp) + regd;
	}
}

LOCALFUNC ui5r my_reg_call DecodeAddr_Indirect(ui3rr ArgDat)
{
	return V_regs.regs[ArgDat];
}

LOCALFUNC ui5r my_reg_call DecodeAddr_APosIncB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 1;

	return a;
}

LOCALFUNC ui5r my_reg_call DecodeAddr_APosIncW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	return a;
}

LOCALFUNC ui5r my_reg_call DecodeAddr_APosIncL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 4;

	return a;
}

LOCALFUNC ui5r my_reg_call DecodeAddr_APreDecB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 1;

	*p = a;

	return a;
}

LOCALFUNC ui5r my_reg_call DecodeAddr_APreDecW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	return a;
}

LOCALFUNC ui5r my_reg_call DecodeAddr_APreDecL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 4;

	*p = a;

	return a;
}

LOCALFUNC ui5r my_reg_call DecodeAddr_ADisp(ui3rr ArgDat)
{
	return V_regs.regs[ArgDat] + nextiSWord();
}

LOCALFUNC ui5r my_reg_call DecodeAddr_AIndex(ui3rr ArgDat)
{
	return get_disp_ea(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeAddr_AbsW(ui3rr ArgDat)
{
	UnusedParam(ArgDat);
	return nextiSWord();
}

LOCALFUNC ui5r my_reg_call DecodeAddr_AbsL(ui3rr ArgDat)
{
	UnusedParam(ArgDat);
	return nextilong();
}

LOCALFUNC ui5r my_reg_call DecodeAddr_PCDisp(ui3rr ArgDat)
{
	CPTR pc = m68k_getpc();

	UnusedParam(ArgDat);
	return pc + nextiSWord();
}

LOCALFUNC ui5r my_reg_call DecodeAddr_PCIndex(ui3rr ArgDat)
{
	UnusedParam(ArgDat);
	return get_disp_ea(m68k_getpc());
}

typedef ui5r (my_reg_call *DecodeAddrP)(ui3rr ArgDat);

LOCALVAR const DecodeAddrP DecodeAddrDispatch[kNumAMds] = {
	(DecodeAddrP)nullpr /* kAMdRegB */,
	(DecodeAddrP)nullpr /* kAMdRegW */,
	(DecodeAddrP)nullpr /* kAMdRegL */,
	DecodeAddr_Indirect /* kAMdIndirectB */,
	DecodeAddr_Indirect /* kAMdIndirectW */,
	DecodeAddr_Indirect /* kAMdIndirectL */,
	DecodeAddr_APosIncB /* kAMdAPosIncB */,
	DecodeAddr_APosIncW /* kAMdAPosIncW */,
	DecodeAddr_APosIncL /* kAMdAPosIncL */,
	DecodeAddr_APosIncW /* kAMdAPosInc7B */,
	DecodeAddr_APreDecB /* kAMdAPreDecB */,
	DecodeAddr_APreDecW /* kAMdAPreDecW */,
	DecodeAddr_APreDecL /* kAMdAPreDecL */,
	DecodeAddr_APreDecW /* kAMdAPreDec7B */,
	DecodeAddr_ADisp /* kAMdADispB */,
	DecodeAddr_ADisp /* kAMdADispW */,
	DecodeAddr_ADisp /* kAMdADispL */,
	DecodeAddr_AIndex /* kAMdAIndexB */,
	DecodeAddr_AIndex /* kAMdAIndexW */,
	DecodeAddr_AIndex /* kAMdAIndexL */,
	DecodeAddr_AbsW /* kAMdAbsWB */,
	DecodeAddr_AbsW /* kAMdAbsWW */,
	DecodeAddr_AbsW /* kAMdAbsWL */,
	DecodeAddr_AbsL /* kAMdAbsLB */,
	DecodeAddr_AbsL /* kAMdAbsLW */,
	DecodeAddr_AbsL /* kAMdAbsLL */,
	DecodeAddr_PCDisp /* kAMdPCDispB */,
	DecodeAddr_PCDisp /* kAMdPCDispW */,
	DecodeAddr_PCDisp /* kAMdPCDispL */,
	DecodeAddr_PCIndex /* kAMdPCIndexB */,
	DecodeAddr_PCIndex /* kAMdPCIndexW */,
	DecodeAddr_PCIndex /* kAMdPCIndexL */,
	(DecodeAddrP)nullpr /* kAMdImmedB */,
	(DecodeAddrP)nullpr /* kAMdImmedW */,
	(DecodeAddrP)nullpr /* kAMdImmedL */,
	(DecodeAddrP)nullpr /* kAMdDat4 */
};

LOCALINLINEFUNC ui5r DecodeAddrSrcDst(DecArgR *f)
{
	return (DecodeAddrDispatch[f->AMd])(f->ArgDat);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_RegB(ui3rr ArgDat)
{
	return ui5r_FromSByte(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_RegW(ui3rr ArgDat)
{
	return ui5r_FromSWord(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_RegL(ui3rr ArgDat)
{
	return ui5r_FromSLong(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_IndirectB(ui3rr ArgDat)
{
	return get_byte(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_IndirectW(ui3rr ArgDat)
{
	return get_word(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_IndirectL(ui3rr ArgDat)
{
	return get_long(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APosIncB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 1;

	return get_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APosIncW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	return get_word(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APosIncL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 4;

	return get_long(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APosInc7B(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	return get_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APreDecB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 1;

	*p = a;

	return get_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APreDecW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	return get_word(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APreDecL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 4;

	*p = a;

	return get_long(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_APreDec7B(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	return get_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_ADispB(ui3rr ArgDat)
{
	return get_byte(DecodeAddr_ADisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_ADispW(ui3rr ArgDat)
{
	return get_word(DecodeAddr_ADisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_ADispL(ui3rr ArgDat)
{
	return get_long(DecodeAddr_ADisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AIndexB(ui3rr ArgDat)
{
	return get_byte(get_disp_ea(V_regs.regs[ArgDat]));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AIndexW(ui3rr ArgDat)
{
	return get_word(get_disp_ea(V_regs.regs[ArgDat]));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AIndexL(ui3rr ArgDat)
{
	return get_long(get_disp_ea(V_regs.regs[ArgDat]));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AbsWB(ui3rr ArgDat)
{
	return get_byte(DecodeAddr_AbsW(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AbsWW(ui3rr ArgDat)
{
	return get_word(DecodeAddr_AbsW(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AbsWL(ui3rr ArgDat)
{
	return get_long(DecodeAddr_AbsW(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AbsLB(ui3rr ArgDat)
{
	return get_byte(DecodeAddr_AbsL(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AbsLW(ui3rr ArgDat)
{
	return get_word(DecodeAddr_AbsL(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_AbsLL(ui3rr ArgDat)
{
	return get_long(DecodeAddr_AbsL(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_PCDispB(ui3rr ArgDat)
{
	return get_byte(DecodeAddr_PCDisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_PCDispW(ui3rr ArgDat)
{
	return get_word(DecodeAddr_PCDisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_PCDispL(ui3rr ArgDat)
{
	return get_long(DecodeAddr_PCDisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_PCIndexB(ui3rr ArgDat)
{
	return get_byte(DecodeAddr_PCIndex(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_PCIndexW(ui3rr ArgDat)
{
	return get_word(DecodeAddr_PCIndex(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_PCIndexL(ui3rr ArgDat)
{
	return get_long(DecodeAddr_PCIndex(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_ImmedB(ui3rr ArgDat)
{
	UnusedParam(ArgDat);
	return nextiSByte();
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_ImmedW(ui3rr ArgDat)
{
	UnusedParam(ArgDat);
	return nextiSWord();
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_ImmedL(ui3rr ArgDat)
{
	UnusedParam(ArgDat);
	return ui5r_FromSLong(nextilong());
}

LOCALFUNC ui5r my_reg_call DecodeGetSrcDst_Dat4(ui3rr ArgDat)
{
	return ArgDat;
}

typedef ui5r (my_reg_call *DecodeGetSrcDstP)(ui3rr ArgDat);

LOCALVAR const DecodeGetSrcDstP DecodeGetSrcDstDispatch[kNumAMds] = {
	DecodeGetSrcDst_RegB /* kAMdRegB */,
	DecodeGetSrcDst_RegW /* kAMdRegW */,
	DecodeGetSrcDst_RegL /* kAMdRegL */,
	DecodeGetSrcDst_IndirectB /* kAMdIndirectB */,
	DecodeGetSrcDst_IndirectW /* kAMdIndirectW */,
	DecodeGetSrcDst_IndirectL /* kAMdIndirectL */,
	DecodeGetSrcDst_APosIncB /* kAMdAPosIncB */,
	DecodeGetSrcDst_APosIncW /* kAMdAPosIncW */,
	DecodeGetSrcDst_APosIncL /* kAMdAPosIncL */,
	DecodeGetSrcDst_APosInc7B /* kAMdAPosInc7B */,
	DecodeGetSrcDst_APreDecB /* kAMdAPreDecB */,
	DecodeGetSrcDst_APreDecW /* kAMdAPreDecW */,
	DecodeGetSrcDst_APreDecL /* kAMdAPreDecL */,
	DecodeGetSrcDst_APreDec7B /* kAMdAPreDec7B */,
	DecodeGetSrcDst_ADispB /* kAMdADispB */,
	DecodeGetSrcDst_ADispW /* kAMdADispW */,
	DecodeGetSrcDst_ADispL /* kAMdADispL */,
	DecodeGetSrcDst_AIndexB /* kAMdAIndexB */,
	DecodeGetSrcDst_AIndexW /* kAMdAIndexW */,
	DecodeGetSrcDst_AIndexL /* kAMdAIndexL */,
	DecodeGetSrcDst_AbsWB /* kAMdAbsWB */,
	DecodeGetSrcDst_AbsWW /* kAMdAbsWW */,
	DecodeGetSrcDst_AbsWL /* kAMdAbsWL */,
	DecodeGetSrcDst_AbsLB /* kAMdAbsLB */,
	DecodeGetSrcDst_AbsLW /* kAMdAbsLW */,
	DecodeGetSrcDst_AbsLL /* kAMdAbsLL */,
	DecodeGetSrcDst_PCDispB /* kAMdPCDispB */,
	DecodeGetSrcDst_PCDispW /* kAMdPCDispW */,
	DecodeGetSrcDst_PCDispL /* kAMdPCDispL */,
	DecodeGetSrcDst_PCIndexB /* kAMdPCIndexB */,
	DecodeGetSrcDst_PCIndexW /* kAMdPCIndexW */,
	DecodeGetSrcDst_PCIndexL /* kAMdPCIndexL */,
	DecodeGetSrcDst_ImmedB /* kAMdImmedB */,
	DecodeGetSrcDst_ImmedW /* kAMdImmedW */,
	DecodeGetSrcDst_ImmedL /* kAMdImmedL */,
	DecodeGetSrcDst_Dat4 /* kAMdDat4 */
};

LOCALINLINEFUNC ui5r DecodeGetSrcDst(DecArgR *f)
{
	return (DecodeGetSrcDstDispatch[f->AMd])(f->ArgDat);
}

LOCALPROC my_reg_call DecodeSetSrcDst_RegB(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];

#if LittleEndianUnaligned
	*(ui3b *)p = v;
#else
	*p = (*p & ~ 0xff) | ((v) & 0xff);
#endif
}

LOCALPROC my_reg_call DecodeSetSrcDst_RegW(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];

#if LittleEndianUnaligned
	*(ui4b *)p = v;
#else
	*p = (*p & ~ 0xffff) | ((v) & 0xffff);
#endif
}

LOCALPROC my_reg_call DecodeSetSrcDst_RegL(ui5r v, ui3rr ArgDat)
{
	V_regs.regs[ArgDat] = v;
}

LOCALPROC my_reg_call DecodeSetSrcDst_IndirectB(ui5r v, ui3rr ArgDat)
{
	put_byte(V_regs.regs[ArgDat], v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_IndirectW(ui5r v, ui3rr ArgDat)
{
	put_word(V_regs.regs[ArgDat], v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_IndirectL(ui5r v, ui3rr ArgDat)
{
	put_long(V_regs.regs[ArgDat], v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APosIncB(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 1;

	put_byte(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APosIncW(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	put_word(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APosIncL(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 4;

	put_long(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APosInc7B(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	put_byte(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APreDecB(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 1;

	*p = a;

	put_byte(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APreDecW(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	put_word(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APreDecL(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 4;

	*p = a;

	put_long(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_APreDec7B(ui5r v, ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	put_byte(a, v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_ADispB(ui5r v, ui3rr ArgDat)
{
	put_byte(V_regs.regs[ArgDat]
		+ nextiSWord(), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_ADispW(ui5r v, ui3rr ArgDat)
{
	put_word(V_regs.regs[ArgDat]
		+ nextiSWord(), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_ADispL(ui5r v, ui3rr ArgDat)
{
	put_long(V_regs.regs[ArgDat]
		+ nextiSWord(), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AIndexB(ui5r v, ui3rr ArgDat)
{
	put_byte(get_disp_ea(V_regs.regs[ArgDat]), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AIndexW(ui5r v, ui3rr ArgDat)
{
	put_word(get_disp_ea(V_regs.regs[ArgDat]), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AIndexL(ui5r v, ui3rr ArgDat)
{
	put_long(get_disp_ea(V_regs.regs[ArgDat]), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AbsWB(ui5r v, ui3rr ArgDat)
{
	put_byte(DecodeAddr_AbsW(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AbsWW(ui5r v, ui3rr ArgDat)
{
	put_word(DecodeAddr_AbsW(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AbsWL(ui5r v, ui3rr ArgDat)
{
	put_long(DecodeAddr_AbsW(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AbsLB(ui5r v, ui3rr ArgDat)
{
	put_byte(DecodeAddr_AbsL(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AbsLW(ui5r v, ui3rr ArgDat)
{
	put_word(DecodeAddr_AbsL(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_AbsLL(ui5r v, ui3rr ArgDat)
{
	put_long(DecodeAddr_AbsL(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_PCDispB(ui5r v, ui3rr ArgDat)
{
	put_byte(DecodeAddr_PCDisp(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_PCDispW(ui5r v, ui3rr ArgDat)
{
	put_word(DecodeAddr_PCDisp(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_PCDispL(ui5r v, ui3rr ArgDat)
{
	put_long(DecodeAddr_PCDisp(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_PCIndexB(ui5r v, ui3rr ArgDat)
{
	put_byte(DecodeAddr_PCIndex(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_PCIndexW(ui5r v, ui3rr ArgDat)
{
	put_word(DecodeAddr_PCIndex(ArgDat), v);
}

LOCALPROC my_reg_call DecodeSetSrcDst_PCIndexL(ui5r v, ui3rr ArgDat)
{
	put_long(DecodeAddr_PCIndex(ArgDat), v);
}

typedef void (my_reg_call *DecodeSetSrcDstP)(ui5r v, ui3rr ArgDat);

LOCALVAR const DecodeSetSrcDstP DecodeSetSrcDstDispatch[kNumAMds] = {
	DecodeSetSrcDst_RegB /* kAMdRegB */,
	DecodeSetSrcDst_RegW /* kAMdRegW */,
	DecodeSetSrcDst_RegL /* kAMdRegL */,
	DecodeSetSrcDst_IndirectB /* kAMdIndirectB */,
	DecodeSetSrcDst_IndirectW /* kAMdIndirectW */,
	DecodeSetSrcDst_IndirectL /* kAMdIndirectL*/,
	DecodeSetSrcDst_APosIncB /* kAMdAPosIncB */,
	DecodeSetSrcDst_APosIncW /* kAMdAPosIncW */,
	DecodeSetSrcDst_APosIncL /* kAMdAPosIncL */,
	DecodeSetSrcDst_APosInc7B /* kAMdAPosInc7B */,
	DecodeSetSrcDst_APreDecB /* kAMdAPreDecB */,
	DecodeSetSrcDst_APreDecW /* kAMdAPreDecW */,
	DecodeSetSrcDst_APreDecL /* kAMdAPreDecL */,
	DecodeSetSrcDst_APreDec7B /* kAMdAPreDec7B */,
	DecodeSetSrcDst_ADispB /* kAMdADispB */,
	DecodeSetSrcDst_ADispW /* kAMdADispW */,
	DecodeSetSrcDst_ADispL /* kAMdADispL */,
	DecodeSetSrcDst_AIndexB /* kAMdAIndexB */,
	DecodeSetSrcDst_AIndexW /* kAMdAIndexW */,
	DecodeSetSrcDst_AIndexL /* kAMdAIndexL */,
	DecodeSetSrcDst_AbsWB /* kAMdAbsWB */,
	DecodeSetSrcDst_AbsWW /* kAMdAbsWW */,
	DecodeSetSrcDst_AbsWL /* kAMdAbsWL */,
	DecodeSetSrcDst_AbsLB /* kAMdAbsLB */,
	DecodeSetSrcDst_AbsLW /* kAMdAbsLW */,
	DecodeSetSrcDst_AbsLL /* kAMdAbsLL */,
	DecodeSetSrcDst_PCDispB /* kAMdPCDispB */,
	DecodeSetSrcDst_PCDispW /* kAMdPCDispW */,
	DecodeSetSrcDst_PCDispL /* kAMdPCDispL */,
	DecodeSetSrcDst_PCIndexB /* kAMdPCIndexB */,
	DecodeSetSrcDst_PCIndexW /* kAMdPCIndexW */,
	DecodeSetSrcDst_PCIndexL /* kAMdPCIndexL */,
	(DecodeSetSrcDstP)nullpr /* kAMdImmedB */,
	(DecodeSetSrcDstP)nullpr /* kAMdImmedW */,
	(DecodeSetSrcDstP)nullpr /* kAMdImmedL */,
	(DecodeSetSrcDstP)nullpr /* kAMdDat4 */
};

LOCALINLINEPROC DecodeSetSrcDst(ui5r v, DecArgR *f)
{
	(DecodeSetSrcDstDispatch[f->AMd])(v, f->ArgDat);
}

LOCALPROC my_reg_call ArgSetDstRegBValue(ui5r v)
{
	ui5r *p = V_regs.ArgAddr.rga;

#if LittleEndianUnaligned
	*(ui3b *)p = v;
#else
	*p = (*p & ~ 0xff) | ((v) & 0xff);
#endif
}

LOCALPROC my_reg_call ArgSetDstRegWValue(ui5r v)
{
	ui5r *p = V_regs.ArgAddr.rga;

#if LittleEndianUnaligned
	*(ui4b *)p = v;
#else
	*p = (*p & ~ 0xffff) | ((v) & 0xffff);
#endif
}

LOCALPROC my_reg_call ArgSetDstRegLValue(ui5r v)
{
	ui5r *p = V_regs.ArgAddr.rga;

	*p = v;
}

LOCALPROC my_reg_call ArgSetDstMemBValue(ui5r v)
{
	put_byte(V_regs.ArgAddr.mem, v);
}

LOCALPROC my_reg_call ArgSetDstMemWValue(ui5r v)
{
	put_word(V_regs.ArgAddr.mem, v);
}

LOCALPROC my_reg_call ArgSetDstMemLValue(ui5r v)
{
	put_long(V_regs.ArgAddr.mem, v);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_RegB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];

	V_regs.ArgAddr.rga = p;
	V_regs.ArgSetDst = ArgSetDstRegBValue;

	return ui5r_FromSByte(*p);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_RegW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];

	V_regs.ArgAddr.rga = p;
	V_regs.ArgSetDst = ArgSetDstRegWValue;

	return ui5r_FromSWord(*p);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_RegL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];

	V_regs.ArgAddr.rga = p;
	V_regs.ArgSetDst = ArgSetDstRegLValue;

	return ui5r_FromSLong(*p);
}

LOCALFUNC ui5r my_reg_call getarg_byte(ui5r a)
{
	V_regs.ArgAddr.mem = a;
	V_regs.ArgSetDst = ArgSetDstMemBValue;

	return get_byte(a);
}

LOCALFUNC ui5r my_reg_call getarg_word(ui5r a)
{
	V_regs.ArgAddr.mem = a;
	V_regs.ArgSetDst = ArgSetDstMemWValue;

	return get_word(a);
}

LOCALFUNC ui5r my_reg_call getarg_long(ui5r a)
{
	V_regs.ArgAddr.mem = a;
	V_regs.ArgSetDst = ArgSetDstMemLValue;

	return get_long(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_IndirectB(ui3rr ArgDat)
{
	return getarg_byte(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_IndirectW(ui3rr ArgDat)
{
	return getarg_word(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_IndirectL(ui3rr ArgDat)
{
	return getarg_long(V_regs.regs[ArgDat]);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APosIncB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 1;

	return getarg_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APosIncW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	return getarg_word(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APosIncL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 4;

	return getarg_long(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APosInc7B(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p;

	*p = a + 2;

	return getarg_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APreDecB(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 1;

	*p = a;

	return getarg_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APreDecW(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	return getarg_word(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APreDecL(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 4;

	*p = a;

	return getarg_long(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_APreDec7B(ui3rr ArgDat)
{
	ui5r *p = &V_regs.regs[ArgDat];
	ui5r a = *p - 2;

	*p = a;

	return getarg_byte(a);
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_ADispB(ui3rr ArgDat)
{
	return getarg_byte(V_regs.regs[ArgDat]
		+ nextiSWord());
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_ADispW(ui3rr ArgDat)
{
	return getarg_word(V_regs.regs[ArgDat]
		+ nextiSWord());
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_ADispL(ui3rr ArgDat)
{
	return getarg_long(V_regs.regs[ArgDat]
		+ nextiSWord());
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AIndexB(ui3rr ArgDat)
{
	return getarg_byte(get_disp_ea(V_regs.regs[ArgDat]));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AIndexW(ui3rr ArgDat)
{
	return getarg_word(get_disp_ea(V_regs.regs[ArgDat]));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AIndexL(ui3rr ArgDat)
{
	return getarg_long(get_disp_ea(V_regs.regs[ArgDat]));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AbsWB(ui3rr ArgDat)
{
	return getarg_byte(DecodeAddr_AbsW(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AbsWW(ui3rr ArgDat)
{
	return getarg_word(DecodeAddr_AbsW(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AbsWL(ui3rr ArgDat)
{
	return getarg_long(DecodeAddr_AbsW(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AbsLB(ui3rr ArgDat)
{
	return getarg_byte(DecodeAddr_AbsL(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AbsLW(ui3rr ArgDat)
{
	return getarg_word(DecodeAddr_AbsL(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_AbsLL(ui3rr ArgDat)
{
	return getarg_long(DecodeAddr_AbsL(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_PCDispB(ui3rr ArgDat)
{
	return getarg_byte(DecodeAddr_PCDisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_PCDispW(ui3rr ArgDat)
{
	return getarg_word(DecodeAddr_PCDisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_PCDispL(ui3rr ArgDat)
{
	return getarg_long(DecodeAddr_PCDisp(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_PCIndexB(ui3rr ArgDat)
{
	return getarg_byte(DecodeAddr_PCIndex(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_PCIndexW(ui3rr ArgDat)
{
	return getarg_word(DecodeAddr_PCIndex(ArgDat));
}

LOCALFUNC ui5r my_reg_call DecodeGetSetSrcDst_PCIndexL(ui3rr ArgDat)
{
	return getarg_long(DecodeAddr_PCIndex(ArgDat));
}

typedef ui5r (my_reg_call *DecodeGetSetSrcDstP)(ui3rr ArgDat);

LOCALVAR const DecodeGetSetSrcDstP
	DecodeGetSetSrcDstDispatch[kNumAMds] =
{
	DecodeGetSetSrcDst_RegB /* kAMdRegB */,
	DecodeGetSetSrcDst_RegW /* kAMdRegW */,
	DecodeGetSetSrcDst_RegL /* kAMdRegL */,
	DecodeGetSetSrcDst_IndirectB /* kAMdIndirectB */,
	DecodeGetSetSrcDst_IndirectW /* kAMdIndirectW */,
	DecodeGetSetSrcDst_IndirectL /* kAMdIndirectL*/,
	DecodeGetSetSrcDst_APosIncB /* kAMdAPosIncB */,
	DecodeGetSetSrcDst_APosIncW /* kAMdAPosIncW */,
	DecodeGetSetSrcDst_APosIncL /* kAMdAPosIncL */,
	DecodeGetSetSrcDst_APosInc7B /* kAMdAPosInc7B */,
	DecodeGetSetSrcDst_APreDecB /* kAMdAPreDecB */,
	DecodeGetSetSrcDst_APreDecW /* kAMdAPreDecW */,
	DecodeGetSetSrcDst_APreDecL /* kAMdAPreDecL */,
	DecodeGetSetSrcDst_APreDec7B /* kAMdAPreDec7B */,
	DecodeGetSetSrcDst_ADispB /* kAMdADispB */,
	DecodeGetSetSrcDst_ADispW /* kAMdADispW */,
	DecodeGetSetSrcDst_ADispL /* kAMdADispL */,
	DecodeGetSetSrcDst_AIndexB /* kAMdAIndexB */,
	DecodeGetSetSrcDst_AIndexW /* kAMdAIndexW */,
	DecodeGetSetSrcDst_AIndexL /* kAMdAIndexL */,
	DecodeGetSetSrcDst_AbsWB /* kAMdAbsWB */,
	DecodeGetSetSrcDst_AbsWW /* kAMdAbsWW */,
	DecodeGetSetSrcDst_AbsWL /* kAMdAbsWL */,
	DecodeGetSetSrcDst_AbsLB /* kAMdAbsLB */,
	DecodeGetSetSrcDst_AbsLW /* kAMdAbsLW */,
	DecodeGetSetSrcDst_AbsLL /* kAMdAbsLL */,
	DecodeGetSetSrcDst_PCDispB /* kAMdPCDispB */,
	DecodeGetSetSrcDst_PCDispW /* kAMdPCDispW */,
	DecodeGetSetSrcDst_PCDispL /* kAMdPCDispL */,
	DecodeGetSetSrcDst_PCIndexB /* kAMdPCIndexB */,
	DecodeGetSetSrcDst_PCIndexW /* kAMdPCIndexW */,
	DecodeGetSetSrcDst_PCIndexL /* kAMdPCIndexL */,
	(DecodeGetSetSrcDstP)nullpr /* kAMdImmedB */,
	(DecodeGetSetSrcDstP)nullpr /* kAMdImmedW */,
	(DecodeGetSetSrcDstP)nullpr /* kAMdImmedL */,
	(DecodeGetSetSrcDstP)nullpr /* kAMdDat4 */
};

LOCALINLINEFUNC ui5r DecodeGetSetSrcDst(DecArgR *f)
{
	return (DecodeGetSetSrcDstDispatch[f->AMd])(f->ArgDat);
}


LOCALINLINEFUNC ui5r DecodeDst(void)
{
	return DecodeAddrSrcDst(&V_regs.CurDecOpY.v[1]);
}

LOCALINLINEFUNC ui5r DecodeGetSetDstValue(void)
{
	return DecodeGetSetSrcDst(&V_regs.CurDecOpY.v[1]);
}

LOCALINLINEPROC ArgSetDstValue(ui5r v)
{
	V_regs.ArgSetDst(v);
}

LOCALINLINEPROC DecodeSetDstValue(ui5r v)
{
	DecodeSetSrcDst(v, &V_regs.CurDecOpY.v[1]);
}

LOCALINLINEFUNC ui5r DecodeGetSrcValue(void)
{
	return DecodeGetSrcDst(&V_regs.CurDecOpY.v[0]);
}

LOCALINLINEFUNC ui5r DecodeGetDstValue(void)
{
	return DecodeGetSrcDst(&V_regs.CurDecOpY.v[1]);
}

LOCALINLINEFUNC ui5r DecodeGetSrcSetDstValue(void)
{
	V_regs.SrcVal = DecodeGetSrcValue();

	return DecodeGetSetDstValue();
}

LOCALINLINEFUNC ui5r DecodeGetSrcGetDstValue(void)
{
	V_regs.SrcVal = DecodeGetSrcValue();

	return DecodeGetDstValue();
}


typedef void (*cond_actP)(void);

LOCALPROC my_reg_call cctrue_T(cond_actP t_act, cond_actP f_act)
{
	UnusedParam(f_act);
	t_act();
}

LOCALPROC my_reg_call cctrue_F(cond_actP t_act, cond_actP f_act)
{
	UnusedParam(t_act);
	f_act();
}

LOCALPROC my_reg_call cctrue_HI(cond_actP t_act, cond_actP f_act)
{
	if (0 == (CFLG | ZFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_LS(cond_actP t_act, cond_actP f_act)
{
	if (0 != (CFLG | ZFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CC(cond_actP t_act, cond_actP f_act)
{
	if (0 == (CFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CS(cond_actP t_act, cond_actP f_act)
{
	if (0 != (CFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_NE(cond_actP t_act, cond_actP f_act)
{
	if (0 == (ZFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_EQ(cond_actP t_act, cond_actP f_act)
{
	if (0 != (ZFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_VC(cond_actP t_act, cond_actP f_act)
{
	if (0 == (VFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_VS(cond_actP t_act, cond_actP f_act)
{
	if (0 != (VFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_PL(cond_actP t_act, cond_actP f_act)
{
	if (0 == (NFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_MI(cond_actP t_act, cond_actP f_act)
{
	if (0 != (NFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_GE(cond_actP t_act, cond_actP f_act)
{
	if (0 == (NFLG ^ VFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_LT(cond_actP t_act, cond_actP f_act)
{
	if (0 != (NFLG ^ VFLG)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_GT(cond_actP t_act, cond_actP f_act)
{
	if (0 == (ZFLG | (NFLG ^ VFLG))) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_LE(cond_actP t_act, cond_actP f_act)
{
	if (0 != (ZFLG | (NFLG ^ VFLG))) {
		t_act();
	} else {
		f_act();
	}
}

#if Have_ASR
#define Ui5rASR(x, s) ((ui5r)(((si5r)(x)) >> (s)))
#else
LOCALFUNC ui5r Ui5rASR(ui5r x, ui5r s)
{
	ui5r v;

	if (ui5r_MSBisSet(x)) {
		v = ~ ((~ x) >> s);
	} else {
		v = x >> s;
	}

	return v;
}
#endif

#if UseLazyCC

LOCALPROC my_reg_call cctrue_TstL_HI(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) > ((ui5b)0)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_LS(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) <= ((ui5b)0)) {
		t_act();
	} else {
		f_act();
	}
}

#if 0 /* always true */
LOCALPROC my_reg_call cctrue_TstL_CC(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) >= ((ui5b)0)) {
		t_act();
	} else {
		f_act();
	}
}
#endif

#if 0 /* always false */
LOCALPROC my_reg_call cctrue_TstL_CS(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) < ((ui5b)0)) {
		t_act();
	} else {
		f_act();
	}
}
#endif

LOCALPROC my_reg_call cctrue_TstL_NE(cond_actP t_act, cond_actP f_act)
{
	if (V_regs.LazyFlagArgDst != 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_EQ(cond_actP t_act, cond_actP f_act)
{
	if (V_regs.LazyFlagArgDst == 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_PL(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)(V_regs.LazyFlagArgDst)) >= 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_MI(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)(V_regs.LazyFlagArgDst)) < 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_GE(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) >= ((si5b)0)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_LT(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) < ((si5b)0)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_GT(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) > ((si5b)0)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_TstL_LE(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) <= ((si5b)0)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_HI(cond_actP t_act, cond_actP f_act)
{
	if (((ui3b)V_regs.LazyFlagArgDst) > ((ui3b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_LS(cond_actP t_act, cond_actP f_act)
{
	if (((ui3b)V_regs.LazyFlagArgDst) <= ((ui3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_CC(cond_actP t_act, cond_actP f_act)
{
	if (((ui3b)V_regs.LazyFlagArgDst) >= ((ui3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_CS(cond_actP t_act, cond_actP f_act)
{
	if (((ui3b)V_regs.LazyFlagArgDst) < ((ui3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_NE(cond_actP t_act, cond_actP f_act)
{
	if (((ui3b)V_regs.LazyFlagArgDst) != ((ui3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_EQ(cond_actP t_act, cond_actP f_act)
{
	if (((ui3b)V_regs.LazyFlagArgDst) == ((ui3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_PL(cond_actP t_act, cond_actP f_act)
{
	if (((si3b)(V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc)) >= 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_MI(cond_actP t_act, cond_actP f_act)
{
	if (((si3b)(V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc)) < 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_GE(cond_actP t_act, cond_actP f_act)
{
	if (((si3b)V_regs.LazyFlagArgDst) >= ((si3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_LT(cond_actP t_act, cond_actP f_act)
{
	if (((si3b)V_regs.LazyFlagArgDst) < ((si3b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_GT(cond_actP t_act, cond_actP f_act)
{
	if (((si3b)V_regs.LazyFlagArgDst) > ((si3b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpB_LE(cond_actP t_act, cond_actP f_act)
{
	if (((si3b)V_regs.LazyFlagArgDst) <= ((si3b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_HI(cond_actP t_act, cond_actP f_act)
{
	if (((ui4b)V_regs.LazyFlagArgDst) > ((ui4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_LS(cond_actP t_act, cond_actP f_act)
{
	if (((ui4b)V_regs.LazyFlagArgDst) <= ((ui4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_CC(cond_actP t_act, cond_actP f_act)
{
	if (((ui4b)V_regs.LazyFlagArgDst) >= ((ui4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_CS(cond_actP t_act, cond_actP f_act)
{
	if (((ui4b)V_regs.LazyFlagArgDst) < ((ui4b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_NE(cond_actP t_act, cond_actP f_act)
{
	if (((ui4b)V_regs.LazyFlagArgDst) != ((ui4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_EQ(cond_actP t_act, cond_actP f_act)
{
	if (((ui4b)V_regs.LazyFlagArgDst) == ((ui4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_PL(cond_actP t_act, cond_actP f_act)
{
	if (((si4b)(V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc)) >= 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_MI(cond_actP t_act, cond_actP f_act)
{
	if (((si4b)(V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc)) < 0) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_GE(cond_actP t_act, cond_actP f_act)
{
	if (((si4b)V_regs.LazyFlagArgDst) >= ((si4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_LT(cond_actP t_act, cond_actP f_act)
{
	if (((si4b)V_regs.LazyFlagArgDst) < ((si4b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_GT(cond_actP t_act, cond_actP f_act)
{
	if (((si4b)V_regs.LazyFlagArgDst) > ((si4b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpW_LE(cond_actP t_act, cond_actP f_act)
{
	if (((si4b)V_regs.LazyFlagArgDst) <= ((si4b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_HI(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) > ((ui5b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_LS(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) <= ((ui5b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_CC(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) >= ((ui5b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_CS(cond_actP t_act, cond_actP f_act)
{
	if (((ui5b)V_regs.LazyFlagArgDst) < ((ui5b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_NE(cond_actP t_act, cond_actP f_act)
{
	if (V_regs.LazyFlagArgDst != V_regs.LazyFlagArgSrc) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_EQ(cond_actP t_act, cond_actP f_act)
{
	if (V_regs.LazyFlagArgDst == V_regs.LazyFlagArgSrc) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_PL(cond_actP t_act, cond_actP f_act)
{
	if ((((si5b)(V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc)) >= 0))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_MI(cond_actP t_act, cond_actP f_act)
{
	if ((((si5b)(V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc)) < 0)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_GE(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) >= ((si5b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_LT(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) < ((si5b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_GT(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) > ((si5b)V_regs.LazyFlagArgSrc)) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_CmpL_LE(cond_actP t_act, cond_actP f_act)
{
	if (((si5b)V_regs.LazyFlagArgDst) <= ((si5b)V_regs.LazyFlagArgSrc))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_Asr_CC(cond_actP t_act, cond_actP f_act)
{
	if (0 ==
		((V_regs.LazyFlagArgDst >> (V_regs.LazyFlagArgSrc - 1)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_Asr_CS(cond_actP t_act, cond_actP f_act)
{
	if (0 !=
		((V_regs.LazyFlagArgDst >> (V_regs.LazyFlagArgSrc - 1)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslB_CC(cond_actP t_act, cond_actP f_act)
{
	if (0 ==
		((V_regs.LazyFlagArgDst >> (8 - V_regs.LazyFlagArgSrc)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslB_CS(cond_actP t_act, cond_actP f_act)
{
	if (0 !=
		((V_regs.LazyFlagArgDst >> (8 - V_regs.LazyFlagArgSrc)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslB_VC(cond_actP t_act, cond_actP f_act)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = ui5r_FromSByte(V_regs.LazyFlagArgDst << cnt);

	if (Ui5rASR(dst, cnt) == V_regs.LazyFlagArgDst) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslB_VS(cond_actP t_act, cond_actP f_act)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = ui5r_FromSByte(V_regs.LazyFlagArgDst << cnt);

	if (Ui5rASR(dst, cnt) != V_regs.LazyFlagArgDst) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslW_CC(cond_actP t_act, cond_actP f_act)
{
	if (0 ==
		((V_regs.LazyFlagArgDst >> (16 - V_regs.LazyFlagArgSrc)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslW_CS(cond_actP t_act, cond_actP f_act)
{
	if (0 !=
		((V_regs.LazyFlagArgDst >> (16 - V_regs.LazyFlagArgSrc)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslW_VC(cond_actP t_act, cond_actP f_act)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = ui5r_FromSWord(V_regs.LazyFlagArgDst << cnt);

	if (Ui5rASR(dst, cnt) == V_regs.LazyFlagArgDst) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslW_VS(cond_actP t_act, cond_actP f_act)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = ui5r_FromSWord(V_regs.LazyFlagArgDst << cnt);

	if (Ui5rASR(dst, cnt) != V_regs.LazyFlagArgDst) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslL_CC(cond_actP t_act, cond_actP f_act)
{
	if (0 ==
		((V_regs.LazyFlagArgDst >> (32 - V_regs.LazyFlagArgSrc)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslL_CS(cond_actP t_act, cond_actP f_act)
{
	if (0 !=
		((V_regs.LazyFlagArgDst >> (32 - V_regs.LazyFlagArgSrc)) & 1))
	{
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslL_VC(cond_actP t_act, cond_actP f_act)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = ui5r_FromSLong(V_regs.LazyFlagArgDst << cnt);

	if (Ui5rASR(dst, cnt) == V_regs.LazyFlagArgDst) {
		t_act();
	} else {
		f_act();
	}
}

LOCALPROC my_reg_call cctrue_AslL_VS(cond_actP t_act, cond_actP f_act)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = ui5r_FromSLong(V_regs.LazyFlagArgDst << cnt);

	if (Ui5rASR(dst, cnt) != V_regs.LazyFlagArgDst) {
		t_act();
	} else {
		f_act();
	}
}

FORWARDPROC my_reg_call cctrue_Dflt(cond_actP t_act, cond_actP f_act);

#endif /* UseLazyCC */

#if UseLazyCC
#define CCdispSz (16 * kNumLazyFlagsKinds)
#else
#define CCdispSz kNumLazyFlagsKinds
#endif

typedef void (my_reg_call *cctrueP)(cond_actP t_act, cond_actP f_act);

LOCALVAR const cctrueP cctrueDispatch[CCdispSz + 1] = {
	cctrue_T /* kLazyFlagsDefault T */,
	cctrue_F /* kLazyFlagsDefault F */,
	cctrue_HI /* kLazyFlagsDefault HI */,
	cctrue_LS /* kLazyFlagsDefault LS */,
	cctrue_CC /* kLazyFlagsDefault CC */,
	cctrue_CS /* kLazyFlagsDefault CS */,
	cctrue_NE /* kLazyFlagsDefault NE */,
	cctrue_EQ /* kLazyFlagsDefault EQ */,
	cctrue_VC /* kLazyFlagsDefault VC */,
	cctrue_VS /* kLazyFlagsDefault VS */,
	cctrue_PL /* kLazyFlagsDefault PL */,
	cctrue_MI /* kLazyFlagsDefault MI */,
	cctrue_GE /* kLazyFlagsDefault GE */,
	cctrue_LT /* kLazyFlagsDefault LT */,
	cctrue_GT /* kLazyFlagsDefault GT */,
	cctrue_LE /* kLazyFlagsDefault LE */,

#if UseLazyCC
	cctrue_T /* kLazyFlagsTstB T */,
	cctrue_F /* kLazyFlagsTstB F */,
	cctrue_Dflt /* kLazyFlagsTstB HI */,
	cctrue_Dflt /* kLazyFlagsTstB LS */,
	cctrue_Dflt /* kLazyFlagsTstB CC */,
	cctrue_Dflt /* kLazyFlagsTstB CS */,
	cctrue_Dflt /* kLazyFlagsTstB NE */,
	cctrue_Dflt /* kLazyFlagsTstB EQ */,
	cctrue_Dflt /* kLazyFlagsTstB VC */,
	cctrue_Dflt /* kLazyFlagsTstB VS */,
	cctrue_Dflt /* kLazyFlagsTstB PL */,
	cctrue_Dflt /* kLazyFlagsTstB MI */,
	cctrue_Dflt /* kLazyFlagsTstB GE */,
	cctrue_Dflt /* kLazyFlagsTstB LT */,
	cctrue_Dflt /* kLazyFlagsTstB GT */,
	cctrue_Dflt /* kLazyFlagsTstB LE */,

	cctrue_T /* kLazyFlagsTstW T */,
	cctrue_F /* kLazyFlagsTstW F */,
	cctrue_Dflt /* kLazyFlagsTstW HI */,
	cctrue_Dflt /* kLazyFlagsTstW LS */,
	cctrue_Dflt /* kLazyFlagsTstW CC */,
	cctrue_Dflt /* kLazyFlagsTstW CS */,
	cctrue_Dflt /* kLazyFlagsTstW NE */,
	cctrue_Dflt /* kLazyFlagsTstW EQ */,
	cctrue_Dflt /* kLazyFlagsTstW VC */,
	cctrue_Dflt /* kLazyFlagsTstW VS */,
	cctrue_Dflt /* kLazyFlagsTstW PL */,
	cctrue_Dflt /* kLazyFlagsTstW MI */,
	cctrue_Dflt /* kLazyFlagsTstW GE */,
	cctrue_Dflt /* kLazyFlagsTstW LT */,
	cctrue_Dflt /* kLazyFlagsTstW GT */,
	cctrue_Dflt /* kLazyFlagsTstW LE */,

	cctrue_T /* kLazyFlagsTstL T */,
	cctrue_F /* kLazyFlagsTstL F */,
	cctrue_TstL_HI /* kLazyFlagsTstL HI */,
	cctrue_TstL_LS /* kLazyFlagsTstL LS */,
	cctrue_T /* cctrue_TstL_CC */ /* kLazyFlagsTstL CC */,
	cctrue_F /* cctrue_TstL_CS */ /* kLazyFlagsTstL CS */,
	cctrue_TstL_NE /* kLazyFlagsTstL NE */,
	cctrue_TstL_EQ /* kLazyFlagsTstL EQ */,
	cctrue_T /* cctrue_Dflt */ /* kLazyFlagsTstL VC */,
	cctrue_F /* cctrue_Dflt */ /* kLazyFlagsTstL VS */,
	cctrue_TstL_PL /* kLazyFlagsTstL PL */,
	cctrue_TstL_MI /* kLazyFlagsTstL MI */,
	cctrue_TstL_GE /* kLazyFlagsTstL GE */,
	cctrue_TstL_LT /* kLazyFlagsTstL LT */,
	cctrue_TstL_GT /* kLazyFlagsTstL GT */,
	cctrue_TstL_LE /* kLazyFlagsTstL LE */,

	cctrue_T /* kLazyFlagsCmpB T */,
	cctrue_F /* kLazyFlagsCmpB F */,
	cctrue_CmpB_HI /* kLazyFlagsCmpB HI */,
	cctrue_CmpB_LS /* kLazyFlagsCmpB LS */,
	cctrue_CmpB_CC /* kLazyFlagsCmpB CC */,
	cctrue_CmpB_CS /* kLazyFlagsCmpB CS */,
	cctrue_CmpB_NE /* kLazyFlagsCmpB NE */,
	cctrue_CmpB_EQ /* kLazyFlagsCmpB EQ */,
	cctrue_Dflt /* kLazyFlagsCmpB VC */,
	cctrue_Dflt /* kLazyFlagsCmpB VS */,
	cctrue_CmpB_PL /* kLazyFlagsCmpB PL */,
	cctrue_CmpB_MI /* kLazyFlagsCmpB MI */,
	cctrue_CmpB_GE /* kLazyFlagsCmpB GE */,
	cctrue_CmpB_LT /* kLazyFlagsCmpB LT */,
	cctrue_CmpB_GT /* kLazyFlagsCmpB GT */,
	cctrue_CmpB_LE /* kLazyFlagsCmpB LE */,

	cctrue_T /* kLazyFlagsCmpW T */,
	cctrue_F /* kLazyFlagsCmpW F */,
	cctrue_CmpW_HI /* kLazyFlagsCmpW HI */,
	cctrue_CmpW_LS /* kLazyFlagsCmpW LS */,
	cctrue_CmpW_CC /* kLazyFlagsCmpW CC */,
	cctrue_CmpW_CS /* kLazyFlagsCmpW CS */,
	cctrue_CmpW_NE /* kLazyFlagsCmpW NE */,
	cctrue_CmpW_EQ /* kLazyFlagsCmpW EQ */,
	cctrue_Dflt /* kLazyFlagsCmpW VC */,
	cctrue_Dflt /* kLazyFlagsCmpW VS */,
	cctrue_CmpW_PL /* kLazyFlagsCmpW PL */,
	cctrue_CmpW_MI /* kLazyFlagsCmpW MI */,
	cctrue_CmpW_GE /* kLazyFlagsCmpW GE */,
	cctrue_CmpW_LT /* kLazyFlagsCmpW LT */,
	cctrue_CmpW_GT /* kLazyFlagsCmpW GT */,
	cctrue_CmpW_LE /* kLazyFlagsCmpW LE */,

	cctrue_T /* kLazyFlagsCmpL T */,
	cctrue_F /* kLazyFlagsCmpL F */,
	cctrue_CmpL_HI /* kLazyFlagsCmpL HI */,
	cctrue_CmpL_LS /* kLazyFlagsCmpL LS */,
	cctrue_CmpL_CC /* kLazyFlagsCmpL CC */,
	cctrue_CmpL_CS /* kLazyFlagsCmpL CS */,
	cctrue_CmpL_NE /* kLazyFlagsCmpL NE */,
	cctrue_CmpL_EQ /* kLazyFlagsCmpL EQ */,
	cctrue_Dflt /* kLazyFlagsCmpL VC */,
	cctrue_Dflt /* kLazyFlagsCmpL VS */,
	cctrue_CmpL_PL /* kLazyFlagsCmpL PL */,
	cctrue_CmpL_MI /* kLazyFlagsCmpL MI */,
	cctrue_CmpL_GE /* kLazyFlagsCmpL GE */,
	cctrue_CmpL_LT /* kLazyFlagsCmpL LT */,
	cctrue_CmpL_GT /* kLazyFlagsCmpL GT */,
	cctrue_CmpL_LE /* kLazyFlagsCmpL LE */,

	cctrue_T /* kLazyFlagsSubB T */,
	cctrue_F /* kLazyFlagsSubB F */,
	cctrue_CmpB_HI /* kLazyFlagsSubB HI */,
	cctrue_CmpB_LS /* kLazyFlagsSubB LS */,
	cctrue_CmpB_CC /* kLazyFlagsSubB CC */,
	cctrue_CmpB_CS /* kLazyFlagsSubB CS */,
	cctrue_CmpB_NE /* kLazyFlagsSubB NE */,
	cctrue_CmpB_EQ /* kLazyFlagsSubB EQ */,
	cctrue_Dflt /* kLazyFlagsSubB VC */,
	cctrue_Dflt /* kLazyFlagsSubB VS */,
	cctrue_CmpB_PL /* kLazyFlagsSubB PL */,
	cctrue_CmpB_MI /* kLazyFlagsSubB MI */,
	cctrue_CmpB_GE /* kLazyFlagsSubB GE */,
	cctrue_CmpB_LT /* kLazyFlagsSubB LT */,
	cctrue_CmpB_GT /* kLazyFlagsSubB GT */,
	cctrue_CmpB_LE /* kLazyFlagsSubB LE */,

	cctrue_T /* kLazyFlagsSubW T */,
	cctrue_F /* kLazyFlagsSubW F */,
	cctrue_CmpW_HI /* kLazyFlagsSubW HI */,
	cctrue_CmpW_LS /* kLazyFlagsSubW LS */,
	cctrue_CmpW_CC /* kLazyFlagsSubW CC */,
	cctrue_CmpW_CS /* kLazyFlagsSubW CS */,
	cctrue_CmpW_NE /* kLazyFlagsSubW NE */,
	cctrue_CmpW_EQ /* kLazyFlagsSubW EQ */,
	cctrue_Dflt /* kLazyFlagsSubW VC */,
	cctrue_Dflt /* kLazyFlagsSubW VS */,
	cctrue_CmpW_PL /* kLazyFlagsSubW PL */,
	cctrue_CmpW_MI /* kLazyFlagsSubW MI */,
	cctrue_CmpW_GE /* kLazyFlagsSubW GE */,
	cctrue_CmpW_LT /* kLazyFlagsSubW LT */,
	cctrue_CmpW_GT /* kLazyFlagsSubW GT */,
	cctrue_CmpW_LE /* kLazyFlagsSubW LE */,

	cctrue_T /* kLazyFlagsSubL T */,
	cctrue_F /* kLazyFlagsSubL F */,
	cctrue_CmpL_HI /* kLazyFlagsSubL HI */,
	cctrue_CmpL_LS /* kLazyFlagsSubL LS */,
	cctrue_CmpL_CC /* kLazyFlagsSubL CC */,
	cctrue_CmpL_CS /* kLazyFlagsSubL CS */,
	cctrue_CmpL_NE /* kLazyFlagsSubL NE */,
	cctrue_CmpL_EQ /* kLazyFlagsSubL EQ */,
	cctrue_Dflt /* kLazyFlagsSubL VC */,
	cctrue_Dflt /* kLazyFlagsSubL VS */,
	cctrue_CmpL_PL /* kLazyFlagsSubL PL */,
	cctrue_CmpL_MI /* kLazyFlagsSubL MI */,
	cctrue_CmpL_GE /* kLazyFlagsSubL GE */,
	cctrue_CmpL_LT /* kLazyFlagsSubL LT */,
	cctrue_CmpL_GT /* kLazyFlagsSubL GT */,
	cctrue_CmpL_LE /* kLazyFlagsSubL LE */,

	cctrue_T /* kLazyFlagsAddB T */,
	cctrue_F /* kLazyFlagsAddB F */,
	cctrue_Dflt /* kLazyFlagsAddB HI */,
	cctrue_Dflt /* kLazyFlagsAddB LS */,
	cctrue_Dflt /* kLazyFlagsAddB CC */,
	cctrue_Dflt /* kLazyFlagsAddB CS */,
	cctrue_Dflt /* kLazyFlagsAddB NE */,
	cctrue_Dflt /* kLazyFlagsAddB EQ */,
	cctrue_Dflt /* kLazyFlagsAddB VC */,
	cctrue_Dflt /* kLazyFlagsAddB VS */,
	cctrue_Dflt /* kLazyFlagsAddB PL */,
	cctrue_Dflt /* kLazyFlagsAddB MI */,
	cctrue_Dflt /* kLazyFlagsAddB GE */,
	cctrue_Dflt /* kLazyFlagsAddB LT */,
	cctrue_Dflt /* kLazyFlagsAddB GT */,
	cctrue_Dflt /* kLazyFlagsAddB LE */,

	cctrue_T /* kLazyFlagsAddW T */,
	cctrue_F /* kLazyFlagsAddW F */,
	cctrue_Dflt /* kLazyFlagsAddW HI */,
	cctrue_Dflt /* kLazyFlagsAddW LS */,
	cctrue_Dflt /* kLazyFlagsAddW CC */,
	cctrue_Dflt /* kLazyFlagsAddW CS */,
	cctrue_Dflt /* kLazyFlagsAddW NE */,
	cctrue_Dflt /* kLazyFlagsAddW EQ */,
	cctrue_Dflt /* kLazyFlagsAddW VC */,
	cctrue_Dflt /* kLazyFlagsAddW VS */,
	cctrue_Dflt /* kLazyFlagsAddW PL */,
	cctrue_Dflt /* kLazyFlagsAddW MI */,
	cctrue_Dflt /* kLazyFlagsAddW GE */,
	cctrue_Dflt /* kLazyFlagsAddW LT */,
	cctrue_Dflt /* kLazyFlagsAddW GT */,
	cctrue_Dflt /* kLazyFlagsAddW LE */,

	cctrue_T /* kLazyFlagsAddL T */,
	cctrue_F /* kLazyFlagsAddL F */,
	cctrue_Dflt /* kLazyFlagsAddL HI */,
	cctrue_Dflt /* kLazyFlagsAddL LS */,
	cctrue_Dflt /* kLazyFlagsAddL CC */,
	cctrue_Dflt /* kLazyFlagsAddL CS */,
	cctrue_Dflt /* kLazyFlagsAddL NE */,
	cctrue_Dflt /* kLazyFlagsAddL EQ */,
	cctrue_Dflt /* kLazyFlagsAddL VC */,
	cctrue_Dflt /* kLazyFlagsAddL VS */,
	cctrue_Dflt /* kLazyFlagsAddL PL */,
	cctrue_Dflt /* kLazyFlagsAddL MI */,
	cctrue_Dflt /* kLazyFlagsAddL GE */,
	cctrue_Dflt /* kLazyFlagsAddL LT */,
	cctrue_Dflt /* kLazyFlagsAddL GT */,
	cctrue_Dflt /* kLazyFlagsAddL LE */,

	cctrue_T /* kLazyFlagsNegB T */,
	cctrue_F /* kLazyFlagsNegB F */,
	cctrue_Dflt /* kLazyFlagsNegB HI */,
	cctrue_Dflt /* kLazyFlagsNegB LS */,
	cctrue_Dflt /* kLazyFlagsNegB CC */,
	cctrue_Dflt /* kLazyFlagsNegB CS */,
	cctrue_Dflt /* kLazyFlagsNegB NE */,
	cctrue_Dflt /* kLazyFlagsNegB EQ */,
	cctrue_Dflt /* kLazyFlagsNegB VC */,
	cctrue_Dflt /* kLazyFlagsNegB VS */,
	cctrue_Dflt /* kLazyFlagsNegB PL */,
	cctrue_Dflt /* kLazyFlagsNegB MI */,
	cctrue_Dflt /* kLazyFlagsNegB GE */,
	cctrue_Dflt /* kLazyFlagsNegB LT */,
	cctrue_Dflt /* kLazyFlagsNegB GT */,
	cctrue_Dflt /* kLazyFlagsNegB LE */,

	cctrue_T /* kLazyFlagsNegW T */,
	cctrue_F /* kLazyFlagsNegW F */,
	cctrue_Dflt /* kLazyFlagsNegW HI */,
	cctrue_Dflt /* kLazyFlagsNegW LS */,
	cctrue_Dflt /* kLazyFlagsNegW CC */,
	cctrue_Dflt /* kLazyFlagsNegW CS */,
	cctrue_Dflt /* kLazyFlagsNegW NE */,
	cctrue_Dflt /* kLazyFlagsNegW EQ */,
	cctrue_Dflt /* kLazyFlagsNegW VC */,
	cctrue_Dflt /* kLazyFlagsNegW VS */,
	cctrue_Dflt /* kLazyFlagsNegW PL */,
	cctrue_Dflt /* kLazyFlagsNegW MI */,
	cctrue_Dflt /* kLazyFlagsNegW GE */,
	cctrue_Dflt /* kLazyFlagsNegW LT */,
	cctrue_Dflt /* kLazyFlagsNegW GT */,
	cctrue_Dflt /* kLazyFlagsNegW LE */,

	cctrue_T /* kLazyFlagsNegL T */,
	cctrue_F /* kLazyFlagsNegL F */,
	cctrue_Dflt /* kLazyFlagsNegL HI */,
	cctrue_Dflt /* kLazyFlagsNegL LS */,
	cctrue_Dflt /* kLazyFlagsNegL CC */,
	cctrue_Dflt /* kLazyFlagsNegL CS */,
	cctrue_Dflt /* kLazyFlagsNegL NE */,
	cctrue_Dflt /* kLazyFlagsNegL EQ */,
	cctrue_Dflt /* kLazyFlagsNegL VC */,
	cctrue_Dflt /* kLazyFlagsNegL VS */,
	cctrue_Dflt /* kLazyFlagsNegL PL */,
	cctrue_Dflt /* kLazyFlagsNegL MI */,
	cctrue_Dflt /* kLazyFlagsNegL GE */,
	cctrue_Dflt /* kLazyFlagsNegL LT */,
	cctrue_Dflt /* kLazyFlagsNegL GT */,
	cctrue_Dflt /* kLazyFlagsNegL LE */,

	cctrue_T /* kLazyFlagsAsrB T */,
	cctrue_F /* kLazyFlagsAsrB F */,
	cctrue_Dflt /* kLazyFlagsAsrB HI */,
	cctrue_Dflt /* kLazyFlagsAsrB LS */,
	cctrue_Asr_CC /* kLazyFlagsAsrB CC */,
	cctrue_Asr_CS /* kLazyFlagsAsrB CS */,
	cctrue_Dflt /* kLazyFlagsAsrB NE */,
	cctrue_Dflt /* kLazyFlagsAsrB EQ */,
	cctrue_Dflt /* kLazyFlagsAsrB VC */,
	cctrue_Dflt /* kLazyFlagsAsrB VS */,
	cctrue_Dflt /* kLazyFlagsAsrB PL */,
	cctrue_Dflt /* kLazyFlagsAsrB MI */,
	cctrue_Dflt /* kLazyFlagsAsrB GE */,
	cctrue_Dflt /* kLazyFlagsAsrB LT */,
	cctrue_Dflt /* kLazyFlagsAsrB GT */,
	cctrue_Dflt /* kLazyFlagsAsrB LE */,

	cctrue_T /* kLazyFlagsAsrW T */,
	cctrue_F /* kLazyFlagsAsrW F */,
	cctrue_Dflt /* kLazyFlagsAsrW HI */,
	cctrue_Dflt /* kLazyFlagsAsrW LS */,
	cctrue_Asr_CC /* kLazyFlagsAsrW CC */,
	cctrue_Asr_CS /* kLazyFlagsAsrW CS */,
	cctrue_Dflt /* kLazyFlagsAsrW NE */,
	cctrue_Dflt /* kLazyFlagsAsrW EQ */,
	cctrue_Dflt /* kLazyFlagsAsrW VC */,
	cctrue_Dflt /* kLazyFlagsAsrW VS */,
	cctrue_Dflt /* kLazyFlagsAsrW PL */,
	cctrue_Dflt /* kLazyFlagsAsrW MI */,
	cctrue_Dflt /* kLazyFlagsAsrW GE */,
	cctrue_Dflt /* kLazyFlagsAsrW LT */,
	cctrue_Dflt /* kLazyFlagsAsrW GT */,
	cctrue_Dflt /* kLazyFlagsAsrW LE */,

	cctrue_T /* kLazyFlagsAsrL T */,
	cctrue_F /* kLazyFlagsAsrL F */,
	cctrue_Dflt /* kLazyFlagsAsrL HI */,
	cctrue_Dflt /* kLazyFlagsAsrL LS */,
	cctrue_Asr_CC /* kLazyFlagsAsrL CC */,
	cctrue_Asr_CS /* kLazyFlagsAsrL CS */,
	cctrue_Dflt /* kLazyFlagsAsrL NE */,
	cctrue_Dflt /* kLazyFlagsAsrL EQ */,
	cctrue_Dflt /* kLazyFlagsAsrL VC */,
	cctrue_Dflt /* kLazyFlagsAsrL VS */,
	cctrue_Dflt /* kLazyFlagsAsrL PL */,
	cctrue_Dflt /* kLazyFlagsAsrL MI */,
	cctrue_Dflt /* kLazyFlagsAsrL GE */,
	cctrue_Dflt /* kLazyFlagsAsrL LT */,
	cctrue_Dflt /* kLazyFlagsAsrL GT */,
	cctrue_Dflt /* kLazyFlagsAsrL LE */,

	cctrue_T /* kLazyFlagsAslB T */,
	cctrue_F /* kLazyFlagsAslB F */,
	cctrue_Dflt /* kLazyFlagsAslB HI */,
	cctrue_Dflt /* kLazyFlagsAslB LS */,
	cctrue_AslB_CC /* kLazyFlagsAslB CC */,
	cctrue_AslB_CS /* kLazyFlagsAslB CS */,
	cctrue_Dflt /* kLazyFlagsAslB NE */,
	cctrue_Dflt /* kLazyFlagsAslB EQ */,
	cctrue_AslB_VC /* kLazyFlagsAslB VC */,
	cctrue_AslB_VS /* kLazyFlagsAslB VS */,
	cctrue_Dflt /* kLazyFlagsAslB PL */,
	cctrue_Dflt /* kLazyFlagsAslB MI */,
	cctrue_Dflt /* kLazyFlagsAslB GE */,
	cctrue_Dflt /* kLazyFlagsAslB LT */,
	cctrue_Dflt /* kLazyFlagsAslB GT */,
	cctrue_Dflt /* kLazyFlagsAslB LE */,

	cctrue_T /* kLazyFlagsAslW T */,
	cctrue_F /* kLazyFlagsAslW F */,
	cctrue_Dflt /* kLazyFlagsAslW HI */,
	cctrue_Dflt /* kLazyFlagsAslW LS */,
	cctrue_AslW_CC /* kLazyFlagsAslW CC */,
	cctrue_AslW_CS /* kLazyFlagsAslW CS */,
	cctrue_Dflt /* kLazyFlagsAslW NE */,
	cctrue_Dflt /* kLazyFlagsAslW EQ */,
	cctrue_AslW_VC /* kLazyFlagsAslW VC */,
	cctrue_AslW_VS /* kLazyFlagsAslW VS */,
	cctrue_Dflt /* kLazyFlagsAslW PL */,
	cctrue_Dflt /* kLazyFlagsAslW MI */,
	cctrue_Dflt /* kLazyFlagsAslW GE */,
	cctrue_Dflt /* kLazyFlagsAslW LT */,
	cctrue_Dflt /* kLazyFlagsAslW GT */,
	cctrue_Dflt /* kLazyFlagsAslW LE */,

	cctrue_T /* kLazyFlagsAslL T */,
	cctrue_F /* kLazyFlagsAslL F */,
	cctrue_Dflt /* kLazyFlagsAslL HI */,
	cctrue_Dflt /* kLazyFlagsAslL LS */,
	cctrue_AslL_CC /* kLazyFlagsAslL CC */,
	cctrue_AslL_CS /* kLazyFlagsAslL CS */,
	cctrue_Dflt /* kLazyFlagsAslL NE */,
	cctrue_Dflt /* kLazyFlagsAslL EQ */,
	cctrue_AslL_VC /* kLazyFlagsAslL VC */,
	cctrue_AslL_VS /* kLazyFlagsAslL VS */,
	cctrue_Dflt /* kLazyFlagsAslL PL */,
	cctrue_Dflt /* kLazyFlagsAslL MI */,
	cctrue_Dflt /* kLazyFlagsAslL GE */,
	cctrue_Dflt /* kLazyFlagsAslL LT */,
	cctrue_Dflt /* kLazyFlagsAslL GT */,
	cctrue_Dflt /* kLazyFlagsAslL LE */,

#if UseLazyZ
	cctrue_T /* kLazyFlagsZSet T */,
	cctrue_F /* kLazyFlagsZSet F */,
	cctrue_Dflt /* kLazyFlagsZSet HI */,
	cctrue_Dflt /* kLazyFlagsZSet LS */,
	cctrue_Dflt /* kLazyFlagsZSet CC */,
	cctrue_Dflt /* kLazyFlagsZSet CS */,
	cctrue_NE /* kLazyFlagsZSet NE */,
	cctrue_EQ /* kLazyFlagsZSet EQ */,
	cctrue_Dflt /* kLazyFlagsZSet VC */,
	cctrue_Dflt /* kLazyFlagsZSet VS */,
	cctrue_Dflt /* kLazyFlagsZSet PL */,
	cctrue_Dflt /* kLazyFlagsZSet MI */,
	cctrue_Dflt /* kLazyFlagsZSet GE */,
	cctrue_Dflt /* kLazyFlagsZSet LT */,
	cctrue_Dflt /* kLazyFlagsZSet GT */,
	cctrue_Dflt /* kLazyFlagsZSet LE */,
#endif
#endif /* UseLazyCC */

	0
};

#if UseLazyCC
LOCALINLINEPROC cctrue(cond_actP t_act, cond_actP f_act)
{
	(cctrueDispatch[V_regs.LazyFlagKind * 16
		+ V_regs.CurDecOpY.v[0].ArgDat])(t_act, f_act);
}
#endif


LOCALPROC NeedDefaultLazyXFlagSubB(void)
{
	XFLG = Bool2Bit(((ui3b)V_regs.LazyXFlagArgDst)
		< ((ui3b)V_regs.LazyXFlagArgSrc));
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagSubW(void)
{
	XFLG = Bool2Bit(((ui4b)V_regs.LazyXFlagArgDst)
		< ((ui4b)V_regs.LazyXFlagArgSrc));
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagSubL(void)
{
	XFLG = Bool2Bit(((ui5b)V_regs.LazyXFlagArgDst)
		< ((ui5b)V_regs.LazyXFlagArgSrc));
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAddB(void)
{
	ui3b src = (ui3b)V_regs.LazyXFlagArgSrc;
	ui3b dst = (ui3b)V_regs.LazyXFlagArgDst;
	ui3b result = dst + src;

	XFLG = Bool2Bit(result < src);
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAddW(void)
{
	ui4b src = (ui4b)V_regs.LazyXFlagArgSrc;
	ui4b dst = (ui4b)V_regs.LazyXFlagArgDst;
	ui4b result = dst + src;

	XFLG = Bool2Bit(result < src);
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAddL(void)
{
	ui5b src = (ui5b)V_regs.LazyXFlagArgSrc;
	ui5b dst = (ui5b)V_regs.LazyXFlagArgDst;
	ui5b result = dst + src;

	XFLG = Bool2Bit(result < src);
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagNegB(void)
{
	XFLG = Bool2Bit(((ui3b)0)
		< ((ui3b)V_regs.LazyXFlagArgDst));
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagNegW(void)
{
	XFLG = Bool2Bit(((ui4b)0)
		< ((ui4b)V_regs.LazyXFlagArgDst));
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagNegL(void)
{
	XFLG = Bool2Bit(((ui5b)0)
		< ((ui5b)V_regs.LazyXFlagArgDst));
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAsr(void)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;

	XFLG = ((dst >> (cnt - 1)) & 1);

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAslB(void)
{
	XFLG = (V_regs.LazyFlagArgDst >> (8 - V_regs.LazyFlagArgSrc)) & 1;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAslW(void)
{
	XFLG = (V_regs.LazyFlagArgDst >> (16 - V_regs.LazyFlagArgSrc)) & 1;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagAslL(void)
{
	XFLG = (V_regs.LazyFlagArgDst >> (32 - V_regs.LazyFlagArgSrc)) & 1;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyXFlagDefault(void)
{
}

typedef void (*NeedLazyFlagP)(void);

LOCALVAR const NeedLazyFlagP
	NeedLazyXFlagDispatch[kNumLazyFlagsKinds + 1] =
{
	NeedDefaultLazyXFlagDefault /* kLazyFlagsDefault */,
	0 /* kLazyFlagsTstB */,
	0 /* kLazyFlagsTstW */,
	0 /* kLazyFlagsTstL */,
	0 /* kLazyFlagsCmpB */,
	0 /* kLazyFlagsCmpW */,
	0 /* kLazyFlagsCmpL */,
	NeedDefaultLazyXFlagSubB /* kLazyFlagsSubB */,
	NeedDefaultLazyXFlagSubW /* kLazyFlagsSubW */,
	NeedDefaultLazyXFlagSubL /* kLazyFlagsSubL */,
	NeedDefaultLazyXFlagAddB /* kLazyFlagsAddB */,
	NeedDefaultLazyXFlagAddW /* kLazyFlagsAddW */,
	NeedDefaultLazyXFlagAddL /* kLazyFlagsAddL */,
	NeedDefaultLazyXFlagNegB /* kLazyFlagsNegB */,
	NeedDefaultLazyXFlagNegW /* kLazyFlagsNegW */,
	NeedDefaultLazyXFlagNegL /* kLazyFlagsNegL */,
	NeedDefaultLazyXFlagAsr  /* kLazyFlagsAsrB */,
	NeedDefaultLazyXFlagAsr  /* kLazyFlagsAsrW */,
	NeedDefaultLazyXFlagAsr  /* kLazyFlagsAsrL */,
	NeedDefaultLazyXFlagAslB /* kLazyFlagsAslB */,
	NeedDefaultLazyXFlagAslW /* kLazyFlagsAslW */,
	NeedDefaultLazyXFlagAslL /* kLazyFlagsAslL */,
#if UseLazyZ
	0 /* kLazyFlagsZSet */,
#endif

	0
};

LOCALPROC NeedDefaultLazyXFlag(void)
{
#if ForceFlagsEval
	if (kLazyFlagsDefault != V_regs.LazyXFlagKind) {
		ReportAbnormalID(0x0103,
			"not kLazyFlagsDefault in NeedDefaultLazyXFlag");
	}
#else
	(NeedLazyXFlagDispatch[V_regs.LazyXFlagKind])();
#endif
}

LOCALPROC NeedDefaultLazyFlagsTstL(void)
{
	ui5r dst = V_regs.LazyFlagArgDst;

	VFLG = CFLG = 0;
	ZFLG = Bool2Bit(dst == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(dst));

	V_regs.LazyFlagKind = kLazyFlagsDefault;
	NeedDefaultLazyXFlag();
}

LOCALPROC NeedDefaultLazyFlagsCmpB(void)
{
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result0 = dst - src;
	ui5r result1 = ui5r_FromUByte(dst)
		- ui5r_FromUByte(src);
	ui5r result = ui5r_FromSByte(result0);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));
	VFLG = (((result0 >> 1) ^ result0) >> 7) & 1;
	CFLG = (result1 >> 8) & 1;

	V_regs.LazyFlagKind = kLazyFlagsDefault;
	NeedDefaultLazyXFlag();
}

LOCALPROC NeedDefaultLazyFlagsCmpW(void)
{
	ui5r result0 = V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc;
	ui5r result = ui5r_FromSWord(result0);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));

	VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	{
		ui5r result1 = ui5r_FromUWord(V_regs.LazyFlagArgDst)
			- ui5r_FromUWord(V_regs.LazyFlagArgSrc);

		CFLG = (result1 >> 16) & 1;
	}

	V_regs.LazyFlagKind = kLazyFlagsDefault;
	NeedDefaultLazyXFlag();
}

LOCALPROC NeedDefaultLazyFlagsCmpL(void)
{
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result = ui5r_FromSLong(dst - src);

	ZFLG = Bool2Bit(result == 0);

	{
		flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));
		flagtype flgs = Bool2Bit(ui5r_MSBisSet(src));
		flagtype flgo = Bool2Bit(ui5r_MSBisSet(dst)) ^ 1;
		flagtype flgsando = flgs & flgo;
		flagtype flgsoro = flgs | flgo;

		NFLG = flgn;
		VFLG = ((flgn | flgsoro) ^ 1) | (flgn & flgsando);
		CFLG = flgsando | (flgn & flgsoro);
	}

	V_regs.LazyFlagKind = kLazyFlagsDefault;
	NeedDefaultLazyXFlag();
}

LOCALPROC NeedDefaultLazyFlagsSubB(void)
{
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result0 = dst - src;
	ui5r result1 = ui5r_FromUByte(dst)
		- ui5r_FromUByte(src);
	ui5r result = ui5r_FromSByte(result0);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));
	VFLG = (((result0 >> 1) ^ result0) >> 7) & 1;
	CFLG = (result1 >> 8) & 1;

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsSubW(void)
{
	ui5r result0 = V_regs.LazyFlagArgDst - V_regs.LazyFlagArgSrc;
	ui5r result = ui5r_FromSWord(result0);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));

	VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	{
		ui5r result1 = ui5r_FromUWord(V_regs.LazyFlagArgDst)
			- ui5r_FromUWord(V_regs.LazyFlagArgSrc);

		CFLG = (result1 >> 16) & 1;
	}

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsSubL(void)
{
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result = ui5r_FromSLong(dst - src);

	ZFLG = Bool2Bit(result == 0);

	{
		flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));
		flagtype flgs = Bool2Bit(ui5r_MSBisSet(src));
		flagtype flgo = Bool2Bit(ui5r_MSBisSet(dst)) ^ 1;
		flagtype flgsando = flgs & flgo;
		flagtype flgsoro = flgs | flgo;

		NFLG = flgn;
		VFLG = ((flgn | flgsoro) ^ 1) | (flgn & flgsando);
		CFLG = flgsando | (flgn & flgsoro);
	}

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsAddB(void)
{
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result0 = dst + src;
	ui5r result1 = ui5r_FromUByte(dst)
		+ ui5r_FromUByte(src);
	ui5r result = ui5r_FromSByte(result0);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));
	VFLG = (((result0 >> 1) ^ result0) >> 7) & 1;
	CFLG = (result1 >> 8);

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsAddW(void)
{
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result0 = dst + src;
	ui5r result1 = ui5r_FromUWord(dst)
		+ ui5r_FromUWord(src);
	ui5r result = ui5r_FromSWord(result0);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));
	VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	CFLG = (result1 >> 16);

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

#if 0
LOCALPROC NeedDefaultLazyFlagsAddCommon(ui5r result)
{
	ZFLG = Bool2Bit(result == 0);
	{
		flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));
		flagtype flgs = Bool2Bit(ui5r_MSBisSet(V_regs.LazyFlagArgSrc));
		flagtype flgo = Bool2Bit(ui5r_MSBisSet(V_regs.LazyFlagArgDst));
		flagtype flgsando = flgs & flgo;
		flagtype flgsoro = flgs | flgo;

		NFLG = flgn;
		flgn ^= 1;
		VFLG = ((flgn | flgsoro) ^ 1) | (flgn & flgsando);
		CFLG = flgsando | (flgn & flgsoro);
	}

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}
#endif

LOCALPROC NeedDefaultLazyFlagsAddL(void)
{
#if 1
	ui5r src = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r result = ui5r_FromSLong(dst + src);

	ZFLG = Bool2Bit(result == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(result));

	{
		ui5r result1;
		ui5r result0;
		ui5r MidCarry = (ui5r_FromUWord(dst)
			+ ui5r_FromUWord(src)) >> 16;

		dst >>= 16;
		src >>= 16;

		result1 = ui5r_FromUWord(dst)
			+ ui5r_FromUWord(src)
			+ MidCarry;
		CFLG = (result1 >> 16);
		result0 = ui5r_FromSWord(dst)
			+ ui5r_FromSWord(src)
			+ MidCarry;
		VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	}

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
#else
	ui5r result = ui5r_FromSLong(V_regs.LazyFlagArgDst
		+ V_regs.LazyFlagArgSrc);

	NeedDefaultLazyFlagsAddCommon(result);
#endif
}

LOCALPROC NeedDefaultLazyFlagsNegCommon(ui5r dstvalue, ui5r result)
{
	flagtype flgs = Bool2Bit(ui5r_MSBisSet(dstvalue));
	flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));

	ZFLG = Bool2Bit(result == 0);
	NFLG = flgn;
	VFLG = flgs & flgn;
	CFLG = flgs | flgn;

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsNegB(void)
{
	ui5r dstvalue = V_regs.LazyFlagArgDst;
	ui5r result = ui5r_FromSByte(0 - dstvalue);

	NeedDefaultLazyFlagsNegCommon(dstvalue, result);
}

LOCALPROC NeedDefaultLazyFlagsNegW(void)
{
	ui5r dstvalue = V_regs.LazyFlagArgDst;
	ui5r result = ui5r_FromSWord(0 - dstvalue);

	NeedDefaultLazyFlagsNegCommon(dstvalue, result);
}

LOCALPROC NeedDefaultLazyFlagsNegL(void)
{
	ui5r dstvalue = V_regs.LazyFlagArgDst;
	ui5r result = ui5r_FromSLong(0 - dstvalue);

	NeedDefaultLazyFlagsNegCommon(dstvalue, result);
}

LOCALPROC NeedDefaultLazyFlagsAsr(void)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;

	NFLG = Bool2Bit(ui5r_MSBisSet(dst));
	VFLG = 0;

	CFLG = ((dst >> (cnt - 1)) & 1);
	dst = Ui5rASR(dst, cnt);
	ZFLG = Bool2Bit(dst == 0);

	XFLG = CFLG;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsAslB(void)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r dstvalue0 = dst;
	ui5r comparevalue;

	dst = dst << (cnt - 1);
	dst = ui5r_FromSByte(dst);
	CFLG = Bool2Bit(ui5r_MSBisSet(dst));
	dst = dst << 1;
	dst = ui5r_FromSByte(dst);
	comparevalue = Ui5rASR(dst, cnt);
	VFLG = Bool2Bit(comparevalue != dstvalue0);
	ZFLG = Bool2Bit(dst == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(dst));

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsAslW(void)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r dstvalue0 = dst;
	ui5r comparevalue;

	dst = dst << (cnt - 1);
	dst = ui5r_FromSWord(dst);
	CFLG = Bool2Bit(ui5r_MSBisSet(dst));
	dst = dst << 1;
	dst = ui5r_FromSWord(dst);
	comparevalue = Ui5rASR(dst, cnt);
	VFLG = Bool2Bit(comparevalue != dstvalue0);
	ZFLG = Bool2Bit(dst == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(dst));

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

LOCALPROC NeedDefaultLazyFlagsAslL(void)
{
	ui5r cnt = V_regs.LazyFlagArgSrc;
	ui5r dst = V_regs.LazyFlagArgDst;
	ui5r dstvalue0 = dst;
	ui5r comparevalue;

	dst = dst << (cnt - 1);
	dst = ui5r_FromSLong(dst);
	CFLG = Bool2Bit(ui5r_MSBisSet(dst));
	dst = dst << 1;
	dst = ui5r_FromSLong(dst);
	comparevalue = Ui5rASR(dst, cnt);
	VFLG = Bool2Bit(comparevalue != dstvalue0);
	ZFLG = Bool2Bit(dst == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(dst));

	XFLG = CFLG;
	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}

#if UseLazyZ
FORWARDPROC NeedDefaultLazyFlagsZSet(void);
#endif

LOCALVAR const NeedLazyFlagP
	NeedLazyFlagDispatch[kNumLazyFlagsKinds + 1] =
{
	NeedDefaultLazyXFlag /* kLazyFlagsDefault */,
	0 /* kLazyFlagsTstB */,
	0 /* kLazyFlagsTstW */,
	NeedDefaultLazyFlagsTstL /* kLazyFlagsTstL */,
	NeedDefaultLazyFlagsCmpB /* kLazyFlagsCmpB */,
	NeedDefaultLazyFlagsCmpW /* kLazyFlagsCmpW */,
	NeedDefaultLazyFlagsCmpL /* kLazyFlagsCmpL */,
	NeedDefaultLazyFlagsSubB /* kLazyFlagsSubB */,
	NeedDefaultLazyFlagsSubW /* kLazyFlagsSubW */,
	NeedDefaultLazyFlagsSubL /* kLazyFlagsSubL */,
	NeedDefaultLazyFlagsAddB /* kLazyFlagsAddB */,
	NeedDefaultLazyFlagsAddW /* kLazyFlagsAddW */,
	NeedDefaultLazyFlagsAddL /* kLazyFlagsAddL */,
	NeedDefaultLazyFlagsNegB /* kLazyFlagsNegB */,
	NeedDefaultLazyFlagsNegW /* kLazyFlagsNegW */,
	NeedDefaultLazyFlagsNegL /* kLazyFlagsNegL */,
	NeedDefaultLazyFlagsAsr  /* kLazyFlagsAsrB */,
	NeedDefaultLazyFlagsAsr  /* kLazyFlagsAsrW */,
	NeedDefaultLazyFlagsAsr  /* kLazyFlagsAsrL */,
	NeedDefaultLazyFlagsAslB /* kLazyFlagsAslB */,
	NeedDefaultLazyFlagsAslW /* kLazyFlagsAslW */,
	NeedDefaultLazyFlagsAslL /* kLazyFlagsAslL */,
#if UseLazyZ
	NeedDefaultLazyFlagsZSet /* kLazyFlagsZSet */,
#endif

	0
};

LOCALPROC NeedDefaultLazyAllFlags0(void)
{
	(NeedLazyFlagDispatch[V_regs.LazyFlagKind])();
}

#if ForceFlagsEval
LOCALPROC NeedDefaultLazyAllFlags(void)
{
	if (kLazyFlagsDefault != V_regs.LazyFlagKind) {
		ReportAbnormalID(0x0104,
			"not kLazyFlagsDefault in NeedDefaultLazyAllFlags");
#if dbglog_HAVE
		dbglog_writelnNum("LazyFlagKind", V_regs.LazyFlagKind);
#endif
	}
}
#else
#define NeedDefaultLazyAllFlags NeedDefaultLazyAllFlags0
#endif

#if ForceFlagsEval
#define HaveSetUpFlags NeedDefaultLazyAllFlags0
#else
#define HaveSetUpFlags()
#endif

#if UseLazyZ
LOCALPROC NeedDefaultLazyFlagsZSet(void)
{
	flagtype SaveZFLG = ZFLG;

	V_regs.LazyFlagKind = V_regs.LazyFlagZSavedKind;
	NeedDefaultLazyAllFlags();

	ZFLG = SaveZFLG;
}
#endif

#if UseLazyCC
LOCALPROC my_reg_call cctrue_Dflt(cond_actP t_act, cond_actP f_act)
{
	NeedDefaultLazyAllFlags();
	cctrue(t_act, f_act);
}
#endif

#if ! UseLazyCC
LOCALINLINEPROC cctrue(cond_actP t_act, cond_actP f_act)
{
	NeedDefaultLazyAllFlags();
	(cctrueDispatch[V_regs.CurDecOpY.v[0].ArgDat])(t_act, f_act);
}
#endif


#define LOCALIPROC LOCALPROC /* LOCALPROCUSEDONCE */

LOCALIPROC DoCodeCmpB(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValue();

	V_regs.LazyFlagKind = kLazyFlagsCmpB;
	V_regs.LazyFlagArgSrc = V_regs.SrcVal;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();
}

LOCALIPROC DoCodeCmpW(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValue();

	V_regs.LazyFlagKind = kLazyFlagsCmpW;
	V_regs.LazyFlagArgSrc = V_regs.SrcVal;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();
}

LOCALIPROC DoCodeCmpL(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValue();

	V_regs.LazyFlagKind = kLazyFlagsCmpL;
	V_regs.LazyFlagArgSrc = V_regs.SrcVal;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();
}

LOCALIPROC DoCodeMoveL(void)
{
	ui5r src = DecodeGetSrcValue();

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = src;

	HaveSetUpFlags();

	DecodeSetDstValue(src);
}

LOCALIPROC DoCodeMoveW(void)
{
	ui5r src = DecodeGetSrcValue();

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = src;

	HaveSetUpFlags();

	DecodeSetDstValue(src);
}

LOCALIPROC DoCodeMoveB(void)
{
	ui5r src = DecodeGetSrcValue();

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = src;

	HaveSetUpFlags();

	DecodeSetDstValue(src);
}

LOCALIPROC DoCodeTst(void)
{
	/* Tst 01001010ssmmmrrr */

	ui5r srcvalue = DecodeGetDstValue();

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = srcvalue;

	HaveSetUpFlags();
}

LOCALIPROC DoCodeBraB(void)
{
	si5r offset = (si5r)(si3b)(ui3b)(V_regs.CurDecOpY.v[1].ArgDat);
	ui3p s = V_pc_p + offset;

	V_pc_p = s;

#if USE_PCLIMIT
	if (my_cond_rare(s >= V_pc_pHi)
		|| my_cond_rare(s < V_regs.pc_pLo))
	{
		Recalc_PC_Block();
	}
#endif
}

LOCALIPROC DoCodeBraW(void)
{
	si5r offset = (si5r)(si4b)(ui4b)do_get_mem_word(V_pc_p);
		/* note that pc not incremented here */
	ui3p s = V_pc_p + offset;

	V_pc_p = s;

#if USE_PCLIMIT
	if (my_cond_rare(s >= V_pc_pHi)
		|| my_cond_rare(s < V_regs.pc_pLo))
	{
		Recalc_PC_Block();
	}
#endif
}

#if WantCloserCyc
LOCALPROC DoCodeBccB_t(void)
{
	V_MaxCyclesToGo -= (10 * kCycleScale + 2 * RdAvgXtraCyc);
	DoCodeBraB();
}
#else
#define DoCodeBccB_t DoCodeBraB
#endif

LOCALPROC DoCodeBccB_f(void)
{
#if WantCloserCyc
	V_MaxCyclesToGo -= (8 * kCycleScale + RdAvgXtraCyc);
#endif
		/* do nothing */
}

LOCALIPROC DoCodeBccB(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	cctrue(DoCodeBccB_t, DoCodeBccB_f);
}

LOCALPROC SkipiWord(void)
{
	V_pc_p += 2;

#if USE_PCLIMIT
	if (my_cond_rare(V_pc_p >= V_pc_pHi)) {
		Recalc_PC_Block();
	}
#endif
}

#if WantCloserCyc
LOCALPROC DoCodeBccW_t(void)
{
	V_MaxCyclesToGo -= (10 * kCycleScale + 2 * RdAvgXtraCyc);
	DoCodeBraW();
}
#else
#define DoCodeBccW_t DoCodeBraW
#endif

#if WantCloserCyc
LOCALPROC DoCodeBccW_f(void)
{
	V_MaxCyclesToGo -= (12 * kCycleScale + 2 * RdAvgXtraCyc);
	SkipiWord();
}
#else
#define DoCodeBccW_f SkipiWord
#endif

LOCALIPROC DoCodeBccW(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	cctrue(DoCodeBccW_t, DoCodeBccW_f);
}


LOCALIPROC DoCodeDBF(void)
{
	/* DBcc 0101cccc11001ddd */

	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = ui5r_FromSWord(*dstp);

	--dstvalue;
#if LittleEndianUnaligned
	*(ui4b *)dstp = dstvalue;
#else
	*dstp = (*dstp & ~ 0xffff) | ((dstvalue) & 0xffff);
#endif

	if ((si5b)dstvalue == -1) {
#if WantCloserCyc
		V_MaxCyclesToGo -= (14 * kCycleScale + 3 * RdAvgXtraCyc);
#endif
		SkipiWord();
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (10 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
		DoCodeBraW();
	}
}

#if WantCloserCyc
LOCALPROC DoCodeDBcc_t(void)
{
	V_MaxCyclesToGo -= (12 * kCycleScale + 2 * RdAvgXtraCyc);
	SkipiWord();
}
#else
#define DoCodeDBcc_t SkipiWord
#endif

LOCALIPROC DoCodeDBcc(void)
{
	/* DBcc 0101cccc11001ddd */

	cctrue(DoCodeDBcc_t, DoCodeDBF);
}

LOCALIPROC DoCodeSwap(void)
{
	/* Swap 0100100001000rrr */
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r src = *dstp;
	ui5r dst = ui5r_FromSLong(((src >> 16) & 0xFFFF)
		| ((src & 0xFFFF) << 16));

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dst;

	HaveSetUpFlags();

	*dstp = dst;
}

LOCALIPROC DoCodeMoveA(void) /* MOVE */
{
	ui5r src = DecodeGetSrcValue();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;

	m68k_areg(dstreg) = src;
}

LOCALIPROC DoCodeMoveQ(void)
{
	/* MoveQ 0111ddd0nnnnnnnn */
	ui5r src = ui5r_FromSByte(V_regs.CurDecOpY.v[0].ArgDat);
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = src;

	HaveSetUpFlags();

	m68k_dreg(dstreg) = src;
}

LOCALIPROC DoCodeAddB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r srcvalue = V_regs.SrcVal;
	ui5r result = ui5r_FromSByte(dstvalue + srcvalue);

	V_regs.LazyFlagKind = kLazyFlagsAddB;
	V_regs.LazyFlagArgSrc = srcvalue;
	V_regs.LazyFlagArgDst = dstvalue;

	V_regs.LazyXFlagKind = kLazyFlagsAddB;
	V_regs.LazyXFlagArgSrc = srcvalue;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeAddW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r srcvalue = V_regs.SrcVal;
	ui5r result = ui5r_FromSWord(dstvalue + srcvalue);

	V_regs.LazyFlagKind = kLazyFlagsAddW;
	V_regs.LazyFlagArgSrc = srcvalue;
	V_regs.LazyFlagArgDst = dstvalue;

	V_regs.LazyXFlagKind = kLazyFlagsAddW;
	V_regs.LazyXFlagArgSrc = srcvalue;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeAddL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r srcvalue = V_regs.SrcVal;
	ui5r result = ui5r_FromSLong(dstvalue + srcvalue);

	V_regs.LazyFlagKind = kLazyFlagsAddL;
	V_regs.LazyFlagArgSrc = srcvalue;
	V_regs.LazyFlagArgDst = dstvalue;

	V_regs.LazyXFlagKind = kLazyFlagsAddL;
	V_regs.LazyXFlagArgSrc = srcvalue;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeSubB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r srcvalue = V_regs.SrcVal;
	ui5r result = ui5r_FromSByte(dstvalue - srcvalue);

	V_regs.LazyFlagKind = kLazyFlagsSubB;
	V_regs.LazyFlagArgSrc = srcvalue;
	V_regs.LazyFlagArgDst = dstvalue;

	V_regs.LazyXFlagKind = kLazyFlagsSubB;
	V_regs.LazyXFlagArgSrc = srcvalue;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeSubW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r srcvalue = V_regs.SrcVal;
	ui5r result = ui5r_FromSWord(dstvalue - srcvalue);

	V_regs.LazyFlagKind = kLazyFlagsSubW;
	V_regs.LazyFlagArgSrc = srcvalue;
	V_regs.LazyFlagArgDst = dstvalue;

	V_regs.LazyXFlagKind = kLazyFlagsSubW;
	V_regs.LazyXFlagArgSrc = srcvalue;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeSubL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r srcvalue = V_regs.SrcVal;
	ui5r result = ui5r_FromSLong(dstvalue - srcvalue);

	V_regs.LazyFlagKind = kLazyFlagsSubL;
	V_regs.LazyFlagArgSrc = srcvalue;
	V_regs.LazyFlagArgDst = dstvalue;

	V_regs.LazyXFlagKind = kLazyFlagsSubL;
	V_regs.LazyXFlagArgSrc = srcvalue;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeLea(void)
{
	/* Lea 0100aaa111mmmrrr */
	ui5r DstAddr = DecodeDst();
	ui5r dstreg = V_regs.CurDecOpY.v[0].ArgDat;

	m68k_areg(dstreg) = DstAddr;
}

LOCALIPROC DoCodePEA(void)
{
	/* PEA 0100100001mmmrrr */
	ui5r DstAddr = DecodeDst();

	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), DstAddr);
}

LOCALIPROC DoCodeBsrB(void)
{
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
	DoCodeBraB();
}

LOCALIPROC DoCodeBsrW(void)
{
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc() + 2);
	DoCodeBraW();
}

#define m68k_logExceptions (dbglog_HAVE && 0)


#ifndef WantDumpAJump
#define WantDumpAJump 0
#endif

#if WantDumpAJump
LOCALPROCUSEDONCE DumpAJump(CPTR toaddr)
{
	CPTR fromaddr = m68k_getpc();
	if ((toaddr > fromaddr) || (toaddr < V_regs.pc))
	{
		dbglog_writeHex(fromaddr);
		dbglog_writeCStr(",");
		dbglog_writeHex(toaddr);
		dbglog_writeReturn();
	}
}
#endif

LOCALPROC my_reg_call m68k_setpc(CPTR newpc)
{
#if WantDumpAJump
	DumpAJump(newpc);
#endif

#if 0
	if (newpc == 0xBD50 /* 401AB4 */) {
		/* Debugger(); */
		/* Exception(5); */ /* try and get macsbug */
	}
#endif

	V_pc_p = V_regs.pc_pLo + (newpc - V_regs.pc);
	if (my_cond_rare(V_pc_p >= V_pc_pHi)
		|| my_cond_rare(V_pc_p < V_regs.pc_pLo))
	{
		Recalc_PC_Block();
	}
}

LOCALIPROC DoCodeJsr(void)
{
	/* Jsr 0100111010mmmrrr */
	ui5r DstAddr = DecodeDst();

	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
	m68k_setpc(DstAddr);
}

LOCALIPROC DoCodeLinkA6(void)
{
	CPTR stackp = m68k_areg(7);
	stackp -= 4;
	put_long(stackp, m68k_areg(6));
	m68k_areg(6) = stackp;
	m68k_areg(7) = stackp + nextiSWord();
}

LOCALIPROC DoCodeUnlkA6(void)
{
	ui5r src = m68k_areg(6);
	m68k_areg(6) = get_long(src);
	m68k_areg(7) = src + 4;
}

LOCALIPROC DoCodeRts(void)
{
	/* Rts 0100111001110101 */
	ui5r NewPC = get_long(m68k_areg(7));
	m68k_areg(7) += 4;
	m68k_setpc(NewPC);
}

LOCALIPROC DoCodeJmp(void)
{
	/* JMP 0100111011mmmrrr */
	ui5r DstAddr = DecodeDst();

	m68k_setpc(DstAddr);
}

LOCALIPROC DoCodeClr(void)
{
	/* Clr 01000010ssmmmrrr */

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = 0;

	HaveSetUpFlags();

	DecodeSetDstValue(0);
}

LOCALIPROC DoCodeAddA(void)
{
	/* ADDA 1101dddm11mmmrrr */
	ui5r dstvalue = DecodeGetSrcSetDstValue();

	ArgSetDstValue(dstvalue + V_regs.SrcVal);
}

LOCALIPROC DoCodeSubA(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();

	ArgSetDstValue(dstvalue - V_regs.SrcVal);
}

LOCALIPROC DoCodeCmpA(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValue();

	V_regs.LazyFlagKind = kLazyFlagsCmpL;
	V_regs.LazyFlagArgSrc = V_regs.SrcVal;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();
}

LOCALFUNC ui4rr m68k_getCR(void)
{
	NeedDefaultLazyAllFlags();

	return (XFLG << 4) | (NFLG << 3) | (ZFLG << 2)
		| (VFLG << 1) | CFLG;
}

LOCALPROC my_reg_call m68k_setCR(ui4rr newcr)
{
	XFLG = (newcr >> 4) & 1;
	NFLG = (newcr >> 3) & 1;
	ZFLG = (newcr >> 2) & 1;
	VFLG = (newcr >> 1) & 1;
	CFLG = newcr & 1;

	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;
}


LOCALFUNC ui4rr m68k_getSR(void)
{
	return m68k_getCR()
			| (V_regs.t1 << 15)
#if Use68020
			| (V_regs.t0 << 14)
#endif
			| (V_regs.s << 13)
#if Use68020
			| (V_regs.m << 12)
#endif
			| (V_regs.intmask << 8);
}

LOCALPROC NeedToGetOut(void)
{
	if (V_MaxCyclesToGo <= 0) {
		/*
			already have gotten out, and exception processing has
			caused another exception, such as because a bad
			stack pointer pointing to a memory mapped device.
		*/
	} else {
		V_regs.MoreCyclesToGo += V_MaxCyclesToGo;
			/* not counting the current instruction */
		V_MaxCyclesToGo = 0;
	}
}

LOCALPROC SetExternalInterruptPending(void)
{
	V_regs.ExternalInterruptPending = trueblnr;
	NeedToGetOut();
}

LOCALPROC my_reg_call m68k_setSR(ui4rr newsr)
{
	CPTR *pnewstk;
	CPTR *poldstk = (V_regs.s != 0) ? (
#if Use68020
		(V_regs.m != 0) ? &V_regs.msp :
#endif
		&V_regs.isp) : &V_regs.usp;
	ui5r oldintmask = V_regs.intmask;

	V_regs.t1 = (newsr >> 15) & 1;
#if Use68020
	V_regs.t0 = (newsr >> 14) & 1;
	if (V_regs.t0 != 0) {
		ReportAbnormalID(0x0105, "t0 flag set in m68k_setSR");
	}
#endif
	V_regs.s = (newsr >> 13) & 1;
#if Use68020
	V_regs.m = (newsr >> 12) & 1;
	if (V_regs.m != 0) {
		ReportAbnormalID(0x0106, "m flag set in m68k_setSR");
	}
#endif
	V_regs.intmask = (newsr >> 8) & 7;

	pnewstk = (V_regs.s != 0) ? (
#if Use68020
		(V_regs.m != 0) ? &V_regs.msp :
#endif
		&V_regs.isp) : &V_regs.usp;

	if (poldstk != pnewstk) {
		*poldstk = m68k_areg(7);
		m68k_areg(7) = *pnewstk;
	}

	if (V_regs.intmask != oldintmask) {
		SetExternalInterruptPending();
	}

	if (V_regs.t1 != 0) {
		NeedToGetOut();
	} else {
		/* V_regs.TracePending = falseblnr; */
	}

	m68k_setCR(newsr);
}

LOCALPROC my_reg_call ExceptionTo(CPTR newpc
#if Use68020
	, int nr
#endif
	)
{
	ui4rr saveSR = m68k_getSR();

	if (0 == V_regs.s) {
		V_regs.usp = m68k_areg(7);
		m68k_areg(7) =
#if Use68020
			(V_regs.m != 0) ? V_regs.msp :
#endif
			V_regs.isp;
		V_regs.s = 1;
	}
#if Use68020
	switch (nr) {
		case 5: /* Zero Divide */
		case 6: /* CHK, CHK2 */
		case 7: /* cpTRAPcc, TRAPCcc, TRAPv */
		case 9: /* Trace */
			m68k_areg(7) -= 4;
			put_long(m68k_areg(7), m68k_getpc());
			m68k_areg(7) -= 2;
			put_word(m68k_areg(7), 0x2000 + nr * 4);
			break;
		default:
			m68k_areg(7) -= 2;
			put_word(m68k_areg(7), nr * 4);
			break;
	}
	/* if V_regs.m should make throw away stack frame */
#endif
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
	m68k_areg(7) -= 2;
	put_word(m68k_areg(7), saveSR);
	m68k_setpc(newpc);
	V_regs.t1 = 0;
#if Use68020
	V_regs.t0 = 0;
	V_regs.m = 0;
#endif
	V_regs.TracePending = falseblnr;
}

LOCALPROC my_reg_call Exception(int nr)
{
	ExceptionTo(get_long(4 * nr
#if Use68020
		+ V_regs.vbr
#endif
		)
#if Use68020
		, nr
#endif
		);
}


LOCALIPROC DoCodeA(void)
{
	BackupPC();
	Exception(0xA);
}

LOCALFUNC ui4rr nextiword_nm(void)
/* NOT sign extended */
{
	return nextiword();
}

LOCALIPROC DoCodeMOVEMRmML(void)
{
	/* MOVEM reg to mem 01001000111100rrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r p = *dstp;

#if Use68020
	{
		int n = 0;

		for (z = 0; z < 16; ++z) {
			if ((regmask & (1 << z)) != 0) {
				n++;
			}
		}
		*dstp = p - n * 4;
	}
#endif
	for (z = 16; --z >= 0; ) {
		if ((regmask & (1 << (15 - z))) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -= (8 * kCycleScale + 2 * WrAvgXtraCyc);
#endif
			p -= 4;
			put_long(p, V_regs.regs[z]);
		}
	}
#if ! Use68020
	*dstp = p;
#endif
}

LOCALIPROC DoCodeMOVEMApRL(void)
{
	/* MOVEM mem to reg 01001100111011rrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r p = *dstp;

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -= (8 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
			V_regs.regs[z] = get_long(p);
			p += 4;
		}
	}
	*dstp = p;
}

LOCALPROC my_reg_call SetCCRforAddX(ui5r dstvalue, ui5r srcvalue,
	ui5r result)
{
	ZFLG &= Bool2Bit(result == 0);

	{
		flagtype flgs = Bool2Bit(ui5r_MSBisSet(srcvalue));
		flagtype flgo = Bool2Bit(ui5r_MSBisSet(dstvalue));
		flagtype flgsando = flgs & flgo;
		flagtype flgsoro = flgs | flgo;
		flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));

		NFLG = flgn;
		flgn ^= 1;
		VFLG = ((flgn | flgsoro) ^ 1) | (flgn & flgsando);
		XFLG = CFLG = flgsando | (flgn & flgsoro);
	}

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeAddXB(void)
{
	NeedDefaultLazyAllFlags();

	{
		ui5r dstvalue = DecodeGetSrcSetDstValue();
		ui5r srcvalue = V_regs.SrcVal;
		ui5r result = ui5r_FromSByte(XFLG + dstvalue + srcvalue);

		SetCCRforAddX(dstvalue, srcvalue, result);
	}
}

LOCALIPROC DoCodeAddXW(void)
{
	if ((kLazyFlagsDefault != V_regs.LazyFlagKind)
		|| (kLazyFlagsDefault != V_regs.LazyXFlagKind))
	{
		NeedDefaultLazyAllFlags();
	}

	{
		ui5r dstvalue = DecodeGetSrcSetDstValue();
		ui5r srcvalue = V_regs.SrcVal;
		ui5r result = ui5r_FromSWord(XFLG + dstvalue + srcvalue);

		SetCCRforAddX(dstvalue, srcvalue, result);
	}
}

LOCALIPROC DoCodeAddXL(void)
{
	if (kLazyFlagsAddL == V_regs.LazyFlagKind) {
		ui5r src = V_regs.LazyFlagArgSrc;
		ui5r dst = V_regs.LazyFlagArgDst;
		ui5r result = ui5r_FromULong(dst + src);

		ZFLG = Bool2Bit(result == 0);
		XFLG = Bool2Bit(result < src);

		V_regs.LazyFlagKind = kLazyFlagsDefault;
		V_regs.LazyXFlagKind = kLazyFlagsDefault;
	} else
	if ((kLazyFlagsDefault == V_regs.LazyFlagKind)
		&& (kLazyFlagsDefault == V_regs.LazyXFlagKind))
	{
		/* ok */
	} else
	{
		NeedDefaultLazyAllFlags();
	}

	{
		ui5r dstvalue = DecodeGetSrcSetDstValue();
		ui5r srcvalue = V_regs.SrcVal;
		ui5r result = ui5r_FromSLong(XFLG + dstvalue + srcvalue);

		SetCCRforAddX(dstvalue, srcvalue, result);
	}
}

LOCALPROC my_reg_call SetCCRforSubX(ui5r dstvalue, ui5r srcvalue,
	ui5r result)
{
	ZFLG &= Bool2Bit(result == 0);

	{
		flagtype flgs = Bool2Bit(ui5r_MSBisSet(srcvalue));
		flagtype flgo = Bool2Bit(ui5r_MSBisSet(dstvalue)) ^ 1;
		flagtype flgsando = flgs & flgo;
		flagtype flgsoro = flgs | flgo;
		flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));

		NFLG = flgn;
		VFLG = ((flgn | flgsoro) ^ 1) | (flgn & flgsando);
		XFLG = CFLG = flgsando | (flgn & flgsoro);
	}

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeSubXB(void)
{
	NeedDefaultLazyAllFlags();

	{
		ui5r dstvalue = DecodeGetSrcSetDstValue();
		ui5r srcvalue = V_regs.SrcVal;
		ui5r result = ui5r_FromSByte(dstvalue - srcvalue - XFLG);

		SetCCRforSubX(dstvalue, srcvalue, result);
	}
}

LOCALIPROC DoCodeSubXW(void)
{
	if ((kLazyFlagsDefault != V_regs.LazyFlagKind)
		|| (kLazyFlagsDefault != V_regs.LazyXFlagKind))
	{
		NeedDefaultLazyAllFlags();
	}

	{
		ui5r dstvalue = DecodeGetSrcSetDstValue();
		ui5r srcvalue = V_regs.SrcVal;
		ui5r result = ui5r_FromSWord(dstvalue - srcvalue - XFLG);

		SetCCRforSubX(dstvalue, srcvalue, result);
	}
}

LOCALIPROC DoCodeSubXL(void)
{
	if (kLazyFlagsSubL == V_regs.LazyFlagKind) {
		ui5r src = V_regs.LazyFlagArgSrc;
		ui5r dst = V_regs.LazyFlagArgDst;
		ui5r result = ui5r_FromSLong(dst - src);

		ZFLG = Bool2Bit(result == 0);
		XFLG = Bool2Bit(((ui5b)dst) < ((ui5b)src));

		V_regs.LazyFlagKind = kLazyFlagsDefault;
		V_regs.LazyXFlagKind = kLazyFlagsDefault;
	} else
	if ((kLazyFlagsDefault == V_regs.LazyFlagKind)
		&& (kLazyFlagsDefault == V_regs.LazyXFlagKind))
	{
		/* ok */
	} else
	{
		NeedDefaultLazyAllFlags();
	}

	{
		ui5r dstvalue = DecodeGetSrcSetDstValue();
		ui5r srcvalue = V_regs.SrcVal;
		ui5r result = ui5r_FromSLong(dstvalue - srcvalue - XFLG);

		SetCCRforSubX(dstvalue, srcvalue, result);
	}
}

LOCALPROC my_reg_call DoCodeNullShift(ui5r dstvalue)
{
	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(dstvalue);
}

LOCALPROC DoCodeOverAsl(ui5r dstvalue)
{
	XFLG = CFLG = 0;
	VFLG = Bool2Bit(0 != dstvalue);
	ZFLG = 1;
	NFLG = 0;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
	V_regs.LazyFlagKind = kLazyFlagsDefault;

	ArgSetDstValue(0);
}

LOCALPROC my_reg_call DoCodeMaxAsr(ui5r dstvalue)
{
	XFLG = CFLG = dstvalue & 1;
	VFLG = Bool2Bit(0 != dstvalue);
	ZFLG = 1;
	NFLG = 0;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
	V_regs.LazyFlagKind = kLazyFlagsDefault;

	ArgSetDstValue(0);
}

LOCALIPROC DoCodeAslB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 8) {
			if (cnt == 8) {
				DoCodeMaxAsr(dstvalue);
			} else {
				DoCodeOverAsl(dstvalue);
			}
		} else {
			ui5r result = ui5r_FromSByte(dstvalue << cnt);

			V_regs.LazyFlagKind = kLazyFlagsAslB;
			V_regs.LazyFlagArgSrc = cnt;
			V_regs.LazyFlagArgDst = dstvalue;

			V_regs.LazyXFlagKind = kLazyFlagsAslB;
			V_regs.LazyXFlagArgSrc = cnt;
			V_regs.LazyXFlagArgDst = dstvalue;

			HaveSetUpFlags();

			ArgSetDstValue(result);
		}
	}
}

LOCALIPROC DoCodeAslW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 16) {
			if (cnt == 16) {
				DoCodeMaxAsr(dstvalue);
			} else {
				DoCodeOverAsl(dstvalue);
			}
		} else {
			ui5r result = ui5r_FromSWord(dstvalue << cnt);

			V_regs.LazyFlagKind = kLazyFlagsAslW;
			V_regs.LazyFlagArgSrc = cnt;
			V_regs.LazyFlagArgDst = dstvalue;

			V_regs.LazyXFlagKind = kLazyFlagsAslW;
			V_regs.LazyXFlagArgSrc = cnt;
			V_regs.LazyXFlagArgDst = dstvalue;

			HaveSetUpFlags();

			ArgSetDstValue(result);
		}
	}
}

LOCALIPROC DoCodeAslL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 32) {
			if (cnt == 32) {
				DoCodeMaxAsr(dstvalue);
			} else {
				DoCodeOverAsl(dstvalue);
			}
		} else {
			ui5r result = ui5r_FromSLong(dstvalue << cnt);

			V_regs.LazyFlagKind = kLazyFlagsAslL;
			V_regs.LazyFlagArgSrc = cnt;
			V_regs.LazyFlagArgDst = dstvalue;

			V_regs.LazyXFlagKind = kLazyFlagsAslL;
			V_regs.LazyXFlagArgSrc = cnt;
			V_regs.LazyXFlagArgDst = dstvalue;

			HaveSetUpFlags();

			ArgSetDstValue(result);
		}
	}
}

LOCALPROC DoCodeOverShift(void)
{
	XFLG = CFLG = 0;
	ZFLG = 1;
	NFLG = 0;
	VFLG = 0;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
	V_regs.LazyFlagKind = kLazyFlagsDefault;

	ArgSetDstValue(0);
}

LOCALPROC DoCodeOverShiftN(void)
{
	NFLG = 1;
	VFLG = 0;
	CFLG = 1;
	XFLG = CFLG;
	ZFLG = 0;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
	V_regs.LazyFlagKind = kLazyFlagsDefault;

	ArgSetDstValue(~ 0);
}

LOCALPROC DoCodeOverAShift(ui5r dstvalue)
{
	if (ui5r_MSBisSet(dstvalue)) {
		DoCodeOverShiftN();
	} else {
		DoCodeOverShift();
	}
}

LOCALIPROC DoCodeAsrB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 8) {
			DoCodeOverAShift(dstvalue);
		} else {
			ui5r result = Ui5rASR(dstvalue, cnt);

			V_regs.LazyFlagKind = kLazyFlagsAsrB;
			V_regs.LazyFlagArgSrc = cnt;
			V_regs.LazyFlagArgDst = dstvalue;

			V_regs.LazyXFlagKind = kLazyFlagsAsrB;
			V_regs.LazyXFlagArgSrc = cnt;
			V_regs.LazyXFlagArgDst = dstvalue;

			HaveSetUpFlags();

			ArgSetDstValue(result);
		}
	}
}

LOCALIPROC DoCodeAsrW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 16) {
			DoCodeOverAShift(dstvalue);
		} else {
			ui5r result = Ui5rASR(dstvalue, cnt);

			V_regs.LazyFlagKind = kLazyFlagsAsrW;
			V_regs.LazyFlagArgSrc = cnt;
			V_regs.LazyFlagArgDst = dstvalue;

			V_regs.LazyXFlagKind = kLazyFlagsAsrW;
			V_regs.LazyXFlagArgSrc = cnt;
			V_regs.LazyXFlagArgDst = dstvalue;

			HaveSetUpFlags();

			ArgSetDstValue(result);
		}
	}
}

LOCALIPROC DoCodeAsrL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 32) {
			DoCodeOverAShift(dstvalue);
		} else {
			ui5r result = Ui5rASR(dstvalue, cnt);

			V_regs.LazyFlagKind = kLazyFlagsAsrL;
			V_regs.LazyFlagArgSrc = cnt;
			V_regs.LazyFlagArgDst = dstvalue;

			V_regs.LazyXFlagKind = kLazyFlagsAsrL;
			V_regs.LazyXFlagArgSrc = cnt;
			V_regs.LazyXFlagArgDst = dstvalue;

			HaveSetUpFlags();

			ArgSetDstValue(result);
		}
	}
}

LOCALPROC my_reg_call DoCodeMaxLslShift(ui5r dstvalue)
{
	XFLG = CFLG = dstvalue & 1;
	ZFLG = 1;
	NFLG = 0;
	VFLG = 0;

	V_regs.LazyXFlagKind = kLazyFlagsDefault;
	V_regs.LazyFlagKind = kLazyFlagsDefault;

	ArgSetDstValue(0);
}

LOCALIPROC DoCodeLslB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 8) {
			if (cnt == 8) {
				DoCodeMaxLslShift(dstvalue);
			} else {
				DoCodeOverShift();
			}
		} else {
			CFLG = (dstvalue >> (8 - cnt)) & 1;
			dstvalue = dstvalue << cnt;
			dstvalue = ui5r_FromSByte(dstvalue);

			ZFLG = Bool2Bit(dstvalue == 0);
			NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
			VFLG = 0;
			XFLG = CFLG;
			V_regs.LazyXFlagKind = kLazyFlagsDefault;
			V_regs.LazyFlagKind = kLazyFlagsDefault;

			ArgSetDstValue(dstvalue);
		}
	}
}

LOCALIPROC DoCodeLslW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 16) {
			if (cnt == 16) {
				DoCodeMaxLslShift(dstvalue);
			} else {
				DoCodeOverShift();
			}
		} else {
			CFLG = (dstvalue >> (16 - cnt)) & 1;
			dstvalue = dstvalue << cnt;
			dstvalue = ui5r_FromSWord(dstvalue);

			ZFLG = Bool2Bit(dstvalue == 0);
			NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
			VFLG = 0;
			XFLG = CFLG;
			V_regs.LazyXFlagKind = kLazyFlagsDefault;
			V_regs.LazyFlagKind = kLazyFlagsDefault;

			ArgSetDstValue(dstvalue);
		}
	}
}

LOCALIPROC DoCodeLslL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		if (cnt >= 32) {
			if (cnt == 32) {
				DoCodeMaxLslShift(dstvalue);
			} else {
				DoCodeOverShift();
			}
		} else {
			CFLG = (dstvalue >> (32 - cnt)) & 1;
			dstvalue = dstvalue << cnt;
			dstvalue = ui5r_FromSLong(dstvalue);

			ZFLG = Bool2Bit(dstvalue == 0);
			NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
			VFLG = 0;
			XFLG = CFLG;
			V_regs.LazyXFlagKind = kLazyFlagsDefault;
			V_regs.LazyFlagKind = kLazyFlagsDefault;

			ArgSetDstValue(dstvalue);
		}
	}
}

LOCALIPROC DoCodeLsrB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else if (cnt > 32) {
		DoCodeOverShift();
	} else {
		dstvalue = ui5r_FromUByte(dstvalue);
		dstvalue = dstvalue >> (cnt - 1);
		CFLG = XFLG = (dstvalue & 1);
		dstvalue = dstvalue >> 1;
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = 0 /* Bool2Bit(ui5r_MSBisSet(dstvalue)) */;
			/* if cnt != 0, always false */
		VFLG = 0;
		V_regs.LazyXFlagKind = kLazyFlagsDefault;
		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeLsrW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else if (cnt > 32) {
		DoCodeOverShift();
	} else {
		dstvalue = ui5r_FromUWord(dstvalue);
		dstvalue = dstvalue >> (cnt - 1);
		CFLG = XFLG = (dstvalue & 1);
		dstvalue = dstvalue >> 1;
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = 0 /* Bool2Bit(ui5r_MSBisSet(dstvalue)) */;
			/* if cnt != 0, always false */
		VFLG = 0;
		V_regs.LazyXFlagKind = kLazyFlagsDefault;
		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeLsrL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else if (cnt > 32) {
		DoCodeOverShift();
	} else {
		dstvalue = ui5r_FromULong(dstvalue);
		dstvalue = dstvalue >> (cnt - 1);
		CFLG = XFLG = (dstvalue & 1);
		dstvalue = dstvalue >> 1;
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = 0 /* Bool2Bit(ui5r_MSBisSet(dstvalue)) */;
			/* if cnt != 0, always false */
		VFLG = 0;
		V_regs.LazyXFlagKind = kLazyFlagsDefault;
		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALFUNC ui5r DecodeGetSrcSetDstValueDfltFlags_nm(void)
{
	NeedDefaultLazyAllFlags();

	return DecodeGetSrcSetDstValue();
}

LOCALPROC my_reg_call DoCodeNullXShift(ui5r dstvalue)
{
	CFLG = XFLG;

	ZFLG = Bool2Bit(dstvalue == 0);
	NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
	VFLG = 0;

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeRxlB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullXShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		for (; cnt; --cnt) {
			CFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
			dstvalue = (dstvalue << 1) | XFLG;
			dstvalue = ui5r_FromSByte(dstvalue);
			XFLG = CFLG;
		}

		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRxlW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullXShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		for (; cnt; --cnt) {
			CFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
			dstvalue = (dstvalue << 1) | XFLG;
			dstvalue = ui5r_FromSWord(dstvalue);
			XFLG = CFLG;
		}

		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRxlL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullXShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		for (; cnt; --cnt) {
			CFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
			dstvalue = (dstvalue << 1) | XFLG;
			dstvalue = ui5r_FromSLong(dstvalue);
			XFLG = CFLG;
		}

		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRxrB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullXShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		dstvalue = ui5r_FromUByte(dstvalue);
		for (; cnt; --cnt) {
			CFLG = dstvalue & 1;
			dstvalue = (dstvalue >> 1) | (((ui5r)XFLG) << 7);
			XFLG = CFLG;
		}
		dstvalue = ui5r_FromSByte(dstvalue);

		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRxrW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullXShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		dstvalue = ui5r_FromUWord(dstvalue);
		for (; cnt; --cnt) {
			CFLG = dstvalue & 1;
			dstvalue = (dstvalue >> 1) | (((ui5r)XFLG) << 15);
			XFLG = CFLG;
		}
		dstvalue = ui5r_FromSWord(dstvalue);

		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRxrL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r cnt = V_regs.SrcVal & 63;

	if (0 == cnt) {
		DoCodeNullXShift(dstvalue);
	} else {
#if WantCloserCyc
		V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

		dstvalue = ui5r_FromULong(dstvalue);
		for (; cnt; --cnt) {
			CFLG = dstvalue & 1;
			dstvalue = (dstvalue >> 1) | (((ui5r)XFLG) << 31);
			XFLG = CFLG;
		}
		dstvalue = ui5r_FromSLong(dstvalue);

		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRolB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
		cnt &= 7;
		if (0 != cnt) {
			ui3b dst = (ui3b)dstvalue;

			dst = (dst >> (8 - cnt))
					| ((dst & ((1 << (8 - cnt)) - 1))
						<< cnt);

			dstvalue = (ui5r)(si5r)(si3b)dst;
		}
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;
		CFLG = (dstvalue & 1);
		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRolW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
		cnt &= 15;
		if (0 != cnt) {
			ui4b dst = (ui4b)dstvalue;

			dst = (dst >> (16 - cnt))
					| ((dst & ((1 << (16 - cnt)) - 1))
						<< cnt);

			dstvalue = (ui5r)(si5r)(si4b)dst;
		}
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;
		CFLG = (dstvalue & 1);
		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRolL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
		cnt &= 31;
		if (0 != cnt) {
			ui5b dst = (ui5b)dstvalue;

			dst = (dst >> (32 - cnt))
					| ((dst & ((1 << (32 - cnt)) - 1))
						<< cnt);

			dstvalue = (ui5r)(si5r)(si5b)dst;
		}
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;
		CFLG = (dstvalue & 1);
		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRorB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
		cnt &= 7;
		if (0 != cnt) {
			ui3b dst = (ui3b)dstvalue;

			dst = (dst >> cnt)
					| ((dst & ((1 << cnt) - 1))
						<< (8 - cnt));

			dstvalue = (ui5r)(si5r)(si3b)dst;
		}
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;
		CFLG = NFLG;

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRorW(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
		cnt &= 15;
		if (0 != cnt) {
			ui4b dst = (ui4b)dstvalue;

			dst = (dst >> cnt)
					| ((dst & ((1 << cnt) - 1))
						<< (16 - cnt));

			dstvalue = (ui5r)(si5r)(si4b)dst;
		}
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;
		CFLG = NFLG;

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}

LOCALIPROC DoCodeRorL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValue();
	ui5r cnt = V_regs.SrcVal & 63;

#if WantCloserCyc
	V_MaxCyclesToGo -= (cnt * 2 * kCycleScale);
#endif

	if (0 == cnt) {
		DoCodeNullShift(dstvalue);
	} else {
		cnt &= 31;
		if (0 != cnt) {
			ui5b dst = (ui5b)dstvalue;

			dst = (dst >> cnt)
					| ((dst & ((1 << cnt) - 1))
						<< (32 - cnt));

			dstvalue = (ui5r)(si5r)(si5b)dst;
		}
		ZFLG = Bool2Bit(dstvalue == 0);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = 0;
		CFLG = NFLG;

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		ArgSetDstValue(dstvalue);
	}
}


#if UseLazyZ
LOCALPROC WillSetZFLG(void)
{
	if (kLazyFlagsZSet == V_regs.LazyFlagKind) {
		/* ok */
	} else if (kLazyFlagsDefault == V_regs.LazyFlagKind) {
		/* also ok */
	} else {
		V_regs.LazyFlagZSavedKind = V_regs.LazyFlagKind;
		V_regs.LazyFlagKind = kLazyFlagsZSet;
	}
}
#else
#define WillSetZFLG NeedDefaultLazyAllFlags
#endif

LOCALINLINEFUNC ui5r DecodeGetSrcGetDstValueSetZ(void)
{
	WillSetZFLG();

	return DecodeGetSrcSetDstValue();
}

LOCALIPROC DoCodeBTstB(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 7;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;
}

LOCALIPROC DoCodeBTstL(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 31;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;
}

LOCALINLINEFUNC ui5r DecodeGetSrcSetDstValueSetZ(void)
{
	WillSetZFLG();

	return DecodeGetSrcSetDstValue();
}

LOCALIPROC DoCodeBChgB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 7;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;

	dstvalue ^= (1 << srcvalue);
	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeBChgL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 31;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;

	dstvalue ^= (1 << srcvalue);
	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeBClrB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 7;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;

	dstvalue &= ~ (1 << srcvalue);
	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeBClrL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 31;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;

	dstvalue &= ~ (1 << srcvalue);
	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeBSetB(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 7;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;

	dstvalue |= (1 << srcvalue);
	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeBSetL(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueSetZ();
	ui5r srcvalue = V_regs.SrcVal & 31;

	ZFLG = ((dstvalue >> srcvalue) ^ 1) & 1;

	dstvalue |= (1 << srcvalue);
	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeAnd(void)
{
	/* DoBinOpAnd(DecodeI_xxxxxxxxssmmmrrr()); */
	ui5r dstvalue = DecodeGetSrcSetDstValue();

	dstvalue &= V_regs.SrcVal;
		/*
			don't need to extend, since excess high
			bits all the same as desired high bit.
		*/

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeOr(void)
{
	/* DoBinOr(DecodeI_xxxxxxxxssmmmrrr()); */
	ui5r dstvalue = DecodeGetSrcSetDstValue();

	dstvalue |= V_regs.SrcVal;
		/*
			don't need to extend, since excess high
			bits all the same as desired high bit.
		*/

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeEor(void)
{
	/* Eor 1011ddd1ssmmmrrr */
	/* DoBinOpEor(DecodeDEa_xxxxdddxssmmmrrr()); */
	ui5r dstvalue = DecodeGetSrcSetDstValue();

	dstvalue ^= V_regs.SrcVal;
		/*
			don't need to extend, since excess high
			bits all the same as desired high bit.
		*/

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeNot(void)
{
	/* Not 01000110ssmmmrrr */
	ui5r dstvalue = DecodeGetSetDstValue();

	dstvalue = ~ dstvalue;

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(dstvalue);
}

LOCALPROC DoCodeScc_t(void)
{
#if WantCloserCyc
	if (kAMdRegB == V_regs.CurDecOpY.v[1].AMd) {
		V_MaxCyclesToGo -= (2 * kCycleScale);
	}
#endif
	DecodeSetDstValue(0xff);
}

LOCALPROC DoCodeScc_f(void)
{
	DecodeSetDstValue(0);
}

LOCALIPROC DoCodeScc(void)
{
	/* Scc 0101cccc11mmmrrr */
	cctrue(DoCodeScc_t, DoCodeScc_f);
}

LOCALIPROC DoCodeEXTL(void)
{
	/* EXT.L */
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = ui5r_FromSWord(*dstp);

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	*dstp = dstvalue;
}

LOCALIPROC DoCodeEXTW(void)
{
	/* EXT.W */
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = ui5r_FromSByte(*dstp);

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

#if LittleEndianUnaligned
	*(ui4b *)dstp = dstvalue;
#else
	*dstp = (*dstp & ~ 0xffff) | (dstvalue & 0xffff);
#endif
}

LOCALIPROC DoCodeNegB(void)
{
	ui5r dstvalue = DecodeGetSetDstValue();
	ui5r result = ui5r_FromSByte(0 - dstvalue);

	V_regs.LazyFlagKind = kLazyFlagsNegB;
	V_regs.LazyFlagArgDst = dstvalue;
	V_regs.LazyXFlagKind = kLazyFlagsNegB;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeNegW(void)
{
	ui5r dstvalue = DecodeGetSetDstValue();
	ui5r result = ui5r_FromSWord(0 - dstvalue);

	V_regs.LazyFlagKind = kLazyFlagsNegW;
	V_regs.LazyFlagArgDst = dstvalue;
	V_regs.LazyXFlagKind = kLazyFlagsNegW;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeNegL(void)
{
	ui5r dstvalue = DecodeGetSetDstValue();
	ui5r result = ui5r_FromSLong(0 - dstvalue);

	V_regs.LazyFlagKind = kLazyFlagsNegL;
	V_regs.LazyFlagArgDst = dstvalue;
	V_regs.LazyXFlagKind = kLazyFlagsNegL;
	V_regs.LazyXFlagArgDst = dstvalue;

	HaveSetUpFlags();

	ArgSetDstValue(result);
}

LOCALPROC my_reg_call SetCCRforNegX(ui5r dstvalue, ui5r result)
{
	ZFLG &= Bool2Bit(result == 0);

	{
		flagtype flgs = Bool2Bit(ui5r_MSBisSet(dstvalue));
		flagtype flgn = Bool2Bit(ui5r_MSBisSet(result));

		NFLG = flgn;
		VFLG = flgs & flgn;
		XFLG = CFLG = flgs | flgn;
	}

	ArgSetDstValue(result);
}

LOCALIPROC DoCodeNegXB(void)
{
	NeedDefaultLazyAllFlags();

	{
		ui5r dstvalue = DecodeGetSetDstValue();
		ui5r result = ui5r_FromSByte(0 - (XFLG + dstvalue));

		SetCCRforNegX(dstvalue, result);
	}
}

LOCALIPROC DoCodeNegXW(void)
{
	if ((kLazyFlagsDefault != V_regs.LazyFlagKind)
		|| (kLazyFlagsDefault != V_regs.LazyXFlagKind))
	{
		NeedDefaultLazyAllFlags();
	}

	{
		ui5r dstvalue = DecodeGetSetDstValue();
		ui5r result = ui5r_FromSWord(0 - (XFLG + dstvalue));

		SetCCRforNegX(dstvalue, result);
	}
}

LOCALIPROC DoCodeNegXL(void)
{
	if (kLazyFlagsNegL == V_regs.LazyFlagKind) {
		NeedDefaultLazyFlagsNegL();
	} else
	if ((kLazyFlagsDefault == V_regs.LazyFlagKind)
		&& (kLazyFlagsDefault == V_regs.LazyXFlagKind))
	{
		/* ok */
	} else
	{
		NeedDefaultLazyAllFlags();
	}

	{
		ui5r dstvalue = DecodeGetSetDstValue();
		ui5r result = ui5r_FromSLong(0 - (XFLG + dstvalue));

		SetCCRforNegX(dstvalue, result);
	}
}

LOCALIPROC DoCodeMulU(void)
{
	/* MulU 1100ddd011mmmrrr */
	ui5r srcvalue = DecodeGetSrcValue();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = *dstp;

	dstvalue = ui5r_FromSLong(ui5r_FromUWord(dstvalue)
		* ui5r_FromUWord(srcvalue));
#if WantCloserCyc
	{
		ui5r v = srcvalue;

		while (v != 0) {
			if ((v & 1) != 0) {
				V_MaxCyclesToGo -= (2 * kCycleScale);
			}
			v >>= 1;
		}
	}
#endif

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	*dstp = dstvalue;
}

LOCALIPROC DoCodeMulS(void)
{
	/* MulS 1100ddd111mmmrrr */
	ui5r srcvalue = DecodeGetSrcValue();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = *dstp;

	dstvalue = ui5r_FromSLong((si5b)(si4b)dstvalue
		* (si5b)(si4b)srcvalue);
#if WantCloserCyc
	{
		ui5r v = (srcvalue << 1);

		while (v != 0) {
			if ((v & 1) != ((v >> 1) & 1)) {
				V_MaxCyclesToGo -= (2 * kCycleScale);
			}
			v >>= 1;
		}
	}
#endif

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	*dstp = dstvalue;
}

LOCALIPROC DoCodeDivU(void)
{
	/* DivU 1000ddd011mmmrrr */
	ui5r srcvalue = DecodeGetSrcValue();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = *dstp;

	if (srcvalue == 0) {
#if WantCloserCyc
		V_MaxCyclesToGo -=
			(38 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(5);
#if m68k_logExceptions
		dbglog_WriteNote("*** zero devide exception");
#endif
	} else {
		ui5b newv = (ui5b)dstvalue / (ui5b)(ui4b)srcvalue;
		ui5b rem = (ui5b)dstvalue % (ui5b)(ui4b)srcvalue;
#if WantCloserCyc
		V_MaxCyclesToGo -= (133 * kCycleScale);
#endif
		if (newv > 0xffff) {
			NeedDefaultLazyAllFlags();

			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			VFLG = CFLG = 0;
			ZFLG = Bool2Bit(((si4b)(newv)) == 0);
			NFLG = Bool2Bit(((si4b)(newv)) < 0);

			V_regs.LazyFlagKind = kLazyFlagsDefault;

			newv = (newv & 0xffff) | ((ui5b)rem << 16);
			dstvalue = newv;
		}
	}

	*dstp = dstvalue;
}

LOCALIPROC DoCodeDivS(void)
{
	/* DivS 1000ddd111mmmrrr */
	ui5r srcvalue = DecodeGetSrcValue();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = *dstp;

	if (srcvalue == 0) {
#if WantCloserCyc
		V_MaxCyclesToGo -=
			(38 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(5);
#if m68k_logExceptions
		dbglog_WriteNote("*** zero devide exception");
#endif
	} else {
		si5b newv = (si5b)dstvalue / (si5b)(si4b)srcvalue;
		ui4b rem = (si5b)dstvalue % (si5b)(si4b)srcvalue;
#if WantCloserCyc
		V_MaxCyclesToGo -= (150 * kCycleScale);
#endif
		if (((newv & 0xffff8000) != 0) &&
			((newv & 0xffff8000) != 0xffff8000))
		{
			NeedDefaultLazyAllFlags();

			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			if (((si4b)rem < 0) != ((si5b)dstvalue < 0)) {
				rem = - rem;
			}
			VFLG = CFLG = 0;
			ZFLG = Bool2Bit(((si4b)(newv)) == 0);
			NFLG = Bool2Bit(((si4b)(newv)) < 0);

			V_regs.LazyFlagKind = kLazyFlagsDefault;

			newv = (newv & 0xffff) | ((ui5b)rem << 16);
			dstvalue = newv;
		}
	}

	*dstp = dstvalue;
}

LOCALIPROC DoCodeExg(void)
{
	/* Exg dd 1100ddd101000rrr, opsize = 4 */
	/* Exg aa 1100ddd101001rrr, opsize = 4 */
	/* Exg da 1100ddd110001rrr, opsize = 4 */

	ui5r srcreg = V_regs.CurDecOpY.v[0].ArgDat;
	ui5r *srcp = &V_regs.regs[srcreg];
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r srcvalue = *srcp;

	*srcp = *dstp;
	*dstp = srcvalue;
}

LOCALIPROC DoCodeMoveEaCR(void)
{
	/* 0100010011mmmrrr */
	m68k_setCR(DecodeGetDstValue());
}

LOCALPROC DoPrivilegeViolation(void)
{
#if WantCloserCyc
	V_MaxCyclesToGo += GetDcoCycles(V_regs.CurDecOp);
	V_MaxCyclesToGo -=
		(34 * kCycleScale + 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
	BackupPC();
	Exception(8);
#if m68k_logExceptions
	dbglog_WriteNote("*** Privilege Violation exception");
#endif
}

LOCALIPROC DoCodeMoveSREa(void)
{
	/* Move from SR 0100000011mmmrrr */
#if Use68020
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else
#endif
	{
		DecodeSetDstValue(m68k_getSR());
	}
}

LOCALIPROC DoCodeMoveEaSR(void)
{
	/* 0100011011mmmrrr */
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		m68k_setSR(DecodeGetDstValue());
	}
}

LOCALIPROC DoCodeOrISR(void)
{
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		V_regs.SrcVal = nextiword_nm();

		m68k_setSR(m68k_getSR() | V_regs.SrcVal);
	}
}

LOCALIPROC DoCodeAndISR(void)
{
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		V_regs.SrcVal = nextiword_nm();

		m68k_setSR(m68k_getSR() & V_regs.SrcVal);
	}
}

LOCALIPROC DoCodeEorISR(void)
{
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		V_regs.SrcVal = nextiword_nm();

		m68k_setSR(m68k_getSR() ^ V_regs.SrcVal);
	}
}

LOCALIPROC DoCodeOrICCR(void)
{
	V_regs.SrcVal = nextiword_nm();

	m68k_setCR(m68k_getCR() | V_regs.SrcVal);
}

LOCALIPROC DoCodeAndICCR(void)
{
	V_regs.SrcVal = nextiword_nm();

	m68k_setCR(m68k_getCR() & V_regs.SrcVal);
}

LOCALIPROC DoCodeEorICCR(void)
{
	V_regs.SrcVal = nextiword_nm();

	m68k_setCR(m68k_getCR() ^ V_regs.SrcVal);
}

LOCALIPROC DoCodeMOVEMApRW(void)
{
	/* MOVEM mem to reg 01001100110011rrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r p = *dstp;

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -= (4 * kCycleScale + RdAvgXtraCyc);
#endif
			V_regs.regs[z] = get_word(p);
			p += 2;
		}
	}
	*dstp = p;
}

LOCALIPROC DoCodeMOVEMRmMW(void)
{
	/* MOVEM reg to mem 01001000110100rrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r p = *dstp;

#if Use68020
	{
		int n = 0;

		for (z = 0; z < 16; ++z) {
			if ((regmask & (1 << z)) != 0) {
				n++;
			}
		}
		*dstp = p - n * 2;
	}
#endif
	for (z = 16; --z >= 0; ) {
		if ((regmask & (1 << (15 - z))) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -= (4 * kCycleScale + WrAvgXtraCyc);
#endif
			p -= 2;
			put_word(p, V_regs.regs[z]);
		}
	}
#if ! Use68020
	*dstp = p;
#endif
}

LOCALIPROC DoCodeMOVEMrmW(void)
{
	/* MOVEM reg to mem 010010001ssmmmrrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r p = DecodeDst();

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -=
				(4 * kCycleScale + WrAvgXtraCyc);
#endif
			put_word(p, V_regs.regs[z]);
			p += 2;
		}
	}
}

LOCALIPROC DoCodeMOVEMrmL(void)
{
	/* MOVEM reg to mem 010010001ssmmmrrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r p = DecodeDst();

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -=
				(8 * kCycleScale + 2 * WrAvgXtraCyc);
#endif
			put_long(p, V_regs.regs[z]);
			p += 4;
		}
	}
}

LOCALIPROC DoCodeMOVEMmrW(void)
{
	/* MOVEM mem to reg 0100110011smmmrrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r p = DecodeDst();

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -=
				(4 * kCycleScale + RdAvgXtraCyc);
#endif
			V_regs.regs[z] = get_word(p);
			p += 2;
		}
	}
}

LOCALIPROC DoCodeMOVEMmrL(void)
{
	/* MOVEM mem to reg 0100110011smmmrrr */
	si4b z;
	ui5r regmask = nextiword_nm();
	ui5r p = DecodeDst();

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			V_MaxCyclesToGo -=
				(8 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
			V_regs.regs[z] = get_long(p);
			p += 4;
		}
	}
}

LOCALIPROC DoCodeAbcd(void)
{
	/* ABCD r 1100ddd100000rrr */
	/* ABCD m 1100ddd100001rrr */

	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r srcvalue = V_regs.SrcVal;

	{
		/* if (V_regs.opsize != 1) a bug */
		int flgs = ui5r_MSBisSet(srcvalue);
		int flgo = ui5r_MSBisSet(dstvalue);
		ui4b newv_lo =
			(srcvalue & 0xF) + (dstvalue & 0xF) + XFLG;
		ui4b newv_hi = (srcvalue & 0xF0) + (dstvalue & 0xF0);
		ui4b newv;

		if (newv_lo > 9) {
			newv_lo += 6;
		}
		newv = newv_hi + newv_lo;
		CFLG = XFLG = Bool2Bit((newv & 0x1F0) > 0x90);
		if (CFLG != 0) {
			newv += 0x60;
		}
		dstvalue = ui5r_FromSByte(newv);
		if (dstvalue != 0) {
			ZFLG = 0;
		}
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = Bool2Bit((flgs != flgo) && ((NFLG != 0) != flgo));
		/*
			but according to my reference book,
			VFLG is Undefined for ABCD
		*/
	}

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeSbcd(void)
{
	ui5r dstvalue = DecodeGetSrcSetDstValueDfltFlags_nm();
	ui5r srcvalue = V_regs.SrcVal;

	{
		int flgs = ui5r_MSBisSet(srcvalue);
		int flgo = ui5r_MSBisSet(dstvalue);
		ui4b newv_lo =
			(dstvalue & 0xF) - (srcvalue & 0xF) - XFLG;
		ui4b newv_hi = (dstvalue & 0xF0) - (srcvalue & 0xF0);
		ui4b newv;

		if (newv_lo > 9) {
			newv_lo -= 6;
			newv_hi -= 0x10;
		}
		newv = newv_hi + (newv_lo & 0xF);
		CFLG = XFLG = Bool2Bit((newv_hi & 0x1F0) > 0x90);
		if (CFLG != 0) {
			newv -= 0x60;
		}
		dstvalue = ui5r_FromSByte(newv);
		if (dstvalue != 0) {
			ZFLG = 0;
		}
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		VFLG = Bool2Bit((flgs != flgo) && ((NFLG != 0) != flgo));
		/*
			but according to my reference book,
			VFLG is Undefined for SBCD
		*/
	}

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeNbcd(void)
{
	/* Nbcd 0100100000mmmrrr */
	ui5r dstvalue = DecodeGetSetDstValue();

	NeedDefaultLazyAllFlags();

	{
		ui4b newv_lo = - (dstvalue & 0xF) - XFLG;
		ui4b newv_hi = - (dstvalue & 0xF0);
		ui4b newv;

		if (newv_lo > 9) {
			newv_lo -= 6;
			newv_hi -= 0x10;
		}
		newv = newv_hi + (newv_lo & 0xF);
		CFLG = XFLG = Bool2Bit((newv_hi & 0x1F0) > 0x90);
		if (CFLG != 0) {
			newv -= 0x60;
		}

		dstvalue = ui5r_FromSByte(newv);
		NFLG = Bool2Bit(ui5r_MSBisSet(dstvalue));
		if (dstvalue != 0) {
			ZFLG = 0;
		}
	}

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeRte(void)
{
	/* Rte 0100111001110011 */
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		ui5r NewPC;
		CPTR stackp = m68k_areg(7);
		ui5r NewSR = get_word(stackp);
		stackp += 2;
		NewPC = get_long(stackp);
		stackp += 4;

#if Use68020
		{
			ui4b format = get_word(stackp);
			stackp += 2;

			switch ((format >> 12) & 0x0F) {
				case 0:
					/* ReportAbnormal("rte stack frame format 0"); */
					break;
				case 1:
					ReportAbnormalID(0x0107,
						"rte stack frame format 1");
					NewPC = m68k_getpc() - 2;
						/* rerun instruction */
					break;
				case 2:
					ReportAbnormalID(0x0108,
						"rte stack frame format 2");
					stackp += 4;
					break;
				case 9:
					ReportAbnormalID(0x0109,
						"rte stack frame format 9");
					stackp += 12;
					break;
				case 10:
					ReportAbnormalID(0x010A,
						"rte stack frame format 10");
					stackp += 24;
					break;
				case 11:
					ReportAbnormalID(0x010B,
						"rte stack frame format 11");
					stackp += 84;
					break;
				default:
					ReportAbnormalID(0x010C,
						"unknown rte stack frame format");
					Exception(14);
					return;
					break;
			}
		}
#endif
		m68k_areg(7) = stackp;
		m68k_setSR(NewSR);
		m68k_setpc(NewPC);
	}
}

LOCALIPROC DoCodeNop(void)
{
	/* Nop 0100111001110001 */
}

LOCALIPROC DoCodeMoveP0(void)
{
	/* MoveP 0000ddd1mm001aaa */
	ui5r srcreg = V_regs.CurDecOpY.v[0].ArgDat;
	ui5r *srcp = &V_regs.regs[srcreg];
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];

	ui5r Displacement = nextiword_nm();
		/* shouldn't this sign extend ? */
	CPTR memp = *srcp + Displacement;

	ui4b val = ((get_byte(memp) & 0x00FF) << 8)
		| (get_byte(memp + 2) & 0x00FF);

	*dstp =
		(*dstp & ~ 0xffff) | (val & 0xffff);

#if 0
	if ((Displacement & 0x00008000) != 0) {
		/* **** for testing only **** */
		BackupPC();
		op_illg();
	}
#endif
}

LOCALIPROC DoCodeMoveP1(void)
{
	/* MoveP 0000ddd1mm001aaa */
	ui5r srcreg = V_regs.CurDecOpY.v[0].ArgDat;
	ui5r *srcp = &V_regs.regs[srcreg];
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];

	ui5r Displacement = nextiword_nm();
		/* shouldn't this sign extend ? */
	CPTR memp = *srcp + Displacement;

	ui5b val = ((get_byte(memp) & 0x00FF) << 24)
		| ((get_byte(memp + 2) & 0x00FF) << 16)
		| ((get_byte(memp + 4) & 0x00FF) << 8)
		| (get_byte(memp + 6) & 0x00FF);

	*dstp = val;
}

LOCALIPROC DoCodeMoveP2(void)
{
	/* MoveP 0000ddd1mm001aaa */
	ui5r srcreg = V_regs.CurDecOpY.v[0].ArgDat;
	ui5r *srcp = &V_regs.regs[srcreg];
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];

	ui5r Displacement = nextiword_nm();
		/* shouldn't this sign extend ? */
	CPTR memp = *srcp + Displacement;

	si4b val = *dstp;

	put_byte(memp, val >> 8);
	put_byte(memp + 2, val);
}

LOCALIPROC DoCodeMoveP3(void)
{
	/* MoveP 0000ddd1mm001aaa */
	ui5r srcreg = V_regs.CurDecOpY.v[0].ArgDat;
	ui5r *srcp = &V_regs.regs[srcreg];
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];

	ui5r Displacement = nextiword_nm();
		/* shouldn't this sign extend ? */
	CPTR memp = *srcp + Displacement;

	si5b val = *dstp;

	put_byte(memp, val >> 24);
	put_byte(memp + 2, val >> 16);
	put_byte(memp + 4, val >> 8);
	put_byte(memp + 6, val);
}

LOCALPROC op_illg(void)
{
	BackupPC();
	Exception(4);
#if m68k_logExceptions
	dbglog_WriteNote("*** illegal instruction exception");
#endif
}

LOCALIPROC DoCodeChk(void)
{
	ui5r dstvalue = DecodeGetSrcGetDstValue();
	ui5r srcvalue = V_regs.SrcVal;

	if (ui5r_MSBisSet(srcvalue)) {
		NeedDefaultLazyAllFlags();

#if WantCloserCyc
		V_MaxCyclesToGo -=
			(30 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		NFLG = 1;
		Exception(6);
	} else if (((si5r)srcvalue) > ((si5r)dstvalue)) {
		NeedDefaultLazyAllFlags();

#if WantCloserCyc
		V_MaxCyclesToGo -=
			(30 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		NFLG = 0;
		Exception(6);
	}
}

LOCALIPROC DoCodeTrap(void)
{
	/* Trap 010011100100vvvv */
	Exception(V_regs.CurDecOpY.v[1].ArgDat);
}

LOCALIPROC DoCodeTrapV(void)
{
	/* TrapV 0100111001110110 */
	NeedDefaultLazyAllFlags();

	if (VFLG != 0) {
#if WantCloserCyc
		V_MaxCyclesToGo += GetDcoCycles(V_regs.CurDecOp);
		V_MaxCyclesToGo -=
			(34 * kCycleScale + 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(7);
	}
}

LOCALIPROC DoCodeRtr(void)
{
	/* Rtr 0100111001110111 */
	ui5r NewPC;
	CPTR stackp = m68k_areg(7);
	ui5r NewCR = get_word(stackp);
	stackp += 2;
	NewPC = get_long(stackp);
	stackp += 4;
	m68k_areg(7) = stackp;
	m68k_setCR(NewCR);
	m68k_setpc(NewPC);
}

LOCALIPROC DoCodeLink(void)
{
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	CPTR stackp = m68k_areg(7);

	stackp -= 4;
	m68k_areg(7) = stackp; /* only matters if dstreg == 7 + 8 */
	put_long(stackp, *dstp);
	*dstp = stackp;
	m68k_areg(7) += ui5r_FromSWord(nextiword_nm());
}

LOCALIPROC DoCodeUnlk(void)
{
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];

	if (dstreg != 7 + 8) {
		ui5r src = *dstp;
		*dstp = get_long(src);
		m68k_areg(7) = src + 4;
	} else {
		/* wouldn't expect this to happen */
		m68k_areg(7) = get_long(m68k_areg(7)) + 4;
	}
}

LOCALIPROC DoCodeMoveRUSP(void)
{
	/* MOVE USP 0100111001100aaa */
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
		ui5r *dstp = &V_regs.regs[dstreg];

		V_regs.usp = *dstp;
	}
}

LOCALIPROC DoCodeMoveUSPR(void)
{
	/* MOVE USP 0100111001101aaa */
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
		ui5r *dstp = &V_regs.regs[dstreg];

		*dstp = V_regs.usp;
	}
}

LOCALIPROC DoCodeTas(void)
{
	/* Tas 0100101011mmmrrr */
	ui5r dstvalue = DecodeGetSetDstValue();

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	dstvalue |= 0x80;

	ArgSetDstValue(dstvalue);
}

LOCALIPROC DoCodeFdefault(void)
{
	BackupPC();
	Exception(0xB);
}

LOCALPROC m68k_setstopped(void)
{
	/* not implemented. doesn't seemed to be used on Mac Plus */
	Exception(4); /* fake an illegal instruction */
#if m68k_logExceptions
	dbglog_WriteNote("*** set stopped");
#endif
}

LOCALIPROC DoCodeStop(void)
{
	/* Stop 0100111001110010 */
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		m68k_setSR(nextiword_nm());
		m68k_setstopped();
	}
}

FORWARDPROC local_customreset(void);

LOCALIPROC DoCodeReset(void)
{
	/* Reset 0100111001100000 */
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		local_customreset();
	}
}

#if Use68020
LOCALIPROC DoCodeCallMorRtm(void)
{
	/* CALLM or RTM 0000011011mmmrrr */
	ReportAbnormalID(0x010D, "CALLM or RTM instruction");
}
#endif

#if Use68020
LOCALIPROC DoCodeMoveCCREa(void)
{
	/* Move from CCR 0100001011mmmrrr */
	DecodeSetDstValue(m68k_getCR());
}
#endif

#if Use68020
LOCALIPROC DoCodeBraL(void)
{
	/* Bra 0110ccccnnnnnnnn */
	si5r offset = ((si5b)(ui5b)nextilong()) - 4;
	ui3p s = V_pc_p + offset;

	V_pc_p = s;

#if USE_PCLIMIT
	if (my_cond_rare(s >= V_pc_pHi)
		|| my_cond_rare(s < V_regs.pc_pLo))
	{
		Recalc_PC_Block();
	}
#endif
}
#endif

#if Use68020
LOCALPROC SkipiLong(void)
{
	V_pc_p += 4;

#if USE_PCLIMIT
	if (my_cond_rare(V_pc_p >= V_pc_pHi)) {
		Recalc_PC_Block();
	}
#endif
}
#endif

#if Use68020
LOCALIPROC DoCodeBccL(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	cctrue(DoCodeBraL, SkipiLong);
}
#endif

#if Use68020
LOCALIPROC DoCodeBsrL(void)
{
	si5r offset = ((si5b)(ui5b)nextilong()) - 4;
	ui3p s = V_pc_p + offset;

	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
	V_pc_p = s;

#if USE_PCLIMIT
	if (my_cond_rare(s >= V_pc_pHi)
		|| my_cond_rare(s < V_regs.pc_pLo))
	{
		Recalc_PC_Block();
	}
#endif

	/* ReportAbnormal("long branch in DoCode6"); */
	/* Used by various Apps */
}
#endif

#if Use68020
LOCALIPROC DoCodeEXTBL(void)
{
	/* EXTB.L */
	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	ui5r dstvalue = ui5r_FromSByte(*dstp);

	V_regs.LazyFlagKind = kLazyFlagsTstL;
	V_regs.LazyFlagArgDst = dstvalue;

	HaveSetUpFlags();

	*dstp = dstvalue;
}
#endif

#if Use68020
LOCALPROC DoCHK2orCMP2(void)
{
	/* CHK2 or CMP2 00000ss011mmmrrr */
	ui5r regv;
	ui5r lower;
	ui5r upper;
	ui5r extra = nextiword_nm();
	ui5r DstAddr = DecodeDst();
	ui5r srcreg = (extra >> 12) & 0x0F;
	ui5r *srcp = &V_regs.regs[srcreg];

	/* ReportAbnormal("CHK2 or CMP2 instruction"); */
	switch (V_regs.CurDecOpY.v[0].ArgDat) {
		case 1:
			if ((extra & 0x8000) == 0) {
				regv = ui5r_FromSByte(*srcp);
			} else {
				regv = ui5r_FromSLong(*srcp);
			}
			lower = get_byte(DstAddr);
			upper = get_byte(DstAddr + 1);
			break;
		case 2:
			if ((extra & 0x8000) == 0) {
				regv = ui5r_FromSWord(*srcp);
			} else {
				regv = ui5r_FromSLong(*srcp);
			}
			lower = get_word(DstAddr);
			upper = get_word(DstAddr + 2);
			break;
		default:
#if ExtraAbnormalReports
			if (4 != V_regs.CurDecOpY.v[0].ArgDat) {
				ReportAbnormalID(0x010E,
					"illegal opsize in CHK2 or CMP2");
			}
#endif
			if ((extra & 0x8000) == 0) {
				regv = ui5r_FromSLong(*srcp);
			} else {
				regv = ui5r_FromSLong(*srcp);
			}
			lower = get_long(DstAddr);
			upper = get_long(DstAddr + 4);
			break;
	}

	NeedDefaultLazyAllFlags();

	ZFLG = Bool2Bit((upper == regv) || (lower == regv));
	CFLG = Bool2Bit((((si5r)lower) <= ((si5r)upper))
			? (((si5r)regv) < ((si5r)lower)
				|| ((si5r)regv) > ((si5r)upper))
			: (((si5r)regv) > ((si5r)upper)
				|| ((si5r)regv) < ((si5r)lower)));

	if ((extra & 0x800) && (CFLG != 0)) {
		Exception(6);
	}
}
#endif

#if Use68020
LOCALPROC DoCAS(void)
{
	/* CAS 00001ss011mmmrrr */
	ui5r srcvalue;
	ui5r dstvalue;

	ui4b src = nextiword_nm();
	int ru = (src >> 6) & 7;
	int rc = src & 7;

	ReportAbnormalID(0x010F, "CAS instruction");
	switch (V_regs.CurDecOpY.v[0].ArgDat) {
		case 1:
			srcvalue = ui5r_FromSByte(V_regs.regs[rc]);
			break;
		case 2:
			srcvalue = ui5r_FromSWord(V_regs.regs[rc]);
			break;
		default:
#if ExtraAbnormalReports
			if (4 != V_regs.CurDecOpY.v[0].ArgDat) {
				ReportAbnormalID(0x0110, "illegal opsize in DoCAS");
			}
#endif
			srcvalue = ui5r_FromSLong(V_regs.regs[rc]);
			break;
	}
	dstvalue = DecodeGetSetDstValue();

	{
		int flgs = ((si5b)srcvalue) < 0;
		int flgo = ((si5b)dstvalue) < 0;
		ui5r newv = dstvalue - srcvalue;
		if (V_regs.CurDecOpY.v[0].ArgDat == 1) {
			newv = ui5r_FromSByte(newv);
		} else if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
			newv = ui5r_FromSWord(newv);
		} else {
			newv = ui5r_FromSLong(newv);
		}
		ZFLG = Bool2Bit(((si5b)newv) == 0);
		NFLG = Bool2Bit(((si5b)newv) < 0);
		VFLG = Bool2Bit((flgs != flgo) && ((NFLG != 0) != flgo));
		CFLG = Bool2Bit(
			(flgs && ! flgo) || ((NFLG != 0) && ((! flgo) || flgs)));

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		if (ZFLG != 0) {
			ArgSetDstValue(m68k_dreg(ru));
		} else {
			V_regs.ArgAddr.rga = &V_regs.regs[rc];

			if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
				*V_regs.ArgAddr.rga =
					(*V_regs.ArgAddr.rga & ~ 0xffff)
						| ((dstvalue) & 0xffff);
			} else if (V_regs.CurDecOpY.v[0].ArgDat < 2) {
				*V_regs.ArgAddr.rga =
					(*V_regs.ArgAddr.rga & ~ 0xff)
						| ((dstvalue) & 0xff);
			} else {
				*V_regs.ArgAddr.rga = dstvalue;
			}
		}
	}
}
#endif

#if Use68020
LOCALPROC DoCAS2(void)
{
	/* CAS2 00001ss011111100 */
	ui5b extra = nextilong();
	int dc2 = extra & 7;
	int du2 = (extra >> 6) & 7;
	int dc1 = (extra >> 16) & 7;
	int du1 = (extra >> 22) & 7;
	CPTR rn1 = V_regs.regs[(extra >> 28) & 0x0F];
	CPTR rn2 = V_regs.regs[(extra >> 12) & 0x0F];
	si5b src = m68k_dreg(dc1);
	si5r dst1;
	si5r dst2;

	ReportAbnormalID(0x0111, "DoCAS2 instruction");
	if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
		dst1 = get_word(rn1);
		dst2 = get_word(rn2);
		src = (si5b)(si4b)src;
	} else {
		dst1 = get_long(rn1);
		dst2 = get_long(rn2);
	}
	{
		int flgs = src < 0;
		int flgo = dst1 < 0;
		si5b newv = dst1 - src;
		if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
			newv = (ui4b)newv;
		}
		ZFLG = Bool2Bit(newv == 0);
		NFLG = Bool2Bit(newv < 0);
		VFLG = Bool2Bit((flgs != flgo) && ((NFLG != 0) != flgo));
		CFLG = Bool2Bit(
			(flgs && ! flgo) || ((NFLG != 0) && ((! flgo) || flgs)));

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		if (ZFLG != 0) {
			src = m68k_dreg(dc2);
			if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
				src = (si5b)(si4b)src;
			}
			flgs = src < 0;
			flgo = dst2 < 0;
			newv = dst2 - src;
			if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
				newv = (ui4b)newv;
			}
			ZFLG = Bool2Bit(newv == 0);
			NFLG = Bool2Bit(newv < 0);
			VFLG = Bool2Bit((flgs != flgo) && ((NFLG != 0) != flgo));
			CFLG = Bool2Bit((flgs && ! flgo)
				|| ((NFLG != 0) && ((! flgo) || flgs)));

			V_regs.LazyFlagKind = kLazyFlagsDefault;
			if (ZFLG != 0) {
				if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
					put_word(rn1, m68k_dreg(du1));
					put_word(rn2, m68k_dreg(du2));
				} else {
					put_word(rn1, m68k_dreg(du1));
					put_word(rn2, m68k_dreg(du2));
				}
			}
		}
	}
	if (ZFLG == 0) {
		if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
			m68k_dreg(du1) =
				(m68k_dreg(du1) & ~ 0xffff) | ((ui5b)dst1 & 0xffff);
			m68k_dreg(du2) =
				(m68k_dreg(du2) & ~ 0xffff) | ((ui5b)dst2 & 0xffff);
		} else {
			m68k_dreg(du1) = dst1;
			m68k_dreg(du2) = dst2;
		}
	}
}
#endif

#if Use68020
LOCALPROC DoMOVES(void)
{
	/* MoveS 00001110ssmmmrrr */
	ReportAbnormalID(0x0112, "MoveS instruction");
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		ui4b extra = nextiword_nm();
		if (extra & 0x0800) {
			ui5b src = V_regs.regs[(extra >> 12) & 0x0F];
			DecodeSetDstValue(src);
		} else {
			ui5r srcvalue = DecodeGetDstValue();
			ui5b rr = (extra >> 12) & 7;
			if (extra & 0x8000) {
				m68k_areg(rr) = srcvalue;
			} else {
				V_regs.ArgAddr.rga = &V_regs.regs[rr];

				if (V_regs.CurDecOpY.v[0].ArgDat == 2) {
					*V_regs.ArgAddr.rga =
						(*V_regs.ArgAddr.rga & ~ 0xffff)
							| ((srcvalue) & 0xffff);
				} else if (V_regs.CurDecOpY.v[0].ArgDat < 2) {
					*V_regs.ArgAddr.rga =
						(*V_regs.ArgAddr.rga & ~ 0xff)
							| ((srcvalue) & 0xff);
				} else {
					*V_regs.ArgAddr.rga = srcvalue;
				}
			}
		}
	}
}
#endif

#define ui5b_lo(x) ((x) & 0x0000FFFF)
#define ui5b_hi(x) (((x) >> 16) & 0x0000FFFF)

#if Use68020
struct ui6r0 {
	ui5b hi;
	ui5b lo;
};
typedef struct ui6r0 ui6r0;
#endif

#if Use68020
LOCALPROC Ui6r_Negate(ui6r0 *v)
{
	v->hi = ~ v->hi;
	v->lo = - v->lo;
	if (v->lo == 0) {
		v->hi++;
	}
}
#endif

#if Use68020
LOCALFUNC blnr my_reg_call Ui6r_IsZero(ui6r0 *v)
{
	return (v->hi == 0) && (v->lo == 0);
}
#endif

#if Use68020
LOCALFUNC blnr my_reg_call Ui6r_IsNeg(ui6r0 *v)
{
	return ((si5b)v->hi) < 0;
}
#endif

#if Use68020
LOCALPROC mul_unsigned(ui5b src1, ui5b src2, ui6r0 *dst)
{
	ui5b src1_lo = ui5b_lo(src1);
	ui5b src2_lo = ui5b_lo(src2);
	ui5b src1_hi = ui5b_hi(src1);
	ui5b src2_hi = ui5b_hi(src2);

	ui5b r0 = src1_lo * src2_lo;
	ui5b r1 = src1_hi * src2_lo;
	ui5b r2 = src1_lo * src2_hi;
	ui5b r3 = src1_hi * src2_hi;

	ui5b ra1 = ui5b_hi(r0) + ui5b_lo(r1) + ui5b_lo(r2);

	dst->lo = (ui5b_lo(ra1) << 16) | ui5b_lo(r0);
	dst->hi = ui5b_hi(ra1) + ui5b_hi(r1) + ui5b_hi(r2) + r3;
}
#endif

#if Use68020
LOCALFUNC blnr div_unsigned(ui6r0 *src, ui5b div,
	ui5b *quot, ui5b *rem)
{
	int i;
	ui5b q = 0;
	ui5b cbit = 0;
	ui5b src_hi = src->hi;
	ui5b src_lo = src->lo;

	if (div <= src_hi) {
		return trueblnr;
	}
	for (i = 0 ; i < 32 ; i++) {
		cbit = src_hi & 0x80000000ul;
		src_hi <<= 1;
		if (src_lo & 0x80000000ul) {
			src_hi++;
		}
		src_lo <<= 1;
		q = q << 1;
		if (cbit || div <= src_hi) {
			q |= 1;
			src_hi -= div;
		}
	}
	*quot = q;
	*rem = src_hi;
	return falseblnr;
}
#endif

#if Use68020
LOCALIPROC DoCodeMulL(void)
{
	/* MULU 0100110000mmmrrr 0rrr0s0000000rrr */
	/* MULS 0100110000mmmrrr 0rrr1s0000000rrr */
	ui6r0 dst;
	ui4b extra = nextiword();
	ui5b r2 = (extra >> 12) & 7;
	ui5b dstvalue = m68k_dreg(r2);
	ui5r srcvalue = DecodeGetDstValue();

	if (extra & 0x800) {
		/* MULS.L - signed */

		si5b src1 = (si5b)srcvalue;
		si5b src2 = (si5b)dstvalue;
		blnr s1 = src1 < 0;
		blnr s2 = src2 < 0;
		blnr sr = s1 != s2;

		/* ReportAbnormal("MULS.L"); */
		/* used by Sys 7.5.5 boot extensions */
		if (s1) {
			src1 = - src1;
		}
		if (s2) {
			src2 = - src2;
		}
		mul_unsigned((ui5b)src1, (ui5b)src2, &dst);
		if (sr) {
			Ui6r_Negate(&dst);
		}
		VFLG = CFLG = 0;
		ZFLG = Bool2Bit(Ui6r_IsZero(&dst));
		NFLG = Bool2Bit(Ui6r_IsNeg(&dst));

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		if (extra & 0x400) {
			m68k_dreg(extra & 7) = dst.hi;
		} else {
			if ((dst.lo & 0x80000000) != 0) {
				if ((dst.hi & 0xffffffff) != 0xffffffff) {
					VFLG = 1;
				}
			} else {
				if (dst.hi != 0) {
					VFLG = 1;
				}
			}
		}
	} else {
		/* MULU.L - unsigned */

		/* ReportAbnormal("MULU.U"); */
		/* Used by various Apps */

		mul_unsigned(srcvalue, dstvalue, &dst);

		VFLG = CFLG = 0;
		ZFLG = Bool2Bit(Ui6r_IsZero(&dst));
		NFLG = Bool2Bit(Ui6r_IsNeg(&dst));

		V_regs.LazyFlagKind = kLazyFlagsDefault;

		if (extra & 0x400) {
			m68k_dreg(extra & 7) = dst.hi;
		} else {
			if (dst.hi != 0) {
				VFLG = 1;
			}
		}
	}
	m68k_dreg(r2) = dst.lo;
}
#endif

#if Use68020
LOCALIPROC DoCodeDivL(void)
{
	/* DIVU 0100110001mmmrrr 0rrr0s0000000rrr */
	/* DIVS 0100110001mmmrrr 0rrr1s0000000rrr */
	/* ReportAbnormal("DIVS/DIVU long"); */
	ui6r0 v2;
	ui5b quot;
	ui5b rem;
	ui4b extra = nextiword();
	ui5b rDr = extra & 7;
	ui5b rDq = (extra >> 12) & 7;
	ui5r src = (ui5b)(si5b)DecodeGetDstValue();

	if (src == 0) {
		Exception(5);
#if m68k_logExceptions
		dbglog_WriteNote("*** zero devide exception");
#endif
		return;
	}
	if (0 != (extra & 0x0800)) {
		/* signed variant */
		blnr sr;
		blnr s2;
		blnr s1 = ((si5b)src < 0);

		v2.lo = (si5b)m68k_dreg(rDq);
		if (extra & 0x0400) {
			v2.hi = (si5b)m68k_dreg(rDr);
		} else {
			v2.hi = ((si5b)v2.lo) < 0 ? -1 : 0;
		}
		s2 = Ui6r_IsNeg(&v2);
		sr = (s1 != s2);
		if (s2) {
			Ui6r_Negate(&v2);
		}
		if (s1) {
			src = - src;
		}
		if (div_unsigned(&v2, src, &quot, &rem)
			|| (sr ? quot > 0x80000000 : quot > 0x7fffffff))
		{
			NeedDefaultLazyAllFlags();

			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			if (sr) {
				quot = - quot;
			}
			if (((si5b)rem < 0) != s2) {
				rem = - rem;
			}
			VFLG = CFLG = 0;
			ZFLG = Bool2Bit(((si5b)quot) == 0);
			NFLG = Bool2Bit(((si5b)quot) < 0);

			V_regs.LazyFlagKind = kLazyFlagsDefault;

			m68k_dreg(rDr) = rem;
			m68k_dreg(rDq) = quot;
		}
	} else {
		/* unsigned */

		v2.lo = (ui5b)m68k_dreg(rDq);
		if (extra & 0x400) {
			v2.hi = (ui5b)m68k_dreg(rDr);
		} else {
			v2.hi = 0;
		}
		if (div_unsigned(&v2, src, &quot, &rem)) {
			NeedDefaultLazyAllFlags();

			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			VFLG = CFLG = 0;
			ZFLG = Bool2Bit(((si5b)quot) == 0);
			NFLG = Bool2Bit(((si5b)quot) < 0);

			V_regs.LazyFlagKind = kLazyFlagsDefault;

			m68k_dreg(rDr) = rem;
			m68k_dreg(rDq) = quot;
		}
	}
}
#endif

#if Use68020
LOCALIPROC DoMoveToControl(void)
{
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		ui4b src = nextiword_nm();
		int regno = (src >> 12) & 0x0F;
		ui5b v = V_regs.regs[regno];

		switch (src & 0x0FFF) {
			case 0x0000:
				V_regs.sfc = v & 7;
				/* ReportAbnormal("DoMoveToControl: sfc"); */
				/* happens on entering macsbug */
				break;
			case 0x0001:
				V_regs.dfc = v & 7;
				/* ReportAbnormal("DoMoveToControl: dfc"); */
				break;
			case 0x0002:
				V_regs.cacr = v & 0x3;
				/* ReportAbnormal("DoMoveToControl: cacr"); */
				/* used by Sys 7.5.5 boot */
				break;
			case 0x0800:
				V_regs.usp = v;
				ReportAbnormalID(0x0113, "DoMoveToControl: usp");
				break;
			case 0x0801:
				V_regs.vbr = v;
				/* ReportAbnormal("DoMoveToControl: vbr"); */
				/* happens on entering macsbug */
				break;
			case 0x0802:
				V_regs.caar = v &0xfc;
				/* ReportAbnormal("DoMoveToControl: caar"); */
				/* happens on entering macsbug */
				break;
			case 0x0803:
				V_regs.msp = v;
				if (V_regs.m == 1) {
					m68k_areg(7) = V_regs.msp;
				}
				/* ReportAbnormal("DoMoveToControl: msp"); */
				/* happens on entering macsbug */
				break;
			case 0x0804:
				V_regs.isp = v;
				if (V_regs.m == 0) {
					m68k_areg(7) = V_regs.isp;
				}
				ReportAbnormalID(0x0114, "DoMoveToControl: isp");
				break;
			default:
				op_illg();
				ReportAbnormalID(0x0115,
					"DoMoveToControl: unknown reg");
				break;
		}
	}
}
#endif

#if Use68020
LOCALIPROC DoMoveFromControl(void)
{
	if (0 == V_regs.s) {
		DoPrivilegeViolation();
	} else {
		ui5b v;
		ui4b src = nextiword_nm();
		int regno = (src >> 12) & 0x0F;

		switch (src & 0x0FFF) {
			case 0x0000:
				v = V_regs.sfc;
				/* ReportAbnormal("DoMoveFromControl: sfc"); */
				/* happens on entering macsbug */
				break;
			case 0x0001:
				v = V_regs.dfc;
				/* ReportAbnormal("DoMoveFromControl: dfc"); */
				/* happens on entering macsbug */
				break;
			case 0x0002:
				v = V_regs.cacr;
				/* ReportAbnormal("DoMoveFromControl: cacr"); */
				/* used by Sys 7.5.5 boot */
				break;
			case 0x0800:
				v = V_regs.usp;
				ReportAbnormalID(0x0116, "DoMoveFromControl: usp");
				break;
			case 0x0801:
				v = V_regs.vbr;
				/* ReportAbnormal("DoMoveFromControl: vbr"); */
				/* happens on entering macsbug */
				break;
			case 0x0802:
				v = V_regs.caar;
				/* ReportAbnormal("DoMoveFromControl: caar"); */
				/* happens on entering macsbug */
				break;
			case 0x0803:
				v = (V_regs.m == 1)
					? m68k_areg(7)
					: V_regs.msp;
				/* ReportAbnormal("DoMoveFromControl: msp"); */
				/* happens on entering macsbug */
				break;
			case 0x0804:
				v = (V_regs.m == 0)
					? m68k_areg(7)
					: V_regs.isp;
				ReportAbnormalID(0x0117, "DoMoveFromControl: isp");
				break;
			default:
				v = 0;
				ReportAbnormalID(0x0118,
					"DoMoveFromControl: unknown reg");
				op_illg();
				break;
		}
		V_regs.regs[regno] = v;
	}
}
#endif

#if Use68020
LOCALIPROC DoCodeBkpt(void)
{
	/* BKPT 0100100001001rrr */
	ReportAbnormalID(0x0119, "BKPT instruction");
	op_illg();
}
#endif

#if Use68020
LOCALIPROC DoCodeRtd(void)
{
	/* Rtd 0100111001110100 */
	ui5r NewPC = get_long(m68k_areg(7));
	si5b offs = nextiSWord();
	/* ReportAbnormal("RTD"); */
	/* used by Sys 7.5.5 boot */
	m68k_areg(7) += (4 + offs);
	m68k_setpc(NewPC);
}
#endif

#if Use68020
LOCALIPROC DoCodeLinkL(void)
{
	/* Link.L 0100100000001rrr */

	ui5r dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui5r *dstp = &V_regs.regs[dstreg];
	CPTR stackp = m68k_areg(7);

	ReportAbnormalID(0x011A, "Link.L");

	stackp -= 4;
	m68k_areg(7) = stackp; /* only matters if dstreg == 7 + 8 */
	put_long(stackp, *dstp);
	*dstp = stackp;
	m68k_areg(7) += (si5b)nextilong();
}
#endif

#if Use68020
LOCALPROC DoCodeTRAPcc_t(void)
{
	ReportAbnormalID(0x011B, "TRAPcc trapping");
	Exception(7);
	/* pc pushed onto stack wrong */
}
#endif

#if Use68020
LOCALPROC DoCodeTRAPcc_f(void)
{
}
#endif

#if Use68020
LOCALIPROC DoCodeTRAPcc(void)
{
	/* TRAPcc 0101cccc11111sss */
	/* ReportAbnormal("TRAPcc"); */
	switch (V_regs.CurDecOpY.v[1].ArgDat) {
		case 2:
			ReportAbnormalID(0x011C, "TRAPcc word data");
			SkipiWord();
			break;
		case 3:
			ReportAbnormalID(0x011D, "TRAPcc long data");
			SkipiLong();
			break;
		case 4:
			/* ReportAbnormal("TRAPcc no data"); */
			/* no optional data */
			break;
		default:
			ReportAbnormalID(0x011E, "TRAPcc illegal format");
			op_illg();
			break;
	}
	cctrue(DoCodeTRAPcc_t, DoCodeTRAPcc_f);
}
#endif

#if Use68020
LOCALIPROC DoCodePack(void)
{
	ui5r offs = nextiSWord();
	ui5r val = DecodeGetSrcValue();

	ReportAbnormalID(0x011F, "PACK");

	val += offs;
	val = ((val >> 4) & 0xf0) | (val & 0xf);

	DecodeSetDstValue(val);
}
#endif

#if Use68020
LOCALIPROC DoCodeUnpk(void)
{
	ui5r offs = nextiSWord();
	ui5r val = DecodeGetSrcValue();

	ReportAbnormalID(0x0120, "UNPK");

	val = (((val & 0xF0) << 4) | (val & 0x0F)) + offs;

	DecodeSetDstValue(val);
}
#endif

#if Use68020
LOCALIPROC DoBitField(void)
{
	ui5b tmp;
	ui5b newtmp;
	CPTR dsta;
	ui5b bf0;
	ui3b bf1;
	ui5b dstreg = V_regs.CurDecOpY.v[1].ArgDat;
	ui4b extra = nextiword();
	ui5b offset = ((extra & 0x0800) != 0)
		? m68k_dreg((extra >> 6) & 7)
		: ((extra >> 6) & 0x1f);
	ui5b width = ((extra & 0x0020) != 0)
		? m68k_dreg(extra & 7)
		: extra;
	ui3b bfa[5];
	ui5b offwid;

	/* ReportAbnormal("Bit Field operator"); */
	/* width = ((width - 1) & 0x1f) + 1; */ /* 0 -> 32 */
	width &= 0x001F; /* except width == 0 really means 32 */
	if (V_regs.CurDecOpY.v[0].AMd == 0) {
		bf0 = m68k_dreg(dstreg);
		offset &= 0x1f;
		tmp = bf0;
		if (0 != offset) {
			tmp = (tmp << offset) | (tmp >> (32 - offset));
		}
	} else {
		/*
			V_regs.ArgKind == AKMemory,
			otherwise illegal and don't get here
		*/
		dsta = DecodeDst();
		dsta += Ui5rASR(offset, 3);
		offset &= 7;
		offwid = offset + ((width == 0) ? 32 : width);

		/* if (offwid > 0) */ {
			bf1 = get_byte(dsta);
			bfa[0] = bf1;
			tmp = ((ui5b)bf1) << (24 + offset);
		}
		if (offwid > 8) {
			bf1 = get_byte(dsta + 1);
			bfa[1] = bf1;
			tmp |= ((ui5b)bf1) << (16 + offset);
		}
		if (offwid > 16) {
			bf1 = get_byte(dsta + 2);
			bfa[2] = bf1;
			tmp |= ((ui5r)bf1) << (8 + offset);
		}
		if (offwid > 24) {
			bf1 = get_byte(dsta + 3);
			bfa[3] = bf1;
			tmp |= ((ui5r)bf1) << (offset);
		}
		if (offwid > 32) {
			bf1 = get_byte(dsta + 4);
			bfa[4] = bf1;
			tmp |= ((ui5r)bf1) >> (8 - offset);
		}
	}

	NFLG = Bool2Bit(((si5b)tmp) < 0);
	if (width != 0) {
		tmp >>= (32 - width);
	}
	ZFLG = tmp == 0;
	VFLG = 0;
	CFLG = 0;

	V_regs.LazyFlagKind = kLazyFlagsDefault;

	newtmp = tmp;

	switch (V_regs.CurDecOpY.v[0].ArgDat) {
		case 0: /* BFTST */
			/* do nothing */
			break;
		case 1: /* BFEXTU */
			m68k_dreg((extra >> 12) & 7) = tmp;
			break;
		case 2: /* BFCHG */
			newtmp = ~ newtmp;
			if (width != 0) {
				newtmp &= ((1 << width) - 1);
			}
			break;
		case 3: /* BFEXTS */
			if (NFLG != 0) {
				m68k_dreg((extra >> 12) & 7) = tmp
					| ((width == 0) ? 0 : (-1 << width));
			} else {
				m68k_dreg((extra >> 12) & 7) = tmp;
			}
			break;
		case 4: /* BFCLR */
			newtmp = 0;
			break;
		case 5: /* BFFFO */
			{
				ui5b mask = 1 << ((width == 0) ? 31 : (width - 1));
				ui5r i = offset;

				while ((0 != mask) && (0 == (tmp & mask))) {
					mask >>= 1;
					i++;
				}
				m68k_dreg((extra >> 12) & 7) = i;
			}
			break;
		case 6: /* BFSET */
			newtmp = (width == 0) ? ~ 0 : ((1 << width) - 1);
			break;
		case 7: /* BFINS */
			newtmp = m68k_dreg((extra >> 12) & 7);
			if (width != 0) {
				newtmp &= ((1 << width) - 1);
			}
			break;
	}

	if (newtmp != tmp) {

		if (width != 0) {
			newtmp <<= (32 - width);
		}

		if (V_regs.CurDecOpY.v[0].AMd == 0) {
			ui5b mask = ~ 0;

			if (width != 0) {
				mask <<= (32 - width);
			}

			if (0 != offset) {
				newtmp = (newtmp >> offset) | (newtmp << (32 - offset));
				mask = (mask >> offset) | (mask << (32 - offset));
			}

			bf0 = (bf0 & ~ mask) | (newtmp);
			m68k_dreg(dstreg) = bf0;
		} else {

			/* if (offwid > 0) */ {
				ui3b mask = ~ (0xFF >> offset);

				bf1 = newtmp >> (24 + offset);
				if (offwid < 8) {
					mask |= (0xFF >> offwid);
				}
				if (mask != 0) {
					bf1 |= bfa[0] & mask;
				}
				put_byte(dsta + 0, bf1);
			}
			if (offwid > 8) {
				bf1 = newtmp >> (16 + offset);
				if (offwid < 16) {
					bf1 |= (bfa[1] & (0xFF >> (offwid - 8)));
				}
				put_byte(dsta + 1, bf1);
			}
			if (offwid > 16) {
				bf1 = newtmp >> (8 + offset);
				if (offwid < 24) {
					bf1 |= (bfa[2] & (0xFF >> (offwid - 16)));
				}
				put_byte(dsta + 2, bf1);
			}
			if (offwid > 24) {
				bf1 = newtmp >> (offset);
				if (offwid < 32) {
					bf1 |= (bfa[3] & (0xFF >> (offwid - 24)));
				}
				put_byte(dsta + 3, bf1);
			}
			if (offwid > 32) {
				bf1 = newtmp << (8 - offset);
				bf1 |= (bfa[4] & (0xFF >> (offwid - 32)));
				put_byte(dsta + 4, bf1);
			}
		}
	}
}
#endif

#if EmMMU | EmFPU
LOCALFUNC blnr DecodeModeRegister(ui5b sz)
{
	blnr IsOk;
	ui4r Dat = V_regs.CurDecOpY.v[0].ArgDat;
	ui4r themode = (Dat >> 3) & 7;
	ui4r thereg = Dat & 7;

	switch (themode) {
		case 0 :
			V_regs.ArgKind = AKRegister;
			V_regs.ArgAddr.rga = &V_regs.regs[thereg];
			IsOk = trueblnr;
			break;
		case 1 :
			V_regs.ArgKind = AKRegister;
			V_regs.ArgAddr.rga = &V_regs.regs[thereg + 8];
			IsOk = trueblnr;
			break;
		case 2 :
			V_regs.ArgKind = AKMemory;
			V_regs.ArgAddr.mem = m68k_areg(thereg);
			IsOk = trueblnr;
			break;
		case 3 :
			V_regs.ArgKind = AKMemory;
			V_regs.ArgAddr.mem = m68k_areg(thereg);
			if ((thereg == 7) && (sz == 1)) {
				m68k_areg(thereg) += 2;
			} else {
				m68k_areg(thereg) += sz;
			}
			IsOk = trueblnr;
			break;
		case 4 :
			V_regs.ArgKind = AKMemory;
			if ((thereg == 7) && (sz == 1)) {
				m68k_areg(thereg) -= 2;
			} else {
				m68k_areg(thereg) -= sz;
			}
			V_regs.ArgAddr.mem = m68k_areg(thereg);
			IsOk = trueblnr;
			break;
		case 5 :
			V_regs.ArgKind = AKMemory;
			V_regs.ArgAddr.mem = m68k_areg(thereg)
				+ nextiSWord();
			IsOk = trueblnr;
			break;
		case 6 :
			V_regs.ArgKind = AKMemory;
			V_regs.ArgAddr.mem = get_disp_ea(m68k_areg(thereg));
			IsOk = trueblnr;
			break;
		case 7 :
			switch (thereg) {
				case 0 :
					V_regs.ArgKind = AKMemory;
					V_regs.ArgAddr.mem = nextiSWord();
					IsOk = trueblnr;
					break;
				case 1 :
					V_regs.ArgKind = AKMemory;
					V_regs.ArgAddr.mem = nextilong();
					IsOk = trueblnr;
					break;
				case 2 :
					V_regs.ArgKind = AKMemory;
					V_regs.ArgAddr.mem = m68k_getpc();
					V_regs.ArgAddr.mem += nextiSWord();
					IsOk = trueblnr;
					break;
				case 3 :
					V_regs.ArgKind = AKMemory;
					V_regs.ArgAddr.mem = get_disp_ea(m68k_getpc());
					IsOk = trueblnr;
					break;
				case 4 :
					V_regs.ArgKind = AKMemory;
					V_regs.ArgAddr.mem = m68k_getpc();
					if (sz == 1) {
						++V_regs.ArgAddr.mem;
					}
					m68k_setpc(V_regs.ArgAddr.mem + sz);
					IsOk = trueblnr;
					break;
				default:
					IsOk = falseblnr;
					break;
			}
			break;
		default:
			IsOk = falseblnr;
			break;
	}

	return IsOk;
}
#endif

#if EmMMU | EmFPU
LOCALFUNC ui5r GetArgValueL(void)
{
	ui5r v;

	if (AKMemory == V_regs.ArgKind) {
		v = get_long(V_regs.ArgAddr.mem);
	} else {
		/* must be AKRegister */
		v = ui5r_FromSLong(*V_regs.ArgAddr.rga);
	}

	return v;
}
#endif

#if EmMMU | EmFPU
LOCALFUNC ui5r GetArgValueW(void)
{
	ui5r v;

	if (AKMemory == V_regs.ArgKind) {
		v = get_word(V_regs.ArgAddr.mem);
	} else {
		/* must be AKRegister */
		v = ui5r_FromSWord(*V_regs.ArgAddr.rga);
	}

	return v;
}
#endif

#if EmMMU | EmFPU
LOCALFUNC ui5r GetArgValueB(void)
{
	ui5r v;

	if (AKMemory == V_regs.ArgKind) {
		v = get_byte(V_regs.ArgAddr.mem);
	} else {
		/* must be AKRegister */
		v = ui5r_FromSByte(*V_regs.ArgAddr.rga);
	}

	return v;
}
#endif

#if EmMMU | EmFPU
LOCALPROC SetArgValueL(ui5r v)
{
	if (AKMemory == V_regs.ArgKind) {
		put_long(V_regs.ArgAddr.mem, v);
	} else {
		/* must be AKRegister */
		*V_regs.ArgAddr.rga = v;
	}
}
#endif

#if EmMMU | EmFPU
LOCALPROC SetArgValueW(ui5r v)
{
	if (AKMemory == V_regs.ArgKind) {
		put_word(V_regs.ArgAddr.mem, v);
	} else {
		/* must be AKRegister */
		*V_regs.ArgAddr.rga =
			(*V_regs.ArgAddr.rga & ~ 0xffff) | ((v) & 0xffff);
	}
}
#endif

#if EmMMU | EmFPU
LOCALPROC SetArgValueB(ui5r v)
{
	if (AKMemory == V_regs.ArgKind) {
		put_byte(V_regs.ArgAddr.mem, v);
	} else {
		/* must be AKRegister */
		*V_regs.ArgAddr.rga =
			(*V_regs.ArgAddr.rga & ~ 0xff) | ((v) & 0xff);
	}
}
#endif


#if EmMMU
LOCALIPROC DoCodeMMU(void)
{
	/*
		Emulate enough of MMU for System 7.5.5 universal
		to boot on Mac Plus 68020. There is one
		spurious "PMOVE TC, (A0)".
		And implement a few more PMOVE operations seen
		when running Disk Copy 6.3.3 and MacsBug.
	*/
	ui4r opcode = ((ui4r)(V_regs.CurDecOpY.v[0].AMd) << 8)
		| V_regs.CurDecOpY.v[0].ArgDat;
	if (opcode == 0xF010) {
		ui4b ew = (int)nextiword_nm();
		if (ew == 0x4200) {
			/* PMOVE TC, (A0) */
			/* fprintf(stderr, "0xF010 0x4200\n"); */
			if (DecodeModeRegister(4)) {
				SetArgValueL(0);
				return;
			}
		} else if ((ew == 0x4E00) || (ew == 0x4A00)) {
			/* PMOVE CRP, (A0) and PMOVE SRP, (A0) */
			/* fprintf(stderr, "0xF010 %x\n", ew); */
			if (DecodeModeRegister(4)) {
				SetArgValueL(0x7FFF0001);
				V_regs.ArgAddr.mem += 4;
				SetArgValueL(0);
				return;
			}
		} else if (ew == 0x6200) {
			/* PMOVE MMUSR, (A0) */
			/* fprintf(stderr, "0xF010 %x\n", ew); */
			if (DecodeModeRegister(2)) {
				SetArgValueW(0);
				return;
			}
		}
		/* fprintf(stderr, "extensions %x\n", ew); */
		BackupPC();
	}
	/* fprintf(stderr, "opcode %x\n", (int)opcode); */
	ReportAbnormalID(0x0121, "MMU op");
	DoCodeFdefault();
}
#endif

#if EmFPU

#include "FPMATHEM.h"
#include "FPCPEMDV.h"

#endif

#if HaveGlbReg
LOCALPROC Em_Swap(void)
{
#ifdef r_pc_p
	{
		ui3p t = g_pc_p;
		g_pc_p = regs.pc_p;
		regs.pc_p = t;
	}
#endif
#ifdef r_MaxCyclesToGo
	{
		si5rr t = g_MaxCyclesToGo;
		g_MaxCyclesToGo = regs.MaxCyclesToGo;
		regs.MaxCyclesToGo = t;
	}
#endif
#ifdef r_pc_pHi
	{
		ui3p t = g_pc_pHi;
		g_pc_pHi = regs.pc_pHi;
		regs.pc_pHi = t;
	}
#endif
#ifdef r_regs
	{
		struct regstruct *t = g_regs;
		g_regs = regs.save_regs;
		regs.save_regs = t;
	}
#endif
}
#endif

#if HaveGlbReg
#define Em_Enter Em_Swap
#else
#define Em_Enter()
#endif

#if HaveGlbReg
#define Em_Exit Em_Swap
#else
#define Em_Exit()
#endif

#if HaveGlbReg
LOCALFUNC blnr LocalMemAccessNtfy(ATTep pT)
{
	blnr v;

	Em_Exit();
	v = MemAccessNtfy(pT);
	Em_Enter();

	return v;
}
#else
#define LocalMemAccessNtfy MemAccessNtfy
#endif

#if HaveGlbReg
LOCALFUNC ui5b LocalMMDV_Access(ATTep p, ui5b Data,
	blnr WriteMem, blnr ByteSize, CPTR addr)
{
	ui5b v;

	Em_Exit();
	v = MMDV_Access(p, Data, WriteMem, ByteSize, addr);
	Em_Enter();

	return v;
}
#else
#define LocalMMDV_Access MMDV_Access
#endif

LOCALPROC local_customreset(void)
{
	Em_Exit();
	customreset();
	Em_Enter();
}

LOCALFUNC ATTep LocalFindATTel(CPTR addr)
{
	ATTep prev;
	ATTep p;

	p = V_regs.HeadATTel;
	if ((addr & p->cmpmask) != p->cmpvalu) {
		do {
			prev = p;
			p = p->Next;
		} while ((addr & p->cmpmask) != p->cmpvalu);

		{
			ATTep next = p->Next;

			if (nullpr == next) {
				/* don't move the end guard */
			} else {
				/* move to first */
				prev->Next = next;
				p->Next = V_regs.HeadATTel;
				V_regs.HeadATTel = p;
			}
		}
	}

	return p;
}

LOCALPROC SetUpMATC(
	MATCp CurMATC,
	ATTep p)
{
	CurMATC->cmpmask = p->cmpmask;
	CurMATC->usemask = p->usemask;
	CurMATC->cmpvalu = p->cmpvalu;
	CurMATC->usebase = p->usebase;
}

LOCALFUNC ui5r my_reg_call get_byte_ext(CPTR addr)
{
	ATTep p;
	ui3p m;
	ui5r AccFlags;
	ui5r Data;

Label_Retry:
	p = LocalFindATTel(addr);
	AccFlags = p->Access;

	if (0 != (AccFlags & kATTA_readreadymask)) {
		SetUpMATC(&V_regs.MATCrdB, p);
		m = p->usebase + (addr & p->usemask);

		Data = *m;
	} else if (0 != (AccFlags & kATTA_mmdvmask)) {
		Data = LocalMMDV_Access(p, 0, falseblnr, trueblnr, addr);
	} else if (0 != (AccFlags & kATTA_ntfymask)) {
		if (LocalMemAccessNtfy(p)) {
			goto Label_Retry;
		} else {
			Data = 0; /* fail */
		}
	} else {
		Data = 0; /* fail */
	}

	return ui5r_FromSByte(Data);
}

LOCALPROC my_reg_call put_byte_ext(CPTR addr, ui5r b)
{
	ATTep p;
	ui3p m;
	ui5r AccFlags;

Label_Retry:
	p = LocalFindATTel(addr);
	AccFlags = p->Access;

	if (0 != (AccFlags & kATTA_writereadymask)) {
		SetUpMATC(&V_regs.MATCwrB, p);
		m = p->usebase + (addr & p->usemask);
		*m = b;
	} else if (0 != (AccFlags & kATTA_mmdvmask)) {
		(void) LocalMMDV_Access(p, b & 0x00FF,
			trueblnr, trueblnr, addr);
	} else if (0 != (AccFlags & kATTA_ntfymask)) {
		if (LocalMemAccessNtfy(p)) {
			goto Label_Retry;
		} else {
			/* fail */
		}
	} else {
		/* fail */
	}
}

LOCALFUNC ui5r my_reg_call get_word_ext(CPTR addr)
{
	ui5r Data;

	if (0 != (addr & 0x01)) {
		ui5r hi = get_byte(addr);
		ui5r lo = get_byte(addr + 1);
		Data = ((hi << 8) & 0x0000FF00)
			| (lo & 0x000000FF);
	} else {
		ATTep p;
		ui3p m;
		ui5r AccFlags;

Label_Retry:
		p = LocalFindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_readreadymask)) {
			SetUpMATC(&V_regs.MATCrdW, p);
			V_regs.MATCrdW.cmpmask |= 0x01;
			m = p->usebase + (addr & p->usemask);
			Data = do_get_mem_word(m);
		} else if (0 != (AccFlags & kATTA_mmdvmask)) {
			Data = LocalMMDV_Access(p, 0, falseblnr, falseblnr, addr);
		} else if (0 != (AccFlags & kATTA_ntfymask)) {
			if (LocalMemAccessNtfy(p)) {
				goto Label_Retry;
			} else {
				Data = 0; /* fail */
			}
		} else {
			Data = 0; /* fail */
		}
	}

	return ui5r_FromSWord(Data);
}

LOCALPROC my_reg_call put_word_ext(CPTR addr, ui5r w)
{
	if (0 != (addr & 0x01)) {
		put_byte(addr, w >> 8);
		put_byte(addr + 1, w);
	} else {
		ATTep p;
		ui3p m;
		ui5r AccFlags;

Label_Retry:
		p = LocalFindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_writereadymask)) {
			SetUpMATC(&V_regs.MATCwrW, p);
			V_regs.MATCwrW.cmpmask |= 0x01;
			m = p->usebase + (addr & p->usemask);
			do_put_mem_word(m, w);
		} else if (0 != (AccFlags & kATTA_mmdvmask)) {
			(void) LocalMMDV_Access(p, w & 0x0000FFFF,
				trueblnr, falseblnr, addr);
		} else if (0 != (AccFlags & kATTA_ntfymask)) {
			if (LocalMemAccessNtfy(p)) {
				goto Label_Retry;
			} else {
				/* fail */
			}
		} else {
			/* fail */
		}
	}
}

LOCALFUNC ui5r my_reg_call get_long_misaligned_ext(CPTR addr)
{
	ui5r hi = get_word(addr);
	ui5r lo = get_word(addr + 2);
	ui5r Data = ((hi << 16) & 0xFFFF0000)
		| (lo & 0x0000FFFF);

	return ui5r_FromSLong(Data);
}

LOCALPROC my_reg_call put_long_misaligned_ext(CPTR addr, ui5r l)
{
	put_word(addr, l >> 16);
	put_word(addr + 2, l);
}

#if FasterAlignedL
LOCALFUNC ui5r my_reg_call get_long_ext(CPTR addr)
{
	ui5r Data;

	if (0 != (addr & 0x03)) {
		ui5r hi = get_word(addr);
		ui5r lo = get_word(addr + 2);
		Data = ((hi << 16) & 0xFFFF0000)
			| (lo & 0x0000FFFF);
	} else {
		ATTep p;
		ui3p m;
		ui5r AccFlags;

Label_Retry:
		p = LocalFindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_readreadymask)) {
			SetUpMATC(&V_regs.MATCrdL, p);
			V_regs.MATCrdL.cmpmask |= 0x03;
			m = p->usebase + (addr & p->usemask);
			Data = do_get_mem_long(m);
		} else if (0 != (AccFlags & kATTA_mmdvmask)) {
			ui5r hi = LocalMMDV_Access(p, 0,
				falseblnr, falseblnr, addr);
			ui5r lo = LocalMMDV_Access(p, 0,
				falseblnr, falseblnr, addr + 2);
			Data = ((hi << 16) & 0xFFFF0000)
				| (lo & 0x0000FFFF);
		} else if (0 != (AccFlags & kATTA_ntfymask)) {
			if (LocalMemAccessNtfy(p)) {
				goto Label_Retry;
			} else {
				Data = 0; /* fail */
			}
		} else {
			Data = 0; /* fail */
		}
	}

	return ui5r_FromSLong(Data);
}
#endif

#if FasterAlignedL
LOCALPROC my_reg_call put_long_ext(CPTR addr, ui5r l)
{
	if (0 != (addr & 0x03)) {
		put_word(addr, l >> 16);
		put_word(addr + 2, l);
	} else {
		ATTep p;
		ui3p m;
		ui5r AccFlags;

Label_Retry:
		p = LocalFindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_writereadymask)) {
			SetUpMATC(&V_regs.MATCwrL, p);
			V_regs.MATCwrL.cmpmask |= 0x03;
			m = p->usebase + (addr & p->usemask);
			do_put_mem_long(m, l);
		} else if (0 != (AccFlags & kATTA_mmdvmask)) {
			(void) LocalMMDV_Access(p, (l >> 16) & 0x0000FFFF,
				trueblnr, falseblnr, addr);
			(void) LocalMMDV_Access(p, l & 0x0000FFFF,
				trueblnr, falseblnr, addr + 2);
		} else if (0 != (AccFlags & kATTA_ntfymask)) {
			if (LocalMemAccessNtfy(p)) {
				goto Label_Retry;
			} else {
				/* fail */
			}
		} else {
			/* fail */
		}
	}
}
#endif

LOCALPROC Recalc_PC_Block(void)
{
	ATTep p;
	CPTR curpc = m68k_getpc();

Label_Retry:
	p = LocalFindATTel(curpc);
	if (my_cond_rare(0 == (p->Access & kATTA_readreadymask))) {
		if (0 != (p->Access & kATTA_ntfymask)) {
			if (LocalMemAccessNtfy(p)) {
				goto Label_Retry;
			}
		}
		/* in trouble if get here */
#if ExtraAbnormalReports
		ReportAbnormalID(0x0122, "Recalc_PC_Block fails");
			/* happens on Restart */
#endif

		V_regs.pc_pLo = V_regs.fakeword;
		V_pc_p = V_regs.pc_pLo;
		V_pc_pHi = V_regs.pc_pLo + 2;
		V_regs.pc = curpc;
	} else {
		ui5r m2 = p->usemask & ~ p->cmpmask;
		m2 = m2 & ~ (m2 + 1);

		V_pc_p = p->usebase + (curpc & p->usemask);
		V_regs.pc_pLo = V_pc_p - (curpc & m2);
		V_pc_pHi = V_regs.pc_pLo + m2 + 1;
		V_regs.pc = curpc - (V_pc_p - V_regs.pc_pLo);
	}
}

LOCALFUNC ui5r my_reg_call Recalc_PC_BlockReturnUi5r(ui5r v)
{
	/*
		Used to prevent compiler from saving
		register on the stack in calling
		functions, when Recalc_PC_Block isn't being called.
	*/
	Recalc_PC_Block();

	return v;
}

LOCALFUNC ui5r nextilong_ext(void)
{
	ui5r r;

	V_pc_p -= 4;

	{
		ui5r hi = nextiword();
		ui5r lo = nextiword();
		r = ((hi << 16) & 0xFFFF0000)
			| (lo & 0x0000FFFF);
	}

	return r;
}

LOCALPROC DoCheckExternalInterruptPending(void)
{
	ui3r level = *V_regs.fIPL;
	if ((level > V_regs.intmask) || (level == 7)) {
#if WantCloserCyc
		V_MaxCyclesToGo -=
			(44 * kCycleScale + 5 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(24 + level);
		V_regs.intmask = level;
	}
}

LOCALPROC do_trace(void)
{
	V_regs.TracePending = trueblnr;
	NeedToGetOut();
}

GLOBALPROC m68k_go_nCycles(ui5b n)
{
	Em_Enter();
	V_MaxCyclesToGo += (n + V_regs.ResidualCycles);
	while (V_MaxCyclesToGo > 0) {

#if 0
		if (V_regs.ResetPending) {
			m68k_DoReset();
		}
#endif
		if (V_regs.TracePending) {
#if WantCloserCyc
			V_MaxCyclesToGo -= (34 * kCycleScale
				+ 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
			Exception(9);
		}
		if (V_regs.ExternalInterruptPending) {
			V_regs.ExternalInterruptPending = falseblnr;
			DoCheckExternalInterruptPending();
		}
		if (V_regs.t1 != 0) {
			do_trace();
		}
		m68k_go_MaxCycles();
		V_MaxCyclesToGo += V_regs.MoreCyclesToGo;
		V_regs.MoreCyclesToGo = 0;
	}

	V_regs.ResidualCycles = V_MaxCyclesToGo;
	V_MaxCyclesToGo = 0;
	Em_Exit();
}

GLOBALFUNC si5r GetCyclesRemaining(void)
{
	si5r v;

	Em_Enter();
	v = V_regs.MoreCyclesToGo + V_MaxCyclesToGo;
	Em_Exit();

	return v;
}

GLOBALPROC SetCyclesRemaining(si5r n)
{
	Em_Enter();

	if (V_MaxCyclesToGo >= n) {
		V_regs.MoreCyclesToGo = 0;
		V_MaxCyclesToGo = n;
	} else {
		V_regs.MoreCyclesToGo = n - V_MaxCyclesToGo;
	}

	Em_Exit();
}

GLOBALFUNC ATTep FindATTel(CPTR addr)
{
	ATTep v;

	Em_Enter();
	v = LocalFindATTel(addr);
	Em_Exit();

	return v;
}

GLOBALFUNC ui3r get_vm_byte(CPTR addr)
{
	ui3r v;

	Em_Enter();
	v = (ui3b) get_byte(addr);
	Em_Exit();

	return v;
}

GLOBALFUNC ui4r get_vm_word(CPTR addr)
{
	ui4r v;

	Em_Enter();
	v = (ui4b) get_word(addr);
	Em_Exit();

	return v;
}

GLOBALFUNC ui5r get_vm_long(CPTR addr)
{
	ui5r v;

	Em_Enter();
	v = (ui5b) get_long(addr);
	Em_Exit();

	return v;
}

GLOBALPROC put_vm_byte(CPTR addr, ui3r b)
{
	Em_Enter();
	put_byte(addr, ui5r_FromSByte(b));
	Em_Exit();
}

GLOBALPROC put_vm_word(CPTR addr, ui4r w)
{
	Em_Enter();
	put_word(addr, ui5r_FromSWord(w));
	Em_Exit();
}

GLOBALPROC put_vm_long(CPTR addr, ui5r l)
{
	Em_Enter();
	put_long(addr, ui5r_FromSLong(l));
	Em_Exit();
}

GLOBALPROC SetHeadATTel(ATTep p)
{
	Em_Enter();

	V_regs.MATCrdB.cmpmask = 0;
	V_regs.MATCrdB.cmpvalu = 0xFFFFFFFF;
	V_regs.MATCwrB.cmpmask = 0;
	V_regs.MATCwrB.cmpvalu = 0xFFFFFFFF;
	V_regs.MATCrdW.cmpmask = 0;
	V_regs.MATCrdW.cmpvalu = 0xFFFFFFFF;
	V_regs.MATCwrW.cmpmask = 0;
	V_regs.MATCwrW.cmpvalu = 0xFFFFFFFF;
#if FasterAlignedL
	V_regs.MATCrdL.cmpmask = 0;
	V_regs.MATCrdL.cmpvalu = 0xFFFFFFFF;
	V_regs.MATCwrL.cmpmask = 0;
	V_regs.MATCwrL.cmpvalu = 0xFFFFFFFF;
#endif
	/* force Recalc_PC_Block soon */
		V_regs.pc = m68k_getpc();
		V_regs.pc_pLo = V_pc_p;
		V_pc_pHi = V_regs.pc_pLo + 2;
	V_regs.HeadATTel = p;

	Em_Exit();
}

GLOBALPROC DiskInsertedPsuedoException(CPTR newpc, ui5b data)
{
	Em_Enter();
	ExceptionTo(newpc
#if Use68020
		, 0
#endif
		);
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), data);
	Em_Exit();
}

GLOBALPROC m68k_IPLchangeNtfy(void)
{
	Em_Enter();
	{
		ui3r level = *V_regs.fIPL;

		if ((level > V_regs.intmask) || (level == 7)) {
			SetExternalInterruptPending();
		}
	}
	Em_Exit();
}

#if WantDumpTable
LOCALPROC InitDumpTable(void)
{
	si5b i;

	for (i = 0; i < kNumIKinds; ++i) {
		DumpTable[i] = 0;
	}
}

LOCALPROC DumpATable(ui5b *p, ui5b n)
{
	si5b i;

	for (i = 0; i < n; ++i) {
		dbglog_writeNum(p[i]);
		dbglog_writeReturn();
	}
}

EXPORTPROC DoDumpTable(void);
GLOBALPROC DoDumpTable(void)
{
	DumpATable(DumpTable, kNumIKinds);
}
#endif

GLOBALPROC m68k_reset(void)
{
	Em_Enter();

#if WantDumpTable
	InitDumpTable();
#endif
	V_MaxCyclesToGo = 0;
	V_regs.MoreCyclesToGo = 0;
	V_regs.ResidualCycles = 0;
	V_pc_p = (ui3p)nullpr;
	V_pc_pHi = (ui3p)nullpr;
	V_regs.pc_pLo = (ui3p)nullpr;

	do_put_mem_word(V_regs.fakeword, 0x4AFC);
		/* illegal instruction opcode */

#if 0
	V_regs.ResetPending = trueblnr;
	NeedToGetOut();
#else
/* Sets the MC68000 reset jump vector... */
	m68k_setpc(get_long(0x00000004));

/* Sets the initial stack vector... */
	m68k_areg(7) = get_long(0x00000000);

	V_regs.s = 1;
#if Use68020
	V_regs.m = 0;
	V_regs.t0 = 0;
#endif
	V_regs.t1 = 0;
	ZFLG = CFLG = NFLG = VFLG = 0;

	V_regs.LazyFlagKind = kLazyFlagsDefault;
	V_regs.LazyXFlagKind = kLazyFlagsDefault;

	V_regs.ExternalInterruptPending = falseblnr;
	V_regs.TracePending = falseblnr;
	V_regs.intmask = 7;

#if Use68020
	V_regs.sfc = 0;
	V_regs.dfc = 0;
	V_regs.vbr = 0;
	V_regs.cacr = 0;
	V_regs.caar = 0;
#endif
#endif

	Em_Exit();
}

#if SmallGlobals
GLOBALPROC MINEM68K_ReserveAlloc(void)
{
	ReserveAllocOneBlock((ui3p *)&regs.disp_table,
		disp_table_sz * 8, 6, falseblnr);
}
#endif

GLOBALPROC MINEM68K_Init(
	ui3b *fIPL)
{
	regs.fIPL = fIPL;
#ifdef r_regs
	regs.save_regs = &regs;
#endif

	M68KITAB_setup(regs.disp_table);
}
