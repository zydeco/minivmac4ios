/*
	SCRNMAPR.h

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
	SCReeN MAPpeR
*/

/* required arguments for this template */

#ifndef ScrnMapr_DoMap /* procedure to be created by this template */
#error "ScrnMapr_DoMap not defined"
#endif
#ifndef ScrnMapr_Src
#error "ScrnMapr_Src not defined"
#endif
#ifndef ScrnMapr_Dst
#error "ScrnMapr_Dst not defined"
#endif
#ifndef ScrnMapr_SrcDepth
#error "ScrnMapr_SrcDepth not defined"
#endif
#ifndef ScrnMapr_DstDepth
#error "ScrnMapr_DstDepth not defined"
#endif
#ifndef ScrnMapr_Map
#error "ScrnMapr_Map not defined"
#endif

/* optional argument for this template */

#ifndef ScrnMapr_Scale
#define ScrnMapr_Scale 1
#endif

/* check of parameters */

#if (ScrnMapr_SrcDepth < 0) || (ScrnMapr_SrcDepth > 3)
#error "bad ScrnMapr_SrcDepth"
#endif

#if (ScrnMapr_DstDepth < ScrnMapr_SrcDepth)
#error "bad ScrnMapr_Dst"
#endif

/* calculate a few things local to this template */

#define ScrnMapr_MapElSz \
	(ScrnMapr_Scale << (ScrnMapr_DstDepth - ScrnMapr_SrcDepth))

#if 0 == (ScrnMapr_MapElSz & 3)
#define ScrnMapr_TranT ui5b
#define ScrnMapr_TranLn2Sz 2
#elif 0 == (ScrnMapr_MapElSz & 1)
#define ScrnMapr_TranT ui4b
#define ScrnMapr_TranLn2Sz 1
#else
#define ScrnMapr_TranT ui3b
#define ScrnMapr_TranLn2Sz 0
#endif

#define ScrnMapr_TranN (ScrnMapr_MapElSz >> ScrnMapr_TranLn2Sz)

#define ScrnMapr_ScrnWB (vMacScreenWidth >> (3 - ScrnMapr_SrcDepth))

/* now define the procedure */

LOCALPROC ScrnMapr_DoMap(si4b top, si4b left,
	si4b bottom, si4b right)
{
	int i;
	int j;
#if (ScrnMapr_TranN > 4) || (ScrnMapr_Scale > 2)
	int k;
#endif
	ui5r t0;
	ScrnMapr_TranT *pMap;
#if ScrnMapr_Scale > 1
	ScrnMapr_TranT *p3;
#endif

	ui4r leftB = left >> (3 - ScrnMapr_SrcDepth);
	ui4r rightB = (right + (1 << (3 - ScrnMapr_SrcDepth)) - 1)
		>> (3 - ScrnMapr_SrcDepth);
	ui4r jn = rightB - leftB;
	ui4r SrcSkip = ScrnMapr_ScrnWB - jn;
	ui3b *pSrc = ((ui3b *)ScrnMapr_Src)
		+ leftB + ScrnMapr_ScrnWB * (ui5r)top;
	ScrnMapr_TranT *pDst = ((ScrnMapr_TranT *)ScrnMapr_Dst)
		+ ((leftB + ScrnMapr_ScrnWB * ScrnMapr_Scale * (ui5r)top)
			* ScrnMapr_TranN);
	ui5r DstSkip = SrcSkip * ScrnMapr_TranN;

	for (i = bottom - top; --i >= 0; ) {
#if ScrnMapr_Scale > 1
		p3 = pDst;
#endif

		for (j = jn; --j >= 0; ) {
			t0 = *pSrc++;
			pMap =
				&((ScrnMapr_TranT *)ScrnMapr_Map)[t0 * ScrnMapr_TranN];

#if ScrnMapr_TranN > 4
			for (k = ScrnMapr_TranN; --k >= 0; ) {
				*pDst++ = *pMap++;
			}
#else

#if ScrnMapr_TranN >= 2
			*pDst++ = *pMap++;
#endif
#if ScrnMapr_TranN >= 3
			*pDst++ = *pMap++;
#endif
#if ScrnMapr_TranN >= 4
			*pDst++ = *pMap++;
#endif
			*pDst++ = *pMap;

#endif /* ! ScrnMapr_TranN > 4 */

		}
		pSrc += SrcSkip;
		pDst += DstSkip;

#if ScrnMapr_Scale > 1
#if ScrnMapr_Scale > 2
		for (k = ScrnMapr_Scale - 1; --k >= 0; )
#endif
		{
			pMap = p3;
			for (j = ScrnMapr_TranN * jn; --j >= 0; ) {
				*pDst++ = *pMap++;
			}
			pDst += DstSkip;
		}
#endif /* ScrnMapr_Scale > 1 */
	}
}

/* undefine template locals and parameters */

#undef ScrnMapr_ScrnWB
#undef ScrnMapr_TranN
#undef ScrnMapr_TranLn2Sz
#undef ScrnMapr_TranT
#undef ScrnMapr_MapElSz

#undef ScrnMapr_DoMap
#undef ScrnMapr_Src
#undef ScrnMapr_Dst
#undef ScrnMapr_SrcDepth
#undef ScrnMapr_DstDepth
#undef ScrnMapr_Map
#undef ScrnMapr_Scale
