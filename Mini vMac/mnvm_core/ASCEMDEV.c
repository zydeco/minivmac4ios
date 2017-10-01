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

/*
	ReportAbnormalID unused 0x0F0E, 0x0F1E - 0x0FFF
*/

LOCALVAR ui3r SoundReg801 = 0;
LOCALVAR ui3r SoundReg802 = 0;
LOCALVAR ui3r SoundReg803 = 0;
LOCALVAR ui3r SoundReg804 = 0;
LOCALVAR ui3r SoundReg805 = 0;
LOCALVAR ui3r SoundReg_Volume = 0; /* 0x806 */
/* LOCALVAR ui3r SoundReg807 = 0; */

LOCALVAR ui3b ASC_SampBuff[0x800];

struct ASC_ChanR {
	ui3b freq[4];
	ui3b phase[4];
};
typedef struct ASC_ChanR ASC_ChanR;

LOCALVAR ASC_ChanR ASC_ChanA[4];

LOCALVAR ui4b ASC_FIFO_Out = 0;
LOCALVAR ui4b ASC_FIFO_InA = 0;
LOCALVAR ui4b ASC_FIFO_InB = 0;
LOCALVAR blnr ASC_Playing = falseblnr;

#define ASC_dolog (dbglog_HAVE && 0)

#ifdef ASC_interrupt_PulseNtfy
IMPORTPROC ASC_interrupt_PulseNtfy(void);
#endif

LOCALPROC ASC_RecalcStatus(void)
{
	if ((1 == SoundReg801) && ASC_Playing) {
		if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x200) {
			SoundReg804 &= ~ 0x01;
		} else {
			SoundReg804 |= 0x01;
		}
		if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x400) {
			SoundReg804 |= 0x02;
		} else {
			SoundReg804 &= ~ 0x02;
		}
		if (0 != (SoundReg802 & 2)) {
			if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x200) {
				SoundReg804 &= ~ 0x04;
			} else {
				SoundReg804 |= 0x04;
			}
			if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x400) {
				SoundReg804 |= 0x08;
			} else {
				SoundReg804 &= ~ 0x08;
			}
		}
	}
}

LOCALPROC ASC_ClearFIFO(void)
{
	ASC_FIFO_Out = 0;
	ASC_FIFO_InA = 0;
	ASC_FIFO_InB = 0;
	ASC_Playing = falseblnr;
	ASC_RecalcStatus();
}

GLOBALFUNC ui5b ASC_Access(ui5b Data, blnr WriteMem, CPTR addr)
{
	if (addr < 0x800) {
		if (WriteMem) {
			if (1 == SoundReg801) {
				if (0 == (addr & 0x400)) {
					if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x400)
					{
#if 0 /* seems to happen in tetris */
						ReportAbnormalID(0x0F01,
							"ASC - Channel A Overflow");
#endif
						SoundReg804 |= 0x02;
					} else {

					ASC_SampBuff[ASC_FIFO_InA & 0x3FF] = Data;

					++ASC_FIFO_InA;
					if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x200)
					{
						if (0 != (SoundReg804 & 0x01)) {
							/* happens normally */
							SoundReg804 &= ~ 0x01;
						}
					} else {
#if 0 /* doesn't seem to be necessary, but doesn't hurt either */
						SoundReg804 |= 0x01;
#endif
					}
					if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x400)
					{
						SoundReg804 |= 0x02;
#if ASC_dolog
						dbglog_WriteNote("ASC : setting full flag A");
#endif
					} else {
						if (0 != (SoundReg804 & 0x02)) {
							ReportAbnormalID(0x0F02, "ASC_Access : "
								"full flag A not already clear");
							SoundReg804 &= ~ 0x02;
						}
					}

					}
				} else {
					if (0 == (SoundReg802 & 2)) {
						ReportAbnormalID(0x0F03,
							"ASC - Channel B for Mono");
					}
					if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x400)
					{
						ReportAbnormalID(0x0F04,
							"ASC - Channel B Overflow");
						SoundReg804 |= 0x08;
					} else {

					ASC_SampBuff[0x400 + (ASC_FIFO_InB & 0x3FF)] = Data;

					++ASC_FIFO_InB;
					if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x200)
					{
						if (0 != (SoundReg804 & 0x04)) {
							/* happens normally */
							SoundReg804 &= ~ 0x04;
						}
					} else {
#if 0 /* doesn't seem to be necessary, but doesn't hurt either */
						SoundReg804 |= 0x04;
#endif
					}
					if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x400)
					{
						SoundReg804 |= 0x08;
#if ASC_dolog
						dbglog_WriteNote("ASC : setting full flag B");
#endif
					} else {
						if (0 != (SoundReg804 & 0x08)) {
							ReportAbnormalID(0x0F05, "ASC_Access : "
								"full flag B not already clear");
							SoundReg804 &= ~ 0x08;
						}
					}

					}
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
#if 0
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
			case 0x800: /* VERSION */
				if (WriteMem) {
					ReportAbnormalID(0x0F06, "ASC - writing VERSION");
				} else {
					Data = 0;
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (VERSION)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x801: /* ENABLE */
				if (WriteMem) {
					if (1 == Data) {
						if (1 != SoundReg801) {
							ASC_ClearFIFO();
						}
					} else {
						if (Data > 2) {
							ReportAbnormalID(0x0F07,
								"ASC - unexpected ENABLE");
						}
					}
					SoundReg801 = Data;
				} else {
					Data = SoundReg801;
					/* happens in LodeRunner */
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (ENABLE)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x802: /* CONTROL */
				if (WriteMem) {
#if 1
					if (0 != SoundReg801) {
						if (SoundReg802 == Data) {
							/*
								this happens normally,
								such as in Lunar Phantom
							*/
						} else {
							if (1 == SoundReg801) {
/*
	happens in dark castle, if play other sound first,
	such as by changing beep sound in sound control panel.
*/
								ASC_ClearFIFO();
							}

#if 0
							ReportAbnormalID(0x0F08,
								"ASC - changing CONTROL while ENABLEd");
#endif
						}
					}
#endif
					if (0 != (Data & ~ 2)) {
						ReportAbnormalID(0x0F09,
							"ASC - unexpected CONTROL value");
					}
					SoundReg802 = Data;
				} else {
					Data = SoundReg802;
					ReportAbnormalID(0x0F0A,
						"ASC - reading CONTROL value");
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (CONTROL)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x803:
				if (WriteMem) {
					if (0 != (Data & ~ 0x80)) {
						ReportAbnormalID(0x0F0B,
							"ASC - unexpected FIFO MODE");
					}
					if (0 != (Data & 0x80)) {
						if (0 != (SoundReg803 & 0x80)) {
							ReportAbnormalID(0x0F0C,
								"ASC - set clear FIFO again");
						} else
						if (1 != SoundReg801) {
#if 0 /* happens in system 6, such as with Lunar Phantom */
							ReportAbnormalID(0x0F0D,
								"ASC - clear FIFO when not FIFO mode");
#endif
						} else
						{
							ASC_ClearFIFO();
							/*
								ASC_interrupt_PulseNtfy();
									Doesn't seem to be needed,
									but doesn't hurt either.
							*/
						}
					}
					SoundReg803 = Data;
				} else {
					Data = SoundReg803;
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (FIFO MODE)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x804:
				if (WriteMem) {
#if 0
					if ((0 != SoundReg804) && (0 != Data)) {
						ReportAbnormalID(0x0F0F,
							"ASC - set FIFO IRQ STATUS when not 0");
					}
#endif
					SoundReg804 = Data;
					if (0 != SoundReg804) {
						ASC_interrupt_PulseNtfy();
						/*
							Generating this interrupt seems
							to be the point of writing to
							this register.
						*/
					}
#if ASC_dolog && 1
					dbglog_AddrAccess(
						"ASC_Access Control (FIFO IRQ STATUS)",
						Data, WriteMem, addr);
#endif
				} else {
					Data = SoundReg804;
#if 0
					if (1 != SoundReg801) {
						/* no, ok, part of normal interrupt handling */
						ReportAbnormalID(0x0F10,
							"ASC - read STATUS when not FIFO");
					}
#endif
					/* SoundReg804 = 0; */
					SoundReg804 &= ~ 0x01;
					SoundReg804 &= ~ 0x04;
						/*
							In lunar phantom, observe checking
							full flag before first write, but
							status was read previous.
						*/
#if ASC_dolog && 1
#if 0
					if (0 != Data)
#endif
					{
						dbglog_AddrAccess(
							"ASC_Access Control (FIFO IRQ STATUS)",
							Data, WriteMem, addr);
					}
#endif
				}
				break;
			case 0x805:
				if (WriteMem) {
					SoundReg805 = Data;
					/* cleared in LodeRunner */
				} else {
					Data = SoundReg805;
					ReportAbnormalID(0x0F11,
						"ASC - reading WAVE CONTROL register");
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (WAVE CONTROL)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x806: /* VOLUME */
				if (WriteMem) {
					SoundReg_Volume = Data >> 5;
					if (0 != (Data & 0x1F)) {
						ReportAbnormalID(0x0F12,
							"ASC - unexpected volume value");
					}
				} else {
					Data = SoundReg_Volume << 5;
					ReportAbnormalID(0x0F13,
						"ASC - reading volume register");
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (VOLUME)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x807: /* CLOCK RATE */
				if (WriteMem) {
					/* SoundReg807 = Data; */
					if (0 != Data) {
						ReportAbnormalID(0x0F14,
							"ASC - nonstandard CLOCK RATE");
					}
				} else {
					/* Data = SoundReg807; */
					ReportAbnormalID(0x0F15,
						"ASC - reading CLOCK RATE");
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (CLOCK RATE)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x808: /* CONTROL */
				if (WriteMem) {
					ReportAbnormalID(0x0F16, "ASC - write to 808");
				} else {
					/* happens on boot System 7.5.5 */
					Data = 0;
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (CONTROL)",
					Data, WriteMem, addr);
#endif
				break;
			case 0x80A: /* ? */
				if (WriteMem) {
					ReportAbnormalID(0x0F17, "ASC - write to 80A");
				} else {
					/*
						happens in system 6, Lunar Phantom,
							soon after new game.
					*/
					Data = 0;
				}
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (80A)",
					Data, WriteMem, addr);
#endif
				break;
			default:
				if (WriteMem) {
				} else {
					Data = 0;
				}
				ReportAbnormalID(0x0F18, "ASC - unknown ASC reg");
#if ASC_dolog && 1
				dbglog_AddrAccess("ASC_Access Control (?)",
					Data, WriteMem, addr);
#endif
				break;
		}
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
			dbglog_AddrAccess("ASC_Access Control (frequency)",
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

			if (WriteMem) {
				ASC_ChanA[chan].phase[b] = Data;
			} else {
				Data = ASC_ChanA[chan].phase[b];
			}
#if ASC_dolog && 1
			dbglog_AddrAccess("ASC_Access Control (phase)",
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

		ReportAbnormalID(0x0F19, "unknown ASC reg");
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

LOCALVAR const ui3r SubTick_n[kNumSubTicks] = {
	23,  23,  23,  23,  23,  23,  23,  24,
	23,  23,  23,  23,  23,  23,  23,  24
};

GLOBALPROC ASC_SubTick(int SubTick)
{
	ui4r actL;
#if MySoundEnabled
	tpSoundSamp p;
#endif
	ui4r i;
	ui4r n = SubTick_n[SubTick];
#if MySoundEnabled
	ui3b SoundVolume = SoundReg_Volume;
#endif

#if MySoundEnabled
label_retry:
	p = MySound_BeginWrite(n, &actL);
#else
	actL = n;
#endif
	if (actL > 0) {

		if (1 == SoundReg801) {
#if MySoundEnabled
			ui3p addr;
#endif

			if (0 != (SoundReg802 & 2)) {

			if (! ASC_Playing) {
				if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x200) {
					if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x200)
					{
						SoundReg804 &= ~ 0x01;
						SoundReg804 &= ~ 0x04;
						ASC_Playing = trueblnr;
#if ASC_dolog
						dbglog_WriteNote("ASC : start stereo playing");
#endif
					} else {
						if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) == 0)
						if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out))
							>= 370)
						{
#if ASC_dolog
							dbglog_WriteNote("ASC : switch to mono");
#endif
							SoundReg802 &= ~ 2;
							/*
								cludge to get Tetris to work,
								may not actually work on real machine.
							*/
						}
					}
				}
			}

			for (i = 0; i < actL; i++) {
				if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) == 0) {
					ASC_Playing = falseblnr;
				}
				if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) == 0) {
					ASC_Playing = falseblnr;
				}
				if (! ASC_Playing) {
#if MySoundEnabled
					*p++ = 0x80;
#endif
				} else
				{

#if MySoundEnabled
				addr = ASC_SampBuff + (ASC_FIFO_Out & 0x3FF);

#if ASC_dolog && 1
				dbglog_StartLine();
				dbglog_writeCStr("out sound ");
				dbglog_writeCStr("[");
				dbglog_writeHex(ASC_FIFO_Out);
				dbglog_writeCStr("]");
				dbglog_writeCStr(" = ");
				dbglog_writeHex(*addr);
				dbglog_writeCStr(" , ");
				dbglog_writeHex(addr[0x400]);
				dbglog_writeReturn();
#endif

				*p++ = ((addr[0] + addr[0x400])
#if 4 == kLn2SoundSampSz
					<< 8
#endif
					) >> 1;
#endif /* MySoundEnabled */

				ASC_FIFO_Out += 1;

				}
			}

			} else {

			/* mono */

			if (! ASC_Playing) {
				if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x200)
				{
					SoundReg804 &= ~ 0x01;
					ASC_Playing = trueblnr;
#if ASC_dolog
					dbglog_WriteNote("ASC : start mono playing");
#endif
				}
			}

			for (i = 0; i < actL; i++) {
				if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) == 0) {
					ASC_Playing = falseblnr;
				}
				if (! ASC_Playing) {
#if MySoundEnabled
					*p++ = 0x80;
#endif
				} else
				{

#if MySoundEnabled
				addr = ASC_SampBuff + (ASC_FIFO_Out & 0x3FF);

#if ASC_dolog && 1
				dbglog_StartLine();
				dbglog_writeCStr("out sound ");
				dbglog_writeCStr("[");
				dbglog_writeHex(ASC_FIFO_Out);
				dbglog_writeCStr("]");
				dbglog_writeCStr(" = ");
				dbglog_writeHex(*addr);
				dbglog_writeCStr(", in buff: ");
				dbglog_writeHex((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out));
				dbglog_writeReturn();
#endif

				*p++ = (addr[0])
#if 4 == kLn2SoundSampSz
					<< 8
#endif
					;
#endif /* MySoundEnabled */

				/* Move the address on */
				/* *addr = 0x80; */
				/* addr += 2; */
				ASC_FIFO_Out += 1;

				}
			}

			}
		} else if (2 == SoundReg801) {
#if MySoundEnabled
			ui4r v;
			ui4r i0;
			ui4r i1;
			ui4r i2;
			ui4r i3;
#endif
			ui5r freq0 = do_get_mem_long(ASC_ChanA[0].freq);
			ui5r freq1 = do_get_mem_long(ASC_ChanA[1].freq);
			ui5r freq2 = do_get_mem_long(ASC_ChanA[2].freq);
			ui5r freq3 = do_get_mem_long(ASC_ChanA[3].freq);
			ui5r phase0 = do_get_mem_long(ASC_ChanA[0].phase);
			ui5r phase1 = do_get_mem_long(ASC_ChanA[1].phase);
			ui5r phase2 = do_get_mem_long(ASC_ChanA[2].phase);
			ui5r phase3 = do_get_mem_long(ASC_ChanA[3].phase);
#if ASC_dolog && 1
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

				phase0 += freq0;
				phase1 += freq1;
				phase2 += freq2;
				phase3 += freq3;

#if MySoundEnabled

#if 1
				i0 = ((phase0 + 0x4000) >> 15) & 0x1FF;
				i1 = ((phase1 + 0x4000) >> 15) & 0x1FF;
				i2 = ((phase2 + 0x4000) >> 15) & 0x1FF;
				i3 = ((phase3 + 0x4000) >> 15) & 0x1FF;
#else
				i0 = ((phase0 + 0x8000) >> 16) & 0x1FF;
				i1 = ((phase1 + 0x8000) >> 16) & 0x1FF;
				i2 = ((phase2 + 0x8000) >> 16) & 0x1FF;
				i3 = ((phase3 + 0x8000) >> 16) & 0x1FF;
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
#endif /* MySoundEnabled */
			}

			do_put_mem_long(ASC_ChanA[0].phase, phase0);
			do_put_mem_long(ASC_ChanA[1].phase, phase1);
			do_put_mem_long(ASC_ChanA[2].phase, phase2);
			do_put_mem_long(ASC_ChanA[3].phase, phase3);
		} else {
#if MySoundEnabled
			for (i = 0; i < actL; i++) {
				*p++ = kCenterSound;
			}
#endif
		}


#if MySoundEnabled
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
		if (n > 0) {
			goto label_retry;
		}
#endif
	}

#if 1
	if ((1 == SoundReg801) && ASC_Playing) {
		if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x200) {
			if (0 != (SoundReg804 & 0x01)) {
				ReportAbnormalID(0x0F1A,
					"half flag A not already clear");
				SoundReg804 &= ~ 0x01;
			}
		} else {
			if (0 != (SoundReg804 & 0x01)) {
				/* happens in lode runner */
			} else {
#if ASC_dolog
				dbglog_WriteNote("setting half flag A");
#endif
				ASC_interrupt_PulseNtfy();
				SoundReg804 |= 0x01;
			}
		}
		if (((ui4b)(ASC_FIFO_InA - ASC_FIFO_Out)) >= 0x400) {
			if (0 == (SoundReg804 & 0x02)) {
				ReportAbnormalID(0x0F1B, "full flag A not already set");
				SoundReg804 |= 0x02;
			}
		} else {
			if (0 != (SoundReg804 & 0x02)) {
				/* ReportAbnormal("full flag A not already clear"); */
				SoundReg804 &= ~ 0x02;
			}
		}
		if (0 != (SoundReg802 & 2)) {
			if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x200) {
				if (0 != (SoundReg804 & 0x04)) {
					ReportAbnormalID(0x0F1C,
						"half flag B not already clear");
					SoundReg804 &= ~ 0x04;
				}
			} else {
				if (0 != (SoundReg804 & 0x04)) {
					/* happens in Lunar Phantom */
				} else {
#if ASC_dolog
					dbglog_WriteNote("setting half flag B");
#endif
					ASC_interrupt_PulseNtfy();
					SoundReg804 |= 0x04;
				}
			}
			if (((ui4b)(ASC_FIFO_InB - ASC_FIFO_Out)) >= 0x400) {
				if (0 == (SoundReg804 & 0x08)) {
					ReportAbnormalID(0x0F1D,
						"full flag B not already set");
					SoundReg804 |= 0x08;
				}
			} else {
				if (0 != (SoundReg804 & 0x08)) {
					/*
						ReportAbnormal("full flag B not already clear");
					*/
					SoundReg804 &= ~ 0x08;
				}
			}
		}
	}
#endif
}
