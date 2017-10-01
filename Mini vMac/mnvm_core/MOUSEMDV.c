/*
	MOUSEMDV.c

	Copyright (C) 2006 Philip Cummins, Paul C. Pratt

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
	MOUSe EMulated DeVice

	Emulation of the mouse in the Mac Plus.

	This code descended from "Mouse-MacOS.c" in Richard F. Bannister's
	Macintosh port of vMac, by Philip Cummins.
*/

#ifndef AllFiles
#include "SYSDEPNS.h"
#include "MYOSGLUE.h"
#include "ENDIANAC.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#include "SCCEMDEV.h"
#include "MINEM68K.h"
#endif

#include "MOUSEMDV.h"

GLOBALPROC Mouse_Update(void)
{
#if HaveMasterMyEvtQLock
	if (0 != MasterMyEvtQLock) {
		--MasterMyEvtQLock;
	}
#endif

	/*
		Check mouse position first. After mouse button or key event,
		can't process another mouse position until following tick,
		otherwise button or key will be in wrong place.
	*/

	/*
		if start doing this too soon after boot,
		will mess up memory check
	*/
	if (Mouse_Enabled()) {
		MyEvtQEl *p;

		if (
#if HaveMasterMyEvtQLock
			(0 == MasterMyEvtQLock) &&
#endif
			(nullpr != (p = MyEvtQOutP())))
		{
#if EmClassicKbrd
#if EnableMouseMotion
			if (MyEvtQElKindMouseDelta == p->kind) {

				if ((p->u.pos.h != 0) || (p->u.pos.v != 0)) {
					put_ram_word(0x0828,
						get_ram_word(0x0828) + p->u.pos.v);
					put_ram_word(0x082A,
						get_ram_word(0x082A) + p->u.pos.h);
					put_ram_byte(0x08CE, get_ram_byte(0x08CF));
						/* Tell MacOS to redraw the Mouse */
				}
				MyEvtQOutDone();
			} else
#endif
#endif
			if (MyEvtQElKindMousePos == p->kind) {
				ui5r NewMouse = (p->u.pos.v << 16) | p->u.pos.h;

				if (get_ram_long(0x0828) != NewMouse) {
					put_ram_long(0x0828, NewMouse);
						/* Set Mouse Position */
					put_ram_long(0x082C, NewMouse);
#if EmClassicKbrd
					put_ram_byte(0x08CE, get_ram_byte(0x08CF));
						/* Tell MacOS to redraw the Mouse */
#else
					put_ram_long(0x0830, NewMouse);
					put_ram_byte(0x08CE, 0xFF);
						/* Tell MacOS to redraw the Mouse */
#endif
				}
				MyEvtQOutDone();
			}
		}
	}

#if EmClassicKbrd
	{
		MyEvtQEl *p;

		if (
#if HaveMasterMyEvtQLock
			(0 == MasterMyEvtQLock) &&
#endif
			(nullpr != (p = MyEvtQOutP())))
		{
			if (MyEvtQElKindMouseButton == p->kind) {
				MouseBtnUp = p->u.press.down ? 0 : 1;
				MyEvtQOutDone();
				MasterMyEvtQLock = 4;
			}
		}
	}
#endif
}

GLOBALPROC Mouse_EndTickNotify(void)
{
	if (Mouse_Enabled()) {
		/* tell platform specific code where the mouse went */
		CurMouseV = get_ram_word(0x082C);
		CurMouseH = get_ram_word(0x082E);
	}
}
