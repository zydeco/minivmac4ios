/*
	DISAM68K.c

	Copyright (C) 2010 Paul C. Pratt

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
	DISAssemble Motorola 68K instructions.
*/

#ifndef AllFiles
#include "SYSDEPNS.h"

#include "ENDIANAC.h"
#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"

#include "M68KITAB.h"
#endif

#include "DISAM68K.h"

LOCALVAR ui5r Disasm_pc;

/*
	don't use get_vm_byte/get_vm_word/get_vm_long
		so as to be sure of no side effects
		(if pc points to memory mapped device)
*/

LOCALVAR ui3p Disasm_pcp;
LOCALVAR ui5r Disasm_pc_blockmask;
LOCALVAR ui3b Disasm_pcp_dummy[2] = {
	0, 0
};

IMPORTFUNC ATTep FindATTel(CPTR addr);

LOCALPROC Disasm_Find_pcp(void)
{
	ATTep p;

	p = FindATTel(Disasm_pc);
	if (0 == (p->Access & kATTA_readreadymask)) {
		Disasm_pcp = Disasm_pcp_dummy;
		Disasm_pc_blockmask = 0;
	} else {
		Disasm_pc_blockmask = p->usemask & ~ p->cmpmask;
		Disasm_pc_blockmask = Disasm_pc_blockmask
			& ~ (Disasm_pc_blockmask + 1);
		Disasm_pcp = p->usebase + (Disasm_pc & p->usemask);
	}
}

LOCALFUNC ui4r Disasm_nextiword(void)
/* NOT sign extended */
{
	ui4r r = do_get_mem_word(Disasm_pcp);
	Disasm_pcp += 2;
	Disasm_pc += 2;
	if (0 == (Disasm_pc_blockmask & Disasm_pc)) {
		Disasm_Find_pcp();
	}
	return r;
}

LOCALINLINEFUNC ui3r Disasm_nextibyte(void)
{
	return (ui3b) Disasm_nextiword();
}

LOCALFUNC ui5r Disasm_nextilong(void)
{
	ui5r hi = Disasm_nextiword();
	ui5r lo = Disasm_nextiword();
	ui5r r = ((hi << 16) & 0xFFFF0000)
		| (lo & 0x0000FFFF);

	return r;
}

LOCALPROC Disasm_setpc(CPTR newpc)
{
	if (newpc != Disasm_pc) {
		Disasm_pc = newpc;

		Disasm_Find_pcp();
	}
}

LOCALVAR ui5b Disasm_opcode;

LOCALVAR ui5b Disasm_opsize;

#define Disasm_b76 ((Disasm_opcode >> 6) & 3)
#define Disasm_b8 ((Disasm_opcode >> 8) & 1)
#define Disasm_mode ((Disasm_opcode >> 3) & 7)
#define Disasm_reg (Disasm_opcode & 7)
#define Disasm_md6 ((Disasm_opcode >> 6) & 7)
#define Disasm_rg9 ((Disasm_opcode >> 9) & 7)

LOCALPROC DisasmOpSizeFromb76(void)
{
	Disasm_opsize = 1 << Disasm_b76;
	switch (Disasm_opsize) {
		case 1 :
			dbglog_writeCStr(".B");
			break;
		case 2 :
			dbglog_writeCStr(".W");
			break;
		case 4 :
			dbglog_writeCStr(".L");
			break;
	}
}

LOCALPROC DisasmModeRegister(ui5b themode, ui5b thereg)
{
	switch (themode) {
		case 0 :
			dbglog_writeCStr("D");
			dbglog_writeHex(thereg);
			break;
		case 1 :
			dbglog_writeCStr("A");
			dbglog_writeHex(thereg);
			break;
		case 2 :
			dbglog_writeCStr("(A");
			dbglog_writeHex(thereg);
			dbglog_writeCStr(")");
			break;
		case 3 :
			dbglog_writeCStr("(A");
			dbglog_writeHex(thereg);
			dbglog_writeCStr(")+");
			break;
		case 4 :
			dbglog_writeCStr("-(A");
			dbglog_writeHex(thereg);
			dbglog_writeCStr(")");
			break;
		case 5 :
			dbglog_writeHex(Disasm_nextiword());
			dbglog_writeCStr("(A");
			dbglog_writeHex(thereg);
			dbglog_writeCStr(")");
			break;
		case 6 :
			dbglog_writeCStr("???");
#if 0
			ArgKind = AKMemory;
			ArgAddr.mem = get_disp_ea(m68k_areg(thereg));
#endif
			break;
		case 7 :
			switch (thereg) {
				case 0 :
					dbglog_writeCStr("(");
					dbglog_writeHex(Disasm_nextiword());
					dbglog_writeCStr(")");
					break;
				case 1 :
					dbglog_writeCStr("(");
					dbglog_writeHex(Disasm_nextilong());
					dbglog_writeCStr(")");
					break;
				case 2 :
					{
						ui5r s = Disasm_pc;
						s += ui5r_FromSWord(Disasm_nextiword());
						dbglog_writeCStr("(");
						dbglog_writeHex(s);
						dbglog_writeCStr(")");
					}
					break;
				case 3 :
					dbglog_writeCStr("???");
#if 0
					ArgKind = AKMemory;
					s = get_disp_ea(Disasm_pc);
#endif
					break;
				case 4 :
					dbglog_writeCStr("#");
					if (Disasm_opsize == 2) {
						dbglog_writeHex(Disasm_nextiword());
					} else if (Disasm_opsize < 2) {
						dbglog_writeHex(Disasm_nextibyte());
					} else {
						dbglog_writeHex(Disasm_nextilong());
					}
					break;
			}
			break;
		case 8 :
			dbglog_writeCStr("#");
			dbglog_writeHex(thereg);
			break;
	}
}

LOCALPROC DisasmStartOne(char *s)
{
	dbglog_writeCStr(s);
}

LOCALPROC Disasm_xxxxxxxxssmmmrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROC DisasmEaD_xxxxdddxssmmmrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	dbglog_writeCStr("D");
	dbglog_writeHex(Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROC DisasmI_xxxxxxxxssmmmrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" #");
	if (Disasm_opsize == 2) {
		dbglog_writeHex(ui5r_FromSWord(Disasm_nextiword()));
	} else if (Disasm_opsize < 2) {
		dbglog_writeHex(ui5r_FromSByte(Disasm_nextibyte()));
	} else {
		dbglog_writeHex(ui5r_FromSLong(Disasm_nextilong()));
	}
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROC DisasmsAA_xxxxdddxssxxxrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	DisasmModeRegister(3, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(3, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALFUNC ui5r Disasm_octdat(ui5r x)
{
	if (x == 0) {
		return 8;
	} else {
		return x;
	}
}

LOCALPROC Disasm_xxxxnnnxssmmmrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" #");

	dbglog_writeHex(Disasm_octdat(Disasm_rg9));
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROC DisasmDEa_xxxxdddxssmmmrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" D");
	dbglog_writeHex(Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROC DisasmEaA_xxxxdddsxxmmmrrr(char *s)
{
	DisasmStartOne(s);

	Disasm_opsize = Disasm_b8 * 2 + 2;
	if (Disasm_opsize == 2) {
		dbglog_writeCStr(".W");
	} else {
		dbglog_writeCStr(".L");
	}
	dbglog_writeCStr(" ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", A");
	dbglog_writeHex(Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROC DisasmDD_xxxxdddxssxxxrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" ");
	DisasmModeRegister(0, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROC DisasmAAs_xxxxdddxssxxxrrr(char *s)
{
	DisasmStartOne(s);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" ");
	DisasmModeRegister(4, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(4, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmTst(void)
{
	/* Tst 01001010ssmmmrrr */
	Disasm_xxxxxxxxssmmmrrr("TST");
}

LOCALPROCUSEDONCE DisasmCompare(void)
{
	/* Cmp 1011ddd0ssmmmrrr */
	DisasmEaD_xxxxdddxssmmmrrr("CMP");
}

LOCALPROCUSEDONCE DisasmCmpI(void)
{
	/* CMPI 00001100ssmmmrrr */
	DisasmI_xxxxxxxxssmmmrrr("CMP");
}

LOCALPROCUSEDONCE DisasmCmpM(void)
{
	/* CmpM 1011ddd1ss001rrr */
	DisasmsAA_xxxxdddxssxxxrrr("CMP");
}

LOCALPROC DisasmCC(void)
{
	switch ((Disasm_opcode >> 8) & 15) {
		case 0:  dbglog_writeCStr("T"); break;
		case 1:  dbglog_writeCStr("F"); break;
		case 2:  dbglog_writeCStr("HI"); break;
		case 3:  dbglog_writeCStr("LS"); break;
		case 4:  dbglog_writeCStr("CC"); break;
		case 5:  dbglog_writeCStr("CS"); break;
		case 6:  dbglog_writeCStr("NE"); break;
		case 7:  dbglog_writeCStr("EQ"); break;
		case 8:  dbglog_writeCStr("VC"); break;
		case 9:  dbglog_writeCStr("VS"); break;
		case 10: dbglog_writeCStr("P"); break;
		case 11: dbglog_writeCStr("MI"); break;
		case 12: dbglog_writeCStr("GE"); break;
		case 13: dbglog_writeCStr("LT"); break;
		case 14: dbglog_writeCStr("GT"); break;
		case 15: dbglog_writeCStr("LE"); break;
		default: break; /* shouldn't get here */
	}
}

LOCALPROCUSEDONCE DisasmBcc(void)
{
	/* Bcc 0110ccccnnnnnnnn */
	ui5b src = ((ui5b)Disasm_opcode) & 255;
	ui5r s = Disasm_pc;

	if (0 == ((Disasm_opcode >> 8) & 15)) {
		DisasmStartOne("BRA");
	} else {
		DisasmStartOne("B");
		DisasmCC();
	}
	dbglog_writeCStr(" ");

	if (src == 0) {
		s += ui5r_FromSWord(Disasm_nextiword());
	} else
#if Use68020
	if (src == 255) {
		s += ui5r_FromSLong(Disasm_nextilong());
		/* ReportAbnormal("long branch in DoCode6"); */
		/* Used by various Apps */
	} else
#endif
	{
		s += ui5r_FromSByte(src);
	}
	dbglog_writeHex(s);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmDBcc(void)
{
	/* DBcc 0101cccc11001ddd */

	ui5r s = Disasm_pc;

	DisasmStartOne("DB");
	DisasmCC();

	dbglog_writeCStr(" D");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeCStr(", ");

	s += (si5b)(si4b)Disasm_nextiword();
	dbglog_writeHex(s);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmSwap(void)
{
	/* Swap 0100100001000rrr */

	DisasmStartOne("SWAP D");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROC DisasmMove(void) /* MOVE */
{
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_md6, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveL(void)
{
	DisasmStartOne("MOVE.L ");
	Disasm_opsize = 4;
	DisasmMove();
}

LOCALPROCUSEDONCE DisasmMoveW(void)
{
	DisasmStartOne("MOVE.W ");
	Disasm_opsize = 2;
	DisasmMove();
}

LOCALPROCUSEDONCE DisasmMoveB(void)
{
	DisasmStartOne("MOVE.B ");
	Disasm_opsize = 1;
	DisasmMove();
}

LOCALPROCUSEDONCE DisasmMoveAL(void)
{
	DisasmStartOne("MOVEA.L ");
	Disasm_opsize = 4;
	DisasmMove();
}

LOCALPROCUSEDONCE DisasmMoveAW(void)
{
	DisasmStartOne("MOVEA.W ");
	Disasm_opsize = 2;
	DisasmMove();
}

LOCALPROCUSEDONCE DisasmMoveQ(void)
{
	/* MoveQ 0111ddd0nnnnnnnn */
	DisasmStartOne("MOVEQ #");
	dbglog_writeHex(ui5r_FromSByte(Disasm_opcode));
	dbglog_writeCStr(", D");
	dbglog_writeHex(Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmAddEaR(void)
{
	DisasmEaD_xxxxdddxssmmmrrr("ADD");
}

LOCALPROCUSEDONCE DisasmAddQ(void)
{
	/* AddQ 0101nnn0ssmmmrrr */
	Disasm_xxxxnnnxssmmmrrr("ADDQ");
}

LOCALPROCUSEDONCE DisasmAddI(void)
{
	DisasmI_xxxxxxxxssmmmrrr("ADDI");
}

LOCALPROCUSEDONCE DisasmAddREa(void)
{
	DisasmDEa_xxxxdddxssmmmrrr("ADD");
}

LOCALPROCUSEDONCE DisasmSubEaR(void)
{
	DisasmEaD_xxxxdddxssmmmrrr("SUB");
}

LOCALPROCUSEDONCE DisasmSubQ(void)
{
	/* SubQ 0101nnn1ssmmmrrr */
	Disasm_xxxxnnnxssmmmrrr("SUBQ");
}

LOCALPROCUSEDONCE DisasmSubI(void)
{
	DisasmI_xxxxxxxxssmmmrrr("SUBI");
}

LOCALPROCUSEDONCE DisasmSubREa(void)
{
	DisasmDEa_xxxxdddxssmmmrrr("SUB");
}

LOCALPROCUSEDONCE DisasmLea(void)
{
	/* Lea 0100aaa111mmmrrr */
	DisasmStartOne("LEA ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", A");
	dbglog_writeHex(Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmPEA(void)
{
	/* PEA 0100100001mmmrrr */
	DisasmStartOne("PEA ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmALine(void)
{
	DisasmStartOne("$");
	dbglog_writeHex(Disasm_opcode);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmBsr(void)
{
	ui5b src = ((ui5b)Disasm_opcode) & 255;
	ui5r s = Disasm_pc;

	DisasmStartOne("BSR ");
	if (src == 0) {
		s += (si5b)(si4b)Disasm_nextiword();
	} else
#if Use68020
	if (src == 255) {
		s += (si5b)Disasm_nextilong();
		/* ReportAbnormal("long branch in DoCode6"); */
		/* Used by various Apps */
	} else
#endif
	{
		s += (si5b)(si3b)src;
	}
	dbglog_writeHex(s);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmJsr(void)
{
	/* Jsr 0100111010mmmrrr */
	DisasmStartOne("JSR ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmLinkA6(void)
{
	DisasmStartOne("LINK A6, ");
	dbglog_writeHex(Disasm_nextiword());
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMOVEMRmM(void)
{
	/* MOVEM reg to mem 0100100011s100rrr */
	si4b z;
	ui5r regmask;

	DisasmStartOne("MOVEM");
	if (Disasm_b76 == 2) {
		dbglog_writeCStr(".W");
	} else {
		dbglog_writeCStr(".L");
	}
	dbglog_writeCStr(" ");
	regmask = Disasm_nextiword();

	for (z = 16; --z >= 0; ) {
		if ((regmask & (1 << (15 - z))) != 0) {
			if (z >= 8) {
				dbglog_writeCStr("A");
				dbglog_writeHex(z - 8);
			} else {
				dbglog_writeCStr("D");
				dbglog_writeHex(z);
			}
		}
	}
	dbglog_writeCStr(", -(A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeCStr(")");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMOVEMApR(void)
{
	/* MOVEM mem to reg 0100110011s011rrr */
	si4b z;
	ui5r regmask;

	regmask = Disasm_nextiword();

	DisasmStartOne("MOVEM");
	if (Disasm_b76 == 2) {
		dbglog_writeCStr(".W");
	} else {
		dbglog_writeCStr(".L");
	}
	dbglog_writeCStr(" (A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeCStr(")+, ");

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
			if (z >= 8) {
				dbglog_writeCStr("A");
				dbglog_writeHex(z - 8);
			} else {
				dbglog_writeCStr("D");
				dbglog_writeHex(z);
			}
		}
	}
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmUnlkA6(void)
{
	DisasmStartOne("UNLINK A6");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmRts(void)
{
	/* Rts 0100111001110101 */
	DisasmStartOne("RTS");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmJmp(void)
{
	/* JMP 0100111011mmmrrr */
	DisasmStartOne("JMP ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmClr(void)
{
	/* Clr 01000010ssmmmrrr */
	Disasm_xxxxxxxxssmmmrrr("CLR");
}

LOCALPROCUSEDONCE DisasmAddA(void)
{
	/* ADDA 1101dddm11mmmrrr */
	DisasmEaA_xxxxdddsxxmmmrrr("ADDA");
}

LOCALPROCUSEDONCE DisasmAddQA(void)
{
	/* 0101nnn0ss001rrr */
	DisasmStartOne("ADDQA #");
	dbglog_writeHex(Disasm_octdat(Disasm_rg9));
	dbglog_writeCStr(", A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmSubQA(void)
{
	/* 0101nnn1ss001rrr */
	DisasmStartOne("SUBQA #");
	dbglog_writeHex(Disasm_octdat(Disasm_rg9));
	dbglog_writeCStr(", A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmSubA(void)
{
	/* SUBA 1001dddm11mmmrrr */
	DisasmEaA_xxxxdddsxxmmmrrr("SUBA");
}

LOCALPROCUSEDONCE DisasmCmpA(void)
{
	DisasmStartOne("CMPA ");
	Disasm_opsize = Disasm_b8 * 2 + 2;
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", A");
	dbglog_writeHex(Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmAddXd(void)
{
	DisasmDD_xxxxdddxssxxxrrr("ADDX");
}

LOCALPROCUSEDONCE DisasmAddXm(void)
{
	DisasmAAs_xxxxdddxssxxxrrr("ADDX");
}

LOCALPROCUSEDONCE DisasmSubXd(void)
{
	DisasmDD_xxxxdddxssxxxrrr("SUBX");
}

LOCALPROCUSEDONCE DisasmSubXm(void)
{
	DisasmAAs_xxxxdddxssxxxrrr("SUBX");
}

LOCALPROC DisasmBinOp1(ui5r x)
{
	if (! Disasm_b8) {
		switch (x) {
			case 0:
				DisasmStartOne("ASR");
				break;
			case 1:
				DisasmStartOne("LSR");
				break;
			case 2:
				DisasmStartOne("RXR");
				break;
			case 3:
				DisasmStartOne("ROR");
				break;
			default:
				/* should not get here */
				break;
		}
	} else {
		switch (x) {
			case 0:
				DisasmStartOne("ASL");
				break;
			case 1:
				DisasmStartOne("LSL");
				break;
			case 2:
				DisasmStartOne("RXL");
				break;
			case 3:
				DisasmStartOne("ROL");
				break;
			default:
				/* should not get here */
				break;
		}
	}
}

LOCALPROCUSEDONCE DisasmRolopNM(void)
{
	DisasmBinOp1(Disasm_rg9);
	dbglog_writeCStr(" ");
	Disasm_opsize = 2;
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmRolopND(void)
{
	/* 1110cccdss0ttddd */
	DisasmBinOp1(Disasm_mode & 3);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" #");
	dbglog_writeHex(Disasm_octdat(Disasm_rg9));
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmRolopDD(void)
{
	/* 1110rrrdss1ttddd */
	DisasmBinOp1(Disasm_mode & 3);
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROC DisasmBinBitOp1(void)
{
	switch (Disasm_b76) {
		case 0:
			DisasmStartOne("BTST");
			break;
		case 1:
			DisasmStartOne("BCHG");
			break;
		case 2:
			DisasmStartOne("BCLR");
			break;
		case 3:
			DisasmStartOne("BSET");
			break;
		default:
			/* should not get here */
			break;
	}
}

LOCALPROCUSEDONCE DisasmBitOpDD(void)
{
	/* dynamic bit, Opcode = 0000ddd1tt000rrr */
	DisasmBinBitOp1();
	Disasm_opsize = 4;
	dbglog_writeCStr(" ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmBitOpDM(void)
{
	/* dynamic bit, Opcode = 0000ddd1ttmmmrrr */
	DisasmBinBitOp1();
	Disasm_opsize = 1;
	dbglog_writeCStr(" ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_mode, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmBitOpND(void)
{
	/* static bit 00001010tt000rrr */
	DisasmBinBitOp1();
	Disasm_opsize = 4;
	dbglog_writeCStr(" #");
	dbglog_writeHex(ui5r_FromSByte(Disasm_nextibyte()));
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmBitOpNM(void)
{
	/* static bit 00001010ttmmmrrr */
	DisasmBinBitOp1();
	Disasm_opsize = 1;
	dbglog_writeCStr(" #");
	dbglog_writeHex(ui5r_FromSByte(Disasm_nextibyte()));
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_mode, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmAndI(void)
{
	DisasmI_xxxxxxxxssmmmrrr("ANDI");
}

LOCALPROCUSEDONCE DisasmAndDEa(void)
{
	/* And 1100ddd1ssmmmrrr */
	DisasmDEa_xxxxdddxssmmmrrr("AND");
}

LOCALPROCUSEDONCE DisasmAndEaD(void)
{
	/* And 1100ddd0ssmmmrrr */
	DisasmEaD_xxxxdddxssmmmrrr("AND");
}

LOCALPROCUSEDONCE DisasmOrI(void)
{
	DisasmI_xxxxxxxxssmmmrrr("ORI");
}

LOCALPROCUSEDONCE DisasmOrDEa(void)
{
	/* OR 1000ddd1ssmmmrrr */
	DisasmDEa_xxxxdddxssmmmrrr("OR");
}

LOCALPROCUSEDONCE DisasmOrEaD(void)
{
	/* OR 1000ddd0ssmmmrrr */
	DisasmEaD_xxxxdddxssmmmrrr("OR");
}

LOCALPROCUSEDONCE DisasmEorI(void)
{
	DisasmI_xxxxxxxxssmmmrrr("EORI");
}

LOCALPROCUSEDONCE DisasmEor(void)
{
	/* Eor 1011ddd1ssmmmrrr */
	DisasmDEa_xxxxdddxssmmmrrr("EOR");
}

LOCALPROCUSEDONCE DisasmNot(void)
{
	/* Not 01000110ssmmmrrr */
	Disasm_xxxxxxxxssmmmrrr("NOT");
}

LOCALPROCUSEDONCE DisasmScc(void)
{
	/* Scc 0101cccc11mmmrrr */
	Disasm_opsize = 1;
	DisasmStartOne("S");
	DisasmCC();
	dbglog_writeCStr(" ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmEXTL(void)
{
	DisasmStartOne("EXT.L D");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmEXTW(void)
{
	DisasmStartOne("EXT.W D");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmNeg(void)
{
	/* Neg 01000100ssmmmrrr */
	Disasm_xxxxxxxxssmmmrrr("NEG");
}

LOCALPROCUSEDONCE DisasmNegX(void)
{
	/* NegX 01000000ssmmmrrr */
	Disasm_xxxxxxxxssmmmrrr("NEGX");
}

LOCALPROCUSEDONCE DisasmMulU(void)
{
	/* MulU 1100ddd011mmmrrr */
	Disasm_opsize = 2;
	DisasmStartOne("MULU ");

	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMulS(void)
{
	/* MulS 1100ddd111mmmrrr */
	Disasm_opsize = 2;
	DisasmStartOne("MULS ");

	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmDivU(void)
{
	/* DivU 1000ddd011mmmrrr */

	Disasm_opsize = 2;
	DisasmStartOne("DIVU ");

	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmDivS(void)
{
	/* DivS 1000ddd111mmmrrr */

	Disasm_opsize = 2;
	DisasmStartOne("DIVS ");

	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmExgdd(void)
{
	/* Exg 1100ddd101000rrr */

	Disasm_opsize = 4;
	DisasmStartOne("EXG ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmExgaa(void)
{
	/* Exg 1100ddd101001rrr */

	Disasm_opsize = 4;
	DisasmStartOne("EXG ");
	DisasmModeRegister(1, Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(1, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmExgda(void)
{
	/* Exg 1100ddd110001rrr */

	Disasm_opsize = 4;
	DisasmStartOne("EXG ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeCStr(", ");
	DisasmModeRegister(1, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveCCREa(void)
{
	/* Move from CCR 0100001011mmmrrr */
	Disasm_opsize = 2;
	DisasmStartOne("MOVE CCR, ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveEaCR(void)
{
	/* 0100010011mmmrrr */
	Disasm_opsize = 2;
	DisasmStartOne("MOVE ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", CCR");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveSREa(void)
{
	/* Move from SR 0100000011mmmrrr */
	Disasm_opsize = 2;
	DisasmStartOne("MOVE SR, ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveEaSR(void)
{
	/* 0100011011mmmrrr */
	Disasm_opsize = 2;
	DisasmStartOne("MOVE ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", SR");
	dbglog_writeReturn();
}

LOCALPROC DisasmBinOpStatusCCR(void)
{
	switch (Disasm_rg9) {
		case 0 :
			DisasmStartOne("OR");
			break;
		case 1 :
			DisasmStartOne("AND");
			break;
		case 5 :
			DisasmStartOne("EOR");
			break;
		default: /* should not happen */
			break;
	}
	DisasmOpSizeFromb76();
	dbglog_writeCStr(" #");
	dbglog_writeHex(ui5r_FromSWord(Disasm_nextiword()));
	if (Disasm_b76 != 0) {
		dbglog_writeCStr(", SR");
	} else {
		dbglog_writeCStr(", CCR");
	}
	dbglog_writeReturn();
}

LOCALPROC disasmreglist(si4b direction, ui5b m1, ui5b r1)
{
	si4b z;
	ui5r regmask;

	DisasmStartOne("MOVEM");

	regmask = Disasm_nextiword();
	Disasm_opsize = 2 * Disasm_b76 - 2;

	if (Disasm_opsize == 2) {
		dbglog_writeCStr(".W");
	} else {
		dbglog_writeCStr(".L");
	}

	dbglog_writeCStr(" ");

	if (direction != 0) {
		DisasmModeRegister(m1, r1);
		dbglog_writeCStr(", ");
	}

	for (z = 0; z < 16; ++z) {
		if ((regmask & (1 << z)) != 0) {
			if (z >= 8) {
				dbglog_writeCStr("A");
				dbglog_writeHex(z - 8);
			} else {
				dbglog_writeCStr("D");
				dbglog_writeHex(z);
			}
		}
	}

	if (direction == 0) {
		dbglog_writeCStr(", ");
		DisasmModeRegister(m1, r1);
	}

	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMOVEMrm(void)
{
	/* MOVEM reg to mem 010010001ssmmmrrr */
	disasmreglist(0, Disasm_mode, Disasm_reg);
}

LOCALPROCUSEDONCE DisasmMOVEMmr(void)
{
	/* MOVEM mem to reg 0100110011smmmrrr */
	disasmreglist(1, Disasm_mode, Disasm_reg);
}

LOCALPROC DisasmByteBinOp(char *s, ui5b m1, ui5b r1, ui5b m2, ui5b r2)
{
	DisasmStartOne(s);
	dbglog_writeCStr(" ");
	DisasmOpSizeFromb76();
	DisasmModeRegister(m1, r1);
	dbglog_writeCStr(", ");
	DisasmModeRegister(m2, r2);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmAbcdr(void)
{
	/* ABCD 1100ddd100000rrr */
	DisasmByteBinOp("ABCD", 0, Disasm_reg, 0, Disasm_rg9);
}

LOCALPROCUSEDONCE DisasmAbcdm(void)
{
	/* ABCD 1100ddd100001rrr */
	DisasmByteBinOp("ABCD", 4, Disasm_reg, 4, Disasm_rg9);
}

LOCALPROCUSEDONCE DisasmSbcdr(void)
{
	/* SBCD 1000xxx100000xxx */
	DisasmByteBinOp("ABCD", 0, Disasm_reg, 0, Disasm_rg9);
}

LOCALPROCUSEDONCE DisasmSbcdm(void)
{
	/* SBCD 1000xxx100001xxx */
	DisasmByteBinOp("ABCD", 4, Disasm_reg, 4, Disasm_rg9);
}

LOCALPROCUSEDONCE DisasmNbcd(void)
{
	/* Nbcd 0100100000mmmrrr */
	Disasm_xxxxxxxxssmmmrrr("NBCD");
}

LOCALPROCUSEDONCE DisasmRte(void)
{
	/* Rte 0100111001110011 */
	DisasmStartOne("RTE");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmNop(void)
{
	/* Nop 0100111001110001 */
	DisasmStartOne("NOP");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveP(void)
{
	/* MoveP 0000ddd1mm001aaa */

	DisasmStartOne("MOVEP");
	if (0 == (Disasm_b76 & 1)) {
		Disasm_opsize = 2;
		dbglog_writeCStr(".W");
	} else {
		Disasm_opsize = 4;
		dbglog_writeCStr(".L");
	}
	dbglog_writeCStr(" ");
	if (Disasm_b76 < 2) {
		DisasmModeRegister(5, Disasm_reg);
		dbglog_writeCStr(", ");
		DisasmModeRegister(0, Disasm_rg9);
	} else {
		DisasmModeRegister(0, Disasm_rg9);
		dbglog_writeCStr(", ");
		DisasmModeRegister(5, Disasm_reg);
	}
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmIllegal(void)
{
	DisasmStartOne("ILLEGAL");
	dbglog_writeReturn();
}

LOCALPROC DisasmCheck(void)
{
	DisasmStartOne("CHK");
	if (2 == Disasm_opsize) {
		dbglog_writeCStr(".W");
	} else {
		dbglog_writeCStr(".L");
	}
	dbglog_writeCStr(" ");

	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeCStr(", ");
	DisasmModeRegister(0, Disasm_rg9);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmChkW(void)
{
	/* Chk.W 0100ddd110mmmrrr */
	Disasm_opsize = 2;
	DisasmCheck();
}

LOCALPROCUSEDONCE DisasmTrap(void)
{
	/* Trap 010011100100vvvv */
	DisasmStartOne("TRAP ");
	dbglog_writeHex(Disasm_opcode & 15);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmTrapV(void)
{
	/* TrapV 0100111001110110 */
	DisasmStartOne("TRAPV");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmRtr(void)
{
	/* Rtr 0100111001110111 */
	DisasmStartOne("RTR");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmLink(void)
{
	DisasmStartOne("LINK A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeCStr(", ");
	dbglog_writeHex(Disasm_nextiword());
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmUnlk(void)
{
	DisasmStartOne("UNLINK A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveRUSP(void)
{
	/* MOVE USP 0100111001100aaa */
	DisasmStartOne("MOVE A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeCStr(", USP");
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmMoveUSPR(void)
{
	/* MOVE USP 0100111001101aaa */
	DisasmStartOne("MOVE USP, A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmTas(void)
{
	/* Tas 0100101011mmmrrr */
	Disasm_opsize = 1;
	DisasmStartOne("TAS");
	dbglog_writeCStr(" ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmFLine(void)
{
	DisasmStartOne("$");
	dbglog_writeHex(Disasm_opcode);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmCallMorRtm(void)
{
	DisasmStartOne("CALLM #");
	dbglog_writeHex(Disasm_nextibyte());
	dbglog_writeCStr(", ");
	DisasmModeRegister(Disasm_mode, Disasm_reg);
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmStop(void)
{
	/* Stop 0100111001110010 */
	DisasmStartOne("STOP #");
	dbglog_writeHex(Disasm_nextiword());
	dbglog_writeReturn();
}

LOCALPROCUSEDONCE DisasmReset(void)
{
	/* Reset 0100111001100000 */
	DisasmStartOne("RESET");
	dbglog_writeReturn();
}

#if Use68020
LOCALPROCUSEDONCE DisasmEXTBL(void)
{
	/* EXTB.L */
	DisasmStartOne("EXTB.L D");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmTRAPcc(void)
{
	/* TRAPcc 0101cccc11111sss */

	DisasmStartOne("TRAP");
	DisasmCC();

	switch (Disasm_reg) {
		case 2:
			dbglog_writeCStr(" ");
			dbglog_writeHex(Disasm_nextiword());
			break;
		case 3:
			dbglog_writeCStr(" ");
			dbglog_writeHex(Disasm_nextilong());
			break;
		case 4:
			/* no optional data */
			break;
		default:
			/* illegal format */
			break;
	}

	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmChkL(void)
{
	/* Chk.L 0100ddd100mmmrrr */
	Disasm_opsize = 4;
	DisasmCheck();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmBkpt(void)
{
	/* BKPT 0100100001001rrr */
	DisasmStartOne("BKPT #");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmDivL(void)
{
	/* DIVU 0100110001mmmrrr 0rrr0s0000000rrr */
	/* DIVS 0100110001mmmrrr 0rrr1s0000000rrr */
	Disasm_opsize = 4;
	DisasmStartOne("DIV");

	{
		ui4b extra = Disasm_nextiword();
		ui5b rDr = extra & 7;
		ui5b rDq = (extra >> 12) & 7;

		if (extra & 0x0800) {
			dbglog_writeCStr("S");
		} else {
			dbglog_writeCStr("U");
		}
		if (extra & 0x0400) {
			dbglog_writeCStr("L");
		}
		dbglog_writeCStr(".L ");

		DisasmModeRegister(Disasm_mode, Disasm_reg);

		dbglog_writeCStr(", ");

		if (rDr != rDq) {
			dbglog_writeCStr("D");
			dbglog_writeHex(rDr);
			dbglog_writeCStr(":");
		}
		dbglog_writeCStr("D");
		dbglog_writeHex(rDq);
	}

	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmMulL(void)
{
	/* MULU 0100110000mmmrrr 0rrr0s0000000rrr */
	/* MULS 0100110000mmmrrr 0rrr1s0000000rrr */

	Disasm_opsize = 4;
	DisasmStartOne("MUL");

	{
		ui4b extra = Disasm_nextiword();
		ui5b rhi = extra & 7;
		ui5b rlo = (extra >> 12) & 7;

		if (extra & 0x0800) {
			dbglog_writeCStr("S");
		} else {
			dbglog_writeCStr("U");
		}

		dbglog_writeCStr(".L ");

		DisasmModeRegister(Disasm_mode, Disasm_reg);

		dbglog_writeCStr(", ");

		if (extra & 0x400) {
			dbglog_writeCStr("D");
			dbglog_writeHex(rhi);
			dbglog_writeCStr(":");
		}
		dbglog_writeCStr("D");
		dbglog_writeHex(rlo);
	}

	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmRtd(void)
{
	/* Rtd 0100111001110100 */
	DisasmStartOne("RTD #");
	dbglog_writeHex((si5b)(si4b)Disasm_nextiword());
	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROC DisasmControlReg(ui4r i)
{
	switch (i) {
		case 0x0000:
			dbglog_writeCStr("SFC");
			break;
		case 0x0001:
			dbglog_writeCStr("DFC");
			break;
		case 0x0002:
			dbglog_writeCStr("CACR");
			break;
		case 0x0800:
			dbglog_writeCStr("USP");
			break;
		case 0x0801:
			dbglog_writeCStr("VBR");
			break;
		case 0x0802:
			dbglog_writeCStr("CAAR");
			break;
		case 0x0803:
			dbglog_writeCStr("MSP");
			break;
		case 0x0804:
			dbglog_writeCStr("ISP");
			break;
		default:
			dbglog_writeCStr("???");
			break;
	}
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmMoveC(void)
{
	/* MOVEC 010011100111101m */
	DisasmStartOne("MOVEC ");

	{
		ui4b src = Disasm_nextiword();
		int regno = (src >> 12) & 0x0F;
		switch (Disasm_reg) {
			case 2:
				DisasmControlReg(src & 0x0FFF);
				dbglog_writeCStr(", ");
				if (regno < 8) {
					dbglog_writeCStr("D");
				} else {
					dbglog_writeCStr("A");
				}
				dbglog_writeHex(regno & 7);
				break;
			case 3:
				if (regno < 8) {
					dbglog_writeCStr("D");
				} else {
					dbglog_writeCStr("A");
				}
				dbglog_writeHex(regno & 7);

				dbglog_writeCStr(", ");

				DisasmControlReg(src & 0x0FFF);
				break;
			default:
				/* illegal */
				break;
		}
	}

	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmLinkL(void)
{
	/* Link.L 0100100000001rrr */
	DisasmStartOne("LINK.L A");
	dbglog_writeHex(Disasm_reg);
	dbglog_writeCStr(", ");
	dbglog_writeHex(Disasm_nextilong());
	dbglog_writeReturn();
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmPack(void)
{
	DisasmStartOne("PACK ???");
	dbglog_writeReturn();
	/* DoCodePack */
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmUnpk(void)
{
	DisasmStartOne("UNPK ???");
	dbglog_writeReturn();
	/* DoCodeUnpk */
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmCHK2orCMP2(void)
{
	DisasmStartOne("CHK2/CMP2 ???");
	dbglog_writeReturn();
	/* DoCHK2orCMP2 */
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmCAS2(void)
{
	DisasmStartOne("CAS2 ???");
	dbglog_writeReturn();
	/* DoCAS2 */
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmCAS(void)
{
	DisasmStartOne("CAS ???");
	dbglog_writeReturn();
	/* DoDoCAS */
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmMOVES(void)
{
	DisasmStartOne("MOVES ???");
	dbglog_writeReturn();
	/* DoMOVES */
}
#endif

#if Use68020
LOCALPROCUSEDONCE DisasmBitField(void)
{
	DisasmStartOne("BitField ???");
	dbglog_writeReturn();
	/* DoBitField */
}
#endif

LOCALFUNC blnr IsValidAddrMode(void)
{
	return (Disasm_mode != 7) || (Disasm_reg < 5);
}

LOCALFUNC blnr IsValidDstAddrMode(void)
{
	return (Disasm_md6 != 7) || (Disasm_rg9 < 2);
}

LOCALFUNC blnr IsValidDataAltAddrMode(void)
{
	blnr IsOk;

	switch (Disasm_mode) {
		case 1:
		default: /* keep compiler happy */
			IsOk = falseblnr;
			break;
		case 0:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			IsOk = trueblnr;
			break;
		case 7:
			IsOk = Disasm_reg < 2;
			break;
	}

	return IsOk;
}

LOCALFUNC blnr IsValidDataAddrMode(void)
{
	blnr IsOk;

	switch (Disasm_mode) {
		case 1:
		default: /* keep compiler happy */
			IsOk = falseblnr;
			break;
		case 0:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			IsOk = trueblnr;
			break;
		case 7:
			IsOk = Disasm_reg < 5;
			break;
	}

	return IsOk;
}

LOCALFUNC blnr IsValidControlAddrMode(void)
{
	blnr IsOk;

	switch (Disasm_mode) {
		case 0:
		case 1:
		case 3:
		case 4:
		default: /* keep compiler happy */
			IsOk = falseblnr;
			break;
		case 2:
		case 5:
		case 6:
			IsOk = trueblnr;
			break;
		case 7:
			IsOk = Disasm_reg < 4;
			break;
	}

	return IsOk;
}

LOCALFUNC blnr IsValidControlAltAddrMode(void)
{
	blnr IsOk;

	switch (Disasm_mode) {
		case 0:
		case 1:
		case 3:
		case 4:
		default: /* keep compiler happy */
			IsOk = falseblnr;
			break;
		case 2:
		case 5:
		case 6:
			IsOk = trueblnr;
			break;
		case 7:
			IsOk = Disasm_reg < 2;
			break;
	}

	return IsOk;
}

LOCALFUNC blnr IsValidAltMemAddrMode(void)
{
	blnr IsOk;

	switch (Disasm_mode) {
		case 0:
		case 1:
		default: /* keep compiler happy */
			IsOk = falseblnr;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
			IsOk = trueblnr;
			break;
		case 7:
			IsOk = Disasm_reg < 2;
			break;
	}

	return IsOk;
}

LOCALPROCUSEDONCE DisasmCode0(void)
{
	if (Disasm_b8 == 1) {
		if (Disasm_mode == 1) {
			/* MoveP 0000ddd1mm001aaa */
			DisasmMoveP();
		} else {
			/* dynamic bit, Opcode = 0000ddd1ttmmmrrr */
			if (Disasm_mode == 0) {
				DisasmBitOpDD();
			} else {
				if (Disasm_b76 == 0) {
					if (IsValidDataAddrMode()) {
						DisasmBitOpDM();
					} else {
						DisasmIllegal();
					}
				} else {
					if (IsValidDataAltAddrMode()) {
						DisasmBitOpDM();
					} else {
						DisasmIllegal();
					}
				}
			}
		}
	} else {
		if (Disasm_rg9 == 4) {
			/* static bit 00001010ssmmmrrr */
			if (Disasm_mode == 0) {
				DisasmBitOpND();
			} else {
				if (Disasm_b76 == 0) {
					if ((Disasm_mode == 7) && (Disasm_reg == 4)) {
						DisasmIllegal();
					} else {
						if (IsValidDataAddrMode()) {
							DisasmBitOpNM();
						} else {
							DisasmIllegal();
						}
					}
				} else {
					if (IsValidDataAltAddrMode()) {
						DisasmBitOpNM();
					} else {
						DisasmIllegal();
					}
				}
			}
		} else
		if (Disasm_b76 == 3) {
#if Use68020
			if (Disasm_rg9 < 3) {
				/* CHK2 or CMP2 00000ss011mmmrrr */
				if (IsValidControlAddrMode()) {
					DisasmCHK2orCMP2();
				} else {
					DisasmIllegal();
				}
			} else
			if (Disasm_rg9 >= 5) {
				if ((Disasm_mode == 7) && (Disasm_reg == 4)) {
					/* CAS2 00001ss011111100 */
					DisasmCAS2();
				} else {
					/* CAS 00001ss011mmmrrr */
					DisasmCAS2();
				}
			} else
			if (Disasm_rg9 == 3) {
				/* CALLM or RTM 0000011011mmmrrr */
				DisasmCallMorRtm();
			} else
#endif
			{
				DisasmIllegal();
			}
		} else
		if (Disasm_rg9 == 6) {
			/* CMPI 00001100ssmmmrrr */
			if (IsValidDataAltAddrMode()) {
				DisasmCmpI();
			} else {
				DisasmIllegal();
			}
		} else if (Disasm_rg9 == 7) {
#if Use68020
			/* MoveS 00001110ssmmmrrr */
			if (IsValidAltMemAddrMode()) {
				DisasmMoveSREa();
			} else {
				DisasmIllegal();
			}
#else
			DisasmIllegal();
#endif
		} else {
			if ((Disasm_mode == 7) && (Disasm_reg == 4)) {
				switch (Disasm_rg9) {
					case 0:
					case 1:
					case 5:
						DisasmBinOpStatusCCR();
						break;
					default:
						DisasmIllegal();
						break;
				}
			} else {
				if (! IsValidDataAltAddrMode()) {
					DisasmIllegal();
				} else {
					switch (Disasm_rg9) {
						case 0:
							DisasmOrI();
							break;
						case 1:
							DisasmAndI();
							break;
						case 2:
							DisasmSubI();
							break;
						case 3:
							DisasmAddI();
							break;
						case 5:
							DisasmEorI();
							break;
						default:
							/*
								for compiler.
								should be 0, 1, 2, 3, or 5
							*/
							DisasmIllegal();
							break;
					}
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DisasmCode1(void)
{
	if ((Disasm_mode == 1) || ! IsValidAddrMode()) {
		DisasmIllegal();
	} else if (Disasm_md6 == 1) { /* MOVEA */
		DisasmIllegal();
	} else if (! IsValidDstAddrMode()) {
		DisasmIllegal();
	} else {
		DisasmMoveB();
	}
}

LOCALPROCUSEDONCE DisasmCode2(void)
{
	if (Disasm_md6 == 1) { /* MOVEA */
		if (IsValidAddrMode()) {
			DisasmMoveAL();
		} else {
			DisasmIllegal();
		}
	} else if (! IsValidAddrMode()) {
		DisasmIllegal();
	} else if (! IsValidDstAddrMode()) {
		DisasmIllegal();
	} else {
		DisasmMoveL();
	}
}

LOCALPROCUSEDONCE DisasmCode3(void)
{
	if (Disasm_md6 == 1) { /* MOVEA */
		if (IsValidAddrMode()) {
			DisasmMoveAW();
		} else {
			DisasmIllegal();
		}
	} else if (! IsValidAddrMode()) {
		DisasmIllegal();
	} else if (! IsValidDstAddrMode()) {
		DisasmIllegal();
	} else {
		DisasmMoveW();
	}
}

LOCALPROCUSEDONCE DisasmCode4(void)
{
	if (Disasm_b8 != 0) {
		switch (Disasm_b76) {
			case 0:
#if Use68020
				/* Chk.L 0100ddd100mmmrrr */
				if (IsValidDataAddrMode()) {
					DisasmChkL();
				} else {
					DisasmIllegal();
				}
#else
				DisasmIllegal();
#endif
				break;
			case 1:
				DisasmIllegal();
				break;
			case 2:
				/* Chk.W 0100ddd110mmmrrr */
				if (IsValidDataAddrMode()) {
					DisasmChkW();
				} else {
					DisasmIllegal();
				}
				break;
			case 3:
			default: /* keep compiler happy */
#if Use68020
				if ((0 == Disasm_mode) && (4 == Disasm_rg9)) {
					DisasmEXTBL();
				} else
#endif
				{
					/* Lea 0100aaa111mmmrrr */
					if (IsValidControlAddrMode()) {
						DisasmLea();
					} else {
						DisasmIllegal();
					}
				}
				break;
		}
	} else {
		switch (Disasm_rg9) {
			case 0:
				if (Disasm_b76 != 3) {
					/* NegX 01000000ssmmmrrr */
					if (IsValidDataAltAddrMode()) {
						DisasmNegX();
					} else {
						DisasmIllegal();
					}
				} else {
#if Use68020
/* reference seems incorrect to say not for 68000 */
#endif
					/* Move from SR 0100000011mmmrrr */
					if (IsValidDataAltAddrMode()) {
						DisasmMoveSREa();
					} else {
						DisasmIllegal();
					}
				}
				break;
			case 1:
				if (Disasm_b76 != 3) {
					/* Clr 01000010ssmmmrrr */
					if (IsValidDataAltAddrMode()) {
						DisasmClr();
					} else {
						DisasmIllegal();
					}
				} else {
#if Use68020
					/* Move from CCR 0100001011mmmrrr */
					if (IsValidDataAltAddrMode()) {
						DisasmMoveCCREa();
					} else {
						DisasmIllegal();
					}
#else
					DisasmIllegal();
#endif
				}
				break;
			case 2:
				if (Disasm_b76 != 3) {
					/* Neg 01000100ssmmmrrr */
					if (IsValidDataAltAddrMode()) {
						DisasmNeg();
					} else {
						DisasmIllegal();
					}
				} else {
					/* Move to CCR 0100010011mmmrrr */
					if (IsValidDataAddrMode()) {
						DisasmMoveEaCR();
					} else {
						DisasmIllegal();
					}
				}
				break;
			case 3:
				if (Disasm_b76 != 3) {
					/* Not 01000110ssmmmrrr */
					if (IsValidDataAltAddrMode()) {
						DisasmNot();
					} else {
						DisasmIllegal();
					}
				} else {
					/* Move from SR 0100011011mmmrrr */
					if (IsValidDataAddrMode()) {
						DisasmMoveEaSR();
					} else {
						DisasmIllegal();
					}
				}
				break;
			case 4:
				switch (Disasm_b76) {
					case 0:
#if Use68020
						if (Disasm_mode == 1) {
							/* Link.L 0100100000001rrr */
							DisasmLinkL();
						} else
#endif
						{
							/* Nbcd 0100100000mmmrrr */
							if (IsValidDataAltAddrMode()) {
								DisasmNbcd();
							} else {
								DisasmIllegal();
							}
						}
						break;
					case 1:
						if (Disasm_mode == 0) {
							/* Swap 0100100001000rrr */
							DisasmSwap();
						} else
#if Use68020
						if (Disasm_mode == 1) {
							DisasmBkpt();
						} else
#endif
						{
							/* PEA 0100100001mmmrrr */
							if (IsValidControlAddrMode()) {
								DisasmPEA();
							} else {
								DisasmIllegal();
							}
						}
						break;
					case 2:
						if (Disasm_mode == 0) {
							/* EXT.W */
							DisasmEXTW();
						} else {
							/*
								MOVEM Disasm_reg
									to mem 01001d001ssmmmrrr
							*/
							if (Disasm_mode == 4) {
								DisasmMOVEMRmM();
							} else {
								if (IsValidControlAltAddrMode()) {
									DisasmMOVEMrm();
								} else {
									DisasmIllegal();
								}
							}
						}
						break;
					case 3:
					default: /* keep compiler happy */
						if (Disasm_mode == 0) {
							/* EXT.L */
							DisasmEXTL();
						} else {
							/*
								MOVEM Disasm_reg
									to mem 01001d001ssmmmrrr
							*/
							if (Disasm_mode == 4) {
								DisasmMOVEMRmM();
							} else {
								if (IsValidControlAltAddrMode()) {
									DisasmMOVEMrm();
								} else {
									DisasmIllegal();
								}
							}
						}
						break;
				}
				break;
			case 5:
				if (Disasm_b76 == 3) {
					if ((Disasm_mode == 7) && (Disasm_reg == 4)) {
						/* the ILLEGAL instruction */
						DisasmIllegal();
					} else {
						/* Tas 0100101011mmmrrr */
						if (IsValidDataAltAddrMode()) {
							DisasmTas();
						} else {
							DisasmIllegal();
						}
					}
				} else {
					/* Tst 01001010ssmmmrrr */
					if (Disasm_b76 == 0) {
						if (IsValidDataAltAddrMode()) {
							DisasmTst();
						} else {
							DisasmIllegal();
						}
					} else {
						if (IsValidAddrMode()) {
							DisasmTst();
						} else {
							DisasmIllegal();
						}
					}
				}
				break;
			case 6:
				if (((Disasm_opcode >> 7) & 1) == 1) {
					/* MOVEM mem to Disasm_reg 0100110011smmmrrr */
					if (Disasm_mode == 3) {
						DisasmMOVEMApR();
					} else {
						if (IsValidControlAddrMode()) {
							DisasmMOVEMmr();
						} else {
							DisasmIllegal();
						}
					}
				} else {
#if Use68020
					if (((Disasm_opcode >> 6) & 1) == 1) {
						/* DIVU 0100110001mmmrrr 0rrr0s0000000rrr */
						/* DIVS 0100110001mmmrrr 0rrr1s0000000rrr */
						DisasmDivL();
					} else {
						/* MULU 0100110000mmmrrr 0rrr0s0000000rrr */
						/* MULS 0100110000mmmrrr 0rrr1s0000000rrr */
						DisasmMulL();
					}
#else
					DisasmIllegal();
#endif
				}
				break;
			case 7:
			default: /* keep compiler happy */
				switch (Disasm_b76) {
					case 0:
						DisasmIllegal();
						break;
					case 1:
						switch (Disasm_mode) {
							case 0:
							case 1:
								/* Trap 010011100100vvvv */
								DisasmTrap();
								break;
							case 2:
								/* Link */
								if (Disasm_reg == 6) {
									DisasmLinkA6();
								} else {
									DisasmLink();
								}
								break;
							case 3:
								/* Unlk */
								if (Disasm_reg == 6) {
									DisasmUnlkA6();
								} else {
									DisasmUnlk();
								}
								break;
							case 4:
								/* MOVE USP 0100111001100aaa */
								DisasmMoveRUSP();
								break;
							case 5:
								/* MOVE USP 0100111001101aaa */
								DisasmMoveUSPR();
								break;
							case 6:
								switch (Disasm_reg) {
									case 0:
										/* Reset 0100111001100000 */
										DisasmReset();
										break;
									case 1:
										/*
											Nop Opcode
												= 0100111001110001
										*/
										DisasmNop();
										break;
									case 2:
										/* Stop 0100111001110010 */
										DisasmStop();
										break;
									case 3:
										/* Rte 0100111001110011 */
										DisasmRte();
										break;
									case 4:
										/* Rtd 0100111001110100 */
#if Use68020
										DisasmRtd();
#else
										DisasmIllegal();
#endif
										break;
									case 5:
										/* Rts 0100111001110101 */
										DisasmRts();
										break;
									case 6:
										/* TrapV 0100111001110110 */
										DisasmTrapV();
										break;
									case 7:
									default: /* keep compiler happy */
										/* Rtr 0100111001110111 */
										DisasmRtr();
										break;
								}
								break;
							case 7:
							default: /* keep compiler happy */
#if Use68020
								/* MOVEC 010011100111101m */
								DisasmMoveC();
#else
								DisasmIllegal();
#endif
								break;
						}
						break;
					case 2:
						/* Jsr 0100111010mmmrrr */
						if (IsValidControlAddrMode()) {
							DisasmJsr();
						} else {
							DisasmIllegal();
						}
						break;
					case 3:
					default: /* keep compiler happy */
						/* JMP 0100111011mmmrrr */
						if (IsValidControlAddrMode()) {
							DisasmJmp();
						} else {
							DisasmIllegal();
						}
						break;
				}
				break;
		}
	}
}

LOCALPROCUSEDONCE DisasmCode5(void)
{
	if (Disasm_b76 == 3) {
		if (Disasm_mode == 1) {
			/* DBcc 0101cccc11001ddd */
			DisasmDBcc();
		} else {
#if Use68020
			if ((Disasm_mode == 7) && (Disasm_reg >= 2)) {
				/* TRAPcc 0101cccc11111sss */
				DisasmTRAPcc();
			} else
#endif
			{
				/* Scc 0101cccc11mmmrrr */
				if (IsValidDataAltAddrMode()) {
					DisasmScc();
				} else {
					DisasmIllegal();
				}
			}
		}
	} else {
		if (Disasm_mode == 1) {
			if (Disasm_b8 == 0) {
				DisasmAddQA(); /* AddQA 0101nnn0ss001rrr */
			} else {
				DisasmSubQA(); /* SubQA 0101nnn1ss001rrr */
			}
		} else {
			if (Disasm_b8 == 0) {
				/* AddQ 0101nnn0ssmmmrrr */
				if (IsValidDataAltAddrMode()) {
					DisasmAddQ();
				} else {
					DisasmIllegal();
				}
			} else {
				/* SubQ 0101nnn1ssmmmrrr */
				if (IsValidDataAltAddrMode()) {
					DisasmSubQ();
				} else {
					DisasmIllegal();
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DisasmCode6(void)
{
	ui5b cond = (Disasm_opcode >> 8) & 15;

	if (cond == 1) {
		/* Bsr 01100001nnnnnnnn */
		DisasmBsr();
	} else if (cond == 0) {
		/* Bra 01100000nnnnnnnn */
		DisasmBcc();
	} else {
		/* Bcc 0110ccccnnnnnnnn */
		DisasmBcc();
	}
}

LOCALPROCUSEDONCE DisasmCode7(void)
{
	if (Disasm_b8 == 0) {
		DisasmMoveQ();
	} else {
		DisasmIllegal();
	}
}

LOCALPROCUSEDONCE DisasmCode8(void)
{
	if (Disasm_b76 == 3) {
		if (Disasm_b8 == 0) {
			/* DivU 1000ddd011mmmrrr */
			if (IsValidDataAddrMode()) {
				DisasmDivU();
			} else {
				DisasmIllegal();
			}
		} else {
			/* DivS 1000ddd111mmmrrr */
			if (IsValidDataAddrMode()) {
				DisasmDivS();
			} else {
				DisasmIllegal();
			}
		}
	} else {
		if (Disasm_b8 == 0) {
			/* OR 1000ddd0ssmmmrrr */
			if (IsValidDataAddrMode()) {
				DisasmOrEaD();
			} else {
				DisasmIllegal();
			}
		} else {
			if (Disasm_mode < 2) {
				switch (Disasm_b76) {
					case 0:
						/* SBCD 1000xxx10000mxxx */
						if (Disasm_mode == 0) {
							DisasmSbcdr();
						} else {
							DisasmSbcdm();
						}
						break;
#if Use68020
					case 1:
						/* PACK 1000rrr10100mrrr */
						DisasmPack();
						break;
					case 2:
						/* UNPK 1000rrr11000mrrr */
						DisasmUnpk();
						break;
#endif
					default:
						DisasmIllegal();
						break;
				}
			} else {
				/* OR 1000ddd1ssmmmrrr */
				if (IsValidDataAltAddrMode()) {
					DisasmOrDEa();
				} else {
					DisasmIllegal();
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DisasmCode9(void)
{
	if (Disasm_b76 == 3) {
		/* SUBA 1001dddm11mmmrrr */
		if (IsValidAddrMode()) {
			DisasmSubA();
		} else {
			DisasmIllegal();
		}
	} else {
		if (Disasm_b8 == 0) {
			/* SUB 1001ddd0ssmmmrrr */
			if (IsValidAddrMode()) {
				DisasmSubEaR();
			} else {
				DisasmIllegal();
			}
		} else {
			if (Disasm_mode == 0) {
				/* SUBX 1001ddd1ss000rrr */
				DisasmSubXd();
			} else if (Disasm_mode == 1) {
				/* SUBX 1001ddd1ss001rrr */
				DisasmSubXm();
			} else {
				/* SUB 1001ddd1ssmmmrrr */
				if (IsValidAltMemAddrMode()) {
					DisasmSubREa();
				} else {
					DisasmIllegal();
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DisasmCodeA(void)
{
	DisasmALine();
}

LOCALPROCUSEDONCE DisasmCodeB(void)
{
	if (Disasm_b76 == 3) {
		/* CMPA 1011ddds11mmmrrr */
		if (IsValidAddrMode()) {
			DisasmCmpA();
		} else {
			DisasmIllegal();
		}
	} else if (Disasm_b8 == 1) {
		if (Disasm_mode == 1) {
			/* CmpM 1011ddd1ss001rrr */
			DisasmCmpM();
		} else {
			/* Eor 1011ddd1ssmmmrrr */
			if (IsValidDataAltAddrMode()) {
				DisasmEor();
			} else {
				DisasmIllegal();
			}
		}
	} else {
		/* Cmp 1011ddd0ssmmmrrr */
		if (IsValidAddrMode()) {
			DisasmCompare();
		} else {
			DisasmIllegal();
		}
	}
}

LOCALPROCUSEDONCE DisasmCodeC(void)
{
	if (Disasm_b76 == 3) {
		if (Disasm_b8 == 0) {
			/* MulU 1100ddd011mmmrrr */
			if (IsValidDataAddrMode()) {
				DisasmMulU();
			} else {
				DisasmIllegal();
			}
		} else {
			/* MulS 1100ddd111mmmrrr */
			if (IsValidDataAddrMode()) {
				DisasmMulS();
			} else {
				DisasmIllegal();
			}
		}
	} else {
		if (Disasm_b8 == 0) {
			/* And 1100ddd0ssmmmrrr */
			if (IsValidDataAddrMode()) {
				DisasmAndEaD();
			} else {
				DisasmIllegal();
			}
		} else {
			if (Disasm_mode < 2) {
				switch (Disasm_b76) {
					case 0:
						/* ABCD 1100ddd10000mrrr */
						if (Disasm_mode == 0) {
							DisasmAbcdr();
						} else {
							DisasmAbcdm();
						}
						break;
					case 1:
						/* Exg 1100ddd10100trrr */
						if (Disasm_mode == 0) {
							DisasmExgdd();
						} else {
							DisasmExgaa();
						}
						break;
					case 2:
					default: /* keep compiler happy */
						if (Disasm_mode == 0) {
							DisasmIllegal();
						} else {
							/* Exg 1100ddd110001rrr */
							DisasmExgda();
						}
						break;
				}
			} else {
				/* And 1100ddd1ssmmmrrr */
				if (IsValidAltMemAddrMode()) {
					DisasmAndDEa();
				} else {
					DisasmIllegal();
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DisasmCodeD(void)
{
	if (Disasm_b76 == 3) {
		/* ADDA 1101dddm11mmmrrr */
		if (IsValidAddrMode()) {
			DisasmAddA();
		} else {
			DisasmIllegal();
		}
	} else {
		if (Disasm_b8 == 0) {
			/* ADD 1101ddd0ssmmmrrr */
			if (IsValidAddrMode()) {
				DisasmAddEaR();
			} else {
				DisasmIllegal();
			}
		} else {
			if (Disasm_mode == 0) {
				DisasmAddXd();
			} else if (Disasm_mode == 1) {
				DisasmAddXm();
			} else {
				/* ADD 1101ddd1ssmmmrrr */
				if (IsValidAltMemAddrMode()) {
					DisasmAddREa();
				} else {
					DisasmIllegal();
				}
			}
		}
	}
}

LOCALPROCUSEDONCE DisasmCodeE(void)
{
	if (Disasm_b76 == 3) {
		if ((Disasm_opcode & 0x0800) != 0) {
#if Use68020
			/* 11101???11mmmrrr */
			switch (Disasm_mode) {
				case 1:
				case 3:
				case 4:
				default: /* keep compiler happy */
					DisasmIllegal();
					break;
				case 0:
				case 2:
				case 5:
				case 6:
					DisasmBitField();
					break;
				case 7:
					switch (Disasm_reg) {
						case 0:
						case 1:
							DisasmBitField();
							break;
						case 2:
						case 3:
							switch ((Disasm_opcode >> 8) & 7) {
								case 0: /* BFTST */
								case 1: /* BFEXTU */
								case 3: /* BFEXTS */
								case 5: /* BFFFO */
									DisasmBitField();
									break;
								default:
									DisasmIllegal();
									break;
							}
							break;
						default:
							DisasmIllegal();
							break;
					}
					break;
			}
#else
			DisasmIllegal();
#endif
		} else {
			/* 11100ttd11mmmddd */
			if (IsValidAltMemAddrMode()) {
				DisasmRolopNM();
			} else {
				DisasmIllegal();
			}
		}
	} else {
		if (Disasm_mode < 4) {
			/* 1110cccdss0ttddd */
			DisasmRolopND();
		} else {
			/* 1110rrrdss1ttddd */
			DisasmRolopDD();
		}
	}
}

LOCALPROCUSEDONCE DisasmCodeF(void)
{
	DisasmFLine();
}

LOCALPROC m68k_Disasm_one(void)
{
	Disasm_opcode = Disasm_nextiword();

	switch (Disasm_opcode >> 12) {
		case 0x0:
			DisasmCode0();
			break;
		case 0x1:
			DisasmCode1();
			break;
		case 0x2:
			DisasmCode2();
			break;
		case 0x3:
			DisasmCode3();
			break;
		case 0x4:
			DisasmCode4();
			break;
		case 0x5:
			DisasmCode5();
			break;
		case 0x6:
			DisasmCode6();
			break;
		case 0x7:
			DisasmCode7();
			break;
		case 0x8:
			DisasmCode8();
			break;
		case 0x9:
			DisasmCode9();
			break;
		case 0xA:
			DisasmCodeA();
			break;
		case 0xB:
			DisasmCodeB();
			break;
		case 0xC:
			DisasmCodeC();
			break;
		case 0xD:
			DisasmCodeD();
			break;
		case 0xE:
			DisasmCodeE();
			break;
		case 0xF:
		default: /* keep compiler happy */
			DisasmCodeF();
			break;
	}
}

#define Ln2SavedPCs 4
#define NumSavedPCs (1 << Ln2SavedPCs)
#define SavedPCsMask (NumSavedPCs - 1)
LOCALVAR ui5r SavedPCs[NumSavedPCs];
LOCALVAR ui5r SavedPCsIn = 0;
LOCALVAR ui5r SavedPCsOut = 0;

#define DisasmIncludeCycles 0

LOCALPROCUSEDONCE DisasmOneAndBack(ui5r pc)
{
#if DisasmIncludeCycles
	dbglog_writeHex(GetCuriCount());
	dbglog_writeCStr(" ");
#endif
	dbglog_writeHex(pc);
	dbglog_writeCStr("  ");
	Disasm_setpc(pc);
	m68k_Disasm_one();
}

LOCALPROCUSEDONCE DisasmSavedPCs(void)
{
	ui5r n = SavedPCsIn - SavedPCsOut;

	if (n != 0) {
		ui5r pc;
#if DisasmIncludeCycles
		ui5r i;
#endif
#if 0
		blnr Skipped = falseblnr;
#endif
		ui5r j = SavedPCsOut;

		SavedPCsOut = SavedPCsIn;
			/*
				do first, prevent recursion
				in case of error while disassembling.
				(i.e. failure to read emulated memory.)
			*/

#if DisasmIncludeCycles
		i = GetCuriCount();
#endif

		if (n > NumSavedPCs) {
			n = NumSavedPCs;
			j = SavedPCsIn - NumSavedPCs;
			dbglog_writeReturn();
#if 0
			Skipped = trueblnr;
#endif
		}

		do {
			--n;
			pc = SavedPCs[j & SavedPCsMask];
#if DisasmIncludeCycles
			dbglog_writeHex(i /* - n */);
			dbglog_writeCStr("-? ");
#endif
			dbglog_writeHex(pc);
			dbglog_writeCStr("  ");
			Disasm_setpc(pc);
			m68k_Disasm_one();
			++j;
		} while (n != 0);

#if 0
		if (Skipped) {
			si4b z;

			for (z = 0; z < 16; ++z) {
				if (z >= 8) {
					dbglog_writeCStr(" A");
					dbglog_writeHex(z - 8);
				} else {
					dbglog_writeCStr(" D");
					dbglog_writeHex(z);
				}
				dbglog_writeCStr(" = ");
				dbglog_writeHex(regs.regs[z]);
				dbglog_writeReturn();
			}
		}
#endif
	}
}

LOCALVAR ui5r DisasmCounter = 0;

GLOBALPROC DisasmOneOrSave(ui5r pc)
{
	if (0 != DisasmCounter) {
		DisasmOneAndBack(pc);
		--DisasmCounter;
	} else {
		SavedPCs[SavedPCsIn & SavedPCsMask] = pc;
		++SavedPCsIn;
	}
}

GLOBALPROC m68k_WantDisasmContext(void)
{
	DisasmSavedPCs();
	DisasmCounter = /* 256 */ 128;
}
