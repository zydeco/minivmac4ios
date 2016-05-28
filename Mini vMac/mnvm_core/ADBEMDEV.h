/*
	ADBEMDEV.h

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

#ifdef ADBEMDEV_H
#error "header already included"
#else
#define ADBEMDEV_H
#endif


EXPORTPROC ADBstate_ChangeNtfy(void);
EXPORTPROC ADB_DoNewState(void);
EXPORTPROC ADB_DataLineChngNtfy(void);
EXPORTPROC ADB_Update(void);
