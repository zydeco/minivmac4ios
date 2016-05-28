/*
	SCRNTRNS.h

	Copyright (C) 2012 Paul C. Pratt

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
	SCReeN TRaNSlater
*/

/* required arguments for this template */

#ifndef ScrnTrns_DoTrans /* procedure to be created by this template */
#error "ScrnTrns_DoTrans not defined"
#endif
#ifndef ScrnTrns_Src
#error "ScrnTrns_Src not defined"
#endif
#ifndef ScrnTrns_Dst
#error "ScrnTrns_Dst not defined"
#endif
#ifndef ScrnTrns_SrcDepth
#error "ScrnTrns_SrcDepth not defined"
#endif
#ifndef ScrnTrns_DstDepth
#error "ScrnTrns_DstDepth not defined"
#endif

/* optional argument for this template */

#ifndef ScrnTrns_Scale
#define ScrnTrns_Scale 1
#endif

#ifndef ScrnTrns_DstZLo
#define ScrnTrns_DstZLo 0
#endif

/* check of parameters */

#if (ScrnTrns_SrcDepth < 4)
#error "bad ScrnTrns_SrcDepth"
#endif

#if (ScrnTrns_DstDepth < 4)
#error "bad ScrnTrns_Dst"
#endif

/* now define the procedure */

LOCALPROC ScrnTrns_DoTrans(si4b top, si4b left,
	si4b bottom, si4b right)
{
	int i;
	int j;
	ui5b t0;
	ui5b t1;
	ui4r jn = right - left;
	ui4r SrcSkip = vMacScreenByteWidth
		- (jn << (ScrnTrns_SrcDepth - 3));
	ui3b *pSrc = ((ui3b *)ScrnTrns_Src)
		+ (left << (ScrnTrns_SrcDepth - 3))
		+ vMacScreenByteWidth * (ui5r)top;
	ui5b *pDst = ((ui5b *)ScrnTrns_Dst)
		+ left * ScrnTrns_Scale
		+ (ui5r)vMacScreenWidth * ScrnTrns_Scale * ScrnTrns_Scale * top;
	ui4r DstSkip = (vMacScreenWidth - jn) * ScrnTrns_Scale;
#if ScrnTrns_Scale > 1
	int k;
	ui5b *p3;
	ui5b *p4;
#endif

	for (i = bottom - top; --i >= 0; ) {
#if ScrnTrns_Scale > 1
		p3 = pDst;
#endif

		for (j = jn; --j >= 0; ) {
#if 4 == ScrnTrns_SrcDepth
			t0 = do_get_mem_word(pSrc);
			pSrc += 2;
			t1 =
#if ScrnTrns_DstZLo
				((t0 & 0x7C00) << 17) |
				((t0 & 0x7000) << 12) |
				((t0 & 0x03E0) << 14) |
				((t0 & 0x0380) << 9) |
				((t0 & 0x001F) << 11) |
				((t0 & 0x001C) << 6);
#else
				((t0 & 0x7C00) << 9) |
				((t0 & 0x7000) << 4) |
				((t0 & 0x03E0) << 6) |
				((t0 & 0x0380) << 1) |
				((t0 & 0x001F) << 3) |
				((t0 & 0x001C) >> 2);
#endif
#if 0
				((t0 & 0x7C00) << 1) |
				((t0 & 0x7000) >> 4) |
				((t0 & 0x03E0) << 14) |
				((t0 & 0x0380) << 9) |
				((t0 & 0x001F) << 27) |
				((t0 & 0x001C) << 22);
#endif

#elif 5 == ScrnTrns_SrcDepth
			t0 = do_get_mem_long(pSrc);
			pSrc += 4;
#if ScrnTrns_DstZLo
			t1 = t0 << 8;
#else
			t1 = t0;
#endif
#endif

#if ScrnTrns_Scale > 1
			for (k = ScrnTrns_Scale; --k >= 0; )
#endif
			{
				*pDst++ = t1;
			}
		}
		pSrc += SrcSkip;
		pDst += DstSkip;

#if ScrnTrns_Scale > 1
#if ScrnTrns_Scale > 2
		for (k = ScrnTrns_Scale - 1; --k >= 0; )
#endif
		{
			p4 = p3;
			for (j = ScrnTrns_Scale * jn; --j >= 0; ) {
				*pDst++ = *p4++;
			}
			pDst += DstSkip;
		}
#endif /* ScrnTrns_Scale > 1 */
	}
}

/* undefine template locals and parameters */

#undef ScrnTrns_DoTrans
#undef ScrnTrns_Src
#undef ScrnTrns_Dst
#undef ScrnTrns_SrcDepth
#undef ScrnTrns_DstDepth
#undef ScrnTrns_Scale
#undef ScrnTrns_DstZLo
