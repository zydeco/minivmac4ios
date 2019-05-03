/*
	CONTROLM.h

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
	CONTROL Mode
*/

#ifdef CONTROLM_H
#error "header already included"
#else
#define CONTROLM_H
#endif

enum {
#if EnableDemoMsg
	SpclModeDemo,
#endif
#if EnableAltKeysMode
	SpclModeAltKeyText,
#endif
#if UseActvCode
	SpclModeActvCode,
#endif
	SpclModeNoRom,
	SpclModeMessage,
#if UseControlKeys
	SpclModeControl,
#endif

	kNumSpclModes
};

LOCALVAR uimr SpecialModes = 0;

LOCALVAR blnr NeedWholeScreenDraw = falseblnr;

#define SpecialModeSet(i) SpecialModes |= (1 << (i))
#define SpecialModeClr(i) SpecialModes &= ~ (1 << (i))
#define SpecialModeTst(i) (0 != (SpecialModes & (1 << (i))))

#define MacMsgDisplayed SpecialModeTst(SpclModeMessage)

LOCALVAR ui3p CntrlDisplayBuff = nullpr;

LOCALPROC DrawCell(unsigned int h, unsigned int v, int x)
{
#if 1
	/* safety check */
	if ((h < ((long)vMacScreenWidth / 8 - 2))
		&& (v < (vMacScreenHeight / 16 - 1)))
#endif
	{
		int i;
		ui3p p0 = ((ui3p)CellData) + 16 * x;

#if 0 != vMacScreenDepth
		if (UseColorMode) {
			ui3p p = CntrlDisplayBuff
				+ ((h + 1) << vMacScreenDepth)
				+ (v * 16 + 11) * vMacScreenByteWidth;

			for (i = 16; --i >= 0; ) {
#if 1 == vMacScreenDepth
				int k;
				ui3b t0 = *p0;
				ui3p p2 = p;
				for (k = 2; --k >= 0; ) {
					*p2++ = (((t0) & 0x80) ? 0xC0 : 0x00)
						| (((t0) & 0x40) ? 0x30 : 0x00)
						| (((t0) & 0x20) ? 0x0C : 0x00)
						| (((t0) & 0x10) ? 0x03 : 0x00);
						/* black RRGGBBAA, white RRGGBBAA */
					t0 <<= 4;
				}
#elif 2 == vMacScreenDepth
				int k;
				ui3b t0 = *p0;
				ui3p p2 = p;
				for (k = 4; --k >= 0; ) {
					*p2++ = (((t0) & 0x40) ? 0x0F : 0x00)
						| (((t0) & 0x80) ? 0xF0 : 0x00);
						/* black RRGGBBAA, white RRGGBBAA */
					t0 <<= 2;
				}
#elif 3 == vMacScreenDepth
				int k;
				ui3b t0 = *p0;
				ui3p p2 = p;
				for (k = 8; --k >= 0; ) {
					*p2++ = ((t0 >> k) & 0x01) ? 0xFF : 0x00;
						/* black RRGGBBAA, white RRGGBBAA */
				}
#elif 4 == vMacScreenDepth
				int k;
				ui4r v;
				ui3b t0 = *p0;
				ui3p p2 = p;
				for (k = 8; --k >= 0; ) {
					v = ((t0 >> k) & 0x01) ? 0x0000 : 0x7FFF;
						/* black RRGGBBAA, white RRGGBBAA */
					/* *((ui4b *)p2)++ = v; need big endian, so : */
					*p2++ = v >> 8;
					*p2++ = v;
				}
#elif 5 == vMacScreenDepth
				int k;
				ui5r v;
				ui3b t0 = *p0;
				ui3p p2 = p;
				for (k = 8; --k >= 0; ) {
					v = ((t0 >> k) & 0x01) ? 0x00000000 : 0x00FFFFFF;
						/* black RRGGBBAA, white RRGGBBAA */
					/* *((ui5b *)p2)++ = v; need big endian, so : */
					*p2++ = v >> 24;
					*p2++ = v >> 16;
					*p2++ = v >> 8;
					*p2++ = v;
				}
#endif
				p += vMacScreenByteWidth;
				p0 ++;
			}
		} else
#endif
		{
			ui3p p = CntrlDisplayBuff + (h + 1)
				+ (v * 16 + 11) * vMacScreenMonoByteWidth;

			for (i = 16; --i >= 0; ) {
				*p = *p0;
				p += vMacScreenMonoByteWidth;
				p0 ++;
			}
		}
	}
}

#define ControlBoxh0 0
#define ControlBoxw 62
#define ControlBoxv0 0

#define hLimit (ControlBoxh0 + ControlBoxw - 1)
#define hStart (ControlBoxh0 + 1)


LOCALVAR int CurCellh0;
LOCALVAR int CurCellv0;

LOCALPROC DrawCellsBeginLine(void)
{
	DrawCell(ControlBoxh0, CurCellv0, kCellMiddleLeft);
	CurCellh0 = hStart;
}

LOCALPROC DrawCellsEndLine(void)
{
	int i;

	for (i = CurCellh0; i < hLimit; ++i) {
		DrawCell(i, CurCellv0, kCellSpace);
	}
	DrawCell(hLimit, CurCellv0, kCellMiddleRight);
	CurCellv0++;
}

LOCALPROC DrawCellsBottomLine(void)
{
	int i;

	DrawCell(ControlBoxh0 + 0, CurCellv0, kCellLowerLeft);
	for (i = hStart; i < hLimit; ++i) {
		DrawCell(i, CurCellv0, kCellLowerMiddle);
	}
	DrawCell(hLimit, CurCellv0, kCellLowerRight);
}

LOCALPROC DrawCellAdvance(int x)
{
	DrawCell(CurCellh0, CurCellv0, x);
	CurCellh0++;
}

LOCALPROC DrawCellsBlankLine(void)
{
	DrawCellsBeginLine();
	DrawCellsEndLine();
}

LOCALPROC DrawCellsFromStr(char *s)
{
	ui3b ps[ClStrMaxLength];
	ui3b cs;
	int L;
	int i;
	int j;
	int w;

	ClStrFromSubstCStr(&L, ps, s);

	i = 0;

	while (i < L) {
		cs = ps[i];
		i++;
		if (CurCellh0 < hLimit) {
			DrawCellAdvance(cs);
		} else {
			/* line is too wide, wrap */
			if (kCellSpace != cs) {
				--i; /* back up one char, at least */

				/* now try backing up to beginning of word */
				j = i;
				w = CurCellh0 - hStart;

				while ((w > 0) && (j > 0)
					&& (ps[j - 1] != kCellSpace))
				{
					--j;
					--w;
				}
				if (w != 0) {
					i = j;
					CurCellh0 = hStart + w;
				}
				/*
					else if w == 0, then have backed up to
					beginning of line, so just let the word
					be split.
				*/
			}
			/*
				else if cs == kCellSpace, just lose the space.
			*/
			DrawCellsEndLine();
				/*
					draw white space over the part of
					the word that have already drawn
				*/
			DrawCellsBeginLine();
		}
	}
}

LOCALPROC DrawCellsOneLineStr(char *s)
{
	DrawCellsBeginLine();
	DrawCellsFromStr(s);
	DrawCellsEndLine();
}

LOCALPROC DrawCellsKeyCommand(char *k, char *s)
{
	DrawCellsBeginLine();
	DrawCellsFromStr(" ");
	DrawCellsFromStr(k);
	DrawCellsFromStr(" - ");
	DrawCellsFromStr(s);
	DrawCellsEndLine();
}

typedef void (*SpclModeBody) (void);

LOCALPROC DrawSpclMode0(char *Title, SpclModeBody Body)
{
	int i;
	int k;

	CurCellv0 = ControlBoxv0 + 0;
	DrawCell(ControlBoxh0 + 0, CurCellv0, kCellUpperLeft);
	k = kCellIcon00;
	for (i = hStart; i < hStart + 4; ++i) {
		DrawCell(i, CurCellv0, k);
		k++;
	}
	for (i = hStart + 4; i < hLimit; ++i) {
		DrawCell(i, CurCellv0, kCellUpperMiddle);
	}
	DrawCell(hLimit, CurCellv0, kCellUpperRight);
	++CurCellv0;

	DrawCellsBeginLine();
	for (i = hStart; i < hStart + 4; ++i) {
		DrawCellAdvance(k);
		k++;
	}
	DrawCellAdvance(kCellSpace);
	DrawCellsFromStr(Title);
	DrawCellsEndLine();

	DrawCellsBeginLine();
	for (i = hStart; i < hStart + 4; ++i) {
		DrawCellAdvance(k);
		k++;
	}
	for (i = hStart + 4; i < hLimit; ++i) {
		DrawCellAdvance(kCellGraySep);
	}
	DrawCellsEndLine();

	if (nullpr != Body) {
		Body();
	}

	DrawCellsBottomLine();
}

#if EnableAltKeysMode
#include "ALTKEYSM.h"
#else
#define Keyboard_UpdateKeyMap1 Keyboard_UpdateKeyMap
#define DisconnectKeyCodes1 DisconnectKeyCodes
#endif

#if WantAbnormalReports || UseActvFile
LOCALPROC ClStrAppendHexNib(int *L0, ui3b *r, ui3r v)
{
	if (v < 10) {
		ClStrAppendChar(L0, r, kCellDigit0 + v);
	} else {
		ClStrAppendChar(L0, r, kCellUpA + (v - 10));
	}
}
#endif

#if WantAbnormalReports || UseActvFile
LOCALPROC ClStrAppendHexByte(int *L0, ui3b *r, ui3r v)
{
	ClStrAppendHexNib(L0, r, (v >> 4) & 0x0F);
	ClStrAppendHexNib(L0, r, v & 0x0F);
}
#endif

#if WantAbnormalReports || UseActvFile
LOCALPROC ClStrAppendHexWord(int *L0, ui3b *r, ui4r v)
{
	ClStrAppendHexByte(L0, r, (v >> 8) & 0xFF);
	ClStrAppendHexByte(L0, r, v & 0xFF);
}
#endif

#if WantAbnormalReports
LOCALPROC DrawCellsOneLineHexWord(ui4r v)
{
	ui3b ps[ClStrMaxLength];
	int L = 0;
	int i;

	ClStrAppendHexWord(&L, ps, v);

	DrawCellsBeginLine();
	for (i = 0; i < L; ++i) {
		DrawCellAdvance(ps[i]);
	}
	DrawCellsEndLine();
}
#endif

LOCALPROC DrawCellsMessageModeBody(void)
{
	DrawCellsOneLineStr(SavedBriefMsg);
	DrawCellsBlankLine();
	DrawCellsOneLineStr(SavedLongMsg);
#if WantAbnormalReports
	if (0 != SavedIDMsg) {
		DrawCellsBlankLine();
		DrawCellsOneLineHexWord(SavedIDMsg);
	}
#endif
}

LOCALPROC DrawMessageMode(void)
{
	DrawSpclMode0(kStrModeMessage, DrawCellsMessageModeBody);
}

LOCALPROC MacMsgDisplayOff(void)
{
	SpecialModeClr(SpclModeMessage);
	SavedBriefMsg = nullpr;
#if WantAbnormalReports
	SavedIDMsg = 0;
#endif
	NeedWholeScreenDraw = trueblnr;
}

LOCALPROC MacMsgDisplayOn(void)
{
	NeedWholeScreenDraw = trueblnr;
	DisconnectKeyCodes1(kKeepMaskControl | kKeepMaskCapsLock);
		/* command */
	SpecialModeSet(SpclModeMessage);
}

LOCALPROC DoMessageModeKey(ui3r key)
{
	if (MKC_C == key) {
		MacMsgDisplayOff();
	}
}

LOCALPROC MacMsgOverride(char *briefMsg, char *longMsg)
{
	if (MacMsgDisplayed) {
		MacMsgDisplayOff();
	}
	MacMsg(briefMsg, longMsg, falseblnr);
}

#if NeedDoMoreCommandsMsg
LOCALPROC DoMoreCommandsMsg(void)
{
	MacMsgOverride(kStrMoreCommandsTitle,
		kStrMoreCommandsMessage);
}
#endif

#if NeedDoAboutMsg
LOCALPROC DoAboutMsg(void)
{
	MacMsgOverride(kStrAboutTitle,
		kStrAboutMessage);
}
#endif

LOCALPROC NoRomMsgDisplayOff(void)
{
	SpecialModeClr(SpclModeNoRom);
	NeedWholeScreenDraw = trueblnr;
}

LOCALPROC NoRomMsgDisplayOn(void)
{
	NeedWholeScreenDraw = trueblnr;
	SpecialModeSet(SpclModeNoRom);
}

LOCALPROC DrawCellsNoRomModeBody(void)
{
	DrawCellsOneLineStr(kStrNoROMMessage);
}

LOCALPROC DrawNoRomMode(void)
{
	DrawSpclMode0(kStrNoROMTitle, DrawCellsNoRomModeBody);
}

#if UseControlKeys

LOCALVAR blnr LastControlKey = falseblnr;
LOCALVAR int CurControlMode = 0;
LOCALVAR int ControlMessage = 0;

enum {
	kCntrlModeOff,
	kCntrlModeBase,
#if WantEnblCtrlRst
	kCntrlModeConfirmReset,
#endif
#if WantEnblCtrlInt
	kCntrlModeConfirmInterrupt,
#endif
	kCntrlModeConfirmQuit,
	kCntrlModeSpeedControl,

	kNumCntrlModes
};

enum {
	kCntrlMsgBaseStart,
#if EnableMagnify
	kCntrlMsgMagnify,
#endif
#if VarFullScreen
	kCntrlMsgFullScreen,
#endif
#if WantEnblCtrlRst
	kCntrlMsgConfirmResetStart,
	kCntrlMsgHaveReset,
	kCntrlMsgResetCancelled,
#endif
#if WantEnblCtrlInt
	kCntrlMsgConfirmInterruptStart,
	kCntrlMsgHaveInterrupted,
	kCntrlMsgInterruptCancelled,
#endif
	kCntrlMsgConfirmQuitStart,
	kCntrlMsgQuitCancelled,
#if WantEnblCtrlKtg
	kCntrlMsgEmCntrl,
#endif
	kCntrlMsgSpeedControlStart,
	kCntrlMsgNewSpeed,
	kCntrlMsgNewStopped,
	kCntrlMsgNewRunInBack,
#if EnableAutoSlow
	kCntrlMsgNewAutoSlow,
#endif
	kCntrlMsgAbout,
	kCntrlMsgHelp,
#if IncludePbufs
	kCntrlMsgOptionsStrCopied,
#endif
#if 0 && (UseActvCode || EnableDemoMsg)
	kCntrlMsgRegStrCopied,
#endif

	kNumCntrlMsgs
};

LOCALPROC DoEnterControlMode(void)
{
	CurControlMode = kCntrlModeBase;
	ControlMessage = kCntrlMsgBaseStart;
	NeedWholeScreenDraw = trueblnr;
	DisconnectKeyCodes1(kKeepMaskControl | kKeepMaskCapsLock);
	SpecialModeSet(SpclModeControl);
}

LOCALPROC DoLeaveControlMode(void)
{
	SpecialModeClr(SpclModeControl);
	CurControlMode = kCntrlModeOff;
	NeedWholeScreenDraw = trueblnr;
}

LOCALPROC Keyboard_UpdateControlKey(blnr down)
{
	if (down != LastControlKey) {
		LastControlKey = down;
		if (down) {
			DoEnterControlMode();
		} else {
			DoLeaveControlMode();
		}
	}
}

LOCALPROC SetSpeedValue(ui3b i)
{
	SpeedValue = i;
	CurControlMode = kCntrlModeBase;
	ControlMessage = kCntrlMsgNewSpeed;
}

#if VarFullScreen
FORWARDPROC ToggleWantFullScreen(void);
#endif

#if IncludeHostTextClipExchange
LOCALPROC HTCEexportSubstCStr(char *s)
{
	int i;
	int L;
	tPbuf j;
#ifdef PbufHaveLock
	int n = ClStrSizeSubstCStr(s);

	if (mnvm_noErr == PbufNew(n, &j)) {
		blnr IsOk = falseblnr;
		ui3p p = PbufLock(j);

		if (nullpr != p) {
			L = 0;
			ClStrAppendSubstCStr(&L, p, s);

			if (L == n) {
				for (i = 0; i < n; ++i) {
					p[i] = Cell2MacAsciiMap[p[i]];
				}
				IsOk = trueblnr;
			}

			PbufUnlock(j);
		}

		if (IsOk) {
			HTCEexport(j);
		} else {
			PbufDispose(j);
		}
	}
#else
	ui3b ps[ClStrMaxLength];

	ClStrFromSubstCStr(&L, ps, s);

	for (i = 0; i < L; ++i) {
		ps[i] = Cell2MacAsciiMap[ps[i]];
	}

	if (mnvm_noErr == PbufNew(L, &j)) {
		PbufTransfer(ps, j, 0, L, trueblnr);
		HTCEexport(j);
	}
#endif
}
#endif

#if IncludeHostTextClipExchange
LOCALPROC CopyOptionsStr(void)
{
	HTCEexportSubstCStr(kBldOpts);
}
#endif

#if 0
#if UseActvCode
FORWARDPROC CopyRegistrationStr(void);
#elif EnableDemoMsg
LOCALPROC CopyRegistrationStr(void)
{
	HTCEexportSubstCStr("^v");
}
#endif
#endif


LOCALPROC DoControlModeKey(ui3r key)
{
	switch (CurControlMode) {
		case kCntrlModeBase:
			switch (key) {
#if WantEnblCtrlKtg
				case MKC_K:
					ControlKeyPressed = ! ControlKeyPressed;
					ControlMessage = kCntrlMsgEmCntrl;
					Keyboard_UpdateKeyMap1(MKC_UnMappedKey,
						ControlKeyPressed);
					break;
#endif
				case MKC_S:
					CurControlMode = kCntrlModeSpeedControl;
					ControlMessage = kCntrlMsgSpeedControlStart;
					break;
#if WantEnblCtrlInt
				case MKC_I:
					CurControlMode = kCntrlModeConfirmInterrupt;
					ControlMessage = kCntrlMsgConfirmInterruptStart;
					break;
#endif
#if WantEnblCtrlRst
				case MKC_R:
					if (! AnyDiskInserted()) {
						WantMacReset = trueblnr;
						ControlMessage = kCntrlMsgHaveReset;
					} else {
						CurControlMode = kCntrlModeConfirmReset;
						ControlMessage = kCntrlMsgConfirmResetStart;
					}
					break;
#endif
				case MKC_Q:
					if (! AnyDiskInserted()) {
						ForceMacOff = trueblnr;
					} else {
						CurControlMode = kCntrlModeConfirmQuit;
						ControlMessage = kCntrlMsgConfirmQuitStart;
					}
					break;
				case MKC_A:
					ControlMessage = kCntrlMsgAbout;
					break;
				case MKC_H:
					ControlMessage = kCntrlMsgHelp;
					break;
#if NeedRequestInsertDisk
				case MKC_O:
					RequestInsertDisk = trueblnr;
					break;
#endif
#if EnableMagnify
				case MKC_M:
					WantMagnify = ! WantMagnify;
					ControlMessage = kCntrlMsgMagnify;
					break;
#endif
#if VarFullScreen
				case MKC_F:
					ToggleWantFullScreen();
					ControlMessage = kCntrlMsgFullScreen;
					break;
#endif
#if IncludeHostTextClipExchange
				case MKC_P:
					CopyOptionsStr();
					ControlMessage = kCntrlMsgOptionsStrCopied;
					break;
#endif
#if 0 && (UseActvCode || EnableDemoMsg)
				case MKC_P:
					CopyRegistrationStr();
					ControlMessage = kCntrlMsgRegStrCopied;
					break;
#endif
#if NeedRequestIthDisk
				case MKC_1:
					RequestIthDisk = 1;
					break;
				case MKC_2:
					RequestIthDisk = 2;
					break;
				case MKC_3:
					RequestIthDisk = 3;
					break;
				case MKC_4:
					RequestIthDisk = 4;
					break;
				case MKC_5:
					RequestIthDisk = 5;
					break;
				case MKC_6:
					RequestIthDisk = 6;
					break;
				case MKC_7:
					RequestIthDisk = 7;
					break;
				case MKC_8:
					RequestIthDisk = 8;
					break;
				case MKC_9:
					RequestIthDisk = 9;
					break;
#endif
			}
			break;
#if WantEnblCtrlRst
		case kCntrlModeConfirmReset:
			switch (key) {
				case MKC_Y:
					WantMacReset = trueblnr;
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgHaveReset;
					break;
				case MKC_R:
					/* ignore, in case of repeat */
					break;
				case MKC_N:
				default:
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgResetCancelled;
					break;
			}
			break;
#endif
#if WantEnblCtrlInt
		case kCntrlModeConfirmInterrupt:
			switch (key) {
				case MKC_Y:
					WantMacInterrupt = trueblnr;
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgHaveInterrupted;
					break;
				case MKC_I:
					/* ignore, in case of repeat */
					break;
				case MKC_N:
				default:
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgInterruptCancelled;
					break;
			}
			break;
#endif
		case kCntrlModeConfirmQuit:
			switch (key) {
				case MKC_Y:
					ForceMacOff = trueblnr;
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgBaseStart;
						/* shouldn't see this message since quitting */
					break;
				case MKC_Q:
					/* ignore, in case of repeat */
					break;
				case MKC_N:
				default:
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgQuitCancelled;
					break;
			}
			break;
		case kCntrlModeSpeedControl:
			switch (key) {
				case MKC_E:
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgBaseStart;
					break;
				case MKC_B:
					RunInBackground = ! RunInBackground;
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgNewRunInBack;
					break;
				case MKC_D:
					if (ROM_loaded) {
						SpeedStopped = ! SpeedStopped;
						CurControlMode = kCntrlModeBase;
						ControlMessage = kCntrlMsgNewStopped;
					}
					break;
#if EnableAutoSlow
				case MKC_W:
					WantNotAutoSlow = ! WantNotAutoSlow;
					CurControlMode = kCntrlModeBase;
					ControlMessage = kCntrlMsgNewAutoSlow;
					break;
#endif
				case MKC_Z:
					SetSpeedValue(0);
					break;
				case MKC_1:
					SetSpeedValue(1);
					break;
				case MKC_2:
					SetSpeedValue(2);
					break;
				case MKC_3:
					SetSpeedValue(3);
					break;
				case MKC_4:
					SetSpeedValue(4);
					break;
				case MKC_5:
					SetSpeedValue(5);
					break;
				case MKC_A:
					SetSpeedValue((ui3b) -1);
					break;
			}
			break;
	}
	NeedWholeScreenDraw = trueblnr;
}

LOCALFUNC char * ControlMode2TitleStr(void)
{
	char *s;

	switch (CurControlMode) {
#if WantEnblCtrlRst
		case kCntrlModeConfirmReset:
			s = kStrModeConfirmReset;
			break;
#endif
#if WantEnblCtrlInt
		case kCntrlModeConfirmInterrupt:
			s = kStrModeConfirmInterrupt;
			break;
#endif
		case kCntrlModeConfirmQuit:
			s = kStrModeConfirmQuit;
			break;
		case kCntrlModeSpeedControl:
			s = kStrModeSpeedControl;
			break;
		case kCntrlModeBase:
		default:
			if (kCntrlMsgHelp == ControlMessage) {
				s = kStrModeControlHelp;
			} else {
				s = kStrModeControlBase;
			}
			break;
	}

	return s;
}

LOCALPROC DrawCellsControlModeBody(void)
{
	switch (ControlMessage) {
		case kCntrlMsgAbout:
			DrawCellsOneLineStr(kStrProgramInfo);

			DrawCellsBlankLine();

			DrawCellsOneLineStr(kStrWorkOfMany);
			DrawCellsOneLineStr(kMaintainerName);
			DrawCellsOneLineStr(kStrForMoreInfo);
			DrawCellsOneLineStr("^w");

			DrawCellsBlankLine();

			DrawCellsBeginLine();
			DrawCellsFromStr(kStrLicense);
			DrawCellsFromStr(kStrDisclaimer);
			DrawCellsEndLine();

			break;

		case kCntrlMsgHelp:
			DrawCellsOneLineStr(kStrHowToLeaveControl);
			DrawCellsOneLineStr(kStrHowToPickACommand);
			DrawCellsBlankLine();
			DrawCellsKeyCommand("A", kStrCmdAbout);
#if NeedRequestInsertDisk
			DrawCellsKeyCommand("O", kStrCmdOpenDiskImage);
#endif
			DrawCellsKeyCommand("Q", kStrCmdQuit);
			DrawCellsKeyCommand("S", kStrCmdSpeedControl);
#if EnableMagnify
			DrawCellsKeyCommand("M", kStrCmdMagnifyToggle);
#endif
#if VarFullScreen
			DrawCellsKeyCommand("F", kStrCmdFullScrnToggle);
#endif
#if WantEnblCtrlKtg
			DrawCellsKeyCommand("K", kStrCmdCtrlKeyToggle);
#endif
#if WantEnblCtrlRst
			DrawCellsKeyCommand("R", kStrCmdReset);
#endif
#if WantEnblCtrlInt
			DrawCellsKeyCommand("I", kStrCmdInterrupt);
#endif
			DrawCellsKeyCommand("P", kStrCmdCopyOptions);
			DrawCellsKeyCommand("H", kStrCmdHelp);
			break;
		case kCntrlMsgSpeedControlStart:
			DrawCellsOneLineStr(kStrCurrentSpeed);
			DrawCellsKeyCommand("Z", "1x");
			DrawCellsKeyCommand("1", "2x");
			DrawCellsKeyCommand("2", "4x");
			DrawCellsKeyCommand("3", "8x");
			DrawCellsKeyCommand("4", "16x");
			DrawCellsKeyCommand("5", "32x");
			DrawCellsKeyCommand("A", kStrSpeedAllOut);
			DrawCellsBlankLine();
			DrawCellsKeyCommand("D", kStrSpeedStopped);
			DrawCellsKeyCommand("B", kStrSpeedBackToggle);
#if EnableAutoSlow
			DrawCellsKeyCommand("W", kStrSpeedAutoSlowToggle);
#endif
			DrawCellsBlankLine();
			DrawCellsKeyCommand("E", kStrSpeedExit);
			break;
		case kCntrlMsgNewSpeed:
			DrawCellsOneLineStr(kStrNewSpeed);
			break;
		case kCntrlMsgNewRunInBack:
			DrawCellsOneLineStr(kStrNewRunInBack);
			break;
		case kCntrlMsgNewStopped:
			DrawCellsOneLineStr(kStrNewStopped);
			break;
#if EnableAutoSlow
		case kCntrlMsgNewAutoSlow:
			DrawCellsOneLineStr(kStrNewAutoSlow);
			break;
#endif
#if EnableMagnify
		case kCntrlMsgMagnify:
			DrawCellsOneLineStr(kStrNewMagnify);
			break;
#endif
#if VarFullScreen
		case kCntrlMsgFullScreen:
			DrawCellsOneLineStr(kStrNewFullScreen);
			break;
#endif
#if IncludeHostTextClipExchange
		case kCntrlMsgOptionsStrCopied:
			DrawCellsOneLineStr(kStrHaveCopiedOptions);
			break;
#endif
#if 0
#if UseActvCode
		case kCntrlMsgRegStrCopied:
			DrawCellsOneLineStr("Registration String copied.");
			break;
#elif EnableDemoMsg
		case kCntrlMsgRegStrCopied:
			DrawCellsOneLineStr("Variation name copied.");
			break;
#endif
#endif
#if WantEnblCtrlRst
		case kCntrlMsgConfirmResetStart:
			DrawCellsOneLineStr(kStrConfirmReset);
			DrawCellsBlankLine();
			DrawCellsKeyCommand("Y", kStrResetDo);
			DrawCellsKeyCommand("N", kStrResetNo);
			break;
		case kCntrlMsgHaveReset:
			DrawCellsOneLineStr(kStrHaveReset);
			break;
		case kCntrlMsgResetCancelled:
			DrawCellsOneLineStr(kStrCancelledReset);
			break;
#endif
#if WantEnblCtrlInt
		case kCntrlMsgConfirmInterruptStart:
			DrawCellsOneLineStr(kStrConfirmInterrupt);
			DrawCellsBlankLine();
			DrawCellsKeyCommand("Y", kStrInterruptDo);
			DrawCellsKeyCommand("N", kStrInterruptNo);
			break;
		case kCntrlMsgHaveInterrupted:
			DrawCellsOneLineStr(kStrHaveInterrupted);
			break;
		case kCntrlMsgInterruptCancelled:
			DrawCellsOneLineStr(kStrCancelledInterrupt);
			break;
#endif
		case kCntrlMsgConfirmQuitStart:
			DrawCellsOneLineStr(kStrConfirmQuit);
			DrawCellsBlankLine();
			DrawCellsKeyCommand("Y", kStrQuitDo);
			DrawCellsKeyCommand("N", kStrQuitNo);
			break;
		case kCntrlMsgQuitCancelled:
			DrawCellsOneLineStr(kStrCancelledQuit);
			break;
#if WantEnblCtrlKtg
		case kCntrlMsgEmCntrl:
			DrawCellsOneLineStr(kStrNewCntrlKey);
			break;
#endif
		case kCntrlMsgBaseStart:
		default:
			DrawCellsOneLineStr(kStrHowToLeaveControl);
			break;
	}
}

LOCALPROC DrawControlMode(void)
{
	DrawSpclMode0(ControlMode2TitleStr(), DrawCellsControlModeBody);
}

#endif /* UseControlKeys */

#if EnableDemoMsg

LOCALPROC DrawDemoMode(void)
{
	CurCellv0 = ControlBoxv0 + ((9 * CurMacDateInSeconds) & 0x0F);
	CurCellh0 = ControlBoxh0 + ((15 * CurMacDateInSeconds) & 0x1F);

	DrawCellAdvance(kCellDemo0);
	DrawCellAdvance(kCellDemo6);
	DrawCellAdvance(kCellDemo6);
	DrawCellAdvance(kCellDemo7);
	DrawCellAdvance(kCellDemo1);
	DrawCellAdvance(kCellDemo2);
	DrawCellAdvance(kCellDemo3);
	DrawCellAdvance(kCellDemo4);
	DrawCellAdvance(kCellDemo7);
	DrawCellAdvance(kCellDemo6);
	DrawCellAdvance(kCellDemo6);
	DrawCellAdvance(kCellDemo5);
}

LOCALPROC DemoModeSecondNotify(void)
{
	NeedWholeScreenDraw = trueblnr;
	SpecialModeSet(SpclModeDemo);
}

#endif /* EnableDemoMsg */

#if UseActvCode
#include "ACTVCODE.h"
#endif

LOCALPROC DrawSpclMode(void)
{
#if UseControlKeys
	if (SpecialModeTst(SpclModeControl)) {
		DrawControlMode();
	} else
#endif
	if (SpecialModeTst(SpclModeMessage)) {
		DrawMessageMode();
	} else
	if (SpecialModeTst(SpclModeNoRom)) {
		DrawNoRomMode();
	} else
#if UseActvCode
	if (SpecialModeTst(SpclModeActvCode)) {
		DrawActvCodeMode();
	} else
#endif
#if EnableAltKeysMode
	if (SpecialModeTst(SpclModeAltKeyText)) {
		DrawAltKeyMode();
	} else
#endif
#if EnableDemoMsg
	if (SpecialModeTst(SpclModeDemo)) {
		DrawDemoMode();
	} else
#endif
	{
		/* should not get here */
	}
}

LOCALFUNC ui3p GetCurDrawBuff(void)
{
	ui3p p = screencomparebuff;

	if (0 != SpecialModes) {
		MyMoveBytes((anyp)p, (anyp)CntrlDisplayBuff,
#if 0 != vMacScreenDepth
			UseColorMode ? vMacScreenNumBytes :
#endif
				vMacScreenMonoNumBytes
			);
		p = CntrlDisplayBuff;

		DrawSpclMode();
	}

	return p;
}

#ifdef WantKeyboard_RemapMac
LOCALFUNC ui3r Keyboard_RemapMac(ui3r key)
{
	switch (key) {
#if MKC_formac_Control != MKC_Control
		case MKC_Control:
			key = MKC_formac_Control;
			break;
#endif
#if MKC_formac_Command != MKC_Command
		case MKC_Command:
			key = MKC_formac_Command;
			break;
#endif
#if MKC_formac_Option != MKC_Option
		case MKC_Option:
			key = MKC_formac_Option;
			break;
#endif
#if MKC_formac_Shift != MKC_Shift
		case MKC_Shift:
			key = MKC_formac_Shift;
			break;
#endif
#if MKC_formac_CapsLock != MKC_CapsLock
		case MKC_CapsLock:
			key = MKC_formac_CapsLock;
			break;
#endif
#if MKC_formac_F1 != MKC_F1
		case MKC_F1:
			key = MKC_formac_F1;
			break;
#endif
#if MKC_formac_F2 != MKC_F2
		case MKC_F2:
			key = MKC_formac_F2;
			break;
#endif
#if MKC_formac_F3 != MKC_F3
		case MKC_F3:
			key = MKC_formac_F3;
			break;
#endif
#if MKC_formac_F4 != MKC_F4
		case MKC_F4:
			key = MKC_formac_F4;
			break;
#endif
#if MKC_formac_F5 != MKC_F5
		case MKC_F5:
			key = MKC_formac_F5;
			break;
#endif
#if MKC_formac_Escape != MKC_Escape
		case MKC_Escape:
			key = MKC_formac_Escape;
			break;
#endif
#if MKC_formac_BackSlash != MKC_BackSlash
		case MKC_BackSlash:
			key = MKC_formac_BackSlash;
			break;
#endif
#if MKC_formac_Slash != MKC_Slash
		case MKC_Slash:
			key = MKC_formac_Slash;
			break;
#endif
#if MKC_formac_Grave != MKC_Grave
		case MKC_Grave:
			key = MKC_formac_Grave;
			break;
#endif
#if MKC_formac_Enter != MKC_Enter
		case MKC_Enter:
			key = MKC_formac_Enter;
			break;
#endif
#if MKC_formac_PageUp != MKC_PageUp
		case MKC_PageUp:
			key = MKC_formac_PageUp;
			break;
#endif
#if MKC_formac_PageDown != MKC_PageDown
		case MKC_PageDown:
			key = MKC_formac_PageDown;
			break;
#endif
#if MKC_formac_Home != MKC_Home
		case MKC_Home:
			key = MKC_formac_Home;
			break;
#endif
#if MKC_formac_End != MKC_End
		case MKC_End:
			key = MKC_formac_End;
			break;
#endif
#if MKC_formac_Help != MKC_Help
		case MKC_Help:
			key = MKC_formac_Help;
			break;
#endif
#if MKC_formac_ForwardDel != MKC_ForwardDel
		case MKC_ForwardDel:
			key = MKC_formac_ForwardDel;
			break;
#endif
		default:
			break;
	}

	return key;
}
#endif /* WantKeyboard_RemapMac */

LOCALPROC Keyboard_UpdateKeyMap2(ui3r key, blnr down)
{
#if UseControlKeys
	if (MKC_CM == key) {
		Keyboard_UpdateControlKey(down);
	} else
#endif
	if ((0 == SpecialModes)
#if EnableAltKeysMode || EnableDemoMsg
			|| (0 == (SpecialModes & ~ (
				0
#if EnableAltKeysMode
				| (1 << SpclModeAltKeyText)
#endif
#if EnableDemoMsg
				| (1 << SpclModeDemo)
#endif
				)))
#endif
			|| (MKC_CapsLock == key)
		)
	{
		/* pass through */
		Keyboard_UpdateKeyMap1(key, down);
	} else {
		if (down) {
#if UseControlKeys
			if (SpecialModeTst(SpclModeControl)) {
				DoControlModeKey(key);
			} else
#endif
			if (SpecialModeTst(SpclModeMessage)) {
				DoMessageModeKey(key);
			} else
#if UseActvCode
			if (SpecialModeTst(SpclModeActvCode)) {
				DoActvCodeModeKey(key);
			} else
#endif
			{
			}
		} /* else if not down ignore */
	}
}

LOCALPROC DisconnectKeyCodes2(void)
{
	DisconnectKeyCodes1(kKeepMaskControl | kKeepMaskCapsLock);
#if UseControlKeys
	Keyboard_UpdateControlKey(falseblnr);
#endif
}

#ifndef CheckRomCheckSum
#define CheckRomCheckSum 1
#endif

#if CheckRomCheckSum
LOCALFUNC ui5r Calc_Checksum(void)
{
	long int i;
	ui5b CheckSum = 0;
	ui3p p = 4 + ROM;

	for (i = (kCheckSumRom_Size - 4) >> 1; --i >= 0; ) {
		CheckSum += do_get_mem_word(p);
		p += 2;
	}

	return CheckSum;
}
#endif

#if CheckRomCheckSum && RomStartCheckSum
LOCALPROC WarnMsgCorruptedROM(void)
{
	MacMsgOverride(kStrCorruptedROMTitle, kStrCorruptedROMMessage);
}
#endif

#if CheckRomCheckSum
LOCALPROC WarnMsgUnsupportedROM(void)
{
	MacMsgOverride(kStrUnsupportedROMTitle,
		kStrUnsupportedROMMessage);
}
#endif

LOCALFUNC tMacErr ROM_IsValid(void)
{
#if CheckRomCheckSum
	ui5r CheckSum = Calc_Checksum();

#if RomStartCheckSum
	if (CheckSum != do_get_mem_long(ROM)) {
		WarnMsgCorruptedROM();
		return mnvm_miscErr;
	} else
#endif
#ifdef kRomCheckSum1
	if (CheckSum == kRomCheckSum1) {
	} else
#endif
#ifdef kRomCheckSum2
	if (CheckSum == kRomCheckSum2) {
	} else
#endif
#ifdef kRomCheckSum3
	if (CheckSum == kRomCheckSum3) {
	} else
#endif
	{
		WarnMsgUnsupportedROM();
		return mnvm_miscErr;
	}
	/*
		Even if ROM is corrupt or unsupported, go ahead and
		try to run anyway. It shouldn't do any harm.
		[update: no, don't]
	*/

#endif /* CheckRomCheckSum */

	ROM_loaded = trueblnr;
	SpeedStopped = falseblnr;

	return mnvm_noErr;
}

LOCALFUNC blnr WaitForRom(void)
{
	if (! ROM_loaded) {
		NoRomMsgDisplayOn();

		SpeedStopped = trueblnr;
		do {
			WaitForNextTick();

			if (ForceMacOff) {
				return falseblnr;
			}
		} while (SpeedStopped);

		NoRomMsgDisplayOff();
	}

	return trueblnr;
}
