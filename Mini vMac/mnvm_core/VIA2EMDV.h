/*
	VIA2EMDV.h

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

#ifdef VIA2EMDV_H
#error "header already included"
#else
#define VIA2EMDV_H
#endif

EXPORTPROC VIA2_Zap(void);
EXPORTPROC VIA2_Reset(void);

EXPORTFUNC ui5b VIA2_Access(ui5b Data, blnr WriteMem, CPTR addr);

EXPORTPROC VIA2_ExtraTimeBegin(void);
EXPORTPROC VIA2_ExtraTimeEnd(void);
#ifdef VIA2_iCA1_PulseNtfy
EXPORTPROC VIA2_iCA1_PulseNtfy(void);
#endif
#ifdef VIA2_iCA2_PulseNtfy
EXPORTPROC VIA2_iCA2_PulseNtfy(void);
#endif
#ifdef VIA2_iCB1_PulseNtfy
EXPORTPROC VIA2_iCB1_PulseNtfy(void);
#endif
#ifdef VIA2_iCB2_PulseNtfy
EXPORTPROC VIA2_iCB2_PulseNtfy(void);
#endif
EXPORTPROC VIA2_DoTimer1Check(void);
EXPORTPROC VIA2_DoTimer2Check(void);

EXPORTFUNC ui4b VIA2_GetT1InvertTime(void);

EXPORTPROC VIA2_ShiftInData(ui3b v);
EXPORTFUNC ui3b VIA2_ShiftOutData(void);
