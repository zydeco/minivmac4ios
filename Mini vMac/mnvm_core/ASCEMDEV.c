/*
	ASCEMDEV.c

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
	Apple Sound Chip EMulated DEVice
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#include "ENDIANAC.h"
#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#include "VIAEMDEV.h"
#endif

#include "ASCEMDEV.h"

LOCALVAR ui3r SoundReg801 = 0;
LOCALVAR ui3r SoundReg802 = 0;
LOCALVAR ui3r SoundReg803 = 0;
LOCALVAR ui3r SoundReg804 = 0;
LOCALVAR ui3r SoundReg805 = 0;
LOCALVAR ui3r SoundReg_Volume = 0; /* 0x806 */
LOCALVAR ui3r SoundReg807 = 0;

LOCALVAR ui4r ASC_InputIndex = 0;

LOCALVAR ui3b ASC_SampBuff[0x800];

struct ASC_ChanR {
	ui3b freq[0x800];
	ui5r phase;
};
typedef struct ASC_ChanR ASC_ChanR;

LOCALVAR ASC_ChanR ASC_ChanA[4];

#define ASC_dolog (dbglog_HAVE && 0)

GLOBALFUNC ui5b ASC_Access(ui5b Data, blnr WriteMem, CPTR addr)
{
	if (addr < 0x800) {
		if (WriteMem) {
			if ((1 == SoundReg801) && (2 == SoundReg802)) {
				if ((2 * 370) == (ASC_InputIndex & 0x3FF)) {
					SoundReg804 |= 0x08;
				} else {
					ui4r j = (addr & 0x400) | (ASC_InputIndex & 0x3FF);
					++ASC_InputIndex;
					ASC_SampBuff[j] = Data;
#if ASC_dolog && 0
					dbglog_AddrAccess("ASC_Access SampBuff.wrap",
						Data, WriteMem, j);
#endif
#if 1
					if ((2 * 370) == (ASC_InputIndex & 0x3FF)) {
						SoundReg804 |= 0x08;
					}
#endif
				}
#if ASC_dolog && 0
				dbglog_writeCStr("ASC_InputIndex =");
				dbglog_writeNum(ASC_InputIndex);
				dbglog_writeReturn();
#endif
			} else {
				ASC_SampBuff[addr] = Data;
			}
		} else {
			Data = ASC_SampBuff[addr];
		}

#if ASC_dolog && 1
#if 1
		if (((addr & 0x1FF) >= 0x04)
			&& ((addr & 0x1FF) < (0x200 - 0x04)))
		{
			/* don't report them all */
		} else
#endif
		{
			dbglog_AddrAccess("ASC_Access SampBuff",
				Data, WriteMem, addr);
		}
#endif
	} else if (addr < 0x810) {
		switch (addr) {
			case 0x800: /* CONTROL */
				if (WriteMem) {
				} else {
					Data = 0;
				}
				break;
			case 0x801: /* ENABLE */
				if (WriteMem) {
					SoundReg801 = Data;
				} else {
					Data = SoundReg801;
				}
				break;
			case 0x802: /* MODE */
				if (WriteMem) {
					SoundReg802 = Data;
				} else {
					Data = SoundReg802;
				}
				break;
			case 0x803:
				if (WriteMem) {
					SoundReg803 = Data;
				} else {
					Data = SoundReg803;
				}
				break;
			case 0x804:
				if (WriteMem) {
					SoundReg804 = Data;
				} else {
					Data = SoundReg804;
					SoundReg804 = 0;
				}
				break;
			case 0x805:
				if (WriteMem) {
					SoundReg805 = Data;
				} else {
					Data = SoundReg805;
				}
				break;
			case 0x806: /* VOLUME */
				if (WriteMem) {
					SoundReg_Volume = Data >> 5;
					if (0 != (Data & 0x1F)) {
						ReportAbnormal("ASC - unexpected volume value");
					}
				} else {
					Data = SoundReg_Volume << 5;
					ReportAbnormal("ASC - reading volume register");
				}
				break;
			case 0x807: /* CHAN */
				if (WriteMem) {
					SoundReg807 = Data;
				} else {
					Data = SoundReg807;
				}
				break;
			default:
				if (WriteMem) {
				} else {
					Data = 0;
				}
				break;
		}
#if ASC_dolog && 1
		if (addr != 0x804) {
			dbglog_AddrAccess("ASC_Access Control",
				Data, WriteMem, addr);
		}
#endif
	} else if (addr < 0x830) {
		ui3r b = addr & 3;
		ui3r chan = ((addr - 0x810) >> 3) & 3;

		if (0 != (addr & 4)) {

			if (WriteMem) {
				ASC_ChanA[chan].freq[b] = Data;
			} else {
				Data = ASC_ChanA[chan].freq[b];
			}
#if ASC_dolog && 1
			dbglog_AddrAccess("ASC_Access Control",
				Data, WriteMem, addr);
#endif
#if ASC_dolog && 0
			dbglog_writeCStr("freq b=");
			dbglog_writeNum(WriteMem);
			dbglog_writeCStr(", chan=");
			dbglog_writeNum(chan);
			dbglog_writeReturn();
#endif
		} else {
#if ASC_dolog && 1
			dbglog_AddrAccess("ASC_Access Control *** unknown reg",
				Data, WriteMem, addr);
#endif
		}
	} else if (addr < 0x838) {
#if ASC_dolog && 1
		dbglog_AddrAccess("ASC_Access Control *** unknown reg",
			Data, WriteMem, addr);
#endif
	} else {
#if ASC_dolog && 1
		dbglog_AddrAccess("ASC_Access Control ? *** unknown reg",
			Data, WriteMem, addr);
#endif

		ReportAbnormal("unknown ASC reg");
	}

	return Data;
}

/*
	approximate volume levels of vMac, so:

	x * vol_mult[SoundVolume] >> 16
		+ vol_offset[SoundVolume]
	= {approx} (x - kCenterSound) / (8 - SoundVolume) + kCenterSound;
*/

LOCALVAR const ui4b vol_mult[] = {
	8192, 9362, 10922, 13107, 16384, 21845, 32768
};

LOCALVAR const trSoundSamp vol_offset[] = {
#if 3 == kLn2SoundSampSz
	112, 110, 107, 103, 96, 86, 64, 0
#elif 4 == kLn2SoundSampSz
	28672, 28087, 27307, 26215, 24576, 21846, 16384, 0
#else
#error "unsupported kLn2SoundSampSz"
#endif
};

LOCALVAR const ui4b SubTick_offset[kNumSubTicks] = {
	0,    23,  46,  69,  92, 115, 138, 161,
	185, 208, 231, 254, 277, 300, 323, 346
};

LOCALVAR const ui3r SubTick_n[kNumSubTicks] = {
	23,  23,  23,  23,  23,  23,  23,  24,
	23,  23,  23,  23,  23,  23,  23,  24
};

#if MySoundEnabled
LOCALVAR ui5b SoundPhase = 0;
#endif

#ifdef ASC_interrupt_PulseNtfy
IMPORTPROC ASC_interrupt_PulseNtfy(void);
#endif

#if MySoundEnabled
GLOBALPROC MacSound_SubTick(int SubTick)
{
	ui4r actL;
	tpSoundSamp p;
	ui4r i;
	ui4r j = 0;
	ui4r n = SubTick_n[SubTick];
	ui3b SoundVolume = SoundReg_Volume;

label_retry:
	p = MySound_BeginWrite(n, &actL);
	if (actL > 0) {

		if (1 == SoundReg801) {
			ui5b StartOffset = SubTick_offset[SubTick] + j;
			ui3p addr = ASC_SampBuff + (2 * StartOffset);

			if (2 == SoundReg802) {
				addr += /* 0x400 */ 1;
			}

			for (i = 0; i < actL; i++) {
				/* Copy sound data, high byte of each word */
#if ASC_dolog && 1
				dbglog_StartLine();
				dbglog_writeCStr("out sound ");
				dbglog_writeCStr("[");
				dbglog_writeHex(StartOffset + i);
				dbglog_writeCStr("]");
				dbglog_writeCStr(" = ");
				dbglog_writeHex(*addr);
				dbglog_writeReturn();
#endif
				*p++ = *addr
#if 4 == kLn2SoundSampSz
					<< 8
#endif
					;

				/* Move the address on */
				*addr = 0x80;
				addr += 2;
			}
		} else if (2 == SoundReg801) {
			ui4r v;
			ui4r i0;
			ui4r i1;
			ui4r i2;
			ui4r i3;
			ui5r freq0 = do_get_mem_long(ASC_ChanA[0].freq);
			ui5r freq1 = do_get_mem_long(ASC_ChanA[1].freq);
			ui5r freq2 = do_get_mem_long(ASC_ChanA[2].freq);
			ui5r freq3 = do_get_mem_long(ASC_ChanA[3].freq);
#if ASC_dolog && 0
			dbglog_writeCStr("freq0=");
			dbglog_writeNum(freq0);
			dbglog_writeCStr(", freq1=");
			dbglog_writeNum(freq1);
			dbglog_writeCStr(", freq2=");
			dbglog_writeNum(freq2);
			dbglog_writeCStr(", freq3=");
			dbglog_writeNum(freq3);
			dbglog_writeReturn();
#endif
			for (i = 0; i < actL; i++) {

				ASC_ChanA[0].phase += freq0;
				ASC_ChanA[1].phase += freq1;
				ASC_ChanA[2].phase += freq2;
				ASC_ChanA[3].phase += freq3;

#if 1
				i0 = ((ASC_ChanA[0].phase + 0x4000) >> 15) & 0x1FF;
				i1 = ((ASC_ChanA[1].phase + 0x4000) >> 15) & 0x1FF;
				i2 = ((ASC_ChanA[2].phase + 0x4000) >> 15) & 0x1FF;
				i3 = ((ASC_ChanA[3].phase + 0x4000) >> 15) & 0x1FF;
#else
				i0 = ((ASC_ChanA[0].phase + 0x8000) >> 16) & 0x1FF;
				i1 = ((ASC_ChanA[1].phase + 0x8000) >> 16) & 0x1FF;
				i2 = ((ASC_ChanA[2].phase + 0x8000) >> 16) & 0x1FF;
				i3 = ((ASC_ChanA[3].phase + 0x8000) >> 16) & 0x1FF;
#endif

				v = ASC_SampBuff[i0]
					+ ASC_SampBuff[0x0200 + i1]
					+ ASC_SampBuff[0x0400 + i2]
					+ ASC_SampBuff[0x0600 + i3];
#if ASC_dolog && 1
				dbglog_StartLine();
				dbglog_writeCStr("i0=");
				dbglog_writeNum(i0);
				dbglog_writeCStr(", i1=");
				dbglog_writeNum(i1);
				dbglog_writeCStr(", i2=");
				dbglog_writeNum(i2);
				dbglog_writeCStr(", i3=");
				dbglog_writeNum(i3);
				dbglog_writeCStr(", output sound v=");
				dbglog_writeNum(v);
				dbglog_writeReturn();
#endif

				*p++ = (v >> 2);

				++SoundPhase;
				SoundPhase &= 0x1FF;
			}
		} else {
			for (i = 0; i < actL; i++) {
				*p++ = kCenterSound;
			}
		}


		if (SoundVolume < 7) {
			/*
				Usually have volume at 7, so this
				is just for completeness.
			*/
			ui5b mult = (ui5b)vol_mult[SoundVolume];
			trSoundSamp offset = vol_offset[SoundVolume];

			p -= actL;
			for (i = 0; i < actL; i++) {
				*p = (trSoundSamp)((ui5b)(*p) * mult >> 16) + offset;
				++p;
			}
		}

		MySound_EndWrite(actL);
		n -= actL;
		j += actL;
		if (n > 0) {
			goto label_retry;
		}
	}
}
#endif

GLOBALPROC ASC_Update(void)
{
	if (1 == SoundReg801) {
		ASC_InputIndex = 0;
		SoundReg804 |= 0x04;
#ifdef ASC_interrupt_PulseNtfy
		ASC_interrupt_PulseNtfy();
#endif
#if ASC_dolog && 1
		dbglog_StartLine();
		dbglog_writeCStr("called ASC_interrupt_PulseNtfy");
		dbglog_writeReturn();
#endif
	}
}
