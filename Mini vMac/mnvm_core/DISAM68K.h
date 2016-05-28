/*
	DISAM68K.h

	Copyright (C) 2010 Paul C. Pratt

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
	DISAssemble Motorola 68K instructions.
*/

#ifdef DIS1M68K_H
#error "header already included"
#else
#define DIS1M68K_H
#endif

EXPORTPROC DisasmOneOrSave(ui5r pc);

EXPORTPROC m68k_WantDisasmContext(void);
