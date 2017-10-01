/*
	ENDIANAC.h

	Copyright (C) 2006 Bernd Schmidt, Paul C. Pratt

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
	ENDIAN ACcess

	Deals with endian issues in memory access.

	This code is adapted from code in the Un*x Amiga Emulator by
	Bernd Schmidt, as found in vMac by Philip Cummins.
*/

#ifdef ENDIANAC_H
#ifndef AllFiles
#error "header already included"
#endif
#else
#define ENDIANAC_H
#endif


#define do_get_mem_byte(a) ((ui3r)*((ui3b *)(a)))

#if BigEndianUnaligned
#define do_get_mem_word(a) ((ui4r)*((ui4b *)(a)))
#else
LOCALINLINEFUNC ui4r do_get_mem_word(ui3p a)
{
#if LittleEndianUnaligned
	ui4b b = (*((ui4b *)(a)));

	return ((b & 0x00FF) << 8) | ((b >> 8) & 0x00FF);
#else
	return (((ui4r)*a) << 8) | ((ui4r)*(a + 1));
#endif
}
#endif

#if BigEndianUnaligned
#define do_get_mem_long(a) ((ui5r)*((ui5b *)(a)))
#elif HaveMySwapUi5r && LittleEndianUnaligned
#define do_get_mem_long(a) (MySwapUi5r((ui5r)*((ui5b *)(a))))
#else
LOCALINLINEFUNC ui5r do_get_mem_long(ui3p a)
{
#if LittleEndianUnaligned
#if 0
	ui5b b = (*((ui5b *)(a)));
	return ((b & 0x000000FF) << 24)
		|  ((b & 0x0000FF00) <<  8)
		|  ((b & 0x00FF0000) >>  8)
		|  ((b & 0xFF000000) >> 24);
#endif
#if 0
	ui5b b = (*((ui5b *)(a)));
	return ((b << 24) & 0xFF000000)
		|  ((b <<  8) & 0x00FF0000)
		|  ((b >>  8) & 0x0000FF00)
		|  ((b >> 24) & 0x000000FF);
	/*
		no, this doesn't do well with apple tools,
		instead try combining two 16 bit swaps.
	*/
#endif
	ui5b b = (*((ui5b *)(a)));
	ui4b b1 = b;
	ui4b b2 = b >> 16;
	ui4b c1 = ((b1 & 0x00FF) << 8) | ((b1 >> 8) & 0x00FF);
	ui4b c2 = ((b2 & 0x00FF) << 8) | ((b2 >> 8) & 0x00FF);

	return (((ui5r)c1) << 16) | ((ui5r)c2);
	/*
		better, though still doesn't use BSWAP
		instruction with apple tools for intel.
	*/
#else
	return (((ui5r)*a) << 24) | (((ui5r)*(a + 1)) << 16)
		| (((ui5r)*(a + 2)) << 8) | ((ui5r)*(a + 3));
#endif
}
#endif

#define do_put_mem_byte(a, v) ((*((ui3b *)(a))) = (v))

#if BigEndianUnaligned
#define do_put_mem_word(a, v) ((*((ui4b *)(a))) = (v))
#else
LOCALINLINEFUNC void do_put_mem_word(ui3p a, ui4r v)
{
#if LittleEndianUnaligned
	ui4b b = ((v & 0x00FF) << 8) | ((v >> 8) & 0x00FF);

	*(ui4b *)a = b;
#else
	*a = v >> 8;
	*(a + 1) = v;
#endif
}
#endif

#if BigEndianUnaligned
#define do_put_mem_long(a, v) ((*((ui5b *)(a))) = (v))
#elif HaveMySwapUi5r && LittleEndianUnaligned
#define do_put_mem_long(a, v) ((*((ui5b *)(a))) = MySwapUi5r(v))
#else
LOCALINLINEFUNC void do_put_mem_long(ui3p a, ui5r v)
{
#if LittleEndianUnaligned
	ui4b b1 = v;
	ui4b b2 = v >> 16;
	ui4b c1 = ((b1 & 0x00FF) << 8) | ((b1 >> 8) & 0x00FF);
	ui4b c2 = ((b2 & 0x00FF) << 8) | ((b2 >> 8) & 0x00FF);

	*(ui5b *)a = (c1 << 16) | c2;
#else
	*a = v >> 24;
	*(a + 1) = v >> 16;
	*(a + 2) = v >> 8;
	*(a + 3) = v;
#endif
}
#endif
