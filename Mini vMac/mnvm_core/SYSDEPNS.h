/*
	SYSDEPNS.h

	Copyright (C) 2006 Bernd Schmidt, Philip Cummins, Paul C. Pratt

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
	SYStem DEPeNdencies.
*/

#ifdef SYSDEPNS_H
#error "header already included"
#else
#define SYSDEPNS_H
#endif

#include "CNFGGLOB.h"


typedef ui3b *ui3p;
typedef ui4b *ui4p;
typedef ui5b *ui5p;

/*
	Largest efficiently supported
	representation types. uimr should be
	large enough to hold number of elements
	of any array we will deal with.
*/
typedef ui5r uimr;
typedef si5r simr;

#define blnr ui3r
#define trueblnr 1
#define falseblnr 0

#define nullpr ((void *) 0)

#define anyp ui3p

/* pascal string, single byte characters */
#define ps3p ui3p

#ifndef MayInline
#define MayInline
#endif

#ifndef MayNotInline
#define MayNotInline
#endif

#ifndef my_reg_call
#define my_reg_call
#endif

#ifndef my_osglu_call
#define my_osglu_call
#endif

#define LOCALVAR static
#ifdef AllFiles
#define GLOBALVAR LOCALVAR
#define EXPORTVAR(t, v)
#else
#define GLOBALVAR
#define EXPORTVAR(t, v) extern t v;
#endif

#define LOCALFUNC static MayNotInline
#define FORWARDFUNC LOCALFUNC
#ifdef AllFiles
#define GLOBALFUNC LOCALFUNC
#define EXPORTFUNC LOCALFUNC
#else
#define GLOBALFUNC MayNotInline
#define EXPORTFUNC extern
#endif
#define IMPORTFUNC EXPORTFUNC
#define TYPEDEFFUNC typedef

#define LOCALPROC LOCALFUNC void
#define GLOBALPROC GLOBALFUNC void
#define EXPORTPROC EXPORTFUNC void
#define IMPORTPROC IMPORTFUNC void
#define FORWARDPROC FORWARDFUNC void
#define TYPEDEFPROC TYPEDEFFUNC void

#define LOCALINLINEFUNC static MayInline
#define LOCALINLINEPROC LOCALINLINEFUNC void

#define LOCALFUNCUSEDONCE LOCALINLINEFUNC
#define LOCALPROCUSEDONCE LOCALINLINEPROC

#define GLOBALOSGLUFUNC GLOBALFUNC my_osglu_call
#define EXPORTOSGLUFUNC EXPORTFUNC my_osglu_call
#define GLOBALOSGLUPROC GLOBALFUNC my_osglu_call void
#define EXPORTOSGLUPROC EXPORTFUNC my_osglu_call void
	/*
		For functions in operating system glue that
		are called by rest of program.
	*/

/*
	best type for ui4r that is probably in register
	(when compiler messes up otherwise)
*/

#ifndef BigEndianUnaligned
#define BigEndianUnaligned 0
#endif

#ifndef LittleEndianUnaligned
#define LittleEndianUnaligned 0
#endif

#ifndef ui3rr
#define ui3rr ui3r
#endif

#ifndef ui4rr
#define ui4rr ui4r
#endif

#ifndef si5rr
#define si5rr si5r
#endif

#ifndef my_align_8
#define my_align_8
#endif

#ifndef my_cond_rare
#define my_cond_rare(x) (x)
#endif

#ifndef Have_ASR
#define Have_ASR 0
#endif

#ifndef HaveMySwapUi5r
#define HaveMySwapUi5r 0
#endif
