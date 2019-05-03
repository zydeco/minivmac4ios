/*
	ADBSHARE.h

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
	Apple Desktop Bus SHAREd code
	shared by emulation of different implementations of ADB
*/

#ifdef ADBSHARE_H
#error "header already included"
#else
#define ADBSHARE_H
#endif


/*
	ReportAbnormalID unused 0x0D08 - 0x0DFF
*/

#define ADB_MaxSzDatBuf 8

LOCALVAR ui3b ADB_SzDatBuf;
LOCALVAR blnr ADB_TalkDatBuf = falseblnr;
LOCALVAR ui3b ADB_DatBuf[ADB_MaxSzDatBuf];
LOCALVAR ui3b ADB_CurCmd = 0;
LOCALVAR ui3b NotSoRandAddr = 1;

LOCALVAR ui3b MouseADBAddress;
LOCALVAR blnr SavedCurMouseButton = falseblnr;
LOCALVAR ui4r MouseADBDeltaH = 0;
LOCALVAR ui4r MouseADBDeltaV = 0;

LOCALPROC ADB_DoMouseTalk(void)
{
	switch (ADB_CurCmd & 3) {
		case 0:
			{
				MyEvtQEl *p;
				ui4b partH;
				ui4b partV;
				blnr overflow = falseblnr;
				blnr MouseButtonChange = falseblnr;

				if (nullpr != (p = MyEvtQOutP())) {
					if (MyEvtQElKindMouseDelta == p->kind) {
						MouseADBDeltaH += p->u.pos.h;
						MouseADBDeltaV += p->u.pos.v;
						MyEvtQOutDone();
					}
				}
				partH = MouseADBDeltaH;
				partV = MouseADBDeltaV;

				if ((si4b)MouseADBDeltaH < 0) {
					partH = - partH;
				}
				if ((si4b)MouseADBDeltaV < 0) {
					partV = - partV;
				}
				if ((partH >> 6) > 0) {
					overflow = trueblnr;
					partH = (1 << 6) - 1;
				}
				if ((partV >> 6) > 0) {
					overflow = trueblnr;
					partV = (1 << 6) - 1;
				}
				if ((si4b)MouseADBDeltaH < 0) {
					partH = - partH;
				}
				if ((si4b)MouseADBDeltaV < 0) {
					partV = - partV;
				}
				MouseADBDeltaH -= partH;
				MouseADBDeltaV -= partV;
				if (! overflow) {
					if (nullpr != (p = MyEvtQOutP())) {
						if (MyEvtQElKindMouseButton == p->kind) {
							SavedCurMouseButton = p->u.press.down;
							MouseButtonChange = trueblnr;
							MyEvtQOutDone();
						}
					}
				}
				if ((0 != partH) || (0 != partV) || MouseButtonChange) {
					ADB_SzDatBuf = 2;
					ADB_TalkDatBuf = trueblnr;
					ADB_DatBuf[0] = (SavedCurMouseButton ? 0x00 : 0x80)
						| (partV & 127);
					ADB_DatBuf[1] = /* 0x00 */ 0x80 | (partH & 127);
				}
			}
			ADBMouseDisabled = 0;
			break;
		case 3:
			ADB_SzDatBuf = 2;
			ADB_TalkDatBuf = trueblnr;
			ADB_DatBuf[0] = 0x60 | (NotSoRandAddr & 0x0f);
			ADB_DatBuf[1] = 0x01;
			NotSoRandAddr += 1;
			break;
		default:
			ReportAbnormalID(0x0D01, "Talk to unknown mouse register");
			break;
	}
}

LOCALPROC ADB_DoMouseListen(void)
{
	switch (ADB_CurCmd & 3) {
		case 3:
			if (ADB_DatBuf[1] == 0xFE) {
				/* change address */
				MouseADBAddress = (ADB_DatBuf[0] & 0x0F);
			} else {
				ReportAbnormalID(0x0D02,
					"unknown listen op to mouse register 3");
			}
			break;
		default:
			ReportAbnormalID(0x0D03,
				"listen to unknown mouse register");
			break;
	}
}

LOCALVAR ui3b KeyboardADBAddress;

LOCALFUNC blnr CheckForADBkeyEvt(ui3b *NextADBkeyevt)
{
	int i;
	blnr KeyDown;

	if (! FindKeyEvent(&i, &KeyDown)) {
		return falseblnr;
	} else {
#if dbglog_HAVE && 0
		if (KeyDown) {
			dbglog_WriteNote("Got a KeyDown");
		}
#endif
		switch (i) {
			case MKC_Control:
				i = 0x36;
				break;
			case MKC_Left:
				i = 0x3B;
				break;
			case MKC_Right:
				i = 0x3C;
				break;
			case MKC_Down:
				i = 0x3D;
				break;
			case MKC_Up:
				i = 0x3E;
				break;
			default:
				/* unchanged */
				break;
		}
		*NextADBkeyevt = (KeyDown ? 0x00 : 0x80) | i;
		return trueblnr;
	}
}

LOCALPROC ADB_DoKeyboardTalk(void)
{
	switch (ADB_CurCmd & 3) {
		case 0:
			{
				ui3b NextADBkeyevt;

				if (CheckForADBkeyEvt(&NextADBkeyevt)) {
					ADB_SzDatBuf = 2;
					ADB_TalkDatBuf = trueblnr;
					ADB_DatBuf[0] = NextADBkeyevt;
					if (! CheckForADBkeyEvt(&NextADBkeyevt)) {
						ADB_DatBuf[1] = 0xFF;
					} else {
						ADB_DatBuf[1] = NextADBkeyevt;
					}
				}
			}
			break;
		case 3:
			ADB_SzDatBuf = 2;
			ADB_TalkDatBuf = trueblnr;
			ADB_DatBuf[0] = 0x60 | (NotSoRandAddr & 0x0f);
			ADB_DatBuf[1] = 0x01;
			NotSoRandAddr += 1;
			break;
		default:
			ReportAbnormalID(0x0D04,
				"Talk to unknown keyboard register");
			break;
	}
}

LOCALPROC ADB_DoKeyboardListen(void)
{
	switch (ADB_CurCmd & 3) {
		case 3:
			if (ADB_DatBuf[1] == 0xFE) {
				/* change address */
				KeyboardADBAddress = (ADB_DatBuf[0] & 0x0F);
			} else {
				ReportAbnormalID(0x0D05,
					"unknown listen op to keyboard register 3");
			}
			break;
		default:
			ReportAbnormalID(0x0D06,
				"listen to unknown keyboard register");
			break;
	}
}

LOCALFUNC blnr CheckForADBanyEvt(void)
{
	MyEvtQEl *p = MyEvtQOutP();
	if (nullpr != p) {
		switch (p->kind) {
			case MyEvtQElKindMouseButton:
			case MyEvtQElKindMouseDelta:
			case MyEvtQElKindKey:
				return trueblnr;
				break;
			default:
				break;
		}
	}

	return (0 != MouseADBDeltaH) && (0 != MouseADBDeltaV);
}

LOCALPROC ADB_DoTalk(void)
{
	ui3b Address = ADB_CurCmd >> 4;
	if (Address == MouseADBAddress) {
		ADB_DoMouseTalk();
	} else if (Address == KeyboardADBAddress) {
		ADB_DoKeyboardTalk();
	}
}

LOCALPROC ADB_EndListen(void)
{
	ui3b Address = ADB_CurCmd >> 4;
	if (Address == MouseADBAddress) {
		ADB_DoMouseListen();
	} else if (Address == KeyboardADBAddress) {
		ADB_DoKeyboardListen();
	}
}

LOCALPROC ADB_DoReset(void)
{
	MouseADBAddress = 3;
	KeyboardADBAddress = 2;
}

LOCALPROC ADB_Flush(void)
{
	ui3b Address = ADB_CurCmd >> 4;

	if ((Address == KeyboardADBAddress)
		|| (Address == MouseADBAddress))
	{
		ADB_SzDatBuf = 2;
		ADB_TalkDatBuf = trueblnr;
		ADB_DatBuf[0] = 0x00;
		ADB_DatBuf[1] = 0x00;
	} else {
		ReportAbnormalID(0x0D07, "Unhandled ADB Flush");
	}
}
