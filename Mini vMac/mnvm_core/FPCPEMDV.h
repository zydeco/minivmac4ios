/*
	FPCPEMDV.h

	Copyright (C) 2007 Ross Martin, Paul C. Pratt

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
	Floating Point CoProcessor Emulated Device
	(included by MINEM68K.c)
*/


LOCALVAR struct fpustruct
{
	myfpr fp[8];
	CPTR FPIAR; /* Floating point instruction address register */
} fpu_dat;

LOCALPROC myfp_SetFPIAR(ui5r v)
{
	fpu_dat.FPIAR = v;
}

LOCALFUNC ui5r myfp_GetFPIAR(void)
{
	return fpu_dat.FPIAR;
}

LOCALFUNC blnr MayNotInline DecodeAddrModeRegister(
	ui5b themode, ui5b thereg)
{
	switch (themode) {
		case 2 :
		case 3 :
		case 4 :
		case 5 :
		case 6 :
			DecodeModeRegister(themode, thereg);
			return trueblnr;
			break;
		case 7 :
			switch (thereg) {
				case 0 :
				case 1 :
				case 2 :
				case 3 :
					DecodeModeRegister(themode, thereg);
					return trueblnr;
					break;
				case 4 :
					regs.ArgKind = AKMemory;
					regs.ArgAddr.mem = m68k_getpc();
					if (regs.opsize == 1) {
						++regs.ArgAddr.mem;
					}
					m68k_setpc(regs.ArgAddr.mem + regs.opsize);
					return trueblnr;
					break;
				default :
					return falseblnr;
					break;
			}
			break;
		default :
			return falseblnr;
			break;
	}
}

LOCALPROC read_long_double(ui5r addr, myfpr *r)
{
	ui4r v2;
	ui5r v1;
	ui5r v0;

	v2 = get_word(addr + 0);
	/* ignore word at offset 2 */
	v1 = get_long(addr + 4);
	v0 = get_long(addr + 8);

	myfp_FromExtendedFormat(r, v2, v1, v0);
}

LOCALPROC write_long_double(ui5r addr, myfpr *xx)
{
	ui4r v2;
	ui5r v1;
	ui5r v0;

	myfp_ToExtendedFormat(xx, &v2, &v1, &v0);

	put_word(addr + 0, v2);
	put_word(addr + 2,  0);
	put_long(addr + 4, v1);
	put_long(addr + 8, v0);
}

LOCALPROC read_double(ui5r addr, myfpr *r)
{
	ui5r v1;
	ui5r v0;

	v1 = get_long(addr + 0);
	v0 = get_long(addr + 4);

	myfp_FromDoubleFormat(r, v1, v0);
}

LOCALPROC write_double(ui5r addr, myfpr *dd)
{
	ui5r v1;
	ui5r v0;

	myfp_ToDoubleFormat(dd, &v1, &v0);

	put_long(addr + 0, v1);
	put_long(addr + 4, v0);
}

#if 0
LOCALPROC read_single(ui5r addr, myfpr *r)
{
	myfp_FromSingleFormat(r, get_long(addr));
}

LOCALPROC write_single(ui5r addr, myfpr *ff)
{
	put_long(addr, myfp_ToSingleFormat(ff));
}
#endif


LOCALFUNC int CheckFPCondition(ui4b predicate)
{
	int condition_true = 0;

	ui3r cc = myfp_GetConditionCodeByte();

	int c_nan  = (cc) & 1;
	/* int c_inf  = (cc >> 1) & 1; */
	int c_zero = (cc >> 2) & 1;
	int c_neg  = (cc >> 3) & 1;

	/*
		printf(
			"FPSR Checked: c_nan=%d, c_zero=%d, c_neg=%d,"
			" predicate=0x%04x\n",
			c_nan, c_zero, c_neg, predicate);
	*/

	switch (predicate) {
		case 0x11: /* SEQ */
		case 0x01: /* EQ */
			condition_true = c_zero;
			break;
		case 0x1E: /* SNE */
		case 0x0E: /* NE */
			condition_true = ! c_zero;
			break;
		case 0x02: /* OGT */
		case 0x12: /* GT */
			condition_true = (! c_neg) && (! c_zero) && (! c_nan);
			break;
		case 0x0D: /* ULE */
		case 0x1D: /* NGT */
			condition_true = c_neg || c_zero || c_nan;
			break;
		case 0x03: /* OGE */
		case 0x13: /* GE */
			condition_true = c_zero || ((! c_neg) && (! c_nan));
			break;
		case 0x0C: /* ULT */
		case 0x1C: /* NGE */
			condition_true = c_nan || ((! c_zero) && c_neg) ;
			break;
		case 0x04: /* OLT */
		case 0x14: /* LT */
			condition_true = c_neg && (! c_nan) && (! c_zero);
			break;
		case 0x0B: /* UGE */
		case 0x1B: /* NLT */
			condition_true = c_nan || c_zero || (! c_neg);
			break;
		case 0x05: /* OLE */
		case 0x15: /* LE */
			condition_true = ((! c_nan) && c_neg) || c_zero;
			break;
		case 0x0A: /* UGT */
		case 0x1A: /* NLE */
			condition_true = c_nan || ((! c_neg) && (! c_zero));
			break;
		case 0x06: /* OGL */
		case 0x16: /* GL */
			condition_true = (! c_nan) && (! c_zero);
			break;
		case 0x09: /* UEQ */
		case 0x19: /* NGL */
			condition_true = c_nan || c_zero;
			break;
		case 0x07: /* OR */
		case 0x17: /* GLE */
			condition_true = ! c_nan;
			break;
		case 0x08: /* NGLE */
		case 0x18: /* NGLE */
			condition_true = c_nan;
			break;
		case 0x00: /* SFALSE */
		case 0x10: /* FALSE */
			condition_true = 0;
			break;
		case 0x0F: /* STRUE */
		case 0x1F: /* TRUE */
			condition_true = 1;
			break;
	}

	/* printf("condition_true=%d\n", condition_true); */

	return condition_true;
}

LOCALPROCUSEDONCE DoCodeFPU_Save(void)
{
	if ((regs.opcode == 0xF327) || (regs.opcode == 0xF32D)) {
#if 0
		regs.opsize = 4; /* actually unsized */
		DecodeModeRegister(mode, reg);
		SetArgValue(0); /* for now, try null state frame */
#endif
		/* 28 byte 68881 IDLE frame */

		regs.opsize = 28; /* actually unsized */
		if (! DecodeAddrModeRegister(mode, reg)) {
			ReportAbnormal(
				"DecodeAddrModeRegister fails in DoCodeFPU_Save");
		} else {
			put_long(regs.ArgAddr.mem, 0x1f180000);
			put_long(regs.ArgAddr.mem + 4, 0);
			put_long(regs.ArgAddr.mem + 8, 0);
			put_long(regs.ArgAddr.mem + 12, 0);
			put_long(regs.ArgAddr.mem + 16, 0);
			put_long(regs.ArgAddr.mem + 20, 0);
			put_long(regs.ArgAddr.mem + 24, 0x70000000);
		}

	} else {
		ReportAbnormal("unimplemented FPU Save");
		DoCodeFdefault();
	}
}

LOCALPROCUSEDONCE DoCodeFPU_Restore(void)
{
	if ((regs.opcode == 0xF35F) || (regs.opcode == 0xF36D)) {
		ui5r dstvalue;
		regs.opsize = 4; /* actually unsized */

		if (! DecodeAddrModeRegister(mode, reg)) {
			ReportAbnormal(
				"DecodeAddrModeRegister fails in DoCodeFPU_Restore");
			DoCodeFdefault();
		} else {
			dstvalue = get_long(regs.ArgAddr.mem);
			if (dstvalue != 0) {
				if (0x1f180000 == dstvalue) {
					if (3 == mode) {
						m68k_areg(reg) = regs.ArgAddr.mem + 28;
					}
				} else {
					ReportAbnormal("unknown restore");
						/* not a null state we saved */
					DoCodeFdefault();
				}
			}
		}
	} else {
		ReportAbnormal("unimplemented FPU Restore");
		DoCodeFdefault();
	}
}

LOCALPROC DoCodeFPU_FBcc(blnr IsLong)
{
	/*
		Also get here for a NOP instruction (opcode 0xF280),
		which is simply a FBF.w with offset 0
	*/

	ui4b predicate = regs.opcode & 0x3F;

	int condition_true = CheckFPCondition(predicate);

#if FastRelativeJump
	ui3p s = regs.pc_p;
#else
	ui5r s = m68k_getpc();
#endif


	if (IsLong) {
		s += ui5r_FromSLong(nextilong());
	} else {
		s += ui5r_FromSWord(nextiword());
	}

	if (condition_true) {
#if FastRelativeJump
		regs.pc_p = s;
#else
		m68k_setpc(s);
#endif
	}

	/* printf("pc_p set to 0x%p in FBcc (32bit)\n", regs.pc_p); */
}

LOCALPROCUSEDONCE DoCodeFPU_DBcc(void)
{
	ui4b word2 = (int)nextiword();

	ui4b predicate = word2 & 0x3F;

	int condition_true = CheckFPCondition(predicate);

#if FastRelativeJump
	ui3p srcvalue = regs.pc_p;
#else
	ui5r srcvalue = m68k_getpc();
#endif

	srcvalue += (si5b)(si4b)nextiword();

	if (! condition_true) {
		ui5b fdb_count = ui5r_FromSWord(m68k_dreg(reg)) - 1;
		m68k_dreg(reg) =
			(m68k_dreg(reg) & ~ 0xFFFF) | (fdb_count & 0xFFFF);
		if ((si5b)fdb_count != -1) {
#if FastRelativeJump
			regs.pc_p = srcvalue;
#else
			m68k_setpc(srcvalue);
#endif

			/* printf("pc_p set to 0x%p in DoCodeDBcc\n", regs.pc_p); */
		}
	}
}

LOCALPROCUSEDONCE DoCodeFPU_Trapcc(void)
{
	ui4b word2 = (int)nextiword();

	ui4b predicate = word2 & 0x3F;

	int condition_true = CheckFPCondition(predicate);

	if (reg == 2) {
		(void) nextiword();
	} else if (reg == 3) {
		(void) nextilong();
	} else if (reg == 4) {
	} else {
		ReportAbnormal("Invalid FTRAPcc (?");
	}

	if (condition_true) {
		ReportAbnormal("FTRAPcc trapping");
		Exception(7);
	}
}

LOCALPROCUSEDONCE DoCodeFPU_Scc(void)
{
	ui4b word2 = (int)nextiword();

	ui4b predicate = word2 & 0x3F;

	int condition_true = CheckFPCondition(predicate);

	regs.opsize = 1;
	DecodeModeRegister(mode, reg);
	if (condition_true) {
		SetArgValue(0xFFFF);
	} else {
		SetArgValue(0x0000);
	}
}

LOCALPROC DoCodeF_InvalidPlusWord(void)
{
	BackupPC();
	DoCodeFdefault();
}

LOCALFUNC int CountCSIAlist(ui4b word2)
{
	ui4b regselect = (word2 >> 10) & 0x7;
	int num = 0;

	if (regselect & 1) {
		num++;
	}
	if (regselect & 2) {
		num++;
	}
	if (regselect & 4) {
		num++;
	}

	return num;
}

LOCALPROCUSEDONCE DoCodeFPU_Move_EA_CSIA(ui4b word2)
{
	int n;
	ui5b ea_value[3];
	ui4b regselect = (word2 >> 10) & 0x7;
	int num = CountCSIAlist(word2);

	if (regselect == 0) {
		ReportAbnormal("Invalid FMOVE instruction");
		DoCodeF_InvalidPlusWord();
		return;
	}

	/* FMOVEM.L <EA>, <FP CR,SR,IAR list> */

	regs.opsize = 4 * num;
	DecodeModeRegister(mode, reg);

	regs.opsize = 4;
	ea_value[0] = GetArgValue();
	if (num > 1) {
		ea_value[1] = get_long(regs.ArgAddr.mem + 4);
	}
	if (num > 2) {
		ea_value[2] = get_long(regs.ArgAddr.mem + 8);
	}

	n = 0;
	if (regselect & (1 << 2)) {
		myfp_SetFPCR(ea_value[n++]);
	}
	if (regselect & (1 << 1)) {
		myfp_SetFPSR(ea_value[n++]);
	}
	if (regselect & (1 << 0)) {
		myfp_SetFPIAR(ea_value[n++]);
	}
}

LOCALPROCUSEDONCE DoCodeFPU_MoveM_CSIA_EA(ui4b word2)
{
	int n;
	ui5b ea_value[3];
	int num = CountCSIAlist(word2);

	ui4b regselect = (word2 >> 10) & 0x7;

	if (regselect == 0) {
		ReportAbnormal("Invalid FMOVE instruction");
		DoCodeF_InvalidPlusWord();
		return;
	}

	/* FMOVEM.L <FP CR,SR,IAR list>, <EA> */

	n = 0;
	if (regselect & (1 << 2)) {
		ea_value[n++] = myfp_GetFPCR();
	}
	if (regselect & (1 << 1)) {
		ea_value[n++] = myfp_GetFPSR();
	}
	if (regselect & (1 << 0)) {
		ea_value[n++] = myfp_GetFPIAR();
	}

	regs.opsize = 4 * num;
	DecodeModeRegister(mode, reg);

	SetArgValue(ea_value[0]);
	if (num > 1) {
		put_long(regs.ArgAddr.mem + 4, ea_value[1]);
	}
	if (num > 2) {
		put_long(regs.ArgAddr.mem + 8, ea_value[2]);
	}
}

LOCALPROCUSEDONCE DoCodeFPU_MoveM_EA_list(ui4b word2)
{
	int i;
	ui5r myaddr;
	ui4b register_list = word2;

	ui4b fmove_mode = (word2 >> 11) & 0x3;

	/* FMOVEM.X <ea>, <list> */

	if ((fmove_mode == 0) || (fmove_mode == 1)) {
		ReportAbnormal("Invalid FMOVEM.X instruction");
		DoCodeF_InvalidPlusWord();
		return;
	}

	if (fmove_mode == 3) {
		/* Dynamic mode */
		register_list = regs.regs[(word2 >> 4) & 7];
	}

	if (mode == 3) {
		/* Postincrement mode */

		for (i = 0; i <= 7; i++) {
			int j = 1 << (7 - i);
			if (j & register_list) {
				regs.opsize = 12;
				DecodeModeRegister(mode, reg);
				read_long_double(regs.ArgAddr.mem, &fpu_dat.fp[i]);
			}
		}
	} else {
		/* Control mode */

		regs.opsize = 12;
		DecodeModeRegister(mode, reg);

		myaddr = regs.ArgAddr.mem;

		for (i = 0; i <= 7; i++) {
			int j = 1 << (7 - i);
			if (j & register_list) {
				read_long_double(myaddr, &fpu_dat.fp[i]);
				myaddr += 12;
			}
		}
	}
}

LOCALPROCUSEDONCE DoCodeFPU_MoveM_list_EA(ui4b word2)
{
	/* FMOVEM.X <list>, <ea> */

	int i;
	ui5r myaddr;
	ui4b register_list = word2;

	ui4b fmove_mode = (word2 >> 11) & 0x3;

	if ((fmove_mode == 1) || (fmove_mode == 3)) {
		/* Dynamic mode */
		register_list = regs.regs[(word2 >> 4) & 7];
	}

	if (mode == 4) {
		/* Predecrement mode */

		for (i = 7; i >= 0; i--) {
			int j = 1 << i;
			if (j & register_list) {
				regs.opsize = 12;
				DecodeModeRegister(mode, reg);
				write_long_double(regs.ArgAddr.mem, &fpu_dat.fp[i]);
			}
		}
	} else {
		/* Control mode */

		regs.opsize = 12;
		DecodeModeRegister(mode, reg);

		myaddr = regs.ArgAddr.mem;

		for (i = 0; i <= 7; i++) {
			int j = 1 << (7 - i);
			if (j & register_list) {
				write_long_double(myaddr, &fpu_dat.fp[i]);
				myaddr += 12;
			}
		}
	}
}

LOCALPROCUSEDONCE DoCodeFPU_MoveCR(ui4b word2)
{
	/* FMOVECR */

	if (regs.opcode != 0xF200) {
		ReportAbnormal("bad opcode in FMOVECR");
		DoCodeF_InvalidPlusWord();
	} else {
		ui4b RomOffset = word2 & 0x7F;
		ui4b DestReg = (word2 >> 7) & 0x7;

		if (! myfp_getCR(&fpu_dat.fp[DestReg], RomOffset)) {
			ReportAbnormal("Invalid constant number in FMOVECR");
			DoCodeF_InvalidPlusWord();
		}
	}
}

LOCALPROC SaveResultAndFPSR(myfpr *DestReg, myfpr *result)
{
	*DestReg = *result;
	myfp_SetConditionCodeByteFromResult(result);
}

LOCALPROC DoCodeFPU_GenOp(ui4b word2, myfpr *source)
{
	myfpr result;
	myfpr t0;
	myfpr *DestReg = &fpu_dat.fp[(word2 >> 7) & 0x7];

	switch (word2 & 0x7F) {

		case 0x00: /* FMOVE */
			SaveResultAndFPSR(DestReg, source);
			break;

		case 0x01: /* FINT */
			myfp_Int(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x02: /* FSINH */
			myfp_Sinh(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x03: /* FINTRZ */
			myfp_IntRZ(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x04: /* FSQRT */
			myfp_Sqrt(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x06: /* FLOGNP1 */
			myfp_LogNP1(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x08: /* FETOXM1 */
			myfp_EToXM1(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x09: /* FTANH */
			myfp_Tanh(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x0A: /* FATAN */
			myfp_ATan(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x0C: /* FASIN */
			myfp_ASin(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x0D: /* FATANH */
			myfp_ATanh(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x0E: /* FSIN */
			myfp_Sin(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x0F: /* FTAN */
			myfp_Tan(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x10: /* FETOX */
			myfp_EToX(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x11: /* FTWOTOX */
			myfp_TwoToX(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x12: /* FTENTOX */
			myfp_TenToX(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x14: /* FLOGN */
			myfp_LogN(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x15: /* FLOG10 */
			myfp_Log10(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x16: /* FLOG2 */
			myfp_Log2(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x18: /* FABS */
			myfp_Abs(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x19: /* FCOSH */
			myfp_Cosh(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x1A: /* FNEG */
			myfp_Neg(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x1C: /* FACOS */
			myfp_ACos(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x1D: /* FCOS */
			myfp_Cos(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x1E: /* FGETEXP */
			myfp_GetExp(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x1F: /* FGETMAN */
			myfp_GetMan(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x20: /* FDIV */
			myfp_Div(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x21: /* FMOD */  /* 0x2D in some docs, 0x21 in others ? */
			myfp_Mod(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x22: /* FADD */
			myfp_Add(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x23: /* FMUL */
			myfp_Mul(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x24: /* FSGLDIV */
			myfp_Div(&t0, DestReg, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x25: /* FREM */
			myfp_Rem(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x26: /* FSCALE */
			myfp_Scale(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x27: /* FSGLMUL */
			myfp_Mul(&t0, DestReg, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x28: /* FSUB */
			myfp_Sub(&result, DestReg, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			/* FSINCOS */
			myfp_SinCos(&result, &fpu_dat.fp[word2 & 0x7], source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x38: /* FCMP */
			myfp_Sub(&result, DestReg, source);
			/* don't save result */
			myfp_SetConditionCodeByteFromResult(&result);
			break;

		case 0x3A: /* FTST */
			myfp_SetConditionCodeByteFromResult(source);
			break;

		/*
			everything after here is not in 68881/68882,
			appears first in 68040
		*/

		case 0x40: /* FSMOVE */
			myfp_RoundToSingle(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x41: /* FSSQRT */
			myfp_Sqrt(&t0, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x44: /* FDMOVE */
			myfp_RoundToDouble(&result, source);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x45: /* FDSQRT */
			myfp_Sqrt(&t0, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x58: /* FSABS */
			myfp_Abs(&t0, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x5A: /* FSNEG */
			myfp_Neg(&t0, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x5C: /* FDABS */
			myfp_Abs(&t0, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x5E: /* FDNEG */
			myfp_Neg(&t0, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x60: /* FSDIV */
			myfp_Div(&t0, DestReg, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x62: /* FSADD */
			myfp_Add(&t0, DestReg, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x63: /* FSMUL */
			myfp_Mul(&t0, DestReg, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x64: /* FDDIV */
			myfp_Div(&t0, DestReg, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x66: /* FDADD */
			myfp_Add(&t0, DestReg, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x67: /* FDMUL */
			myfp_Mul(&t0, DestReg, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x68: /* FSSUB */
			myfp_Sub(&t0, DestReg, source);
			myfp_RoundToSingle(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		case 0x6C: /* FDSUB */
			myfp_Sub(&t0, DestReg, source);
			myfp_RoundToDouble(&result, &t0);
			SaveResultAndFPSR(DestReg, &result);
			break;

		default:
			ReportAbnormal("Invalid Floating Point Instruction");
			DoCodeF_InvalidPlusWord();
			break;
	}
}

LOCALPROCUSEDONCE DoCodeFPU_GenOpReg(ui4b word2)
{
	ui4r regselect = (word2 >> 10) & 0x7;

	DoCodeFPU_GenOp(word2, &fpu_dat.fp[regselect]);
}

LOCALPROCUSEDONCE DoCodeFPU_GenOpEA(ui4b word2)
{
	myfpr source;

	switch ((word2 >> 10) & 0x7) {
		case 0: /* long-word integer */
			regs.opsize = 4;
			DecodeModeRegister(mode, reg);
			myfp_FromLong(&source, GetArgValue());
			DoCodeFPU_GenOp(word2, &source);
			break;
		case 1: /* Single-Precision real */
			regs.opsize = 4;
			DecodeModeRegister(mode, reg);
			myfp_FromSingleFormat(&source, GetArgValue());
			DoCodeFPU_GenOp(word2, &source);
			break;
		case 2: /* extended precision real */
			regs.opsize = 12;
			if (! DecodeAddrModeRegister(mode, reg)) {
				ReportAbnormal(
					"DecodeAddrModeRegister fails GetFPSource X");
				DoCodeF_InvalidPlusWord();
			} else {
				read_long_double(regs.ArgAddr.mem, &source);
				DoCodeFPU_GenOp(word2, &source);
			}
			break;
		case 3: /* packed-decimal real */
			ReportAbnormal("Packed Decimal in GetFPSource");
			regs.opsize = 16;
				/* correct? just set to a constant for now */
			if (! DecodeAddrModeRegister(mode, reg)) {
				ReportAbnormal(
					"DecodeAddrModeRegister fails GetFPSource P");
				DoCodeF_InvalidPlusWord();
			} else {
				/* *r = 9123456789.0; */
				DoCodeFPU_GenOp(word2, &source);
			}
			break;
		case 4: /* Word integer */
			regs.opsize = 2;
			DecodeModeRegister(mode, reg);
			myfp_FromLong(&source, GetArgValue());
			DoCodeFPU_GenOp(word2, &source);
			break;
		case 5: /* Double-precision real */
			regs.opsize = 8;
			if (! DecodeAddrModeRegister(mode, reg)) {
				ReportAbnormal(
					"DecodeAddrModeRegister fails GetFPSource D");
				DoCodeF_InvalidPlusWord();
			} else {
				read_double(regs.ArgAddr.mem, &source);
				DoCodeFPU_GenOp(word2, &source);
			}
			break;
		case 6: /* Byte Integer */
			regs.opsize = 1;
			DecodeModeRegister(mode, reg);
			myfp_FromLong(&source, GetArgValue());
			DoCodeFPU_GenOp(word2, &source);
			break;
		case 7: /* Not a valid source specifier */
			DoCodeFPU_MoveCR(word2);
			break;
		default:
			/* should not be able to get here */
			break;
	}
}

LOCALPROCUSEDONCE DoCodeFPU_Move_FP_EA(ui4b word2)
{
	/* FMOVE FP?, <EA> */

	ui4r SourceReg = (word2 >> 7) & 0x7;
	myfpr *source = &fpu_dat.fp[SourceReg];

	switch ((word2 >> 10) & 0x7) {
		case 0: /* long-word integer */
			regs.opsize = 4;
			DecodeModeRegister(mode, reg);
			SetArgValue(myfp_ToLong(source));
			break;
		case 1: /* Single-Precision real */
			regs.opsize = 4;
			DecodeModeRegister(mode, reg);
			SetArgValue(myfp_ToSingleFormat(source));
			break;
		case 2: /* extended precision real */
			regs.opsize = 12;
			if (! DecodeAddrModeRegister(mode, reg)) {
				ReportAbnormal("DecodeAddrModeRegister fails FMOVE X");
				DoCodeF_InvalidPlusWord();
			} else {
				write_long_double(regs.ArgAddr.mem, source);
			}
			break;
		case 3: /* packed-decimal real */
			ReportAbnormal("Packed Decimal in FMOVE");
			regs.opsize = 16; /* correct? don't even write anything */
			if (! DecodeAddrModeRegister(mode, reg)) {
				ReportAbnormal("DecodeAddrModeRegister fails FMOVE P");
				DoCodeF_InvalidPlusWord();
			} else {
				/* ? */
			}
			break;
		case 4: /* Word integer */
			regs.opsize = 2;
			DecodeModeRegister(mode, reg);
			SetArgValue(myfp_ToLong(source));
			break;
		case 5: /* Double-precision real */
			regs.opsize = 8;
			if (! DecodeAddrModeRegister(mode, reg)) {
				ReportAbnormal("DecodeAddrModeRegister fails FMOVE D");
				DoCodeF_InvalidPlusWord();
			} else {
				write_double(regs.ArgAddr.mem, source);
			}
			break;
		case 6: /* Byte Integer */
			regs.opsize = 1;
			DecodeModeRegister(mode, reg);
			SetArgValue(myfp_ToLong(source));
			break;
		default:
			ReportAbnormal("Bad Source Specifier in FMOVE");
			DoCodeF_InvalidPlusWord();
			break;
	}
}

LOCALPROCUSEDONCE DoCodeFPU_md60(void)
{
	ui4b word2 = (int)nextiword();

	switch ((word2 >> 13) & 0x7) {
		case 0:
			DoCodeFPU_GenOpReg(word2);
			break;
		case 2:
			DoCodeFPU_GenOpEA(word2);
			break;
		case 3:
			DoCodeFPU_Move_FP_EA(word2);
			break;
		case 4:
			DoCodeFPU_Move_EA_CSIA(word2);
			break;
		case 5:
			DoCodeFPU_MoveM_CSIA_EA(word2);
			break;
		case 6:
			DoCodeFPU_MoveM_EA_list(word2);
			break;
		case 7:
			DoCodeFPU_MoveM_list_EA(word2);
			break;
		default:
			ReportAbnormal("Invalid Floating Point Instruction");
			DoCodeF_InvalidPlusWord();
			break;
	}
}

LOCALPROCUSEDONCE DoCodeFPU(void)
{
	switch (md6) {
		case 0:
			DoCodeFPU_md60();
			break;
		case 1:
			if (mode == 1) {
				DoCodeFPU_DBcc();
			} else if (mode == 7) {
				DoCodeFPU_Trapcc();
			} else {
				DoCodeFPU_Scc();
			}
			break;
		case 2:
			DoCodeFPU_FBcc(falseblnr);
			break;
		case 3:
			DoCodeFPU_FBcc(trueblnr);
			break;
		case 4:
			DoCodeFPU_Save();
			break;
		case 5:
			DoCodeFPU_Restore();
			break;
		default:
			/* fall through */
			ReportAbnormal("Invalid Floating Point Instruction");
			DoCodeFdefault();
			break;
	}
}
