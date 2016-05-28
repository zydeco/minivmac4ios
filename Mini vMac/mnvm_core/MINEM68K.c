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
	MINimum EMulator of 68K cpu

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

typedef unsigned char flagtype;


/* Memory Address Translation Cache record */

struct MATCr {
	ui5r cmpmask;
	ui5r cmpvalu;
	ui5r usemask;
	ui3p usebase;
};
typedef struct MATCr MATCr;
typedef MATCr *MATCp;

/*
	This variable was introduced because a program could do a Bcc from
	within chip memory to a location within expansion memory. With a
	pointer variable the program counter would point to the wrong
	location. With this variable unset the program counter is always
	correct, but programs will run slower (about 4%).
	Usually, you'll want to have this defined.

	vMac REQUIRES this. It allows for fun things like Restart.
*/

#ifndef USE_POINTER
#define USE_POINTER 1
#endif

#define AKMemory 0
#define AKRegister 1
#define AKConstant 2

union ArgAddrT {
	ui5r mem;
	ui5r *rga;
};
typedef union ArgAddrT ArgAddrT;

LOCALVAR struct regstruct
{
	ui5r regs[16]; /* Data and Address registers */
	ui5r pc; /* Program Counter */
	CPTR usp; /* User Stack Pointer */
	CPTR isp; /* Interrupt Stack Pointer */
#if Use68020
	CPTR msp; /* Master Stack Pointer */
#endif

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

	flagtype TracePending;
	flagtype ExternalInterruptPending;
#if 0
	flagtype ResetPending;
#endif
	ui3b *fIPL;

#if Use68020
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
	MATCr MATCrdB;
	MATCr MATCwrB;
	MATCr MATCrdW;
	MATCr MATCwrW;
	MATCr MATCex;
	ATTep HeadATTel;
	DecOpR CurDecOp;


#if USE_POINTER
	ui3p pc_p;
	ui3p pc_oldp;
#endif
	ui5b opsize;
	ui5b ArgKind;
	ArgAddrT ArgAddr;
	ui5b SrcVal;
	ui5b opcode;
	si5r MaxCyclesToGo;
	si5r MoreCyclesToGo;
	si5r ResidualCycles;
	ui3b fakeword[2];

#define disp_table_sz (256 * 256)
#if SmallGlobals
	DecOpR *disp_table;
#else
	DecOpR disp_table[disp_table_sz];
#endif
} regs;

#define ui5r_MSBisSet(x) (((si5r)(x)) < 0)

#define ZFLG regs.z
#define NFLG regs.n
#define CFLG regs.c
#define VFLG regs.v
#define XFLG regs.x

LOCALFUNC ui4b m68k_getCR(void)
{
	return (XFLG << 4) | (NFLG << 3) | (ZFLG << 2)
		| (VFLG << 1) | CFLG;
}

LOCALFUNC MayInline void m68k_setCR(ui4b newcr)
{
	XFLG = (newcr >> 4) & 1;
	NFLG = (newcr >> 3) & 1;
	ZFLG = (newcr >> 2) & 1;
	VFLG = (newcr >> 1) & 1;
	CFLG = newcr & 1;
}

LOCALFUNC MayInline blnr cctrue(void)
{
	switch ((regs.opcode >> 8) & 15) {
		case 0:  return trueblnr;                   /* T */
		case 1:  return falseblnr;                  /* F */
		case 2:  return (! CFLG) && (! ZFLG);       /* HI */
		case 3:  return CFLG || ZFLG;               /* LS */
		case 4:  return ! CFLG;                     /* CC */
		case 5:  return CFLG;                       /* CS */
		case 6:  return ! ZFLG;                     /* NE */
		case 7:  return ZFLG;                       /* EQ */
		case 8:  return ! VFLG;                     /* VC */
		case 9:  return VFLG;                       /* VS */
		case 10: return ! NFLG;                     /* PL */
		case 11: return NFLG;                       /* MI */
		case 12: return NFLG == VFLG;               /* GE */
		case 13: return NFLG != VFLG;               /* LT */
		case 14: return (! ZFLG) && (NFLG == VFLG); /* GT */
		case 15: return ZFLG || (NFLG != VFLG);     /* LE */
		default: return falseblnr; /* shouldn't get here */
	}
}

LOCALPROC ALU_CmpB(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result0 = dstvalue - srcvalue;
	ui5r result1 = ui5r_FromUByte(dstvalue) - ui5r_FromUByte(srcvalue);
	ui5r result = ui5r_FromSByte(result0);

	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (((result0 >> 1) ^ result0) >> 7) & 1;
	CFLG = (result1 >> 8) & 1;
}

LOCALPROC ALU_CmpW(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result0 = dstvalue - srcvalue;
	ui5r result1 = ui5r_FromUWord(dstvalue) - ui5r_FromUWord(srcvalue);
	ui5r result = ui5r_FromSWord(result0);

	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	CFLG = (result1 >> 16) & 1;
}

LOCALPROC ALU_CmpL(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(dstvalue - srcvalue);

	int flgs = ui5r_MSBisSet(srcvalue);
	int flgo = ui5r_MSBisSet(dstvalue);
	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs != flgo) && (NFLG != flgo);
	CFLG = (flgs && ! flgo) || (NFLG && ((! flgo) || flgs));
}

LOCALFUNC ui5r ALU_AddB(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result0 = dstvalue + srcvalue;
	ui5r result1 = ui5r_FromUByte(dstvalue) + ui5r_FromUByte(srcvalue);
	ui5r result = ui5r_FromSByte(result0);

	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (((result0 >> 1) ^ result0) >> 7) & 1;
	XFLG = CFLG = (result1 >> 8);

	return result;
}

LOCALFUNC ui5r ALU_AddW(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result0 = dstvalue + srcvalue;
	ui5r result1 = ui5r_FromUWord(dstvalue) + ui5r_FromUWord(srcvalue);
	ui5r result = ui5r_FromSWord(result0);

	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	XFLG = CFLG = (result1 >> 16);

	return result;
}

LOCALFUNC ui5r ALU_AddL(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(dstvalue + srcvalue);

	int flgs = ui5r_MSBisSet(srcvalue);
	int flgo = ui5r_MSBisSet(dstvalue);
	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && flgo && ! NFLG) || ((! flgs) && (! flgo) && NFLG);
	XFLG = CFLG = (flgs && flgo) || ((! NFLG) && (flgo || flgs));

	return result;
}

LOCALFUNC ui5r ALU_SubB(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result0 = dstvalue - srcvalue;
	ui5r result1 = ui5r_FromUByte(dstvalue) - ui5r_FromUByte(srcvalue);
	ui5r result = ui5r_FromSByte(result0);

	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (((result0 >> 1) ^ result0) >> 7) & 1;
	XFLG = CFLG = (result1 >> 8) & 1;

	return result;
}

LOCALFUNC ui5r ALU_SubW(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result0 = dstvalue - srcvalue;
	ui5r result1 = ui5r_FromUWord(dstvalue) - ui5r_FromUWord(srcvalue);
	ui5r result = ui5r_FromSWord(result0);

	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (((result0 >> 1) ^ result0) >> 15) & 1;
	XFLG = CFLG = (result1 >> 16) & 1;

	return result;
}

LOCALFUNC ui5r ALU_SubL(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(dstvalue - srcvalue);

	int flgs = ui5r_MSBisSet(srcvalue);
	int flgo = ui5r_MSBisSet(dstvalue);
	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs != flgo) && (NFLG != flgo);
	XFLG = CFLG = (flgs && ! flgo) || (NFLG && ((! flgo) || flgs));

	return result;
}

LOCALFUNC ui5r ALU_NegB(ui5r dstvalue)
{
	ui5r result = ui5r_FromSByte(0 - dstvalue);

	int flgs = ui5r_MSBisSet(dstvalue);
	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && NFLG);
	XFLG = CFLG = (flgs || NFLG);

	return result;
}

LOCALFUNC ui5r ALU_NegW(ui5r dstvalue)
{
	ui5r result = ui5r_FromSWord(0 - dstvalue);

	int flgs = ui5r_MSBisSet(dstvalue);
	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && NFLG);
	XFLG = CFLG = (flgs || NFLG);

	return result;
}

LOCALFUNC ui5r ALU_NegL(ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(0 - dstvalue);

	int flgs = ui5r_MSBisSet(dstvalue);
	ZFLG = (result == 0);
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && NFLG);
	XFLG = CFLG = (flgs || NFLG);

	return result;
}

LOCALFUNC ui5r ALU_NegXB(ui5r dstvalue)
{
	ui5r result = ui5r_FromSByte(0 - dstvalue - (XFLG ? 1 : 0));

	int flgs = ui5r_MSBisSet(dstvalue);
	if (result != 0) {
		ZFLG = 0;
	}
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && NFLG);
	XFLG = CFLG = (flgs || NFLG);

	return result;
}

LOCALFUNC ui5r ALU_NegXW(ui5r dstvalue)
{
	ui5r result = ui5r_FromSWord(0 - dstvalue - (XFLG ? 1 : 0));

	int flgs = ui5r_MSBisSet(dstvalue);
	if (result != 0) {
		ZFLG = 0;
	}
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && NFLG);
	XFLG = CFLG = (flgs || NFLG);

	return result;
}

LOCALFUNC ui5r ALU_NegXL(ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(0 - dstvalue - (XFLG ? 1 : 0));

	int flgs = ui5r_MSBisSet(dstvalue);
	if (result != 0) {
		ZFLG = 0;
	}
	NFLG = ui5r_MSBisSet(result);
	VFLG = (flgs && NFLG);
	XFLG = CFLG = (flgs || NFLG);

	return result;
}

LOCALPROC SetCCRforAddX(ui5r srcvalue, ui5r dstvalue, ui5r result)
{
	int flgs = ui5r_MSBisSet(srcvalue);
	int flgo = ui5r_MSBisSet(dstvalue);
	if (result != 0) {
		ZFLG = 0;
	}
	NFLG = ui5r_MSBisSet(result);
	XFLG = CFLG = (flgs && flgo) || ((! NFLG) && (flgo || flgs));
	VFLG = (flgs && flgo && ! NFLG) || ((! flgs) && (! flgo) && NFLG);
}

LOCALFUNC ui5r ALU_AddXB(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSByte(dstvalue + srcvalue + (XFLG ? 1 : 0));

	SetCCRforAddX(srcvalue, dstvalue, result);

	return result;
}

LOCALFUNC ui5r ALU_AddXW(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSWord(dstvalue + srcvalue + (XFLG ? 1 : 0));

	SetCCRforAddX(srcvalue, dstvalue, result);

	return result;
}

LOCALFUNC ui5r ALU_AddXL(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(dstvalue + srcvalue + (XFLG ? 1 : 0));

	SetCCRforAddX(srcvalue, dstvalue, result);

	return result;
}

LOCALPROC SetCCRforSubX(ui5r srcvalue, ui5r dstvalue, ui5r result)
{
	int flgs = ui5r_MSBisSet(srcvalue);
	int flgo = ui5r_MSBisSet(dstvalue);
	if (result != 0) {
		ZFLG = 0;
	}
	NFLG = ui5r_MSBisSet(result);
	VFLG = ((! flgs) && flgo && (! NFLG)) || (flgs && (! flgo) && NFLG);
	XFLG = CFLG = (flgs && (! flgo)) || (NFLG && ((! flgo) || flgs));
}

LOCALFUNC ui5r ALU_SubXB(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSByte(dstvalue - srcvalue - (XFLG ? 1 : 0));

	SetCCRforSubX(srcvalue, dstvalue, result);

	return result;
}

LOCALFUNC ui5r ALU_SubXW(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSWord(dstvalue - srcvalue - (XFLG ? 1 : 0));

	SetCCRforSubX(srcvalue, dstvalue, result);

	return result;
}

LOCALFUNC ui5r ALU_SubXL(ui5r srcvalue, ui5r dstvalue)
{
	ui5r result = ui5r_FromSLong(dstvalue - srcvalue - (XFLG ? 1 : 0));

	SetCCRforSubX(srcvalue, dstvalue, result);

	return result;
}


#define m68k_logExceptions (dbglog_HAVE && 0)


GLOBALFUNC ATTep FindATTel(CPTR addr)
{
	ATTep prev;
	ATTep p;

	p = regs.HeadATTel;
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
				p->Next = regs.HeadATTel;
				regs.HeadATTel = p;
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

LOCALFUNC ui5r get_byte_ext(CPTR addr)
{
	ATTep p;
	ui3p m;
	ui5r AccFlags;
	ui5r Data;

Label_Retry:
	p = FindATTel(addr);
	AccFlags = p->Access;

	if (0 != (AccFlags & kATTA_readreadymask)) {
		SetUpMATC(&regs.MATCrdB, p);
		m = p->usebase + (addr & p->usemask);

		Data = *m;
	} else if (0 != (AccFlags & kATTA_mmdvmask)) {
		Data = MMDV_Access(p, 0, falseblnr, trueblnr, addr);
	} else if (0 != (AccFlags & kATTA_ntfymask)) {
		if (MemAccessNtfy(p)) {
			goto Label_Retry;
		} else {
			Data = 0; /* fail */
		}
	} else {
		Data = 0; /* fail */
	}

	return ui5r_FromSByte(Data);
}

LOCALFUNC MayNotInline ui5r get_byte(CPTR addr)
{
	ui3p m = (addr & regs.MATCrdB.usemask) + regs.MATCrdB.usebase;

	if ((addr & regs.MATCrdB.cmpmask) == regs.MATCrdB.cmpvalu) {
		return ui5r_FromSByte(*m);
	} else {
		return get_byte_ext(addr);
	}
}

LOCALPROC put_byte_ext(CPTR addr, ui5r b)
{
	ATTep p;
	ui3p m;
	ui5r AccFlags;

Label_Retry:
	p = FindATTel(addr);
	AccFlags = p->Access;

	if (0 != (AccFlags & kATTA_writereadymask)) {
		SetUpMATC(&regs.MATCwrB, p);
		m = p->usebase + (addr & p->usemask);
		*m = b;
	} else if (0 != (AccFlags & kATTA_mmdvmask)) {
		(void) MMDV_Access(p, b & 0x00FF, trueblnr, trueblnr, addr);
	} else if (0 != (AccFlags & kATTA_ntfymask)) {
		if (MemAccessNtfy(p)) {
			goto Label_Retry;
		} else {
			/* fail */
		}
	} else {
		/* fail */
	}
}

LOCALPROC MayNotInline put_byte(CPTR addr, ui5r b)
{
	ui3p m = (addr & regs.MATCwrB.usemask) + regs.MATCwrB.usebase;
	if ((addr & regs.MATCwrB.cmpmask) == regs.MATCwrB.cmpvalu) {
		*m = b;
	} else {
		put_byte_ext(addr, b);
	}
}

LOCALFUNC ui5r get_word_ext(CPTR addr)
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
		p = FindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_readreadymask)) {
			SetUpMATC(&regs.MATCrdW, p);
			regs.MATCrdW.cmpmask |= 0x01;
			m = p->usebase + (addr & p->usemask);
			Data = do_get_mem_word(m);
		} else if (0 != (AccFlags & kATTA_mmdvmask)) {
			Data = MMDV_Access(p, 0, falseblnr, falseblnr, addr);
		} else if (0 != (AccFlags & kATTA_ntfymask)) {
			if (MemAccessNtfy(p)) {
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

LOCALFUNC MayNotInline ui5r get_word(CPTR addr)
{
	ui3p m = (addr & regs.MATCrdW.usemask) + regs.MATCrdW.usebase;
	if ((addr & regs.MATCrdW.cmpmask) == regs.MATCrdW.cmpvalu) {
		return ui5r_FromSWord(do_get_mem_word(m));
	} else {
		return get_word_ext(addr);
	}
}

LOCALPROC put_word_ext(CPTR addr, ui5r w)
{
	if (0 != (addr & 0x01)) {
		put_byte(addr, w >> 8);
		put_byte(addr + 1, w);
	} else {
		ATTep p;
		ui3p m;
		ui5r AccFlags;

Label_Retry:
		p = FindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_writereadymask)) {
			SetUpMATC(&regs.MATCwrW, p);
			regs.MATCwrW.cmpmask |= 0x01;
			m = p->usebase + (addr & p->usemask);
			do_put_mem_word(m, w);
		} else if (0 != (AccFlags & kATTA_mmdvmask)) {
			(void) MMDV_Access(p, w & 0x0000FFFF,
				trueblnr, falseblnr, addr);
		} else if (0 != (AccFlags & kATTA_ntfymask)) {
			if (MemAccessNtfy(p)) {
				goto Label_Retry;
			} else {
				/* fail */
			}
		} else {
			/* fail */
		}
	}
}

LOCALPROC MayNotInline put_word(CPTR addr, ui5r w)
{
	ui3p m = (addr & regs.MATCwrW.usemask) + regs.MATCwrW.usebase;
	if ((addr & regs.MATCwrW.cmpmask) == regs.MATCwrW.cmpvalu) {
		do_put_mem_word(m, w);
	} else {
		put_word_ext(addr, w);
	}
}

LOCALFUNC ui5r get_long_ext(CPTR addr)
{
	ui5r hi = get_word(addr);
	ui5r lo = get_word(addr + 2);
	ui5r Data = ((hi << 16) & 0xFFFF0000)
		| (lo & 0x0000FFFF);

	return ui5r_FromSLong(Data);
}

LOCALFUNC MayNotInline ui5r get_long(CPTR addr)
{
	CPTR addr2 = addr + 2;
	ui3p m = (addr & regs.MATCrdW.usemask) + regs.MATCrdW.usebase;
	ui3p m2 = (addr2 & regs.MATCrdW.usemask) + regs.MATCrdW.usebase;
	if (((addr & regs.MATCrdW.cmpmask) == regs.MATCrdW.cmpvalu)
		&& ((addr2 & regs.MATCrdW.cmpmask) == regs.MATCrdW.cmpvalu))
	{
		ui5r hi = do_get_mem_word(m);
		ui5r lo = do_get_mem_word(m2);
		ui5r Data = ((hi << 16) & 0xFFFF0000)
			| (lo & 0x0000FFFF);

		return ui5r_FromSLong(Data);
	} else {
		return get_long_ext(addr);
	}
}

LOCALPROC put_long_ext(CPTR addr, ui5r l)
{
	put_word(addr, l >> 16);
	put_word(addr + 2, l);
}

LOCALPROC MayNotInline put_long(CPTR addr, ui5r l)
{
	CPTR addr2 = addr + 2;
	ui3p m = (addr & regs.MATCwrW.usemask) + regs.MATCwrW.usebase;
	ui3p m2 = (addr2 & regs.MATCwrW.usemask) + regs.MATCwrW.usebase;
	if (((addr & regs.MATCwrW.cmpmask) == regs.MATCwrW.cmpvalu)
		&& ((addr2 & regs.MATCwrW.cmpmask) == regs.MATCwrW.cmpvalu))
	{
		do_put_mem_word(m, l >> 16);
		do_put_mem_word(m2, l);
	} else {
		put_long_ext(addr, l);
	}
}


#if ! USE_POINTER
LOCALFUNC ui4r get_pc_word_ext(void)
{
	ui4r Data;
	CPTR addr = regs.pc;

	if (0 != (addr & 0x01)) {
		Data = 0; /* fail */
		/* should be an error, if had way to return it */
#if 0
		ui4r hi = get_byte(addr);
		ui4r lo = get_byte(addr + 1);
		Data = ((hi << 8) & 0x0000FF00)
			| (lo & 0x000000FF);
#endif
	} else {
		ATTep p;
		ui3p m;
		ui5r AccFlags;

Label_Retry:
		p = FindATTel(addr);
		AccFlags = p->Access;

		if (0 != (AccFlags & kATTA_readreadymask)) {
			SetUpMATC(&regs.MATCex, p);
			regs.MATCex.cmpmask |= 0x01;
			m = p->usebase + (addr & p->usemask);
			Data = do_get_mem_word(m);
		} else
		/*
			no, don't run from device
			if (0 != (AccFlags & kATTA_mmdvmask)) {
				Data = MMDV_Access(p, 0, falseblnr, falseblnr, addr);
			} else
		*/
		if (0 != (AccFlags & kATTA_ntfymask)) {
			if (MemAccessNtfy(p)) {
				goto Label_Retry;
			} else {
				Data = 0; /* fail */
			}
		} else {
			Data = 0; /* fail */
		}
	}

	return Data;
}
#endif

LOCALFUNC MayInline ui4r nextiword(void)
/* NOT sign extended */
{
#if USE_POINTER
	ui4r r = do_get_mem_word(regs.pc_p);
	regs.pc_p += 2;
	return r;
#else
	ui3p m;
	ui4r Data;
	CPTR addr = regs.pc;

	m = (addr & regs.MATCex.usemask) + regs.MATCex.usebase;
	if ((addr & regs.MATCex.cmpmask) == regs.MATCex.cmpvalu) {
		Data = do_get_mem_word(m);
	} else {
		Data = get_pc_word_ext();
	}
	regs.pc = addr + 2;
	return Data;
#endif
}

LOCALFUNC MayInline ui3r nextibyte(void)
{
#if USE_POINTER
	ui3r r = do_get_mem_byte(regs.pc_p + 1);
	regs.pc_p += 2;
	return r;
#else
	return (ui3b) nextiword();
#endif
}

LOCALFUNC MayInline ui5r nextilong(void)
{
#if USE_POINTER
	ui5r r = do_get_mem_long(regs.pc_p);
	regs.pc_p += 4;
#else
	ui5r hi = nextiword();
	ui5r lo = nextiword();
	ui5r r = ((hi << 16) & 0xFFFF0000)
		| (lo & 0x0000FFFF);
#endif
	return r;
}

LOCALFUNC MayInline void BackupPC(void)
{
#if USE_POINTER
	regs.pc_p -= 2;
#else
	regs.pc -= 2;
#endif
}

LOCALFUNC MayInline void SkipiWord(void)
{
#if USE_POINTER
	regs.pc_p += 2;
#else
	regs.pc += 2;
#endif
}

#if Use68020
LOCALFUNC MayInline void SkipiLong(void)
{
#if USE_POINTER
	regs.pc_p += 4;
#else
	regs.pc += 4;
#endif
}
#endif

#ifndef WantDumpAJump
#define WantDumpAJump 0
#endif

#if WantDumpAJump
LOCALPROC DumpAJump(CPTR toaddr)
{
#if USE_POINTER
	CPTR fromaddr = regs.pc + (regs.pc_p - regs.pc_oldp);
	if ((toaddr > fromaddr) || (toaddr < regs.pc))
#else
	CPTR fromaddr = regs.pc;
#endif
	{
		dbglog_writeHex(fromaddr);
		dbglog_writeCStr(",");
		dbglog_writeHex(toaddr);
		dbglog_writeReturn();
	}
}
#endif

#if USE_POINTER
LOCALFUNC ui3p get_pc_real_address(CPTR addr)
{
	ui3p v;
	ATTep p;

Label_Retry:
	p = FindATTel(addr);
	if (0 == (p->Access & kATTA_readreadymask))
	{
		if (0 != (p->Access & kATTA_ntfymask)) {
			if (MemAccessNtfy(p)) {
				goto Label_Retry;
			}
		}
		/* in trouble if get here */
		/* ReportAbnormal("get_pc_real_address fails"); */
		v = regs.fakeword;
		regs.MATCex.cmpmask = 0;
		regs.MATCex.cmpvalu = 0xFFFFFFFF;
	} else {
		SetUpMATC(&regs.MATCex, p);
		v = (addr & p->usemask) + p->usebase;
	}

	return v;
}
#endif

LOCALFUNC MayInline void m68k_setpc(CPTR newpc)
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

#if USE_POINTER
	{
		ui3p m;

		m = (newpc & regs.MATCex.usemask) + regs.MATCex.usebase;
		if ((newpc & regs.MATCex.cmpmask) != regs.MATCex.cmpvalu)
		{
			m = get_pc_real_address(newpc);
		}

		regs.pc_p = regs.pc_oldp = m;
	}
#endif

	regs.pc = newpc;
}

LOCALFUNC MayInline CPTR m68k_getpc(void)
{
#if USE_POINTER
	return regs.pc + (regs.pc_p - regs.pc_oldp);
#else
	return regs.pc;
#endif
}

LOCALFUNC ui4b m68k_getSR(void)
{
	return (regs.t1 << 15)
#if Use68020
			| (regs.t0 << 14)
#endif
			| (regs.s << 13)
#if Use68020
			| (regs.m << 12)
#endif
			| (regs.intmask << 8)
			| m68k_getCR();
}

LOCALPROC NeedToGetOut(void)
{
	if (regs.MaxCyclesToGo <= 0) {
		/*
			already have gotten out, and exception processing has
			caused another exception, such as because a bad
			stack pointer pointing to a memory mapped device.
		*/
	} else {
		regs.MoreCyclesToGo += regs.MaxCyclesToGo;
			/* not counting the current instruction */
		regs.MaxCyclesToGo = 0;
	}
}

LOCALPROC SetExternalInterruptPending(void)
{
	regs.ExternalInterruptPending = trueblnr;
	NeedToGetOut();
}

#define m68k_dreg(num) (regs.regs[(num)])
#define m68k_areg(num) (regs.regs[(num) + 8])

LOCALPROC m68k_setSR(ui4r newsr)
{
	CPTR *pnewstk;
	CPTR *poldstk = regs.s ? (
#if Use68020
		regs.m ? &regs.msp :
#endif
		&regs.isp) : &regs.usp;
	ui5r oldintmask = regs.intmask;

	m68k_setCR(newsr);
	regs.t1 = (newsr >> 15) & 1;
#if Use68020
	regs.t0 = (newsr >> 14) & 1;
	if (regs.t0) {
		ReportAbnormal("t0 flag set in m68k_setSR");
	}
#endif
	regs.s = (newsr >> 13) & 1;
#if Use68020
	regs.m = (newsr >> 12) & 1;
	if (regs.m) {
		ReportAbnormal("m flag set in m68k_setSR");
	}
#endif
	regs.intmask = (newsr >> 8) & 7;

	pnewstk = regs.s ? (
#if Use68020
		regs.m ? &regs.msp :
#endif
		&regs.isp) : &regs.usp;

	if (poldstk != pnewstk) {
		*poldstk = m68k_areg(7);
		m68k_areg(7) = *pnewstk;
	}

	if (regs.intmask != oldintmask) {
		SetExternalInterruptPending();
	}

	if (regs.t1) {
		NeedToGetOut();
	} else {
		/* regs.TracePending = falseblnr; */
	}
}

#ifndef FastRelativeJump
#define FastRelativeJump (1 && USE_POINTER)
#endif

LOCALPROC ExceptionTo(CPTR newpc
#if Use68020
	, int nr
#endif
	)
{
	ui4b saveSR = m68k_getSR();

	if (! regs.s) {
		regs.usp = m68k_areg(7);
		m68k_areg(7) =
#if Use68020
			regs.m ? regs.msp :
#endif
			regs.isp;
		regs.s = 1;
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
	/* if regs.m should make throw away stack frame */
#endif
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
	m68k_areg(7) -= 2;
	put_word(m68k_areg(7), saveSR);
	m68k_setpc(newpc);
	regs.t1 = 0;
#if Use68020
	regs.t0 = 0;
	regs.m = 0;
#endif
	regs.TracePending = falseblnr;
}

LOCALPROC Exception(int nr)
{
	ExceptionTo(get_long(4 * nr
#if Use68020
		+ regs.vbr
#endif
		)
#if Use68020
		, nr
#endif
		);
}

LOCALFUNC MayNotInline ui5b get_disp_ea(ui5b base)
{
	ui4b dp = nextiword();
	int regno = (dp >> 12) & 0x0F;
	si5b regd = regs.regs[regno];
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
				ReportAbnormal("Extension Word: dp reserved");
				break;
			case 1:
				/* no displacement */
				/* ReportAbnormal("Extension Word: no displacement"); */
				/* used by Sys 7.5.5 boot */
				break;
			case 2:
				base += (si5b)(si4b)nextiword();
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
				ReportAbnormal("Extension Word: reserved dp form");
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
					base += (si5b)(si4b)nextiword();
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


LOCALFUNC MayNotInline ArgAddrT DecodeSrcDst(ui5r f)
{
	ui5r *p;
	ArgAddrT v;

	switch (GetDcoFldAMd(f)) {
		case kAMdReg :
			v.rga = &regs.regs[GetDcoFldArgDat(f)];
			break;
		case kAMdIndirect :
			p = &regs.regs[GetDcoFldArgDat(f)];
			v.mem = *p;
			break;
		case kAMdAPosIncB :
			p = &regs.regs[GetDcoFldArgDat(f)];
			v.mem = *p;
			*p += 1;
			break;
		case kAMdAPosIncW :
			p = &regs.regs[GetDcoFldArgDat(f)];
			v.mem = *p;
			*p += 2;
			break;
		case kAMdAPosIncL :
			p = &regs.regs[GetDcoFldArgDat(f)];
			v.mem = *p;
			*p += 4;
			break;
		case kAMdAPreDecB :
			p = &regs.regs[GetDcoFldArgDat(f)];
			*p -= 1;
			v.mem = *p;
			break;
		case kAMdAPreDecW :
			p = &regs.regs[GetDcoFldArgDat(f)];
			*p -= 2;
			v.mem = *p;
			break;
		case kAMdAPreDecL :
			p = &regs.regs[GetDcoFldArgDat(f)];
			*p -= 4;
			v.mem = *p;
			break;
		case kAMdADisp :
			p = &regs.regs[GetDcoFldArgDat(f)];
			v.mem = *p + ui5r_FromSWord(nextiword());
			break;
		case kAMdAIndex :
			p = &regs.regs[GetDcoFldArgDat(f)];
			v.mem = get_disp_ea(*p);
			break;
		case kAMdAbsW :
			v.mem = ui5r_FromSWord(nextiword());
			break;
		case kAMdAbsL :
			v.mem = nextilong();
			break;
		case kAMdPCDisp :
			v.mem = m68k_getpc();
			v.mem += ui5r_FromSWord(nextiword());
			break;
		case kAMdPCIndex :
			v.mem = get_disp_ea(m68k_getpc());
			break;
		case kAMdImmedB :
			v.mem = ui5r_FromSByte(nextibyte());
			break;
		case kAMdImmedW :
			v.mem = ui5r_FromSWord(nextiword());
			break;
		case kAMdImmedL :
			v.mem = ui5r_FromSLong(nextilong());
			break;
		case kAMdDat4 :
		default: /* make compiler happy, should not happen */
			v.mem = GetDcoFldArgDat(f);
			break;
	}

	return v;
}

LOCALFUNC MayNotInline ui5r GetSrcDstValue(ui5r f, ArgAddrT addr)
{
	ui5r v;

	switch (GetDcoFldArgk(f)) {
		case kArgkRegB:
			v = ui5r_FromSByte(*addr.rga);
			break;
		case kArgkRegW:
			v = ui5r_FromSWord(*addr.rga);
			break;
		case kArgkRegL:
			v = ui5r_FromSLong(*addr.rga);
			break;
		case kArgkMemB:
			v = get_byte(addr.mem);
			break;
		case kArgkMemW:
			v = get_word(addr.mem);
			break;
		case kArgkMemL:
			v = get_long(addr.mem);
			break;
		case kArgkCnst:
		default: /* for compiler. shouldn't be any other cases */
			v = addr.mem;
			break;
	}

	return v;
}

LOCALPROC MayNotInline SetSrcDstValue(ui5r f, ArgAddrT addr, ui5r v)
{
	switch (GetDcoFldArgk(f)) {
		case kArgkRegB:
			*addr.rga = (*addr.rga & ~ 0xff) | ((v) & 0xff);
			break;
		case kArgkRegW:
			*addr.rga = (*addr.rga & ~ 0xffff) | ((v) & 0xffff);
			break;
		case kArgkRegL:
			*addr.rga = v;
			break;
		case kArgkMemB:
			put_byte(addr.mem, v);
			break;
		case kArgkMemW:
			put_word(addr.mem, v);
			break;
		case kArgkMemL:
			put_long(addr.mem, v);
			break;
		default: /* for compiler. shouldn't be any other cases */
			break;
	}
}

LOCALFUNC MayInline ArgAddrT DecodeSrc(void)
{
	return DecodeSrcDst(regs.CurDecOp.B);
}

LOCALFUNC MayInline ui5r GetSrcValue(ArgAddrT addr)
{
	return GetSrcDstValue(regs.CurDecOp.B, addr);
}

LOCALFUNC MayInline ArgAddrT DecodeDst(void)
{
	return DecodeSrcDst(regs.CurDecOp.A);
}

LOCALFUNC MayInline ui5r GetDstValue(ArgAddrT addr)
{
	return GetSrcDstValue(regs.CurDecOp.A, addr);
}

LOCALPROC MayInline SetDstValue(ArgAddrT addr, ui5r v)
{
	SetSrcDstValue(regs.CurDecOp.A, addr, v);
}

LOCALFUNC MayNotInline ui5r DecodeSrcDstGet(void)
{
	ArgAddrT SrcAddr = DecodeSrc();
	ui5r srcvalue = GetSrcValue(SrcAddr);
	ArgAddrT DstAddr = DecodeDst();
	ui5r dstvalue = GetDstValue(DstAddr);

	regs.SrcVal = srcvalue;
	regs.ArgAddr = DstAddr;

	return dstvalue;
}

LOCALFUNC MayNotInline ui5r DecodeDstGet(void)
{
	ArgAddrT DstAddr = DecodeDst();
	ui5r dstvalue = GetDstValue(DstAddr);

	regs.ArgAddr = DstAddr;

	return dstvalue;
}

LOCALPROC MayNotInline SetDstArgValue(ui5r v)
{
	SetDstValue(regs.ArgAddr, v);
}

LOCALPROCUSEDONCE DoCodeTst(void)
{
	/* Tst 01001010ssmmmrrr */

	ArgAddrT DstAddr = DecodeDst();
	ui5r srcvalue = GetDstValue(DstAddr);

	VFLG = CFLG = 0;
	ZFLG = (srcvalue == 0);
	NFLG = ui5r_MSBisSet(srcvalue);
}

LOCALPROCUSEDONCE DoCodeCmpB(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	ALU_CmpB(regs.SrcVal, dstvalue);
}

LOCALPROCUSEDONCE DoCodeCmpW(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	ALU_CmpW(regs.SrcVal, dstvalue);
}

LOCALPROCUSEDONCE DoCodeCmpL(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	ALU_CmpL(regs.SrcVal, dstvalue);
}

LOCALPROCUSEDONCE DoCodeBraB(void)
{
	ui5b src = ((ui5b)regs.opcode) & 255;
#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif

	s += (si3b)(ui3b)src;

#if FastRelativeJump
	regs.pc_p = s;
#else
	m68k_setpc(s);
#endif
}

LOCALPROCUSEDONCE DoCodeBraW(void)
{
#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif

	s += (si4b)(ui4b)nextiword();

#if FastRelativeJump
	regs.pc_p = s;
#else
	m68k_setpc(s);
#endif
}

#if Use68020
LOCALPROCUSEDONCE DoCodeBraL(void)
{
#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif

	s += (si5b)(ui5b)nextilong();

	/* Bra 0110ccccnnnnnnnn */
#if FastRelativeJump
	regs.pc_p = s;
#else
	m68k_setpc(s);
#endif
}
#endif

LOCALPROCUSEDONCE DoCodeBccB(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	if (cctrue()) {
#if WantCloserCyc
		regs.MaxCyclesToGo -= (10 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
		DoCodeBraB();
	} else {
#if WantCloserCyc
		regs.MaxCyclesToGo -= (8 * kCycleScale + RdAvgXtraCyc);
#endif
		/* do nothing */
	}
}

LOCALPROCUSEDONCE DoCodeBccW(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	if (cctrue()) {
#if WantCloserCyc
		regs.MaxCyclesToGo -= (10 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
		DoCodeBraW();
	} else {
#if WantCloserCyc
		regs.MaxCyclesToGo -= (12 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
		SkipiWord();
	}
}

#if Use68020
LOCALPROCUSEDONCE DoCodeBccL(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	if (cctrue()) {
		DoCodeBraL();
	} else {
		SkipiLong();
	}
}
#endif

#define reg (regs.opcode & 7)
#define rg9 ((regs.opcode >> 9) & 7)

LOCALPROCUSEDONCE DoCodeDBcc(void)
{
	/* DBcc 0101cccc11001ddd */

	ui5r dstvalue;
#if FastRelativeJump
	ui3p srcvalue = regs.pc_p;
#else
	ui5r srcvalue = m68k_getpc();
#endif

	srcvalue += (si4b)(ui4b)nextiword();
	if (cctrue()) {
#if WantCloserCyc
		regs.MaxCyclesToGo -= (12 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
	} else {
		dstvalue = ui5r_FromSWord(m68k_dreg(reg));
		--dstvalue;
		m68k_dreg(reg) = (m68k_dreg(reg) & ~ 0xffff)
			| ((dstvalue) & 0xffff);
		if ((si5b)dstvalue == -1) {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (14 * kCycleScale + 3 * RdAvgXtraCyc);
#endif
		} else {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (10 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
#if FastRelativeJump
			regs.pc_p = srcvalue;
#else
			m68k_setpc(srcvalue);
#endif
		}
	}
}

LOCALPROCUSEDONCE DoCodeSwap(void)
{
	/* Swap 0100100001000rrr */
	ui5r srcreg = reg;
	ui5r src = m68k_dreg(srcreg);
	ui5r dst = ui5r_FromSLong(((src >> 16) & 0xFFFF)
		| ((src & 0xFFFF) << 16));
	VFLG = CFLG = 0;
	ZFLG = (dst == 0);
	NFLG = ui5r_MSBisSet(dst);
	m68k_dreg(srcreg) = dst;
}

LOCALPROCUSEDONCE DoCodeMove(void)
{
	ArgAddrT SrcAddr = DecodeSrc();
	ui5r src = GetSrcValue(SrcAddr);
	ArgAddrT DstAddr = DecodeDst();

	VFLG = CFLG = 0;
	ZFLG = (src == 0);
	NFLG = ui5r_MSBisSet(src);
	SetDstValue(DstAddr, src);
}

LOCALPROC DoCodeMoveA(void) /* MOVE */
{
	ArgAddrT SrcAddr;
	ui5r src;

	SrcAddr = DecodeSrc();
	src = GetSrcValue(SrcAddr);
	m68k_areg(rg9) = src;
}

LOCALPROCUSEDONCE DoCodeMoveQ(void)
{
	/* MoveQ 0111ddd0nnnnnnnn */
	ui5r src = ui5r_FromSByte(regs.opcode);
	ui5r dstreg = rg9;
	VFLG = CFLG = 0;
	ZFLG = (src == 0);
	NFLG = ui5r_MSBisSet(src);
	m68k_dreg(dstreg) = src;
}

LOCALPROCUSEDONCE DoCodeAddB(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_AddB(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeAddW(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_AddW(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeAddL(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_AddL(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeSubB(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_SubB(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeSubW(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_SubW(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeSubL(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_SubL(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeLea(void)
{
	ArgAddrT DstAddr;
	/* Lea 0100aaa111mmmrrr */
	DstAddr = DecodeDst();
	m68k_areg(rg9) = DstAddr.mem;
}

LOCALPROCUSEDONCE DoCodePEA(void)
{
	ArgAddrT DstAddr;
	/* PEA 0100100001mmmrrr */
	DstAddr = DecodeDst();
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), DstAddr.mem);
}

LOCALPROCUSEDONCE DoCodeA(void)
{
	BackupPC();
	Exception(0xA);
}

LOCALPROCUSEDONCE DoCodeBsrB(void)
{
	ui5b src = ((ui5b)regs.opcode) & 255;
#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif

	s += (si3b)(ui3b)src;

	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
#if FastRelativeJump
	regs.pc_p = s;
#else
	m68k_setpc(s);
#endif
}

LOCALPROCUSEDONCE DoCodeBsrW(void)
{
#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif

	s += (si4b)(ui4b)nextiword();

	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
#if FastRelativeJump
	regs.pc_p = s;
#else
	m68k_setpc(s);
#endif
}

#if Use68020
LOCALPROCUSEDONCE DoCodeBsrL(void)
{
#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif

	s += (si5b)(ui5b)nextilong();
	/* ReportAbnormal("long branch in DoCode6"); */
	/* Used by various Apps */

	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
#if FastRelativeJump
	regs.pc_p = s;
#else
	m68k_setpc(s);
#endif
}
#endif

LOCALPROCUSEDONCE DoCodeJsr(void)
{
	/* Jsr 0100111010mmmrrr */
	ArgAddrT DstAddr = DecodeDst();
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), m68k_getpc());
	m68k_setpc(DstAddr.mem);
}

LOCALPROCUSEDONCE DoCodeLinkA6(void)
{
	CPTR stackp = m68k_areg(7);
	stackp -= 4;
	put_long(stackp, m68k_areg(6));
	m68k_areg(6) = stackp;
	m68k_areg(7) = stackp + ui5r_FromSWord(nextiword());
}

LOCALPROCUSEDONCE DoCodeMOVEMRmML(void)
{
	/* MOVEM reg to mem 01001000111100rrr */
	si4b z;
	ui5r regmask = nextiword();
	ui5r p = m68k_areg(reg);

#if Use68020
	{
		int n = 0;

		for (z = 0; z < 16; ++z) {
			if ((regmask & (1 << z)) != 0) {
				n++;
			}
		}
		m68k_areg(reg) = p - n * 4;
	}
#endif
	for (z = 16; --z >= 0; ) {
		if ((regmask & (1 << (15 - z))) != 0) {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (8 * kCycleScale + 2 * WrAvgXtraCyc);
#endif
			p -= 4;
			put_long(p, regs.regs[z]);
		}
	}
#if ! Use68020
	m68k_areg(reg) = p;
#endif
}

LOCALPROCUSEDONCE DoCodeMOVEMApRL(void)
{
	/* MOVEM mem to reg 01001100111011rrr */
	si4b z;
	ui5r regmask = nextiword();
	ui5r p = m68k_areg(reg);

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (8 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
			regs.regs[z] = get_long(p);
			p += 4;
		}
	}
	m68k_areg(reg) = p;
}

LOCALPROCUSEDONCE DoCodeUnlkA6(void)
{
	ui5r src = m68k_areg(6);
	m68k_areg(6) = get_long(src);
	m68k_areg(7) = src + 4;
}

LOCALPROCUSEDONCE DoCodeRts(void)
{
	/* Rts 0100111001110101 */
	ui5r NewPC = get_long(m68k_areg(7));
	m68k_areg(7) += 4;
	m68k_setpc(NewPC);
}

LOCALPROCUSEDONCE DoCodeJmp(void)
{
	/* JMP 0100111011mmmrrr */
	ArgAddrT DstAddr = DecodeDst();
	m68k_setpc(DstAddr.mem);
}

LOCALPROCUSEDONCE DoCodeClr(void)
{
	/* Clr 01000010ssmmmrrr */

	ArgAddrT DstAddr = DecodeDst();
	VFLG = CFLG = 0;
	ZFLG = 1;
	NFLG = 0;
	SetDstValue(DstAddr, 0);
}

LOCALPROCUSEDONCE DoCodeAddA(void)
{
	/* ADDA 1101dddm11mmmrrr */
	ui5r dstvalue = DecodeSrcDstGet();

	dstvalue += regs.SrcVal;
	SetDstArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeSubA(void)
{
	ui5r dstvalue = DecodeSrcDstGet();

	dstvalue -= regs.SrcVal;
	SetDstArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeCmpA(void)
{
	ui5r dstvalue = DecodeSrcDstGet();

	{
		int flgs = ui5r_MSBisSet(regs.SrcVal);
		int flgo = ui5r_MSBisSet(dstvalue);
		dstvalue -= regs.SrcVal;
		dstvalue = ui5r_FromSLong(dstvalue);
		ZFLG = (dstvalue == 0);
		NFLG = ui5r_MSBisSet(dstvalue);
		VFLG = (flgs != flgo) && (NFLG != flgo);
		CFLG = (flgs && ! flgo) || (NFLG && ((! flgo) || flgs));
	}
}

LOCALPROCUSEDONCE DoCodeAddXB(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_AddXB(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeAddXW(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_AddXW(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeAddXL(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_AddXL(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeSubXB(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_SubXB(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeSubXW(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_SubXW(regs.SrcVal, dstvalue));
}

LOCALPROCUSEDONCE DoCodeSubXL(void)
{
	ui5r dstvalue = DecodeSrcDstGet();
	SetDstArgValue(ALU_SubXL(regs.SrcVal, dstvalue));
}

LOCALPROC SetArgKindReg(ui5b thereg)
{
	regs.ArgKind = AKRegister;
	regs.ArgAddr.rga = &regs.regs[thereg];
}

LOCALPROC MayNotInline DecodeModeRegister(ui5b themode, ui5b thereg)
{
	switch (themode) {
		case 0 :
			SetArgKindReg(thereg);
			break;
		case 1 :
			SetArgKindReg(thereg + 8);
			break;
		case 2 :
			regs.ArgKind = AKMemory;
			regs.ArgAddr.mem = m68k_areg(thereg);
			break;
		case 3 :
			regs.ArgKind = AKMemory;
			regs.ArgAddr.mem = m68k_areg(thereg);
			if ((thereg == 7) && (regs.opsize == 1)) {
				m68k_areg(thereg) += 2;
			} else {
				m68k_areg(thereg) += regs.opsize;
			}
			break;
		case 4 :
			regs.ArgKind = AKMemory;
			if ((thereg == 7) && (regs.opsize == 1)) {
				m68k_areg(thereg) -= 2;
			} else {
				m68k_areg(thereg) -= regs.opsize;
			}
			regs.ArgAddr.mem = m68k_areg(thereg);
			break;
		case 5 :
			regs.ArgKind = AKMemory;
			regs.ArgAddr.mem = m68k_areg(thereg)
				+ ui5r_FromSWord(nextiword());
			break;
		case 6 :
			regs.ArgKind = AKMemory;
			regs.ArgAddr.mem = get_disp_ea(m68k_areg(thereg));
			break;
		case 7 :
			switch (thereg) {
				case 0 :
					regs.ArgKind = AKMemory;
					regs.ArgAddr.mem = ui5r_FromSWord(nextiword());
					break;
				case 1 :
					regs.ArgKind = AKMemory;
					regs.ArgAddr.mem = nextilong();
					break;
				case 2 :
					regs.ArgKind = AKMemory;
					regs.ArgAddr.mem = m68k_getpc();
					regs.ArgAddr.mem += ui5r_FromSWord(nextiword());
					break;
				case 3 :
					regs.ArgKind = AKMemory;
					regs.ArgAddr.mem = get_disp_ea(m68k_getpc());
					break;
				case 4 :
					regs.ArgKind = AKConstant;
					if (regs.opsize == 2) {
						regs.ArgAddr.mem = ui5r_FromSWord(nextiword());
					} else if (regs.opsize < 2) {
						regs.ArgAddr.mem = ui5r_FromSByte(nextibyte());
					} else {
						regs.ArgAddr.mem = ui5r_FromSLong(nextilong());
					}
					break;
			}
			break;
		case 8 :
			regs.ArgKind = AKConstant;
			regs.ArgAddr.mem = thereg;
			break;
	}
}

LOCALFUNC ui5r GetArgValue(void)
{
	ui5r v;

	switch (regs.ArgKind) {
		case AKMemory:
			if (regs.opsize == 2) {
				v = get_word(regs.ArgAddr.mem);
			} else if (regs.opsize < 2) {
				v = get_byte(regs.ArgAddr.mem);
			} else {
				v = get_long(regs.ArgAddr.mem);
			}
			break;
		case AKRegister:
			v = *regs.ArgAddr.rga;
			if (regs.opsize == 2) {
				v = ui5r_FromSWord(v);
			} else if (regs.opsize < 2) {
				v = ui5r_FromSByte(v);
			} else {
				v = ui5r_FromSLong(v);
			}
			break;
		case AKConstant:
		default: /* for compiler. shouldn't be any other cases */
			v = regs.ArgAddr.mem;
			break;
	}
	return v;
}

LOCALPROC SetArgValue(ui5r v)
{
	if (regs.ArgKind == AKRegister) {
		if (regs.opsize == 2) {
			*regs.ArgAddr.rga =
				(*regs.ArgAddr.rga & ~ 0xffff) | ((v) & 0xffff);
		} else if (regs.opsize < 2) {
			*regs.ArgAddr.rga =
				(*regs.ArgAddr.rga & ~ 0xff) | ((v) & 0xff);
		} else {
			*regs.ArgAddr.rga = v;
		}
	} else {
		/* must be AKMemory */
		/* should not get here for AKConstant */
		if (regs.opsize == 2) {
			put_word(regs.ArgAddr.mem, v);
		} else if (regs.opsize < 2) {
			put_byte(regs.ArgAddr.mem, v);
		} else {
			put_long(regs.ArgAddr.mem, v);
		}
	}
}

#define b76 ((regs.opcode >> 6) & 3)
#define b8 ((regs.opcode >> 8) & 1)
#define mode ((regs.opcode >> 3) & 7)
#define md6 ((regs.opcode >> 6) & 7)

LOCALPROC FindOpSizeFromb76(void)
{
	regs.opsize = 1 << b76;
#if 0
	switch (b76) {
		case 0 :
			regs.opsize = 1;
			break;
		case 1 :
			regs.opsize = 2;
			break;
		case 2 :
			regs.opsize = 4;
			break;
	}
#endif
}

LOCALFUNC ui5r octdat(ui5r x)
{
	if (x == 0) {
		return 8;
	} else {
		return x;
	}
}

#define extendopsizedstvalue() \
	if (regs.opsize == 2) {\
		dstvalue = ui5r_FromSWord(dstvalue);\
	} else if (regs.opsize < 2) {\
		dstvalue = ui5r_FromSByte(dstvalue);\
	} else {\
		dstvalue = ui5r_FromSLong(dstvalue);\
	}

#define unextendopsizedstvalue() \
	if (regs.opsize == 2) {\
		dstvalue = ui5r_FromUWord(dstvalue);\
	} else if (regs.opsize < 2) {\
		dstvalue = ui5r_FromUByte(dstvalue);\
	} else {\
		dstvalue = ui5r_FromULong(dstvalue);\
	}

#define BinOpASL 0
#define BinOpASR 1
#define BinOpLSL 2
#define BinOpLSR 3
#define BinOpRXL 4
#define BinOpRXR 5
#define BinOpROL 6
#define BinOpROR 7

LOCALPROC DoBinOp1(ui5r srcvalue, ui5r binop)
{
	ui5r dstvalue;
	ui5r cnt = srcvalue & 63;

	dstvalue = GetArgValue();
	switch (binop) {
		case BinOpASL:
			{
				ui5r dstvalue0 = dstvalue;
				ui5r comparevalue;
				if (! cnt) {
					VFLG = 0;
					CFLG = 0;
				} else {
					if (cnt > 32) {
						dstvalue = 0;
					} else {
						dstvalue = dstvalue << (cnt - 1);
					}
					extendopsizedstvalue();
					CFLG = XFLG = ui5r_MSBisSet(dstvalue);
					dstvalue = dstvalue << 1;
					extendopsizedstvalue();
				}
				if (ui5r_MSBisSet(dstvalue)) {
					comparevalue = - ((- dstvalue) >> cnt);
				} else {
					comparevalue = dstvalue >> cnt;
				}
				VFLG = (comparevalue != dstvalue0);
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
			}
			break;
		case BinOpASR:
			{
				NFLG = ui5r_MSBisSet(dstvalue);
				VFLG = 0;
				if (! cnt) {
					CFLG = 0;
				} else {
					if (NFLG) {
						dstvalue = (~ dstvalue);
					}
					unextendopsizedstvalue();
					if (cnt > 32) {
						dstvalue = 0;
					} else {
						dstvalue = dstvalue >> (cnt - 1);
					}
					CFLG = (dstvalue & 1) != 0;
					dstvalue = dstvalue >> 1;
					if (NFLG) {
						CFLG = ! CFLG;
						dstvalue = (~ dstvalue);
					}
					XFLG = CFLG;
				}
				ZFLG = (dstvalue == 0);
			}
			break;
		case BinOpLSL:
			{
				if (! cnt) {
					CFLG = 0;
				} else {
					if (cnt > 32) {
						dstvalue = 0;
					} else {
						dstvalue = dstvalue << (cnt - 1);
					}
					extendopsizedstvalue();
					CFLG = XFLG = ui5r_MSBisSet(dstvalue);
					dstvalue = dstvalue << 1;
					extendopsizedstvalue();
				}
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
				VFLG = 0;
			}
			break;
		case BinOpLSR:
			{
				if (! cnt) {
					CFLG = 0;
				} else {
					unextendopsizedstvalue();
					if (cnt > 32) {
						dstvalue = 0;
					} else {
						dstvalue = dstvalue >> (cnt - 1);
					}
					CFLG = XFLG = (dstvalue & 1) != 0;
					dstvalue = dstvalue >> 1;
				}
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
					/* if cnt != 0, always false */
				VFLG = 0;
			}
			break;
		case BinOpROL:
			{
				if (! cnt) {
					CFLG = 0;
				} else {
					for (; cnt; --cnt) {
						CFLG = ui5r_MSBisSet(dstvalue);
						dstvalue = dstvalue << 1;
						if (CFLG) {
							dstvalue = dstvalue | 1;
						}
						extendopsizedstvalue();
					}
				}
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
				VFLG = 0;
			}
			break;
		case BinOpRXL:
			{
				if (! cnt) {
					CFLG = XFLG;
				} else {
					for (; cnt; --cnt) {
						CFLG = ui5r_MSBisSet(dstvalue);
						dstvalue = dstvalue << 1;
						if (XFLG) {
							dstvalue = dstvalue | 1;
						}
						extendopsizedstvalue();
						XFLG = CFLG;
					}
				}
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
				VFLG = 0;
			}
			break;
		case BinOpROR:
			{
				ui5r cmask = (ui5r)1 << (regs.opsize * 8 - 1);
				if (! cnt) {
					CFLG = 0;
				} else {
					unextendopsizedstvalue();
					for (; cnt; --cnt) {
						CFLG = (dstvalue & 1) != 0;
						dstvalue = dstvalue >> 1;
						if (CFLG) {
							dstvalue = dstvalue | cmask;
						}
					}
					extendopsizedstvalue();
				}
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
				VFLG = 0;
			}
			break;
		case BinOpRXR:
			{
				ui5r cmask = (ui5r)1 << (regs.opsize * 8 - 1);
				if (! cnt) {
					CFLG = XFLG;
				} else {
					unextendopsizedstvalue();
					for (; cnt; --cnt) {
						CFLG = (dstvalue & 1) != 0;
						dstvalue = dstvalue >> 1;
						if (XFLG) {
							dstvalue = dstvalue | cmask;
						}
						XFLG = CFLG;
					}
					extendopsizedstvalue();
				}
				ZFLG = (dstvalue == 0);
				NFLG = ui5r_MSBisSet(dstvalue);
				VFLG = 0;
			}
			break;
		default:
			/* should not get here */
			break;
	}
	SetArgValue(dstvalue);
}

LOCALFUNC ui5r rolops(ui5r x)
{
	ui5r binop;

	binop = (x << 1);
	if (! b8) {
		binop++; /* 'R' */
	} /* else 'L' */
	return binop;
}

LOCALPROCUSEDONCE DoCodeRolopNM(void)
{
	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	DoBinOp1(1, rolops(rg9));
}

LOCALPROCUSEDONCE DoCodeRolopND(void)
{
	/* 1110cccdss0ttddd */
	FindOpSizeFromb76();
	SetArgKindReg(reg);
	DoBinOp1(octdat(rg9), rolops(mode & 3));
}

LOCALPROCUSEDONCE DoCodeRolopDD(void)
{
	/* 1110rrrdss1ttddd */
	ui5r srcvalue;
	FindOpSizeFromb76();
	SetArgKindReg(rg9);
	srcvalue = GetArgValue();
	SetArgKindReg(reg);
#if WantCloserCyc
	regs.MaxCyclesToGo -= ((srcvalue & 63) * 2 * kCycleScale);
#endif
	DoBinOp1(srcvalue, rolops(mode & 3));
}

#define BinOpBTst 0
#define BinOpBChg 1
#define BinOpBClr 2
#define BinOpBSet 3

LOCALPROC DoBinBitOp1(ui5r srcvalue)
{
	ui5r dstvalue;
	ui5r binop;

	dstvalue = GetArgValue();

	ZFLG = ((dstvalue & ((ui5r)1 << srcvalue)) == 0);
	binop = b76;
	if (binop != BinOpBTst) {
		switch (binop) {
			case BinOpBChg:
				dstvalue ^= (1 << srcvalue);
				break;
			case BinOpBClr:
				dstvalue &= ~ (1 << srcvalue);
				break;
			case BinOpBSet:
				dstvalue |= (1 << srcvalue);
				break;
			default:
				/* should not get here */
				break;
		}
		SetArgValue(dstvalue);
	}
}

LOCALPROCUSEDONCE DoCodeBitOpDD(void)
{
	/* dynamic bit, Opcode = 0000ddd1tt000rrr */
	ui5r srcvalue = (ui5r_FromSByte(m68k_dreg(rg9))) & 31;
	regs.opsize = 4;
	SetArgKindReg(reg);
	DoBinBitOp1(srcvalue);
}

LOCALPROCUSEDONCE DoCodeBitOpDM(void)
{
	/* dynamic bit, Opcode = 0000ddd1ttmmmrrr */
	ui5r srcvalue = (ui5r_FromSByte(m68k_dreg(rg9))) & 7;
	regs.opsize = 1;
	DecodeModeRegister(mode, reg);
	DoBinBitOp1(srcvalue);
}

LOCALPROCUSEDONCE DoCodeBitOpND(void)
{
	/* static bit 00001010tt000rrr */
	ui5r srcvalue = (ui5r_FromSByte(nextibyte())) & 31;
	regs.opsize = 4;
	SetArgKindReg(reg);
	DoBinBitOp1(srcvalue);
}

LOCALPROCUSEDONCE DoCodeBitOpNM(void)
{
	/* static bit 00001010ttmmmrrr */
	ui5r srcvalue = (ui5r_FromSByte(nextibyte())) & 7;
	regs.opsize = 1;
	DecodeModeRegister(mode, reg);
	DoBinBitOp1(srcvalue);
}

LOCALPROCUSEDONCE DoCodeAnd(void)
{
	/* DoBinOpAnd(DecodeI_xxxxxxxxssmmmrrr()); */
	ui5r dstvalue = DecodeSrcDstGet();

	dstvalue &= regs.SrcVal;
		/*
			don't need to extend, since excess high
			bits all the same as desired high bit.
		*/
	VFLG = CFLG = 0;
	ZFLG = (dstvalue == 0);
	NFLG = ui5r_MSBisSet(dstvalue);

	SetDstArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeOr(void)
{
	/* DoBinOr(DecodeI_xxxxxxxxssmmmrrr()); */
	ui5r dstvalue = DecodeSrcDstGet();

	dstvalue |= regs.SrcVal;
		/*
			don't need to extend, since excess high
			bits all the same as desired high bit.
		*/
	VFLG = CFLG = 0;
	ZFLG = (dstvalue == 0);
	NFLG = ui5r_MSBisSet(dstvalue);

	SetDstArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeEor(void)
{
	/* Eor 1011ddd1ssmmmrrr */
	/* DoBinOpEor(DecodeDEa_xxxxdddxssmmmrrr()); */
	ui5r dstvalue = DecodeSrcDstGet();

	dstvalue ^= regs.SrcVal;
		/*
			don't need to extend, since excess high
			bits all the same as desired high bit.
		*/
	VFLG = CFLG = 0;
	ZFLG = (dstvalue == 0);
	NFLG = ui5r_MSBisSet(dstvalue);

	SetDstArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeNot(void)
{
	/* Not 01000110ssmmmrrr */
	ui5r dstvalue = DecodeDstGet();

	dstvalue = ~ dstvalue;
	ZFLG = (dstvalue == 0);
	NFLG = ui5r_MSBisSet(dstvalue);
	VFLG = CFLG = 0;

	SetDstArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeScc(void)
{
	/* Scc 0101cccc11mmmrrr */
	regs.opsize = 1;
	DecodeModeRegister(mode, reg);
	if (cctrue()) {
#if WantCloserCyc
		if (mode == 0) {
			regs.MaxCyclesToGo -= (2 * kCycleScale);
		}
#endif
		SetArgValue(0xff);
	} else {
		SetArgValue(0);
	}
}

LOCALPROCUSEDONCE DoCodeEXTL(void)
{
	/* EXT.L */
	ui5r srcreg = reg;
	ui5r src = m68k_dreg(srcreg);
	ui5r dst = ui5r_FromSWord(src);
	VFLG = CFLG = 0;
	ZFLG = (dst == 0);
	NFLG = ui5r_MSBisSet(dst);
	m68k_dreg(srcreg) = dst;
}

LOCALPROCUSEDONCE DoCodeEXTW(void)
{
	/* EXT.W */
	ui5r srcreg = reg;
	ui5r src = m68k_dreg(srcreg);
	ui5r dst = ui5r_FromSByte(src);
	VFLG = CFLG = 0;
	ZFLG = (dst == 0);
	NFLG = ui5r_MSBisSet(dst);
	m68k_dreg(srcreg) = (m68k_dreg(srcreg) & ~ 0xffff) | (dst & 0xffff);
}

LOCALPROCUSEDONCE DoCodeNegB(void)
{
	ui5r dstvalue = DecodeDstGet();
	SetDstArgValue(ALU_NegB(dstvalue));
}

LOCALPROCUSEDONCE DoCodeNegW(void)
{
	ui5r dstvalue = DecodeDstGet();
	SetDstArgValue(ALU_NegW(dstvalue));
}

LOCALPROCUSEDONCE DoCodeNegL(void)
{
	ui5r dstvalue = DecodeDstGet();
	SetDstArgValue(ALU_NegL(dstvalue));
}

LOCALPROCUSEDONCE DoCodeNegXB(void)
{
	ui5r dstvalue = DecodeDstGet();
	SetDstArgValue(ALU_NegXB(dstvalue));
}

LOCALPROCUSEDONCE DoCodeNegXW(void)
{
	ui5r dstvalue = DecodeDstGet();
	SetDstArgValue(ALU_NegXW(dstvalue));
}

LOCALPROCUSEDONCE DoCodeNegXL(void)
{
	ui5r dstvalue = DecodeDstGet();
	SetDstArgValue(ALU_NegXL(dstvalue));
}

LOCALPROCUSEDONCE DoCodeMulU(void)
{
	/* MulU 1100ddd011mmmrrr */
	ui5r srcvalue;
	ui5r dstvalue;

	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	srcvalue = GetArgValue();
	dstvalue = ui5r_FromSLong(ui5r_FromUWord(regs.regs[rg9])
		* ui5r_FromUWord(srcvalue));
#if WantCloserCyc
	{
		ui5r v = srcvalue;

		while (v != 0) {
			if ((v & 1) != 0) {
				regs.MaxCyclesToGo -= (2 * kCycleScale);
			}
			v >>= 1;
		}
	}
#endif
	VFLG = CFLG = 0;
	ZFLG = (dstvalue == 0);
	NFLG = ui5r_MSBisSet(dstvalue);
	regs.regs[rg9] = dstvalue;
}

LOCALPROCUSEDONCE DoCodeMulS(void)
{
	/* MulS 1100ddd111mmmrrr */
	ui5r srcvalue;
	ui5r dstvalue;

	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	srcvalue = GetArgValue();
	dstvalue = ui5r_FromSLong((si5b)(si4b)regs.regs[rg9]
		* (si5b)(si4b)srcvalue);
#if WantCloserCyc
	{
		ui5r v = (srcvalue << 1);

		while (v != 0) {
			if ((v & 1) != ((v >> 1) & 1)) {
				regs.MaxCyclesToGo -= (2 * kCycleScale);
			}
			v >>= 1;
		}
	}
#endif
	VFLG = CFLG = 0;
	ZFLG = (dstvalue == 0);
	NFLG = ui5r_MSBisSet(dstvalue);
	regs.regs[rg9] = dstvalue;
}

LOCALPROCUSEDONCE DoCodeDivU(void)
{
	/* DivU 1000ddd011mmmrrr */
	ui5r srcvalue;
	ui5r dstvalue;

	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	srcvalue = GetArgValue();
	dstvalue = regs.regs[rg9];
	if (srcvalue == 0) {
#if WantCloserCyc
		regs.MaxCyclesToGo -=
			(38 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(5);
#if m68k_logExceptions
		dbglog_StartLine();
		dbglog_writeCStr("*** zero devide exception");
		dbglog_writeReturn();
#endif
	} else {
		ui5b newv = (ui5b)dstvalue / (ui5b)(ui4b)srcvalue;
		ui5b rem = (ui5b)dstvalue % (ui5b)(ui4b)srcvalue;
#if WantCloserCyc
		regs.MaxCyclesToGo -= (133 * kCycleScale);
#endif
		if (newv > 0xffff) {
			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			VFLG = CFLG = 0;
			ZFLG = ((si4b)(newv)) == 0;
			NFLG = ((si4b)(newv)) < 0;
			newv = (newv & 0xffff) | ((ui5b)rem << 16);
			dstvalue = newv;
		}
	}
	regs.regs[rg9] = dstvalue;
}

LOCALPROCUSEDONCE DoCodeDivS(void)
{
	/* DivS 1000ddd111mmmrrr */
	ui5r srcvalue;
	ui5r dstvalue;

	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	srcvalue = GetArgValue();
	dstvalue = regs.regs[rg9];
	if (srcvalue == 0) {
#if WantCloserCyc
		regs.MaxCyclesToGo -=
			(38 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(5);
#if m68k_logExceptions
		dbglog_StartLine();
		dbglog_writeCStr("*** zero devide exception");
		dbglog_writeReturn();
#endif
	} else {
		si5b newv = (si5b)dstvalue / (si5b)(si4b)srcvalue;
		ui4b rem = (si5b)dstvalue % (si5b)(si4b)srcvalue;
#if WantCloserCyc
		regs.MaxCyclesToGo -= (150 * kCycleScale);
#endif
		if (((newv & 0xffff8000) != 0) &&
			((newv & 0xffff8000) != 0xffff8000))
		{
			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			if (((si4b)rem < 0) != ((si5b)dstvalue < 0)) {
				rem = - rem;
			}
			VFLG = CFLG = 0;
			ZFLG = ((si4b)(newv)) == 0;
			NFLG = ((si4b)(newv)) < 0;
			newv = (newv & 0xffff) | ((ui5b)rem << 16);
			dstvalue = newv;
		}
	}
	regs.regs[rg9] = dstvalue;
}

LOCALPROCUSEDONCE DoCodeExgdd(void)
{
	/* Exg 1100ddd101000rrr, opsize = 4 */
	ui5r srcreg = rg9;
	ui5r dstreg = reg;
	ui5r src = m68k_dreg(srcreg);
	ui5r dst = m68k_dreg(dstreg);
	m68k_dreg(srcreg) = dst;
	m68k_dreg(dstreg) = src;
}

LOCALPROCUSEDONCE DoCodeExgaa(void)
{
	/* Exg 1100ddd101001rrr, opsize = 4 */
	ui5r srcreg = rg9;
	ui5r dstreg = reg;
	ui5r src = m68k_areg(srcreg);
	ui5r dst = m68k_areg(dstreg);
	m68k_areg(srcreg) = dst;
	m68k_areg(dstreg) = src;
}

LOCALPROCUSEDONCE DoCodeExgda(void)
{
	/* Exg 1100ddd110001rrr, opsize = 4 */
	ui5r srcreg = rg9;
	ui5r dstreg = reg;
	ui5r src = m68k_dreg(srcreg);
	ui5r dst = m68k_areg(dstreg);
	m68k_dreg(srcreg) = dst;
	m68k_areg(dstreg) = src;
}

LOCALPROCUSEDONCE DoCodeMoveCCREa(void)
{
	/* Move from CCR 0100001011mmmrrr */
#if ! Use68020
	ReportAbnormal("Move from CCR");
#endif
	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	SetArgValue(m68k_getSR() & 0xFF);
}

LOCALPROCUSEDONCE DoCodeMoveEaCR(void)
{
	/* 0100010011mmmrrr */
	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	m68k_setCR(GetArgValue());
}

LOCALPROCUSEDONCE DoCodeMoveSREa(void)
{
	/* Move from SR 0100000011mmmrrr */
	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	SetArgValue(m68k_getSR());
}

LOCALPROCUSEDONCE DoCodeMoveEaSR(void)
{
	/* 0100011011mmmrrr */
	regs.opsize = 2;
	DecodeModeRegister(mode, reg);
	m68k_setSR(GetArgValue());
}

LOCALPROC DoPrivilegeViolation(void)
{
#if WantCloserCyc
	regs.MaxCyclesToGo += GetDcoCycles(&regs.CurDecOp);
	regs.MaxCyclesToGo -=
		(34 * kCycleScale + 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
	BackupPC();
	Exception(8);
}

LOCALPROC DoBinOpStatusCCR(void)
{
	blnr IsStatus = (b76 != 0);
	ui5r srcvalue;
	ui5r dstvalue;

	FindOpSizeFromb76();
	if (IsStatus && (! regs.s)) {
		DoPrivilegeViolation();
	} else {
		srcvalue = ui5r_FromSWord(nextiword());
		dstvalue = m68k_getSR();
		switch (rg9) {
			case 0 :
				dstvalue |= srcvalue;
				break;
			case 1 :
				dstvalue &= srcvalue;
				break;
			case 5 :
				dstvalue ^= srcvalue;
				break;
			default: /* should not happen */
				break;
		}
		if (IsStatus) {
			m68k_setSR(dstvalue);
		} else {
			m68k_setCR(dstvalue);
		}
	}
}

LOCALPROCUSEDONCE DoCodeMOVEMApRW(void)
{
	/* MOVEM mem to reg 01001100110011rrr */
	si4b z;
	ui5r regmask = nextiword();
	ui5r p = m68k_areg(reg);

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (4 * kCycleScale + RdAvgXtraCyc);
#endif
			regs.regs[z] = get_word(p);
			p += 2;
		}
	}
	m68k_areg(reg) = p;
}

LOCALPROCUSEDONCE DoCodeMOVEMRmMW(void)
{
	/* MOVEM reg to mem 01001000110100rrr */
	si4b z;
	ui5r regmask = nextiword();
	ui5r p = m68k_areg(reg);

#if Use68020
	{
		int n = 0;

		for (z = 0; z < 16; ++z) {
			if ((regmask & (1 << z)) != 0) {
				n++;
			}
		}
		m68k_areg(reg) = p - n * 2;
	}
#endif
	for (z = 16; --z >= 0; ) {
		if ((regmask & (1 << (15 - z))) != 0) {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (4 * kCycleScale + WrAvgXtraCyc);
#endif
			p -= 2;
			put_word(p, regs.regs[z]);
		}
	}
#if ! Use68020
	m68k_areg(reg) = p;
#endif
}

LOCALPROC reglist(si4b direction, ui5b m1, ui5b r1)
{
	si4b z;
	ui5r p;
	ui5r regmask;

	regmask = nextiword();
	regs.opsize = 2 * b76 - 2;
	DecodeModeRegister(m1, r1);
	p = regs.ArgAddr.mem;
	if (direction == 0) {
		if (regs.opsize == 2) {
			for (z = 0; z < 16; ++z) {
				if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
					regs.MaxCyclesToGo -=
						(4 * kCycleScale + WrAvgXtraCyc);
#endif
					put_word(p, regs.regs[z]);
					p += 2;
				}
			}
		} else {
			for (z = 0; z < 16; ++z) {
				if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
					regs.MaxCyclesToGo -=
						(8 * kCycleScale + 2 * WrAvgXtraCyc);
#endif
					put_long(p, regs.regs[z]);
					p += 4;
				}
			}
		}
	} else {
		if (regs.opsize == 2) {
			for (z = 0; z < 16; ++z) {
				if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
					regs.MaxCyclesToGo -=
						(4 * kCycleScale + RdAvgXtraCyc);
#endif
					regs.regs[z] = get_word(p);
					p += 2;
				}
			}
		} else {
			for (z = 0; z < 16; ++z) {
				if ((regmask & (1 << z)) != 0) {
#if WantCloserCyc
					regs.MaxCyclesToGo -=
						(8 * kCycleScale + 2 * RdAvgXtraCyc);
#endif
					regs.regs[z] = get_long(p);
					p += 4;
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DoCodeMOVEMrm(void)
{
	/* MOVEM reg to mem 010010001ssmmmrrr */
	reglist(0, mode, reg);
}

LOCALPROCUSEDONCE DoCodeMOVEMmr(void)
{
	/* MOVEM mem to reg 0100110011smmmrrr */
	reglist(1, mode, reg);
}

LOCALPROC DoBinOpAbcd(ui5b m1, ui5b r1, ui5b m2, ui5b r2)
{
	ui5r srcvalue;
	ui5r dstvalue;

	regs.opsize = 1;
	DecodeModeRegister(m1, r1);
	srcvalue = GetArgValue();
	DecodeModeRegister(m2, r2);
	dstvalue = GetArgValue();
	{
		/* if (regs.opsize != 1) a bug */
		int flgs = ui5r_MSBisSet(srcvalue);
		int flgo = ui5r_MSBisSet(dstvalue);
		ui4b newv_lo =
			(srcvalue & 0xF) + (dstvalue & 0xF) + (XFLG ? 1 : 0);
		ui4b newv_hi = (srcvalue & 0xF0) + (dstvalue & 0xF0);
		ui4b newv;

		if (newv_lo > 9) {
			newv_lo += 6;
		}
		newv = newv_hi + newv_lo;
		CFLG = XFLG = (newv & 0x1F0) > 0x90;
		if (CFLG) {
			newv += 0x60;
		}
		dstvalue = ui5r_FromSByte(newv);
		if (dstvalue != 0) {
			ZFLG = 0;
		}
		NFLG = ui5r_MSBisSet(dstvalue);
		VFLG = (flgs != flgo) && (NFLG != flgo);
		/*
			but according to my reference book,
			VFLG is Undefined for ABCD
		*/
	}
	SetArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeAbcdr(void)
{
	/* ABCD 1100ddd100000rrr */
	DoBinOpAbcd(0, reg, 0, rg9);
}

LOCALPROCUSEDONCE DoCodeAbcdm(void)
{
	/* ABCD 1100ddd100001rrr */
	DoBinOpAbcd(4, reg, 4, rg9);
}

LOCALPROC DoBinOpSbcd(ui5b m1, ui5b r1, ui5b m2, ui5b r2)
{
	ui5r srcvalue;
	ui5r dstvalue;

	regs.opsize = 1;
	DecodeModeRegister(m1, r1);
	srcvalue = GetArgValue();
	DecodeModeRegister(m2, r2);
	dstvalue = GetArgValue();
	{
		int flgs = ui5r_MSBisSet(srcvalue);
		int flgo = ui5r_MSBisSet(dstvalue);
		ui4b newv_lo =
			(dstvalue & 0xF) - (srcvalue & 0xF) - (XFLG ? 1 : 0);
		ui4b newv_hi = (dstvalue & 0xF0) - (srcvalue & 0xF0);
		ui4b newv;

		if (newv_lo > 9) {
			newv_lo -= 6;
			newv_hi -= 0x10;
		}
		newv = newv_hi + (newv_lo & 0xF);
		CFLG = XFLG = (newv_hi & 0x1F0) > 0x90;
		if (CFLG) {
			newv -= 0x60;
		}
		dstvalue = ui5r_FromSByte(newv);
		if (dstvalue != 0) {
			ZFLG = 0;
		}
		NFLG = ui5r_MSBisSet(dstvalue);
		VFLG = (flgs != flgo) && (NFLG != flgo);
		/*
			but according to my reference book,
			VFLG is Undefined for SBCD
		*/
	}
	SetArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeSbcdr(void)
{
	/* SBCD 1000xxx100000xxx */
	DoBinOpSbcd(0, reg, 0, rg9);
}

LOCALPROCUSEDONCE DoCodeSbcdm(void)
{
	/* SBCD 1000xxx100001xxx */
	DoBinOpSbcd(4, reg, 4, rg9);
}

LOCALPROCUSEDONCE DoCodeNbcd(void)
{
	/* Nbcd 0100100000mmmrrr */
	ui5r dstvalue;

	regs.opsize = 1;
	DecodeModeRegister(mode, reg);
	dstvalue = GetArgValue();
	{
		ui4b newv_lo = - (dstvalue & 0xF) - (XFLG ? 1 : 0);
		ui4b newv_hi = - (dstvalue & 0xF0);
		ui4b newv;

		if (newv_lo > 9) {
			newv_lo -= 6;
			newv_hi -= 0x10;
		}
		newv = newv_hi + (newv_lo & 0xF);
		CFLG = XFLG = (newv_hi & 0x1F0) > 0x90;
		if (CFLG) {
			newv -= 0x60;
		}

		dstvalue = ui5r_FromSByte(newv);
		NFLG = ui5r_MSBisSet(dstvalue);
		if (dstvalue != 0) {
			ZFLG = 0;
		}
	}
	SetArgValue(dstvalue);
}

LOCALPROCUSEDONCE DoCodeRte(void)
{
	/* Rte 0100111001110011 */
	if (! regs.s) {
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
					ReportAbnormal("rte stack frame format 1");
					NewPC = m68k_getpc() - 2;
						/* rerun instruction */
					break;
				case 2:
					ReportAbnormal("rte stack frame format 2");
					stackp += 4;
					break;
				case 9:
					ReportAbnormal("rte stack frame format 9");
					stackp += 12;
					break;
				case 10:
					ReportAbnormal("rte stack frame format 10");
					stackp += 24;
					break;
				case 11:
					ReportAbnormal("rte stack frame format 11");
					stackp += 84;
					break;
				default:
					ReportAbnormal("unknown rte stack frame format");
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

LOCALPROCUSEDONCE DoCodeNop(void)
{
	/* Nop 0100111001110001 */
}

LOCALPROCUSEDONCE DoCodeMoveP(void)
{
	/* MoveP 0000ddd1mm001aaa */
	ui5r TheReg = reg;
	ui5r TheRg9 = rg9;
	ui5r Displacement = nextiword();
		/* shouldn't this sign extend ? */
	CPTR memp = m68k_areg(TheReg) + Displacement;

#if 0
	if ((Displacement & 0x00008000) != 0) {
		/* **** for testing only **** */
		BackupPC();
		op_illg();
	}
#endif

	switch (b76) {
		case 0:
			{
				ui4b val = ((get_byte(memp) & 0x00FF) << 8)
					| (get_byte(memp + 2) & 0x00FF);

				m68k_dreg(TheRg9) =
					(m68k_dreg(TheRg9) & ~ 0xffff) | (val & 0xffff);
			}
			break;
		case 1:
			{
				ui5b val = ((get_byte(memp) & 0x00FF) << 24)
					| ((get_byte(memp + 2) & 0x00FF) << 16)
					| ((get_byte(memp + 4) & 0x00FF) << 8)
					| (get_byte(memp + 6) & 0x00FF);

				m68k_dreg(TheRg9) = val;
			}
			break;
		case 2:
			{
				si4b src = m68k_dreg(TheRg9);

				put_byte(memp, src >> 8);
				put_byte(memp + 2, src);
			}
			break;
		case 3:
			{
				si5b src = m68k_dreg(TheRg9);

				put_byte(memp, src >> 24);
				put_byte(memp + 2, src >> 16);
				put_byte(memp + 4, src >> 8);
				put_byte(memp + 6, src);
			}
			break;
	}
}

LOCALPROC op_illg(void)
{
	BackupPC();
	Exception(4);
}

LOCALPROC DoCheck(void)
{
	ui5r srcvalue;
	ui5r dstvalue;

	DecodeModeRegister(mode, reg);
	srcvalue = GetArgValue();
	DecodeModeRegister(0, rg9);
	dstvalue = GetArgValue();
	if (ui5r_MSBisSet(dstvalue)) {
#if WantCloserCyc
		regs.MaxCyclesToGo -=
			(30 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		NFLG = 1;
		Exception(6);
	} else if (((si5r)dstvalue) > ((si5r)srcvalue)) {
#if WantCloserCyc
		regs.MaxCyclesToGo -=
			(30 * kCycleScale + 3 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		NFLG = 0;
		Exception(6);
	}
}

LOCALPROCUSEDONCE DoCodeChkW(void)
{
	/* Chk.W 0100ddd110mmmrrr */
	regs.opsize = 2;
	DoCheck();
}

LOCALPROCUSEDONCE DoCodeTrap(void)
{
	/* Trap 010011100100vvvv */
	Exception((regs.opcode & 15) + 32);
}

LOCALPROCUSEDONCE DoCodeTrapV(void)
{
	/* TrapV 0100111001110110 */
	if (VFLG) {
#if WantCloserCyc
		regs.MaxCyclesToGo += GetDcoCycles(&regs.CurDecOp);
		regs.MaxCyclesToGo -=
			(34 * kCycleScale + 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(7);
	}
}

LOCALPROCUSEDONCE DoCodeRtr(void)
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

LOCALPROCUSEDONCE DoCodeLink(void)
{
	ui5r srcreg = reg;
	CPTR stackp = m68k_areg(7);
	stackp -= 4;
	m68k_areg(7) = stackp; /* only matters if srcreg == 7 */
	put_long(stackp, m68k_areg(srcreg));
	m68k_areg(srcreg) = stackp;
	m68k_areg(7) += ui5r_FromSWord(nextiword());
}

LOCALPROCUSEDONCE DoCodeUnlk(void)
{
	ui5r srcreg = reg;
	if (srcreg != 7) {
		ui5r src = m68k_areg(srcreg);
		m68k_areg(srcreg) = get_long(src);
		m68k_areg(7) = src + 4;
	} else {
		/* wouldn't expect this to happen */
		m68k_areg(7) = get_long(m68k_areg(7)) + 4;
	}
}

LOCALPROCUSEDONCE DoCodeMoveRUSP(void)
{
	/* MOVE USP 0100111001100aaa */
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		regs.usp = m68k_areg(reg);
	}
}

LOCALPROCUSEDONCE DoCodeMoveUSPR(void)
{
	/* MOVE USP 0100111001101aaa */
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		m68k_areg(reg) = regs.usp;
	}
}

LOCALPROCUSEDONCE DoCodeTas(void)
{
	/* Tas 0100101011mmmrrr */
	ui5r dstvalue;

	regs.opsize = 1;
	DecodeModeRegister(mode, reg);
	dstvalue = GetArgValue();

	{
		ZFLG = (dstvalue == 0);
		NFLG = ui5r_MSBisSet(dstvalue);
		VFLG = CFLG = 0;
		dstvalue |= 0x80;
	}
	SetArgValue(dstvalue);
}

LOCALPROC DoCodeFdefault(void)
{
	BackupPC();
	Exception(0xB);
}

#if EmMMU
LOCALPROCUSEDONCE DoCodeMMU(void)
{
	/*
		Emulate enough of MMU for System 7.5.5 universal
		to boot on Mac Plus 68020. There is one
		spurious "PMOVE TC, (A0)".
		And implement a few more PMOVE operations seen
		when running Disk Copy 6.3.3 and MacsBug.
	*/
	if (regs.opcode == 0xF010) {
		ui4b ew = (int)nextiword();
		if (ew == 0x4200) {
			/* PMOVE TC, (A0) */
			/* fprintf(stderr, "0xF010 0x4200\n"); */
			regs.opsize = 4;
			DecodeModeRegister(mode, reg);
			SetArgValue(0);
			return;
		} else if ((ew == 0x4E00) || (ew == 0x4A00)) {
			/* PMOVE CRP, (A0) and PMOVE SRP, (A0) */
			/* fprintf(stderr, "0xF010 %x\n", ew); */
			regs.opsize = 4;
			DecodeModeRegister(mode, reg);
			SetArgValue(0x7FFF0001);
			regs.ArgAddr.mem += 4;
			SetArgValue(0);
			return;
		} else if (ew == 0x6200) {
			/* PMOVE MMUSR, (A0) */
			/* fprintf(stderr, "0xF010 %x\n", ew); */
			regs.opsize = 2;
			DecodeModeRegister(mode, reg);
			SetArgValue(0);
			return;
		}
		/* fprintf(stderr, "extensions %x\n", ew); */
		BackupPC();
	}
	/* fprintf(stderr, "opcode %x\n", (int)regs.opcode); */
	ReportAbnormal("MMU op");
	DoCodeFdefault();
}
#endif

#if EmFPU

#if 0
#include "FPMATHNT.h"
#else
#include "FPMATHEM.h"
#endif

#include "FPCPEMDV.h"

#endif

LOCALPROCUSEDONCE DoCodeF(void)
{
	/* ReportAbnormal("DoCodeF"); */
#if EmMMU
	if (0 == rg9) {
		DoCodeMMU();
	} else
#endif
#if EmFPU
	if (1 == rg9) {
		DoCodeFPU();
	} else
#endif
	{
		DoCodeFdefault();
	}
}

LOCALPROCUSEDONCE DoCodeCallMorRtm(void)
{
	/* CALLM or RTM 0000011011mmmrrr */
	ReportAbnormal("CALLM or RTM instruction");
}

LOCALPROC m68k_setstopped(void)
{
	/* not implemented. doesn't seemed to be used on Mac Plus */
	Exception(4); /* fake an illegal instruction */
}

LOCALPROCUSEDONCE DoCodeStop(void)
{
	/* Stop 0100111001110010 */
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		m68k_setSR(nextiword());
		m68k_setstopped();
	}
}

LOCALPROCUSEDONCE DoCodeReset(void)
{
	/* Reset 0100111001100000 */
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		customreset();
	}
}

#if Use68020
LOCALPROCUSEDONCE DoCodeEXTBL(void)
{
	/* EXTB.L */
	ui5r srcreg = reg;
	ui5r src = m68k_dreg(srcreg);
	ui5r dst = ui5r_FromSByte(src);
	VFLG = CFLG = 0;
	ZFLG = (dst == 0);
	NFLG = ui5r_MSBisSet(dst);
	m68k_dreg(srcreg) = dst;
}
#endif

#if Use68020
LOCALPROC DoCHK2orCMP2(void)
{
	/* CHK2 or CMP2 00000ss011mmmrrr */
	ui5r regv;
	ui5r lower;
	ui5r upper;
	ui5r extra = nextiword();

	/* ReportAbnormal("CHK2 or CMP2 instruction"); */
	switch ((regs.opcode >> 9) & 3) {
		case 0:
			regs.opsize = 1;
			break;
		case 1:
			regs.opsize = 2;
			break;
		case 2:
			regs.opsize = 4;
			break;
		default:
			ReportAbnormal("illegal opsize in CHK2 or CMP2");
			break;
	}
	if ((extra & 0x8000) == 0) {
		DecodeModeRegister(0, (extra >> 12) & 0x07);
		regv = GetArgValue();
	} else {
		regv = ui5r_FromSLong(m68k_areg((extra >> 12) & 0x07));
	}
	DecodeModeRegister(mode, reg);
	/* regs.ArgKind == AKMemory, otherwise illegal and don't get here */
	lower = GetArgValue();
	regs.ArgAddr.mem += regs.opsize;
	upper = GetArgValue();

	ZFLG = (upper == regv) || (lower == regv);
	CFLG = (((si5r)lower) <= ((si5r)upper))
			? (((si5r)regv) < ((si5r)lower)
				|| ((si5r)regv) > ((si5r)upper))
			: (((si5r)regv) > ((si5r)upper)
				|| ((si5r)regv) < ((si5r)lower));
	if ((extra & 0x800) && CFLG) {
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

	ui4b src = nextiword();
	int ru = (src >> 6) & 7;
	int rc = src & 7;

	ReportAbnormal("CAS instruction");
	switch ((regs.opcode >> 9) & 3) {
		case 1 :
			regs.opsize = 1;
			break;
		case 2 :
			regs.opsize = 2;
			break;
		case 3 :
			regs.opsize = 4;
			break;
	}

	DecodeModeRegister(0, rc);
	srcvalue = GetArgValue();
	DecodeModeRegister(mode, reg);
	dstvalue = GetArgValue();
	{
		int flgs = ((si5b)srcvalue) < 0;
		int flgo = ((si5b)dstvalue) < 0;
		ui5r newv = dstvalue - srcvalue;
		if (regs.opsize == 1) {
			newv = ui5r_FromSByte(newv);
		} else if (regs.opsize == 2) {
			newv = ui5r_FromSWord(newv);
		} else {
			newv = ui5r_FromSLong(newv);
		}
		ZFLG = (((si5b)newv) == 0);
		NFLG = (((si5b)newv) < 0);
		VFLG = (flgs != flgo) && (NFLG != flgo);
		CFLG = (flgs && ! flgo) || (NFLG && ((! flgo) || flgs));
		if (ZFLG) {
			SetArgValue(m68k_dreg(ru));
		} else {
			DecodeModeRegister(0, rc);
			SetArgValue(dstvalue);
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
	CPTR rn1 = regs.regs[(extra >> 28) & 0x0F];
	CPTR rn2 = regs.regs[(extra >> 12) & 0x0F];
	si5b src = m68k_dreg(dc1);
	si5r dst1;
	si5r dst2;

	ReportAbnormal("DoCAS2 instruction");
	switch ((regs.opcode >> 9) & 3) {
		case 1 :
			op_illg();
			return;
			break;
		case 2 :
			regs.opsize = 2;
			break;
		case 3 :
			regs.opsize = 4;
			break;
	}
	if (regs.opsize == 2) {
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
		if (regs.opsize == 2) {
			newv = (ui4b)newv;
		}
		ZFLG = (newv == 0);
		NFLG = (newv < 0);
		VFLG = (flgs != flgo) && (NFLG != flgo);
		CFLG = (flgs && ! flgo) || (NFLG && ((! flgo) || flgs));
		if (ZFLG) {
			src = m68k_dreg(dc2);
			if (regs.opsize == 2) {
				src = (si5b)(si4b)src;
			}
			flgs = src < 0;
			flgo = dst2 < 0;
			newv = dst2 - src;
			if (regs.opsize == 2) {
				newv = (ui4b)newv;
			}
			ZFLG = (newv == 0);
			NFLG = (newv < 0);
			VFLG = (flgs != flgo) && (NFLG != flgo);
			CFLG = (flgs && ! flgo) || (NFLG && ((! flgo) || flgs));
			if (ZFLG) {
				if (regs.opsize == 2) {
					put_word(rn1, m68k_dreg(du1));
					put_word(rn2, m68k_dreg(du2));
				} else {
					put_word(rn1, m68k_dreg(du1));
					put_word(rn2, m68k_dreg(du2));
				}
			}
		}
	}
	if (! ZFLG) {
		if (regs.opsize == 2) {
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
	ReportAbnormal("MoveS instruction");
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		ui4b extra = nextiword();
		FindOpSizeFromb76();
		DecodeModeRegister(mode, reg);
		if (extra & 0x0800) {
			ui5b src = regs.regs[(extra >> 12) & 0x0F];
			SetArgValue(src);
		} else {
			ui5b rr = (extra >> 12) & 7;
			si5b srcvalue = GetArgValue();
			if (extra & 0x8000) {
				m68k_areg(rr) = srcvalue;
			} else {
				DecodeModeRegister(0, rr);
				SetArgValue(srcvalue);
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
LOCALFUNC blnr Ui6r_IsZero(ui6r0 *v)
{
	return (v->hi == 0) && (v->lo == 0);
}
#endif

#if Use68020
LOCALFUNC blnr Ui6r_IsNeg(ui6r0 *v)
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
LOCALPROC DoMulL(void)
{
	ui6r0 dst;
	ui5r srcvalue;
	ui4b extra = nextiword();
	ui5b r2 = (extra >> 12) & 7;
	ui5b dstvalue = m68k_dreg(r2);

	DecodeModeRegister(mode, reg);
	srcvalue = GetArgValue();

	if (extra & 0x800) {
		/* MULS.L - signed */

		si5b src1 = (si5b)srcvalue;
		si5b src2 = (si5b)dstvalue;
		flagtype s1 = src1 < 0;
		flagtype s2 = src2 < 0;
		flagtype sr = s1 != s2;

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
		ZFLG = Ui6r_IsZero(&dst);
		NFLG = Ui6r_IsNeg(&dst);
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
		ZFLG = Ui6r_IsZero(&dst);
		NFLG = Ui6r_IsNeg(&dst);
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
LOCALPROC DoDivL(void)
{
	ui6r0 v2;
	ui5b src;
	ui5b quot;
	ui5b rem;
	ui4b extra = nextiword();
	ui5b rDr = extra & 7;
	ui5b rDq = (extra >> 12) & 7;

	DecodeModeRegister(mode, reg);
	src = (ui5b)(si5b)GetArgValue();

	if (src == 0) {
		Exception(5);
#if m68k_logExceptions
		dbglog_StartLine();
		dbglog_writeCStr("*** zero devide exception");
		dbglog_writeReturn();
#endif
		return;
	}
	if (0 != (extra & 0x0800)) {
		/* signed variant */
		flagtype sr;
		flagtype s2;
		flagtype s1 = ((si5b)src < 0);

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
			ZFLG = ((si5b)quot) == 0;
			NFLG = ((si5b)quot) < 0;
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
			VFLG = NFLG = 1;
			CFLG = 0;
		} else {
			VFLG = CFLG = 0;
			ZFLG = ((si5b)quot) == 0;
			NFLG = ((si5b)quot) < 0;
			m68k_dreg(rDr) = rem;
			m68k_dreg(rDq) = quot;
		}
	}
}
#endif

#if Use68020
LOCALPROC DoMoveToControl(void)
{
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		ui4b src = nextiword();
		int regno = (src >> 12) & 0x0F;
		ui5b v = regs.regs[regno];

		switch (src & 0x0FFF) {
			case 0x0000:
				regs.sfc = v & 7;
				/* ReportAbnormal("DoMoveToControl: sfc"); */
				/* happens on entering macsbug */
				break;
			case 0x0001:
				regs.dfc = v & 7;
				/* ReportAbnormal("DoMoveToControl: dfc"); */
				break;
			case 0x0002:
				regs.cacr = v & 0x3;
				/* ReportAbnormal("DoMoveToControl: cacr"); */
				/* used by Sys 7.5.5 boot */
				break;
			case 0x0800:
				regs.usp = v;
				ReportAbnormal("DoMoveToControl: usp");
				break;
			case 0x0801:
				regs.vbr = v;
				/* ReportAbnormal("DoMoveToControl: vbr"); */
				/* happens on entering macsbug */
				break;
			case 0x0802:
				regs.caar = v &0xfc;
				/* ReportAbnormal("DoMoveToControl: caar"); */
				/* happens on entering macsbug */
				break;
			case 0x0803:
				regs.msp = v;
				if (regs.m == 1) {
					m68k_areg(7) = regs.msp;
				}
				/* ReportAbnormal("DoMoveToControl: msp"); */
				/* happens on entering macsbug */
				break;
			case 0x0804:
				regs.isp = v;
				if (regs.m == 0) {
					m68k_areg(7) = regs.isp;
				}
				ReportAbnormal("DoMoveToControl: isp");
				break;
			default:
				op_illg();
				ReportAbnormal("DoMoveToControl: unknown reg");
				break;
		}
	}
}
#endif

#if Use68020
LOCALPROC DoMoveFromControl(void)
{
	if (! regs.s) {
		DoPrivilegeViolation();
	} else {
		ui5b v;
		ui4b src = nextiword();
		int regno = (src >> 12) & 0x0F;

		switch (src & 0x0FFF) {
			case 0x0000:
				v = regs.sfc;
				/* ReportAbnormal("DoMoveFromControl: sfc"); */
				/* happens on entering macsbug */
				break;
			case 0x0001:
				v = regs.dfc;
				/* ReportAbnormal("DoMoveFromControl: dfc"); */
				/* happens on entering macsbug */
				break;
			case 0x0002:
				v = regs.cacr;
				/* ReportAbnormal("DoMoveFromControl: cacr"); */
				/* used by Sys 7.5.5 boot */
				break;
			case 0x0800:
				v = regs.usp;
				ReportAbnormal("DoMoveFromControl: usp");
				break;
			case 0x0801:
				v = regs.vbr;
				/* ReportAbnormal("DoMoveFromControl: vbr"); */
				/* happens on entering macsbug */
				break;
			case 0x0802:
				v = regs.caar;
				/* ReportAbnormal("DoMoveFromControl: caar"); */
				/* happens on entering macsbug */
				break;
			case 0x0803:
				v = (regs.m == 1)
					? m68k_areg(7)
					: regs.msp;
				/* ReportAbnormal("DoMoveFromControl: msp"); */
				/* happens on entering macsbug */
				break;
			case 0x0804:
				v = (regs.m == 0)
					? m68k_areg(7)
					: regs.isp;
				ReportAbnormal("DoMoveFromControl: isp");
				break;
			default:
				v = 0;
				ReportAbnormal("DoMoveFromControl: unknown reg");
				op_illg();
				break;
		}
		regs.regs[regno] = v;
	}
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeMoveC(void)
{
	/* MOVEC 010011100111101m */
	/* ReportAbnormal("MOVEC"); */
	switch (reg) {
		case 2:
			DoMoveFromControl();
			break;
		case 3:
			DoMoveToControl();
			break;
		default:
			op_illg();
			break;
	}
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeChkL(void)
{
	/* Chk.L 0100ddd100mmmrrr */
	ReportAbnormal("CHK.L instruction");
	regs.opsize = 4;
	DoCheck();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeBkpt(void)
{
	/* BKPT 0100100001001rrr */
	ReportAbnormal("BKPT instruction");
	op_illg();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeDivL(void)
{
	regs.opsize = 4;
	/* DIVU 0100110001mmmrrr 0rrr0s0000000rrr */
	/* DIVS 0100110001mmmrrr 0rrr1s0000000rrr */
	/* ReportAbnormal("DIVS/DIVU long"); */
	DoDivL();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeMulL(void)
{
	regs.opsize = 4;
	/* MULU 0100110000mmmrrr 0rrr0s0000000rrr */
	/* MULS 0100110000mmmrrr 0rrr1s0000000rrr */
	DoMulL();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeRtd(void)
{
	/* Rtd 0100111001110100 */
	ui5r NewPC = get_long(m68k_areg(7));
	si5b offs = (si5b)(si4b)nextiword();
	/* ReportAbnormal("RTD"); */
	/* used by Sys 7.5.5 boot */
	m68k_areg(7) += (4 + offs);
	m68k_setpc(NewPC);
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeLinkL(void)
{
	/* Link.L 0100100000001rrr */

	ui5b srcreg = reg;
	CPTR stackp = m68k_areg(7);

	ReportAbnormal("Link.L");

	stackp -= 4;
	m68k_areg(7) = stackp; /* only matters if srcreg == 7 */
	put_long(stackp, m68k_areg(srcreg));
	m68k_areg(srcreg) = stackp;
	m68k_areg(7) += (si5b)nextilong();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeTRAPcc(void)
{
	/* TRAPcc 0101cccc11111sss */
	/* ReportAbnormal("TRAPcc"); */
	switch (reg) {
		case 2:
			ReportAbnormal("TRAPcc word data");
			(void) nextiword();
			break;
		case 3:
			ReportAbnormal("TRAPcc long data");
			(void) nextilong();
			break;
		case 4:
			/* no optional data */
			break;
		default:
			ReportAbnormal("TRAPcc illegal format");
			op_illg();
			break;
	}
	if (cctrue()) {
		ReportAbnormal("TRAPcc trapping");
		Exception(7);
		/* pc pushed onto stack wrong */
	}
}
#endif

#if Use68020
LOCALPROC DoUNPK(void)
{
	ui5r val;
	ui5r m1 = ((regs.opcode >> 3) & 1) << 2;
	ui5r srcreg = reg;
	ui5r dstreg = rg9;
	ui5r offs = ui5r_FromSWord(nextiword());

	regs.opsize = 1;
	DecodeModeRegister(m1, srcreg);
	val = GetArgValue();

	val = (((val & 0xF0) << 4) | (val & 0x0F)) + offs;

	regs.opsize = 2;
	DecodeModeRegister(m1, dstreg);
	SetArgValue(val);
}
#endif

#if Use68020
LOCALPROC DoPACK(void)
{
	ui5r val;
	ui5r m1 = ((regs.opcode >> 3) & 1) << 2;
	ui5r srcreg = reg;
	ui5r dstreg = rg9;
	ui5r offs = ui5r_FromSWord(nextiword());

	regs.opsize = 2;
	DecodeModeRegister(m1, srcreg);
	val = GetArgValue();

	val += offs;
	val = ((val >> 4) & 0xf0) | (val & 0xf);

	regs.opsize = 1;
	DecodeModeRegister(m1, dstreg);
	SetArgValue(val);
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodePack(void)
{
	ReportAbnormal("PACK");
	DoPACK();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DoCodeUnpk(void)
{
	ReportAbnormal("UNPK");
	DoUNPK();
}
#endif

#if Use68020
LOCALPROC DoBitField(void)
{
	ui5b tmp;
	ui5b newtmp;
	si5b dsta;
	ui5b bf0;
	ui3b bf1;
	ui5b dstreg = regs.opcode & 7;
	ui4b extra = nextiword();
	si5b offset = ((extra & 0x0800) != 0)
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
	if (mode == 0) {
		bf0 = m68k_dreg(dstreg);
		offset &= 0x1f;
		tmp = bf0;
		if (0 != offset) {
			tmp = (tmp << offset) | (tmp >> (32 - offset));
		}
	} else {
		DecodeModeRegister(mode, reg);
		/*
			regs.ArgKind == AKMemory,
			otherwise illegal and don't get here
		*/
		dsta = regs.ArgAddr.mem;
		dsta +=
			(offset >> 3) | (offset & 0x80000000 ? ~ 0x1fffffff : 0);
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

	NFLG = ((si5b)tmp) < 0;
	if (width != 0) {
		tmp >>= (32 - width);
	}
	ZFLG = tmp == 0;
	VFLG = 0;
	CFLG = 0;

	newtmp = tmp;

	switch ((regs.opcode >> 8) & 7) {
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
			if (NFLG) {
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
				si5b i = offset;

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

		if (mode == 0) {
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

#ifndef WantDumpTable
#define WantDumpTable 0
#endif

#if WantDumpTable
LOCALVAR ui5b DumpTable[kNumIKinds];

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

LOCALPROC m68k_go_MaxCycles(void)
{
	/*
		Main loop of emulator.

		Always execute at least one instruction,
		even if regs.MaxInstructionsToGo == 0.
		Needed for trace flag to work.
	*/

	do {

#if WantDisasm
		DisasmOneOrSave(m68k_getpc());
#endif

		regs.opcode = nextiword();

		regs.CurDecOp = regs.disp_table[regs.opcode];
#if WantDumpTable
		DumpTable[GetDcoMainClas(&regs.CurDecOp)] ++;
#endif
		regs.MaxCyclesToGo -= GetDcoCycles(&regs.CurDecOp);

		switch (GetDcoMainClas(&regs.CurDecOp)) {
			case kIKindTst :
				DoCodeTst();
				break;
			case kIKindCmpB :
				DoCodeCmpB();
				break;
			case kIKindCmpW :
				DoCodeCmpW();
				break;
			case kIKindCmpL :
				DoCodeCmpL();
				break;
			case kIKindBccB :
				DoCodeBccB();
				break;
			case kIKindBccW :
				DoCodeBccW();
				break;
#if Use68020
			case kIKindBccL :
				DoCodeBccL();
				break;
#endif
			case kIKindBraB :
				DoCodeBraB();
				break;
			case kIKindBraW :
				DoCodeBraW();
				break;
#if Use68020
			case kIKindBraL :
				DoCodeBraL();
				break;
#endif
			case kIKindDBcc :
				DoCodeDBcc();
				break;
			case kIKindDBF :
				DoCodeDBcc();
				break;
			case kIKindSwap :
				DoCodeSwap();
				break;
			case kIKindMoveL :
				DoCodeMove();
				break;
			case kIKindMoveW :
				DoCodeMove();
				break;
			case kIKindMoveB :
				DoCodeMove();
				break;
			case kIKindMoveAL :
				DoCodeMoveA();
				break;
			case kIKindMoveAW :
				DoCodeMoveA();
				break;
			case kIKindMoveQ:
				DoCodeMoveQ();
				break;
			case kIKindAddB :
				DoCodeAddB();
				break;
			case kIKindAddW :
				DoCodeAddW();
				break;
			case kIKindAddL :
				DoCodeAddL();
				break;
			case kIKindSubB :
				DoCodeSubB();
				break;
			case kIKindSubW :
				DoCodeSubW();
				break;
			case kIKindSubL :
				DoCodeSubL();
				break;
			case kIKindLea :
				DoCodeLea();
				break;
			case kIKindPEA :
				DoCodePEA();
				break;
			case kIKindA :
				DoCodeA();
				break;
			case kIKindBsrB :
				DoCodeBsrB();
				break;
			case kIKindBsrW :
				DoCodeBsrW();
				break;
#if Use68020
			case kIKindBsrL :
				DoCodeBsrL();
				break;
#endif
			case kIKindJsr :
				DoCodeJsr();
				break;
			case kIKindLinkA6 :
				DoCodeLinkA6();
				break;
			case kIKindMOVEMRmML :
				DoCodeMOVEMRmML();
				break;
			case kIKindMOVEMApRL :
				DoCodeMOVEMApRL();
				break;
			case kIKindUnlkA6 :
				DoCodeUnlkA6();
				break;
			case kIKindRts :
				DoCodeRts();
				break;
			case kIKindJmp :
				DoCodeJmp();
				break;
			case kIKindClr :
				DoCodeClr();
				break;
			case kIKindAddA :
				DoCodeAddA();
				break;
			case kIKindAddQA :
				DoCodeAddA();
				/* DoCodeAddQA(); */
				break;
			case kIKindSubA :
				DoCodeSubA();
				break;
			case kIKindSubQA :
				DoCodeSubA();
				/* DoCodeSubQA(); */
				break;
			case kIKindCmpA :
				DoCodeCmpA();
				break;
			case kIKindAddXB :
				DoCodeAddXB();
				break;
			case kIKindAddXW :
				DoCodeAddXW();
				break;
			case kIKindAddXL :
				DoCodeAddXL();
				break;
			case kIKindSubXB :
				DoCodeSubXB();
				break;
			case kIKindSubXW :
				DoCodeSubXW();
				break;
			case kIKindSubXL :
				DoCodeSubXL();
				break;

			case kIKindRolopNM :
				DoCodeRolopNM();
				break;
			case kIKindRolopND :
				DoCodeRolopND();
				break;
			case kIKindRolopDD :
				DoCodeRolopDD();
				break;
			case kIKindBitOpDD :
				DoCodeBitOpDD();
				break;
			case kIKindBitOpDM :
				DoCodeBitOpDM();
				break;
			case kIKindBitOpND :
				DoCodeBitOpND();
				break;
			case kIKindBitOpNM :
				DoCodeBitOpNM();
				break;

			case kIKindAndI :
				DoCodeAnd();
				/* DoCodeAndI(); */
				break;
			case kIKindAndEaD :
				DoCodeAnd();
				/* DoCodeAndEaD(); */
				break;
			case kIKindAndDEa :
				DoCodeAnd();
				/* DoCodeAndDEa(); */
				break;
			case kIKindOrI :
				DoCodeOr();
				break;
			case kIKindOrEaD :
				/* DoCodeOrEaD(); */
				DoCodeOr();
				break;
			case kIKindOrDEa :
				/* DoCodeOrDEa(); */
				DoCodeOr();
				break;
			case kIKindEor :
				DoCodeEor();
				break;
			case kIKindEorI :
				DoCodeEor();
				break;
			case kIKindNot :
				DoCodeNot();
				break;

			case kIKindScc :
				DoCodeScc();
				break;
			case kIKindEXTL :
				DoCodeEXTL();
				break;
			case kIKindEXTW :
				DoCodeEXTW();
				break;
			case kIKindNegB :
				DoCodeNegB();
				break;
			case kIKindNegW :
				DoCodeNegW();
				break;
			case kIKindNegL :
				DoCodeNegL();
				break;
			case kIKindNegXB :
				DoCodeNegXB();
				break;
			case kIKindNegXW :
				DoCodeNegXW();
				break;
			case kIKindNegXL :
				DoCodeNegXL();
				break;

			case kIKindMulU :
				DoCodeMulU();
				break;
			case kIKindMulS :
				DoCodeMulS();
				break;
			case kIKindDivU :
				DoCodeDivU();
				break;
			case kIKindDivS :
				DoCodeDivS();
				break;
			case kIKindExgdd :
				DoCodeExgdd();
				break;
			case kIKindExgaa :
				DoCodeExgaa();
				break;
			case kIKindExgda :
				DoCodeExgda();
				break;

			case kIKindMoveCCREa :
				DoCodeMoveCCREa();
				break;
			case kIKindMoveEaCCR :
				DoCodeMoveEaCR();
				break;
			case kIKindMoveSREa :
				DoCodeMoveSREa();
				break;
			case kIKindMoveEaSR :
				DoCodeMoveEaSR();
				break;
			case kIKindBinOpStatusCCR :
				DoBinOpStatusCCR();
				break;

			case kIKindMOVEMApRW :
				DoCodeMOVEMApRW();
				break;
			case kIKindMOVEMRmMW :
				DoCodeMOVEMRmMW();
				break;
			case kIKindMOVEMrm :
				DoCodeMOVEMrm();
				break;
			case kIKindMOVEMmr :
				DoCodeMOVEMmr();
				break;

			case kIKindAbcdr :
				DoCodeAbcdr();
				break;
			case kIKindAbcdm :
				DoCodeAbcdm();
				break;
			case kIKindSbcdr :
				DoCodeSbcdr();
				break;
			case kIKindSbcdm :
				DoCodeSbcdm();
				break;
			case kIKindNbcd :
				DoCodeNbcd();
				break;

			case kIKindRte :
				DoCodeRte();
				break;
			case kIKindNop :
				DoCodeNop();
				break;
			case kIKindMoveP :
				DoCodeMoveP();
				break;
			case kIKindIllegal :
				op_illg();
				break;

			case kIKindChkW :
				DoCodeChkW();
				break;
			case kIKindTrap :
				DoCodeTrap();
				break;
			case kIKindTrapV :
				DoCodeTrapV();
				break;
			case kIKindRtr :
				DoCodeRtr();
				break;
			case kIKindLink :
				DoCodeLink();
				break;
			case kIKindUnlk :
				DoCodeUnlk();
				break;
			case kIKindMoveRUSP :
				DoCodeMoveRUSP();
				break;
			case kIKindMoveUSPR :
				DoCodeMoveUSPR();
				break;
			case kIKindTas :
				DoCodeTas();
				break;
			case kIKindF :
				DoCodeF();
				break;
			case kIKindCallMorRtm :
				DoCodeCallMorRtm();
				break;
			case kIKindStop :
				DoCodeStop();
				break;
			case kIKindReset :
				DoCodeReset();
				break;
#if Use68020
			case kIKindEXTBL :
				DoCodeEXTBL();
				break;
			case kIKindTRAPcc :
				DoCodeTRAPcc();
				break;
			case kIKindChkL :
				DoCodeChkL();
				break;
			case kIKindBkpt :
				DoCodeBkpt();
				break;
			case kIKindDivL :
				DoCodeDivL();
				break;
			case kIKindMulL :
				DoCodeMulL();
				break;
			case kIKindRtd :
				DoCodeRtd();
				break;
			case kIKindMoveC :
				DoCodeMoveC();
				break;
			case kIKindLinkL :
				DoCodeLinkL();
				break;
			case kIKindPack :
				DoCodePack();
				break;
			case kIKindUnpk :
				DoCodeUnpk();
				break;
			case kIKindCHK2orCMP2 :
				DoCHK2orCMP2();
				break;
			case kIKindCAS2 :
				DoCAS2();
				break;
			case kIKindCAS :
				DoCAS();
				break;
			case kIKindMoveS :
				DoMOVES();
				break;
			case kIKindBitField :
				DoBitField();
				break;
#endif
		}
	} while (regs.MaxCyclesToGo > 0);
}

GLOBALFUNC si5r GetCyclesRemaining(void)
{
	return regs.MoreCyclesToGo + regs.MaxCyclesToGo;
}

GLOBALPROC SetCyclesRemaining(si5r n)
{
	if (regs.MaxCyclesToGo >= n) {
		regs.MoreCyclesToGo = 0;
		regs.MaxCyclesToGo = n;
	} else {
		regs.MoreCyclesToGo = n - regs.MaxCyclesToGo;
	}
}

GLOBALFUNC ui3r get_vm_byte(CPTR addr)
{
	return (ui3b) get_byte(addr);
}

GLOBALFUNC ui4r get_vm_word(CPTR addr)
{
	return (ui4b) get_word(addr);
}

GLOBALFUNC ui5r get_vm_long(CPTR addr)
{
	return (ui5b) get_long(addr);
}

GLOBALPROC put_vm_byte(CPTR addr, ui3r b)
{
	put_byte(addr, ui5r_FromSByte(b));
}

GLOBALPROC put_vm_word(CPTR addr, ui4r w)
{
	put_word(addr, ui5r_FromSWord(w));
}

GLOBALPROC put_vm_long(CPTR addr, ui5r l)
{
	put_long(addr, ui5r_FromSLong(l));
}

GLOBALPROC SetHeadATTel(ATTep p)
{
	regs.MATCrdB.cmpmask = 0;
	regs.MATCrdB.cmpvalu = 0xFFFFFFFF;
	regs.MATCwrB.cmpmask = 0;
	regs.MATCwrB.cmpvalu = 0xFFFFFFFF;
	regs.MATCrdW.cmpmask = 0;
	regs.MATCrdW.cmpvalu = 0xFFFFFFFF;
	regs.MATCwrW.cmpmask = 0;
	regs.MATCwrW.cmpvalu = 0xFFFFFFFF;
	regs.MATCex.cmpmask = 0;
	regs.MATCex.cmpvalu = 0xFFFFFFFF;
	regs.HeadATTel = p;
}

LOCALPROC do_trace(void)
{
	regs.TracePending = trueblnr;
	NeedToGetOut();
}

GLOBALPROC DiskInsertedPsuedoException(CPTR newpc, ui5b data)
{
	ExceptionTo(newpc
#if Use68020
		, 0
#endif
		);
	m68k_areg(7) -= 4;
	put_long(m68k_areg(7), data);
}

LOCALPROC DoCheckExternalInterruptPending(void)
{
	ui3r level = *regs.fIPL;
	if ((level > regs.intmask) || (level == 7)) {
#if WantCloserCyc
		regs.MaxCyclesToGo -=
			(44 * kCycleScale + 5 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
		Exception(24 + level);
		regs.intmask = level;
	}
}

GLOBALPROC m68k_IPLchangeNtfy(void)
{
	ui3r level = *regs.fIPL;
	if ((level > regs.intmask) || (level == 7)) {
		SetExternalInterruptPending();
	}
}

#if WantDumpTable
FORWARDPROC InitDumpTable(void);
#endif

GLOBALPROC m68k_reset(void)
{
#if WantDumpTable
	InitDumpTable();
#endif
	regs.MaxCyclesToGo = 0;
	regs.MoreCyclesToGo = 0;
	regs.ResidualCycles = 0;

	do_put_mem_word(regs.fakeword, 0x4AFC);
		/* illegal instruction opcode */

#if 0
	regs.ResetPending = trueblnr;
	NeedToGetOut();
#else
/* Sets the MC68000 reset jump vector... */
	m68k_setpc(get_long(0x00000004));

/* Sets the initial stack vector... */
	m68k_areg(7) = get_long(0x00000000);

	regs.s = 1;
#if Use68020
	regs.m = 0;
	regs.t0 = 0;
#endif
	regs.t1 = 0;
	ZFLG = CFLG = NFLG = VFLG = 0;
	regs.ExternalInterruptPending = falseblnr;
	regs.TracePending = falseblnr;
	regs.intmask = 7;

#if Use68020
	regs.sfc = 0;
	regs.dfc = 0;
	regs.vbr = 0;
	regs.cacr = 0;
	regs.caar = 0;
#endif
#endif
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

	M68KITAB_setup(regs.disp_table);
}

GLOBALPROC m68k_go_nCycles(ui5b n)
{
	regs.MaxCyclesToGo += (n + regs.ResidualCycles);
	while (regs.MaxCyclesToGo > 0) {

#if 0
		if (regs.ResetPending) {
			m68k_DoReset();
		}
#endif
		if (regs.TracePending) {
#if WantCloserCyc
			regs.MaxCyclesToGo -= (34 * kCycleScale
				+ 4 * RdAvgXtraCyc + 3 * WrAvgXtraCyc);
#endif
			Exception(9);
		}
		if (regs.ExternalInterruptPending) {
			regs.ExternalInterruptPending = falseblnr;
			DoCheckExternalInterruptPending();
		}
		if (regs.t1) {
			do_trace();
		}
		m68k_go_MaxCycles();
		regs.MaxCyclesToGo += regs.MoreCyclesToGo;
		regs.MoreCyclesToGo = 0;
	}

	regs.ResidualCycles = regs.MaxCyclesToGo;
	regs.MaxCyclesToGo = 0;
}
