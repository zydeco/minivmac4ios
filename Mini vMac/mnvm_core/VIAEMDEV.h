/*
	VIAEMDEV.h

	Copyright (C) 2004 Philip Cummins, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#ifdef VIAEMDEV_H
#error "header already included"
#else
#define VIAEMDEV_H
#endif

EXPORTPROC VIA1_Zap(void);
EXPORTPROC VIA1_Reset(void);

EXPORTFUNC ui5b VIA1_Access(ui5b Data, blnr WriteMem, CPTR addr);

EXPORTPROC VIA1_ExtraTimeBegin(void);
EXPORTPROC VIA1_ExtraTimeEnd(void);
#ifdef VIA1_iCA1_PulseNtfy
EXPORTPROC VIA1_iCA1_PulseNtfy(void);
#endif
#ifdef VIA1_iCA2_PulseNtfy
EXPORTPROC VIA1_iCA2_PulseNtfy(void);
#endif
#ifdef VIA1_iCB1_PulseNtfy
EXPORTPROC VIA1_iCB1_PulseNtfy(void);
#endif
#ifdef VIA1_iCB2_PulseNtfy
EXPORTPROC VIA1_iCB2_PulseNtfy(void);
#endif
EXPORTPROC VIA1_DoTimer1Check(void);
EXPORTPROC VIA1_DoTimer2Check(void);

EXPORTFUNC ui4b VIA1_GetT1InvertTime(void);

EXPORTPROC VIA1_ShiftInData(ui3b v);
EXPORTFUNC ui3b VIA1_ShiftOutData(void);
