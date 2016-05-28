/*
	SCRNEMDV.c

	Copyright (C) 2006 Philip Cummins, Richard F. Bannister,
		Paul C. Pratt

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
	SCReeN EMulated DeVice

	Emulation of the screen in the Mac Plus.

	This code descended from "Screen-MacOS.c" in Richard F. Bannister's
	Macintosh port of vMac, by Philip Cummins.
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"
#include "ENDIANAC.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#endif

#include "SCRNEMDV.h"

#if ! IncludeVidMem
#define kMain_Offset      0x5900
#define kAlternate_Offset 0xD900
#define kMain_Buffer      (kRAM_Size - kMain_Offset)
#define kAlternate_Buffer (kRAM_Size - kAlternate_Offset)
#endif

GLOBALPROC Screen_EndTickNotify(void)
{
	ui3p screencurrentbuff;

#if IncludeVidMem
	screencurrentbuff = VidMem;
#else
	if (SCRNvPage2 == 1) {
		screencurrentbuff = get_ram_address(kMain_Buffer);
	} else {
		screencurrentbuff = get_ram_address(kAlternate_Buffer);
	}
#endif

	Screen_OutputFrame(screencurrentbuff);
}
