/*
	ADBEMDEV.c

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
	Apple Desktop Bus EMulated DEVice
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#endif

#include "ADBEMDEV.h"

#ifdef _VIA_Debug
#include <stdio.h>
#endif

/*
	ReportAbnormalID unused 0x0C06 - 0x0CFF
*/

IMPORTPROC ADB_ShiftOutData(ui3b v);
IMPORTFUNC ui3b ADB_ShiftInData(void);

#include "ADBSHARE.h"

LOCALVAR blnr ADB_ListenDatBuf;
LOCALVAR ui3b ADB_IndexDatBuf;

GLOBALPROC ADB_DoNewState(void)
{
	ui3b state = ADB_st1 * 2 + ADB_st0;
#ifdef _VIA_Debug
	fprintf(stderr, "ADB_DoNewState: %d\n", state);
#endif
	{
		ADB_Int = 1;
		switch (state) {
			case 0: /* Start a new command */
				if (ADB_ListenDatBuf) {
					ADB_ListenDatBuf = falseblnr;
					ADB_SzDatBuf = ADB_IndexDatBuf;
					ADB_EndListen();
				}
				ADB_TalkDatBuf = falseblnr;
				ADB_IndexDatBuf = 0;
				ADB_CurCmd = ADB_ShiftInData();
					/* which sets interrupt, acknowleding command */
#ifdef _VIA_Debug
				fprintf(stderr, "in: %d\n", ADB_CurCmd);
#endif
				switch ((ADB_CurCmd >> 2) & 3) {
					case 0: /* reserved */
						switch (ADB_CurCmd & 3) {
							case 0: /* Send Reset */
								ADB_DoReset();
								break;
							case 1: /* Flush */
								ADB_Flush();
								break;
							case 2: /* reserved */
							case 3: /* reserved */
								ReportAbnormalID(0x0C01,
									"Reserved ADB command");
								break;
						}
						break;
					case 1: /* reserved */
						ReportAbnormalID(0x0C02,
							"Reserved ADB command");
						break;
					case 2: /* listen */
						ADB_ListenDatBuf = trueblnr;
#ifdef _VIA_Debug
						fprintf(stderr, "*** listening\n");
#endif
						break;
					case 3: /* talk */
						ADB_DoTalk();
						break;
				}
				break;
			case 1: /* Transfer date byte (even) */
			case 2: /* Transfer date byte (odd) */
				if (! ADB_ListenDatBuf) {
					/*
						will get here even if no pending talk data,
						when there is pending event from device
						other than the one polled by the last talk
						command. this probably indicates a bug.
					*/
					if ((! ADB_TalkDatBuf)
						|| (ADB_IndexDatBuf >= ADB_SzDatBuf))
					{
						ADB_ShiftOutData(0xFF);
						ADB_Data = 1;
						ADB_Int = 0;
					} else {
#ifdef _VIA_Debug
						fprintf(stderr, "*** talk one\n");
#endif
						ADB_ShiftOutData(ADB_DatBuf[ADB_IndexDatBuf]);
						ADB_Data = 1;
						ADB_IndexDatBuf += 1;
					}
				} else {
					if (ADB_IndexDatBuf >= ADB_MaxSzDatBuf) {
						ReportAbnormalID(0x0C03, "ADB listen too much");
							/* ADB_MaxSzDatBuf isn't big enough */
						(void) ADB_ShiftInData();
					} else {
#ifdef _VIA_Debug
						fprintf(stderr, "*** listen one\n");
#endif
						ADB_DatBuf[ADB_IndexDatBuf] = ADB_ShiftInData();
						ADB_IndexDatBuf += 1;
					}
				}
				break;
			case 3: /* idle */
				if (ADB_ListenDatBuf) {
					ReportAbnormalID(0x0C04, "ADB idle follows listen");
					/* apparently doesn't happen */
				}
				if (ADB_TalkDatBuf) {
					if (ADB_IndexDatBuf != 0) {
						ReportAbnormalID(0x0C05,
							"idle when not done talking");
					}
					ADB_ShiftOutData(0xFF);
					/* ADB_Int = 0; */
				} else if (CheckForADBanyEvt()) {
					if (((ADB_CurCmd >> 2) & 3) == 3) {
						ADB_DoTalk();
					}
					ADB_ShiftOutData(0xFF);
					/* ADB_Int = 0; */
				}
				break;
		}
	}
}

GLOBALPROC ADBstate_ChangeNtfy(void)
{
#ifdef _VIA_Debug
	fprintf(stderr, "ADBstate_ChangeNtfy: %d, %d, %d\n",
		ADB_st1, ADB_st0, GetCuriCount());
#endif
	ICT_add(kICT_ADB_NewState,
		348160UL * kCycleScale / 64 * kMyClockMult);
		/*
			Macintosh Family Hardware Reference say device "must respond
			to talk command within 260 microseconds", which translates
			to about 190 instructions. But haven't seen much problems
			even for very large values (tens of thousands), and do see
			problems for small values. 50 is definitely too small,
			mouse doesn't move smoothly. There may still be some
			signs of this problem with 150.

			On the other hand, how fast the device must respond may
			not be related to how fast the ADB transceiver responds.
		*/
}

GLOBALPROC ADB_DataLineChngNtfy(void)
{
#ifdef _VIA_Debug
	fprintf(stderr, "ADB_DataLineChngNtfy: %d\n", ADB_Data);
#endif
}

GLOBALPROC ADB_Update(void)
{
	ui3b state = ADB_st1 * 2 + ADB_st0;

	if (state == 3) { /* idle */
		if (ADB_TalkDatBuf) {
			/* ignore, presumably being taken care of */
		} else if (CheckForADBanyEvt())
		{
			if (((ADB_CurCmd >> 2) & 3) == 3) {
				ADB_DoTalk();
			}
			ADB_ShiftOutData(0xFF);
				/*
					Wouldn't expect this would be needed unless
					there is actually talk data. But without it,
					ADB never polls the other devices. Clearing
					ADB_Int has no effect.
				*/
			/*
				ADB_Int = 0;
				seems to have no effect, which probably indicates a bug
			*/
		}
	}
}
