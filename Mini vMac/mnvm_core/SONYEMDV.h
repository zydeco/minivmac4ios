/*
	SONYEMDV.h

	Copyright (C) 2004 Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#ifdef SONYEMDV_H
#error "header already included"
#else
#define SONYEMDV_H
#endif

EXPORTPROC ExtnDisk_Access(CPTR p);
EXPORTPROC ExtnSony_Access(CPTR p);

EXPORTPROC Sony_SetQuitOnEject(void);

EXPORTPROC Sony_EjectAllDisks(void);
EXPORTPROC Sony_Reset(void);

EXPORTPROC Sony_Update(void);
