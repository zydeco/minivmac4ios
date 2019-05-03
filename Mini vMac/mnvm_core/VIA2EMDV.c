/*
	VIA2EMDV.c

	Copyright (C) 2008 Philip Cummins, Paul C. Pratt

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
	Versatile Interface Adapter EMulated DEVice

	Emulates the VIA found in the Mac Plus.

	This code adapted from vMac by Philip Cummins.
*/

#ifndef AllFiles
#include "SYSDEPNS.h"

#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#endif

#include "VIA2EMDV.h"

/*
	ReportAbnormalID unused 0x0510 - 0x05FF
*/

#ifdef VIA2_iA0_ChangeNtfy
IMPORTPROC VIA2_iA0_ChangeNtfy(void);
#endif

#ifdef VIA2_iA1_ChangeNtfy
IMPORTPROC VIA2_iA1_ChangeNtfy(void);
#endif

#ifdef VIA2_iA2_ChangeNtfy
IMPORTPROC VIA2_iA2_ChangeNtfy(void);
#endif

#ifdef VIA2_iA3_ChangeNtfy
IMPORTPROC VIA2_iA3_ChangeNtfy(void);
#endif

#ifdef VIA2_iA4_ChangeNtfy
IMPORTPROC VIA2_iA4_ChangeNtfy(void);
#endif

#ifdef VIA2_iA5_ChangeNtfy
IMPORTPROC VIA2_iA5_ChangeNtfy(void);
#endif

#ifdef VIA2_iA6_ChangeNtfy
IMPORTPROC VIA2_iA6_ChangeNtfy(void);
#endif

#ifdef VIA2_iA7_ChangeNtfy
IMPORTPROC VIA2_iA7_ChangeNtfy(void);
#endif

#ifdef VIA2_iB0_ChangeNtfy
IMPORTPROC VIA2_iB0_ChangeNtfy(void);
#endif

#ifdef VIA2_iB1_ChangeNtfy
IMPORTPROC VIA2_iB1_ChangeNtfy(void);
#endif

#ifdef VIA2_iB2_ChangeNtfy
IMPORTPROC VIA2_iB2_ChangeNtfy(void);
#endif

#ifdef VIA2_iB3_ChangeNtfy
IMPORTPROC VIA2_iB3_ChangeNtfy(void);
#endif

#ifdef VIA2_iB4_ChangeNtfy
IMPORTPROC VIA2_iB4_ChangeNtfy(void);
#endif

#ifdef VIA2_iB5_ChangeNtfy
IMPORTPROC VIA2_iB5_ChangeNtfy(void);
#endif

#ifdef VIA2_iB6_ChangeNtfy
IMPORTPROC VIA2_iB6_ChangeNtfy(void);
#endif

#ifdef VIA2_iB7_ChangeNtfy
IMPORTPROC VIA2_iB7_ChangeNtfy(void);
#endif

#ifdef VIA2_iCB2_ChangeNtfy
IMPORTPROC VIA2_iCB2_ChangeNtfy(void);
#endif

#define Ui3rPowOf2(p) (1 << (p))
#define Ui3rTestBit(i, p) (((i) & Ui3rPowOf2(p)) != 0)

#define VIA2_ORA_CanInOrOut (VIA2_ORA_CanIn | VIA2_ORA_CanOut)

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 7)
#ifdef VIA2_iA7
#error "VIA2_iA7 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 6)
#ifdef VIA2_iA6
#error "VIA2_iA6 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 5)
#ifdef VIA2_iA5
#error "VIA2_iA5 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 4)
#ifdef VIA2_iA4
#error "VIA2_iA4 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 3)
#ifdef VIA2_iA3
#error "VIA2_iA3 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 2)
#ifdef VIA2_iA2
#error "VIA2_iA2 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 1)
#ifdef VIA2_iA1
#error "VIA2_iA1 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORA_CanInOrOut, 0)
#ifdef VIA2_iA0
#error "VIA2_iA0 defined but not used"
#endif
#endif

#define VIA2_ORB_CanInOrOut (VIA2_ORB_CanIn | VIA2_ORB_CanOut)

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 7)
#ifdef VIA2_iB7
#error "VIA2_iB7 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 6)
#ifdef VIA2_iB6
#error "VIA2_iB6 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 5)
#ifdef VIA2_iB5
#error "VIA2_iB5 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 4)
#ifdef VIA2_iB4
#error "VIA2_iB4 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 3)
#ifdef VIA2_iB3
#error "VIA2_iB3 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 2)
#ifdef VIA2_iB2
#error "VIA2_iB2 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 1)
#ifdef VIA2_iB1
#error "VIA2_iB1 defined but not used"
#endif
#endif

#if ! Ui3rTestBit(VIA2_ORB_CanInOrOut, 0)
#ifdef VIA2_iB0
#error "VIA2_iB0 defined but not used"
#endif
#endif

typedef struct {
	ui5b T1C_F;  /* Timer 1 Counter Fixed Point */
	ui5b T2C_F;  /* Timer 2 Counter Fixed Point */
	ui3b ORB;    /* Buffer B */
	/* ui3b ORA_H;     Buffer A with Handshake */
	ui3b DDR_B;  /* Data Direction Register B */
	ui3b DDR_A;  /* Data Direction Register A */
	ui3b T1L_L;  /* Timer 1 Latch Low */
	ui3b T1L_H;  /* Timer 1 Latch High */
	ui3b T2L_L;  /* Timer 2 Latch Low */
	ui3b SR;     /* Shift Register */
	ui3b ACR;    /* Auxiliary Control Register */
	ui3b PCR;    /* Peripheral Control Register */
	ui3b IFR;    /* Interrupt Flag Register */
	ui3b IER;    /* Interrupt Enable Register */
	ui3b ORA;    /* Buffer A */
} VIA2_Ty;

LOCALVAR VIA2_Ty VIA2_D;

#define kIntCA2 0 /* One_Second */
#define kIntCA1 1 /* Vertical_Blanking */
#define kIntSR 2 /* Keyboard_Data_Ready */
#define kIntCB2 3 /* Keyboard_Data */
#define kIntCB1 4 /* Keyboard_Clock */
#define kIntT2 5 /* Timer_2 */
#define kIntT1 6 /* Timer_1 */

#define VIA2_dolog (dbglog_HAVE && 0)

/* VIA2_Get_ORA : VIA Get Port A Data */
/*
	This function queries VIA Port A interfaced hardware
	about their status
*/

LOCALFUNC ui3b VIA2_Get_ORA(ui3b Selection)
{
	ui3b Value = (~ VIA2_ORA_CanIn) & Selection & VIA2_ORA_FloatVal;

#if Ui3rTestBit(VIA2_ORA_CanIn, 7)
	if (Ui3rTestBit(Selection, 7)) {
		Value |= (VIA2_iA7 << 7);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 6)
	if (Ui3rTestBit(Selection, 6)) {
		Value |= (VIA2_iA6 << 6);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 5)
	if (Ui3rTestBit(Selection, 5)) {
		Value |= (VIA2_iA5 << 5);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 4)
	if (Ui3rTestBit(Selection, 4)) {
		Value |= (VIA2_iA4 << 4);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 3)
	if (Ui3rTestBit(Selection, 3)) {
		Value |= (VIA2_iA3 << 3);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 2)
	if (Ui3rTestBit(Selection, 2)) {
		Value |= (VIA2_iA2 << 2);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 1)
	if (Ui3rTestBit(Selection, 1)) {
		Value |= (VIA2_iA1 << 1);
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanIn, 0)
	if (Ui3rTestBit(Selection, 0)) {
		Value |= (VIA2_iA0 << 0);
	}
#endif

	return Value;
}

/* VIA2_Get_ORB : VIA Get Port B Data */
/*
	This function queries VIA Port B interfaced hardware
	about their status
*/

LOCALFUNC ui3b VIA2_Get_ORB(ui3b Selection)
{
	ui3b Value = (~ VIA2_ORB_CanIn) & Selection & VIA2_ORB_FloatVal;

#if Ui3rTestBit(VIA2_ORB_CanIn, 7)
	if (Ui3rTestBit(Selection, 7)) {
		Value |= (VIA2_iB7 << 7);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 6)
	if (Ui3rTestBit(Selection, 6)) {
		Value |= (VIA2_iB6 << 6);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 5)
	if (Ui3rTestBit(Selection, 5)) {
		Value |= (VIA2_iB5 << 5);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 4)
	if (Ui3rTestBit(Selection, 4)) {
		Value |= (VIA2_iB4 << 4);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 3)
	if (Ui3rTestBit(Selection, 3)) {
		Value |= (VIA2_iB3 << 3);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 2)
	if (Ui3rTestBit(Selection, 2)) {
		Value |= (VIA2_iB2 << 2);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 1)
	if (Ui3rTestBit(Selection, 1)) {
		Value |= (VIA2_iB1 << 1);
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanIn, 0)
	if (Ui3rTestBit(Selection, 0)) {
		Value |= (VIA2_iB0 << 0);
	}
#endif

	return Value;
}

#define ViaORcheckBit(p, x) \
	(Ui3rTestBit(Selection, p) && \
	((v = (Data >> p) & 1) != x))

LOCALPROC VIA2_Put_ORA(ui3b Selection, ui3b Data)
{
#if 0 != VIA2_ORA_CanOut
	ui3b v;
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 7)
	if (ViaORcheckBit(7, VIA2_iA7)) {
		VIA2_iA7 = v;
#ifdef VIA2_iA7_ChangeNtfy
		VIA2_iA7_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 6)
	if (ViaORcheckBit(6, VIA2_iA6)) {
		VIA2_iA6 = v;
#ifdef VIA2_iA6_ChangeNtfy
		VIA2_iA6_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 5)
	if (ViaORcheckBit(5, VIA2_iA5)) {
		VIA2_iA5 = v;
#ifdef VIA2_iA5_ChangeNtfy
		VIA2_iA5_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 4)
	if (ViaORcheckBit(4, VIA2_iA4)) {
		VIA2_iA4 = v;
#ifdef VIA2_iA4_ChangeNtfy
		VIA2_iA4_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 3)
	if (ViaORcheckBit(3, VIA2_iA3)) {
		VIA2_iA3 = v;
#ifdef VIA2_iA3_ChangeNtfy
		VIA2_iA3_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 2)
	if (ViaORcheckBit(2, VIA2_iA2)) {
		VIA2_iA2 = v;
#ifdef VIA2_iA2_ChangeNtfy
		VIA2_iA2_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 1)
	if (ViaORcheckBit(1, VIA2_iA1)) {
		VIA2_iA1 = v;
#ifdef VIA2_iA1_ChangeNtfy
		VIA2_iA1_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORA_CanOut, 0)
	if (ViaORcheckBit(0, VIA2_iA0)) {
		VIA2_iA0 = v;
#ifdef VIA2_iA0_ChangeNtfy
		VIA2_iA0_ChangeNtfy();
#endif
	}
#endif
}

LOCALPROC VIA2_Put_ORB(ui3b Selection, ui3b Data)
{
#if 0 != VIA2_ORB_CanOut
	ui3b v;
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 7)
	if (ViaORcheckBit(7, VIA2_iB7)) {
		VIA2_iB7 = v;
#ifdef VIA2_iB7_ChangeNtfy
		VIA2_iB7_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 6)
	if (ViaORcheckBit(6, VIA2_iB6)) {
		VIA2_iB6 = v;
#ifdef VIA2_iB6_ChangeNtfy
		VIA2_iB6_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 5)
	if (ViaORcheckBit(5, VIA2_iB5)) {
		VIA2_iB5 = v;
#ifdef VIA2_iB5_ChangeNtfy
		VIA2_iB5_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 4)
	if (ViaORcheckBit(4, VIA2_iB4)) {
		VIA2_iB4 = v;
#ifdef VIA2_iB4_ChangeNtfy
		VIA2_iB4_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 3)
	if (ViaORcheckBit(3, VIA2_iB3)) {
		VIA2_iB3 = v;
#ifdef VIA2_iB3_ChangeNtfy
		VIA2_iB3_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 2)
	if (ViaORcheckBit(2, VIA2_iB2)) {
		VIA2_iB2 = v;
#ifdef VIA2_iB2_ChangeNtfy
		VIA2_iB2_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 1)
	if (ViaORcheckBit(1, VIA2_iB1)) {
		VIA2_iB1 = v;
#ifdef VIA2_iB1_ChangeNtfy
		VIA2_iB1_ChangeNtfy();
#endif
	}
#endif

#if Ui3rTestBit(VIA2_ORB_CanOut, 0)
	if (ViaORcheckBit(0, VIA2_iB0)) {
		VIA2_iB0 = v;
#ifdef VIA2_iB0_ChangeNtfy
		VIA2_iB0_ChangeNtfy();
#endif
	}
#endif
}

LOCALPROC VIA2_SetDDR_A(ui3b Data)
{
	ui3b floatbits = VIA2_D.DDR_A & ~ Data;
	ui3b unfloatbits = Data & ~ VIA2_D.DDR_A;

	if (floatbits != 0) {
		VIA2_Put_ORA(floatbits, VIA2_ORA_FloatVal);
	}
	VIA2_D.DDR_A = Data;
	if (unfloatbits != 0) {
		VIA2_Put_ORA(unfloatbits, VIA2_D.ORA);
	}
	if ((Data & ~ VIA2_ORA_CanOut) != 0) {
		ReportAbnormalID(0x0501,
			"Set VIA2_D.DDR_A unexpected direction");
	}
}

LOCALPROC VIA2_SetDDR_B(ui3b Data)
{
	ui3b floatbits = VIA2_D.DDR_B & ~ Data;
	ui3b unfloatbits = Data & ~ VIA2_D.DDR_B;

	if (floatbits != 0) {
		VIA2_Put_ORB(floatbits, VIA2_ORB_FloatVal);
	}
	VIA2_D.DDR_B = Data;
	if (unfloatbits != 0) {
		VIA2_Put_ORB(unfloatbits, VIA2_D.ORB);
	}
	if ((Data & ~ VIA2_ORB_CanOut) != 0) {
		ReportAbnormalID(0x0502,
			"Set VIA2_D.DDR_B unexpected direction");
	}
}


LOCALPROC VIA2_CheckInterruptFlag(void)
{
	ui3b NewInterruptRequest =
		((VIA2_D.IFR & VIA2_D.IER) != 0) ? 1 : 0;

	if (NewInterruptRequest != VIA2_InterruptRequest) {
		VIA2_InterruptRequest = NewInterruptRequest;
#ifdef VIA2_interruptChngNtfy
		VIA2_interruptChngNtfy();
#endif
	}
}


LOCALVAR ui3b VIA2_T1_Active = 0;
LOCALVAR ui3b VIA2_T2_Active = 0;

LOCALVAR blnr VIA2_T1IntReady = falseblnr;

LOCALPROC VIA2_Clear(void)
{
	VIA2_D.ORA   = 0; VIA2_D.DDR_A = 0;
	VIA2_D.ORB   = 0; VIA2_D.DDR_B = 0;
	VIA2_D.T1L_L = VIA2_D.T1L_H = 0x00;
	VIA2_D.T2L_L = 0x00;
	VIA2_D.T1C_F = 0;
	VIA2_D.T2C_F = 0;
	VIA2_D.SR = VIA2_D.ACR = 0x00;
	VIA2_D.PCR   = VIA2_D.IFR   = VIA2_D.IER   = 0x00;
	VIA2_T1_Active = VIA2_T2_Active = 0x00;
	VIA2_T1IntReady = falseblnr;
}

GLOBALPROC VIA2_Zap(void)
{
	VIA2_Clear();
	VIA2_InterruptRequest = 0;
}

GLOBALPROC VIA2_Reset(void)
{
	VIA2_SetDDR_A(0);
	VIA2_SetDDR_B(0);

	VIA2_Clear();

	VIA2_CheckInterruptFlag();
}

LOCALPROC VIA2_SetInterruptFlag(ui3b VIA_Int)
{
	VIA2_D.IFR |= ((ui3b)1 << VIA_Int);
	VIA2_CheckInterruptFlag();
}

LOCALPROC VIA2_ClrInterruptFlag(ui3b VIA_Int)
{
	VIA2_D.IFR &= ~ ((ui3b)1 << VIA_Int);
	VIA2_CheckInterruptFlag();
}

#ifdef _VIA_Debug
#include <stdio.h>
#endif

GLOBALPROC VIA2_ShiftInData(ui3b v)
{
	/*
		external hardware generates 8 pulses on CB1,
		writes 8 bits to CB2
	*/
	ui3b ShiftMode = (VIA2_D.ACR & 0x1C) >> 2;

	if (ShiftMode != 3) {
#if ExtraAbnormalReports
		if (ShiftMode == 0) {
			/* happens on reset */
		} else {
			ReportAbnormalID(0x0503, "VIA Not ready to shift in");
				/*
					Observed (rarely) in Crystal Quest played
					at 1x speed in "-t mc64".
				*/
		}
#endif
	} else {
		VIA2_D.SR = v;
		VIA2_SetInterruptFlag(kIntSR);
		VIA2_SetInterruptFlag(kIntCB1);
	}
}

GLOBALFUNC ui3b VIA2_ShiftOutData(void)
{
	/*
		external hardware generates 8 pulses on CB1,
		reads 8 bits from CB2
	*/
	if (((VIA2_D.ACR & 0x1C) >> 2) != 7) {
		ReportAbnormalID(0x0504, "VIA Not ready to shift out");
		return 0;
	} else {
		VIA2_SetInterruptFlag(kIntSR);
		VIA2_SetInterruptFlag(kIntCB1);
		VIA2_iCB2 = (VIA2_D.SR & 1);
		return VIA2_D.SR;
	}
}

#define CyclesPerViaTime (10 * kMyClockMult)
#define CyclesScaledPerViaTime (kCycleScale * CyclesPerViaTime)

LOCALVAR blnr VIA2_T1Running = trueblnr;
LOCALVAR iCountt VIA2_T1LastTime = 0;

GLOBALPROC VIA2_DoTimer1Check(void)
{
	if (VIA2_T1Running) {
		iCountt NewTime = GetCuriCount();
		iCountt deltaTime = (NewTime - VIA2_T1LastTime);
		if (deltaTime != 0) {
			ui5b Temp = VIA2_D.T1C_F; /* Get Timer 1 Counter */
			ui5b deltaTemp =
				(deltaTime / CyclesPerViaTime) << (16 - kLn2CycleScale);
					/* may overflow */
			ui5b NewTemp = Temp - deltaTemp;
			if ((deltaTime > (0x00010000UL * CyclesScaledPerViaTime))
				|| ((Temp <= deltaTemp) && (Temp != 0)))
			{
				if ((VIA2_D.ACR & 0x40) != 0) { /* Free Running? */
					/* Reload Counter from Latches */
					ui4b v = (VIA2_D.T1L_H << 8) + VIA2_D.T1L_L;
					ui4b ntrans = 1 + ((v == 0) ? 0 :
						(((deltaTemp - Temp) / v) >> 16));
					NewTemp += (((ui5b)v * ntrans) << 16);
#if Ui3rTestBit(VIA2_ORB_CanOut, 7)
					if ((VIA2_D.ACR & 0x80) != 0) { /* invert ? */
						if ((ntrans & 1) != 0) {
							VIA2_iB7 ^= 1;
#ifdef VIA2_iB7_ChangeNtfy
							VIA2_iB7_ChangeNtfy();
#endif
						}
					}
#endif
					VIA2_SetInterruptFlag(kIntT1);
#if VIA2_dolog && 1
					dbglog_WriteNote("VIA2 Timer 1 Interrupt");
#endif
				} else {
					if (VIA2_T1_Active == 1) {
						VIA2_T1_Active = 0;
						VIA2_SetInterruptFlag(kIntT1);
#if VIA2_dolog && 1
						dbglog_WriteNote("VIA2 Timer 1 Interrupt");
#endif
					}
				}
			}

			VIA2_D.T1C_F = NewTemp;
			VIA2_T1LastTime = NewTime;
		}

		VIA2_T1IntReady = falseblnr;
		if ((VIA2_D.IFR & (1 << kIntT1)) == 0) {
			if (((VIA2_D.ACR & 0x40) != 0) || (VIA2_T1_Active == 1)) {
				ui5b NewTemp = VIA2_D.T1C_F; /* Get Timer 1 Counter */
				ui5b NewTimer;
#ifdef _VIA_Debug
				fprintf(stderr, "posting Timer1Check, %d, %d\n",
					Temp, GetCuriCount());
#endif
				if (NewTemp == 0) {
					NewTimer = (0x00010000UL * CyclesScaledPerViaTime);
				} else {
					NewTimer =
						(1 + (NewTemp >> (16 - kLn2CycleScale)))
							* CyclesPerViaTime;
				}
				ICT_add(kICT_VIA2_Timer1Check, NewTimer);
				VIA2_T1IntReady = trueblnr;
			}
		}
	}
}

LOCALPROC CheckT1IntReady(void)
{
	if (VIA2_T1Running) {
		blnr NewT1IntReady = falseblnr;

		if ((VIA2_D.IFR & (1 << kIntT1)) == 0) {
			if (((VIA2_D.ACR & 0x40) != 0) || (VIA2_T1_Active == 1)) {
				NewT1IntReady = trueblnr;
			}
		}

		if (VIA2_T1IntReady != NewT1IntReady) {
			VIA2_T1IntReady = NewT1IntReady;
			if (NewT1IntReady) {
				VIA2_DoTimer1Check();
			}
		}
	}
}

GLOBALFUNC ui4b VIA2_GetT1InvertTime(void)
{
	ui4b v;

	if ((VIA2_D.ACR & 0xC0) == 0xC0) {
		v = (VIA2_D.T1L_H << 8) + VIA2_D.T1L_L;
	} else {
		v = 0;
	}
	return v;
}

LOCALVAR blnr VIA2_T2Running = trueblnr;
LOCALVAR blnr VIA2_T2C_ShortTime = falseblnr;
LOCALVAR iCountt VIA2_T2LastTime = 0;

GLOBALPROC VIA2_DoTimer2Check(void)
{
	if (VIA2_T2Running) {
		iCountt NewTime = GetCuriCount();
		ui5b Temp = VIA2_D.T2C_F; /* Get Timer 2 Counter */
		iCountt deltaTime = (NewTime - VIA2_T2LastTime);
		ui5b deltaTemp = (deltaTime / CyclesPerViaTime)
			<< (16 - kLn2CycleScale); /* may overflow */
		ui5b NewTemp = Temp - deltaTemp;
		if (VIA2_T2_Active == 1) {
			if ((deltaTime > (0x00010000UL * CyclesScaledPerViaTime))
				|| ((Temp <= deltaTemp) && (Temp != 0)))
			{
				VIA2_T2C_ShortTime = falseblnr;
				VIA2_T2_Active = 0;
				VIA2_SetInterruptFlag(kIntT2);
#if VIA2_dolog && 1
				dbglog_WriteNote("VIA2 Timer 2 Interrupt");
#endif
			} else {
				ui5b NewTimer;
#ifdef _VIA_Debug
				fprintf(stderr, "posting Timer2Check, %d, %d\n",
					Temp, GetCuriCount());
#endif
				if (NewTemp == 0) {
					NewTimer = (0x00010000UL * CyclesScaledPerViaTime);
				} else {
					NewTimer = (1 + (NewTemp >> (16 - kLn2CycleScale)))
						* CyclesPerViaTime;
				}
				ICT_add(kICT_VIA2_Timer2Check, NewTimer);
			}
		}
		VIA2_D.T2C_F = NewTemp;
		VIA2_T2LastTime = NewTime;
	}
}

#define kORB    0x00
#define kORA_H  0x01
#define kDDR_B  0x02
#define kDDR_A  0x03
#define kT1C_L  0x04
#define kT1C_H  0x05
#define kT1L_L  0x06
#define kT1L_H  0x07
#define kT2_L   0x08
#define kT2_H   0x09
#define kSR     0x0A
#define kACR    0x0B
#define kPCR    0x0C
#define kIFR    0x0D
#define kIER    0x0E
#define kORA    0x0F

GLOBALFUNC ui5b VIA2_Access(ui5b Data, blnr WriteMem, CPTR addr)
{
	switch (addr) {
		case kORB   :
#if VIA2_CB2modesAllowed != 0x01
			if ((VIA2_D.PCR & 0xE0) == 0)
#endif
			{
				VIA2_ClrInterruptFlag(kIntCB2);
			}
			VIA2_ClrInterruptFlag(kIntCB1);
			if (WriteMem) {
				VIA2_D.ORB = Data;
				VIA2_Put_ORB(VIA2_D.DDR_B, VIA2_D.ORB);
			} else {
				Data = (VIA2_D.ORB & VIA2_D.DDR_B)
					| VIA2_Get_ORB(~ VIA2_D.DDR_B);
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kORB", Data, WriteMem);
#endif
			break;
		case kDDR_B :
			if (WriteMem) {
				VIA2_SetDDR_B(Data);
			} else {
				Data = VIA2_D.DDR_B;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kDDR_B", Data, WriteMem);
#endif
			break;
		case kDDR_A :
			if (WriteMem) {
				VIA2_SetDDR_A(Data);
			} else {
				Data = VIA2_D.DDR_A;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kDDR_A", Data, WriteMem);
#endif
			break;
		case kT1C_L :
			if (WriteMem) {
				VIA2_D.T1L_L = Data;
			} else {
				VIA2_ClrInterruptFlag(kIntT1);
				VIA2_DoTimer1Check();
				Data = (VIA2_D.T1C_F & 0x00FF0000) >> 16;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kT1C_L", Data, WriteMem);
#endif
			break;
		case kT1C_H :
			if (WriteMem) {
				VIA2_D.T1L_H = Data;
				VIA2_ClrInterruptFlag(kIntT1);
				VIA2_D.T1C_F = (Data << 24) + (VIA2_D.T1L_L << 16);
				if ((VIA2_D.ACR & 0x40) == 0) {
					VIA2_T1_Active = 1;
				}
				VIA2_T1LastTime = GetCuriCount();
				VIA2_DoTimer1Check();
			} else {
				VIA2_DoTimer1Check();
				Data = (VIA2_D.T1C_F & 0xFF000000) >> 24;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kT1C_H", Data, WriteMem);
#endif
			break;
		case kT1L_L :
			if (WriteMem) {
				VIA2_D.T1L_L = Data;
			} else {
				Data = VIA2_D.T1L_L;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kT1L_L", Data, WriteMem);
#endif
			break;
		case kT1L_H :
			if (WriteMem) {
				VIA2_D.T1L_H = Data;
			} else {
				Data = VIA2_D.T1L_H;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kT1L_H", Data, WriteMem);
#endif
			break;
		case kT2_L  :
			if (WriteMem) {
				VIA2_D.T2L_L = Data;
			} else {
				VIA2_ClrInterruptFlag(kIntT2);
				VIA2_DoTimer2Check();
				Data = (VIA2_D.T2C_F & 0x00FF0000) >> 16;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kT2_L", Data, WriteMem);
#endif
			break;
		case kT2_H  :
			if (WriteMem) {
				VIA2_D.T2C_F = (Data << 24) + (VIA2_D.T2L_L << 16);
				VIA2_ClrInterruptFlag(kIntT2);
				VIA2_T2_Active = 1;

				if ((VIA2_D.T2C_F < (128UL << 16))
					&& (VIA2_D.T2C_F != 0))
				{
					VIA2_T2C_ShortTime = trueblnr;
					VIA2_T2Running = trueblnr;
					/*
						Running too many instructions during
						a short timer interval can crash when
						playing sounds in System 7. So
						in this case don't let timer pause.
					*/
				}
				VIA2_T2LastTime = GetCuriCount();
				VIA2_DoTimer2Check();
			} else {
				VIA2_DoTimer2Check();
				Data = (VIA2_D.T2C_F & 0xFF000000) >> 24;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kT2_H", Data, WriteMem);
#endif
			break;
		case kSR:
#ifdef _VIA_Debug
			fprintf(stderr, "VIA2_D.SR: %d, %d, %d\n",
				WriteMem, ((VIA2_D.ACR & 0x1C) >> 2), Data);
#endif
			if (WriteMem) {
				VIA2_D.SR = Data;
			}
			VIA2_ClrInterruptFlag(kIntSR);
			switch ((VIA2_D.ACR & 0x1C) >> 2) {
				case 3 : /* Shifting In */
					break;
				case 6 : /* shift out under o2 clock */
					if ((! WriteMem) || (VIA2_D.SR != 0)) {
						ReportAbnormalID(0x0505,
							"VIA shift mode 6, non zero");
					} else {
#ifdef _VIA_Debug
						fprintf(stderr, "posting Foo2Task\n");
#endif
						if (VIA2_iCB2 != 0) {
							VIA2_iCB2 = 0;
#ifdef VIA2_iCB2_ChangeNtfy
							VIA2_iCB2_ChangeNtfy();
#endif
						}
					}
#if 0 /* possibly should do this. seems not to affect anything. */
					VIA2_SetInterruptFlag(kIntSR); /* don't wait */
#endif
					break;
				case 7 : /* Shifting Out */
					break;
			}
			if (! WriteMem) {
				Data = VIA2_D.SR;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kSR", Data, WriteMem);
#endif
			break;
		case kACR:
			if (WriteMem) {
#if 1
				if ((VIA2_D.ACR & 0x10) != ((ui3b)Data & 0x10)) {
					/* shift direction has changed */
					if ((Data & 0x10) == 0) {
						/*
							no longer an output,
							set data to float value
						*/
						if (VIA2_iCB2 == 0) {
							VIA2_iCB2 = 1;
#ifdef VIA2_iCB2_ChangeNtfy
							VIA2_iCB2_ChangeNtfy();
#endif
						}
					}
				}
#endif
				VIA2_D.ACR = Data;
				if ((VIA2_D.ACR & 0x20) != 0) {
					/* Not pulse counting? */
					ReportAbnormalID(0x0506,
						"Set VIA2_D.ACR T2 Timer pulse counting");
				}
				switch ((VIA2_D.ACR & 0xC0) >> 6) {
					/* case 1: happens in early System 6 */
					case 2:
						ReportAbnormalID(0x0507,
							"Set VIA2_D.ACR T1 Timer mode 2");
						break;
				}
				CheckT1IntReady();
				switch ((VIA2_D.ACR & 0x1C) >> 2) {
					case 0: /* this isn't sufficient */
						VIA2_ClrInterruptFlag(kIntSR);
						break;
					case 1:
					case 2:
					case 4:
					case 5:
						ReportAbnormalID(0x0508,
							"Set VIA2_D.ACR shift mode 1,2,4,5");
						break;
					default:
						break;
				}
				if ((VIA2_D.ACR & 0x03) != 0) {
					ReportAbnormalID(0x0509,
						"Set VIA2_D.ACR T2 Timer latching enabled");
				}
			} else {
				Data = VIA2_D.ACR;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kACR", Data, WriteMem);
#endif
			break;
		case kPCR:
			if (WriteMem) {
				VIA2_D.PCR = Data;
#define Ui3rSetContains(s, i) (((s) & (1 << (i))) != 0)
				if (! Ui3rSetContains(VIA2_CB2modesAllowed,
					(VIA2_D.PCR >> 5) & 0x07))
				{
					ReportAbnormalID(0x050A,
						"Set VIA2_D.PCR CB2 Control mode?");
				}
				if ((VIA2_D.PCR & 0x10) != 0) {
					ReportAbnormalID(0x050B,
						"Set VIA2_D.PCR CB1 INTERRUPT CONTROL?");
				}
				if (! Ui3rSetContains(VIA2_CA2modesAllowed,
					(VIA2_D.PCR >> 1) & 0x07))
				{
					ReportAbnormalID(0x050C,
						"Set VIA2_D.PCR CA2 INTERRUPT CONTROL?");
				}
				if ((VIA2_D.PCR & 0x01) != 0) {
					ReportAbnormalID(0x050D,
						"Set VIA2_D.PCR CA1 INTERRUPT CONTROL?");
				}
			} else {
				Data = VIA2_D.PCR;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kPCR", Data, WriteMem);
#endif
			break;
		case kIFR:
			if (WriteMem) {
				VIA2_D.IFR = VIA2_D.IFR & ((~ Data) & 0x7F);
					/* Clear Flag Bits */
				VIA2_CheckInterruptFlag();
				CheckT1IntReady();
			} else {
				Data = VIA2_D.IFR;
				if ((VIA2_D.IFR & VIA2_D.IER) != 0) {
					Data |= 0x80;
				}
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kIFR", Data, WriteMem);
#endif
			break;
		case kIER   :
			if (WriteMem) {
				if ((Data & 0x80) == 0) {
					VIA2_D.IER = VIA2_D.IER & ((~ Data) & 0x7F);
						/* Clear Enable Bits */
#if 0 != VIA2_IER_Never0
					/*
						of course, will be 0 initially,
						this just checks not cleared later.
					*/
					if ((Data & VIA2_IER_Never0) != 0) {
						ReportAbnormalID(0x050E, "IER Never0 clr");
					}
#endif
				} else {
					VIA2_D.IER = VIA2_D.IER | (Data & 0x7F);
						/* Set Enable Bits */
#if 0 != VIA2_IER_Never1
					if ((VIA2_D.IER & VIA2_IER_Never1) != 0) {
						ReportAbnormalID(0x050F, "IER Never1 set");
					}
#endif
				}
				VIA2_CheckInterruptFlag();
			} else {
				Data = VIA2_D.IER | 0x80;
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kIER", Data, WriteMem);
#endif
			break;
		case kORA   :
		case kORA_H :
			if ((VIA2_D.PCR & 0xE) == 0) {
				VIA2_ClrInterruptFlag(kIntCA2);
			}
			VIA2_ClrInterruptFlag(kIntCA1);
			if (WriteMem) {
				VIA2_D.ORA = Data;
				VIA2_Put_ORA(VIA2_D.DDR_A, VIA2_D.ORA);
			} else {
				Data = (VIA2_D.ORA & VIA2_D.DDR_A)
					| VIA2_Get_ORA(~ VIA2_D.DDR_A);
			}
#if VIA2_dolog && 1
			dbglog_Access("VIA2_Access kORA", Data, WriteMem);
#endif
			break;
	}
	return Data;
}

GLOBALPROC VIA2_ExtraTimeBegin(void)
{
	if (VIA2_T1Running) {
		VIA2_DoTimer1Check(); /* run up to this moment */
		VIA2_T1Running = falseblnr;
	}
	if (VIA2_T2Running & (! VIA2_T2C_ShortTime)) {
		VIA2_DoTimer2Check(); /* run up to this moment */
		VIA2_T2Running = falseblnr;
	}
}

GLOBALPROC VIA2_ExtraTimeEnd(void)
{
	if (! VIA2_T1Running) {
		VIA2_T1Running = trueblnr;
		VIA2_T1LastTime = GetCuriCount();
		VIA2_DoTimer1Check();
	}
	if (! VIA2_T2Running) {
		VIA2_T2Running = trueblnr;
		VIA2_T2LastTime = GetCuriCount();
		VIA2_DoTimer2Check();
	}
}

/* VIA Interrupt Interface */

#ifdef VIA2_iCA1_PulseNtfy
GLOBALPROC VIA2_iCA1_PulseNtfy(void)
{
	VIA2_SetInterruptFlag(kIntCA1);
}
#endif

#ifdef VIA2_iCA2_PulseNtfy
GLOBALPROC VIA2_iCA2_PulseNtfy(void)
{
	VIA2_SetInterruptFlag(kIntCA2);
}
#endif

#ifdef VIA2_iCB1_PulseNtfy
GLOBALPROC VIA2_iCB1_PulseNtfy(void)
{
	VIA2_SetInterruptFlag(kIntCB1);
}
#endif

#ifdef VIA2_iCB2_PulseNtfy
GLOBALPROC VIA2_iCB2_PulseNtfy(void)
{
	VIA2_SetInterruptFlag(kIntCB2);
}
#endif
