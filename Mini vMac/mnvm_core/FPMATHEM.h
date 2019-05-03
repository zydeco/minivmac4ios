/*
	FPMATHEM.h

	Copyright (C) 2007 John R. Hauser, Stanislav Shwartsman,
		Ross Martin, Paul C. Pratt

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
	Floating Point MATH implemented with software EMulation

	Based on the SoftFloat IEC/IEEE Floating-point Arithmetic
	Package, Release 2b, written by John R. Hauser.

	Also uses extensions to SoftFloat, written for
	Bochs (x86 achitecture simulator), by Stanislav Shwartsman.
*/

/*
	original SoftFloat Copyright notice:

	Written by John R. Hauser.  This work was made possible in part by the
	International Computer Science Institute, located at Suite 600, 1947 Center
	Street, Berkeley, California 94704.  Funding was partially provided by the
	National Science Foundation under grant MIP-9311980.  The original version
	of this code was written as part of a project to build a fixed-point vector
	processor in collaboration with the University of California at Berkeley,
	overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
	is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
	arithmetic/SoftFloat.html'.

	THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
	been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
	RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
	AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
	COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
	EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
	INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
	OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

	Derivative works are acceptable, even for commercial purposes, so long as
	(1) the source code for the derivative work includes prominent notice that
	the work is derivative, and (2) the source code includes prominent notice with
	these four paragraphs for those parts of this code that are retained.
*/

/*
	original Stanislav Shwartsman Copyright notice:

	This source file is an extension to the SoftFloat IEC/IEEE Floating-point
	Arithmetic Package, Release 2b, written for Bochs (x86 achitecture simulator)
	floating point emulation.

	THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
	been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
	RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
	AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
	COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
	EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
	INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
	OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

	Derivative works are acceptable, even for commercial purposes, so long as
	(1) the source code for the derivative work includes prominent notice that
	the work is derivative, and (2) the source code includes prominent notice with
	these four paragraphs for those parts of this code that are retained.

	.*============================================================================
	* Written for Bochs (x86 achitecture simulator) by
	*            Stanislav Shwartsman [sshwarts at sourceforge net]
	* ==========================================================================*.
*/


/*
	ReportAbnormalID unused 0x0204 - 0x02FF
*/


/* soft float stuff */

/*
	should avoid 64 bit literals.
*/

typedef ui3r flag; /* 0/1 */

/*
	To fix: softfloat representation of
	denormalized extended precision numbers
	is different than on 68881.
*/

#define cIncludeFPUUnused cIncludeUnused

/* ----- from original file "softfloat.h" ----- */

/*======================================================================

This C header file is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b.

["original SoftFloat Copyright notice" went here, included near top of
this file.]

======================================================================*/

/*----------------------------------------------------------------------------
| The macro `FLOATX80' must be defined to enable the extended double-precision
| floating-point format `floatx80'.  If this macro is not defined, the
| `floatx80' type will not be defined, and none of the functions that either
| input or output the `floatx80' type will be defined.  The same applies to
| the `FLOAT128' macro and the quadruple-precision format `float128'.
*----------------------------------------------------------------------------*/
#define FLOATX80
#define FLOAT128

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point types.
*----------------------------------------------------------------------------*/

typedef struct {
	ui6b low;
	unsigned short high;
} floatx80;

#ifdef FLOAT128
typedef struct {
	ui6b low, high;
} float128;
#endif


/* ----- end from original file "softfloat.h" ----- */


/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point rounding mode.
*----------------------------------------------------------------------------*/
enum {
	float_round_nearest_even = 0,
	float_round_down         = 1,
	float_round_up           = 2,
	float_round_to_zero      = 3
};

/*----------------------------------------------------------------------------
| Floating-point rounding mode, extended double-precision rounding precision,
| and exception flags.
*----------------------------------------------------------------------------*/

LOCALVAR si3r float_rounding_mode = float_round_nearest_even;


/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point exception flags.
*----------------------------------------------------------------------------*/

enum {
	float_flag_invalid   =  1,
	float_flag_divbyzero =  4,
	float_flag_overflow  =  8,
	float_flag_underflow = 16,
	float_flag_inexact   = 32
};
LOCALVAR si3r float_exception_flags = 0;

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision rounding precision.  Valid
| values are 32, 64, and 80.
*----------------------------------------------------------------------------*/

LOCALVAR si3r floatx80_rounding_precision = 80;

/*----------------------------------------------------------------------------
| Primitive arithmetic functions, including multi-word arithmetic, and
| division and square root approximations.  (Can be specialized to target if
| desired.)
*----------------------------------------------------------------------------*/

/* ----- from original file "softfloat-macros" ----- */

/*============================================================================

This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b.

["original SoftFloat Copyright notice" went here, included near top of this file.]

=============================================================================*/

/*----------------------------------------------------------------------------
| Shifts `a' right by the number of bits given in `count'.  If any nonzero
| bits are shifted off, they are ``jammed'' into the least significant bit of
| the result by setting the least significant bit to 1.  The value of `count'
| can be arbitrarily large; in particular, if `count' is greater than 32, the
| result will be either 0 or 1, depending on whether `a' is zero or nonzero.
| The result is stored in the location pointed to by `zPtr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shift32RightJamming( ui5b a, si4r count, ui5b *zPtr )
{
	ui5b z;

	if ( count == 0 ) {
		z = a;
	}
	else if ( count < 32 ) {
		z = ( a>>count ) | ( ( a<<( ( - count ) & 31 ) ) != 0 );
	}
	else {
		z = ( a != 0 );
	}
	*zPtr = z;

}

/*----------------------------------------------------------------------------
| Shifts `a' right by the number of bits given in `count'.  If any nonzero
| bits are shifted off, they are ``jammed'' into the least significant bit of
| the result by setting the least significant bit to 1.  The value of `count'
| can be arbitrarily large; in particular, if `count' is greater than 64, the
| result will be either 0 or 1, depending on whether `a' is zero or nonzero.
| The result is stored in the location pointed to by `zPtr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shift64RightJamming( ui6b a, si4r count, ui6b *zPtr )
{
	ui6b z;

	if ( count == 0 ) {
		z = a;
	}
	else if ( count < 64 ) {
		z = ( a>>count ) | ( ( a<<( ( - count ) & 63 ) ) != 0 );
	}
	else {
		z = ( a != 0 );
	}
	*zPtr = z;

}

/*----------------------------------------------------------------------------
| Shifts the 128-bit value formed by concatenating `a0' and `a1' right by 64
| _plus_ the number of bits given in `count'.  The shifted result is at most
| 64 nonzero bits; this is stored at the location pointed to by `z0Ptr'.  The
| bits shifted off form a second 64-bit result as follows:  The _last_ bit
| shifted off is the most-significant bit of the extra result, and the other
| 63 bits of the extra result are all zero if and only if _all_but_the_last_
| bits shifted off were all zero.  This extra result is stored in the location
| pointed to by `z1Ptr'.  The value of `count' can be arbitrarily large.
|     (This routine makes more sense if `a0' and `a1' are considered to form
| a fixed-point value with binary point between `a0' and `a1'.  This fixed-
| point value is shifted right by the number of bits given in `count', and
| the integer part of the result is returned at the location pointed to by
| `z0Ptr'.  The fractional part of the result may be slightly corrupted as
| described above, and is returned at the location pointed to by `z1Ptr'.)
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shift64ExtraRightJamming(
	ui6b a0, ui6b a1, si4r count, ui6b *z0Ptr, ui6b *z1Ptr )
{
	ui6b z0, z1;
	si3r negCount = ( - count ) & 63;

	if ( count == 0 ) {
		z1 = a1;
		z0 = a0;
	}
	else if ( count < 64 ) {
		z1 = ( a0<<negCount ) | ( a1 != 0 );
		z0 = a0>>count;
	}
	else {
		if ( count == 64 ) {
			z1 = a0 | ( a1 != 0 );
		}
		else {
			z1 = ( ( a0 | a1 ) != 0 );
		}
		z0 = 0;
	}
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 128-bit value formed by concatenating `a0' and `a1' right by the
| number of bits given in `count'.  Any bits shifted off are lost.  The value
| of `count' can be arbitrarily large; in particular, if `count' is greater
| than 128, the result will be 0.  The result is broken into two 64-bit pieces
| which are stored at the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shift128Right(
	ui6b a0, ui6b a1, si4r count, ui6b *z0Ptr, ui6b *z1Ptr )
{
	ui6b z0, z1;
	si3r negCount = ( - count ) & 63;

	if ( count == 0 ) {
		z1 = a1;
		z0 = a0;
	}
	else if ( count < 64 ) {
		z1 = ( a0<<negCount ) | ( a1>>count );
		z0 = a0>>count;
	}
	else {
		z1 = ( count < 64 ) ? ( a0>>( count & 63 ) ) : 0;
		z0 = 0;
	}
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 128-bit value formed by concatenating `a0' and `a1' right by the
| number of bits given in `count'.  If any nonzero bits are shifted off, they
| are ``jammed'' into the least significant bit of the result by setting the
| least significant bit to 1.  The value of `count' can be arbitrarily large;
| in particular, if `count' is greater than 128, the result will be either
| 0 or 1, depending on whether the concatenation of `a0' and `a1' is zero or
| nonzero.  The result is broken into two 64-bit pieces which are stored at
| the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shift128RightJamming(
	ui6b a0, ui6b a1, si4r count, ui6b *z0Ptr, ui6b *z1Ptr )
{
	ui6b z0, z1;
	si3r negCount = ( - count ) & 63;

	if ( count == 0 ) {
		z1 = a1;
		z0 = a0;
	}
	else if ( count < 64 ) {
		z1 = ( a0<<negCount ) | ( a1>>count ) | ( ( a1<<negCount ) != 0 );
		z0 = a0>>count;
	}
	else {
		if ( count == 64 ) {
			z1 = a0 | ( a1 != 0 );
		}
		else if ( count < 128 ) {
			z1 = ( a0>>( count & 63 ) ) | ( ( ( a0<<negCount ) | a1 ) != 0 );
		}
		else {
			z1 = ( ( a0 | a1 ) != 0 );
		}
		z0 = 0;
	}
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 192-bit value formed by concatenating `a0', `a1', and `a2' right
| by 64 _plus_ the number of bits given in `count'.  The shifted result is
| at most 128 nonzero bits; these are broken into two 64-bit pieces which are
| stored at the locations pointed to by `z0Ptr' and `z1Ptr'.  The bits shifted
| off form a third 64-bit result as follows:  The _last_ bit shifted off is
| the most-significant bit of the extra result, and the other 63 bits of the
| extra result are all zero if and only if _all_but_the_last_ bits shifted off
| were all zero.  This extra result is stored in the location pointed to by
| `z2Ptr'.  The value of `count' can be arbitrarily large.
|     (This routine makes more sense if `a0', `a1', and `a2' are considered
| to form a fixed-point value with binary point between `a1' and `a2'.  This
| fixed-point value is shifted right by the number of bits given in `count',
| and the integer part of the result is returned at the locations pointed to
| by `z0Ptr' and `z1Ptr'.  The fractional part of the result may be slightly
| corrupted as described above, and is returned at the location pointed to by
| `z2Ptr'.)
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shift128ExtraRightJamming(
	ui6b a0,
	ui6b a1,
	ui6b a2,
	si4r count,
	ui6b *z0Ptr,
	ui6b *z1Ptr,
	ui6b *z2Ptr)
{
	ui6b z0, z1, z2;
	si3r negCount = ( - count ) & 63;

	if ( count == 0 ) {
		z2 = a2;
		z1 = a1;
		z0 = a0;
	}
	else {
		if ( count < 64 ) {
			z2 = a1<<negCount;
			z1 = ( a0<<negCount ) | ( a1>>count );
			z0 = a0>>count;
		}
		else {
			if ( count == 64 ) {
				z2 = a1;
				z1 = a0;
			}
			else {
				a2 |= a1;
				if ( count < 128 ) {
					z2 = a0<<negCount;
					z1 = a0>>( count & 63 );
				}
				else {
					z2 = ( count == 128 ) ? a0 : ( a0 != 0 );
					z1 = 0;
				}
			}
			z0 = 0;
		}
		z2 |= ( a2 != 0 );
	}
	*z2Ptr = z2;
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Shifts the 128-bit value formed by concatenating `a0' and `a1' left by the
| number of bits given in `count'.  Any bits shifted off are lost.  The value
| of `count' must be less than 64.  The result is broken into two 64-bit
| pieces which are stored at the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC shortShift128Left(
	ui6b a0, ui6b a1, si4r count, ui6b *z0Ptr, ui6b *z1Ptr )
{

	*z1Ptr = a1<<count;
	*z0Ptr =
		( count == 0 ) ? a0 : ( a0<<count ) | ( a1>>( ( - count ) & 63 ) );

}

/*----------------------------------------------------------------------------
| Adds the 128-bit value formed by concatenating `a0' and `a1' to the 128-bit
| value formed by concatenating `b0' and `b1'.  Addition is modulo 2^128, so
| any carry out is lost.  The result is broken into two 64-bit pieces which
| are stored at the locations pointed to by `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC add128(
	ui6b a0, ui6b a1, ui6b b0, ui6b b1, ui6b *z0Ptr, ui6b *z1Ptr )
{
	ui6b z1;

	z1 = a1 + b1;
	*z1Ptr = z1;
	*z0Ptr = a0 + b0 + ( z1 < a1 );
}

/*----------------------------------------------------------------------------
| Adds the 192-bit value formed by concatenating `a0', `a1', and `a2' to the
| 192-bit value formed by concatenating `b0', `b1', and `b2'.  Addition is
| modulo 2^192, so any carry out is lost.  The result is broken into three
| 64-bit pieces which are stored at the locations pointed to by `z0Ptr',
| `z1Ptr', and `z2Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC add192(
	ui6b a0,
	ui6b a1,
	ui6b a2,
	ui6b b0,
	ui6b b1,
	ui6b b2,
	ui6b *z0Ptr,
	ui6b *z1Ptr,
	ui6b *z2Ptr)
{
	ui6b z0, z1, z2;
	si3r carry0, carry1;

	z2 = a2 + b2;
	carry1 = ( z2 < a2 );
	z1 = a1 + b1;
	carry0 = ( z1 < a1 );
	z0 = a0 + b0;
	z1 += carry1;
	z0 += ( z1 < carry1 );
	z0 += carry0;
	*z2Ptr = z2;
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Subtracts the 128-bit value formed by concatenating `b0' and `b1' from the
| 128-bit value formed by concatenating `a0' and `a1'.  Subtraction is modulo
| 2^128, so any borrow out (carry out) is lost.  The result is broken into two
| 64-bit pieces which are stored at the locations pointed to by `z0Ptr' and
| `z1Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC
 sub128(
	 ui6b a0, ui6b a1, ui6b b0, ui6b b1, ui6b *z0Ptr, ui6b *z1Ptr )
{

	*z1Ptr = a1 - b1;
	*z0Ptr = a0 - b0 - ( a1 < b1 );

}

/*----------------------------------------------------------------------------
| Subtracts the 192-bit value formed by concatenating `b0', `b1', and `b2'
| from the 192-bit value formed by concatenating `a0', `a1', and `a2'.
| Subtraction is modulo 2^192, so any borrow out (carry out) is lost.  The
| result is broken into three 64-bit pieces which are stored at the locations
| pointed to by `z0Ptr', `z1Ptr', and `z2Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC
 sub192(
	 ui6b a0,
	 ui6b a1,
	 ui6b a2,
	 ui6b b0,
	 ui6b b1,
	 ui6b b2,
	 ui6b *z0Ptr,
	 ui6b *z1Ptr,
	 ui6b *z2Ptr
 )
{
	ui6b z0, z1, z2;
	si3r borrow0, borrow1;

	z2 = a2 - b2;
	borrow1 = ( a2 < b2 );
	z1 = a1 - b1;
	borrow0 = ( a1 < b1 );
	z0 = a0 - b0;
	z0 -= ( z1 < borrow1 );
	z1 -= borrow1;
	z0 -= borrow0;
	*z2Ptr = z2;
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Multiplies `a' by `b' to obtain a 128-bit product.  The product is broken
| into two 64-bit pieces which are stored at the locations pointed to by
| `z0Ptr' and `z1Ptr'.
*----------------------------------------------------------------------------*/


#ifndef HaveUi5to6Mul
#define HaveUi5to6Mul 1
#endif

#if HaveUi5to6Mul
LOCALINLINEPROC Ui5to6Mul( ui5b src1, ui5b src2, ui6b *z)
{
	*z = ((ui6b) src1) * src2;
}
#else

LOCALINLINEPROC Ui6fromHiLo(ui5b hi, ui5b lo, ui6b *z)
{
	*z = (((ui6b)(hi)) << 32) + lo;
#if 0
	z->lo = hi;
	z->hi = lo;
#endif
}

LOCALPROC Ui5to6Mul( ui5b src1, ui5b src2, ui6b *z)
{
	ui4b src1_lo = ui5b_lo(src1);
	ui4b src2_lo = ui5b_lo(src2);
	ui4b src1_hi = ui5b_hi(src1);
	ui4b src2_hi = ui5b_hi(src2);

	ui5b r0 = ( (ui6b) src1_lo ) * src2_lo;
	ui5b r1 = ( (ui6b) src1_hi ) * src2_lo;
	ui5b r2 = ( (ui6b) src1_lo ) * src2_hi;
	ui5b r3 = ( (ui6b) src1_hi ) * src2_hi;

	ui5b ra1 = ui5b_hi(r0) + ui5b_lo(r1) + ui5b_lo(r2);

	ui5b lo = (ui5b_lo(ra1) << 16) | ui5b_lo(r0);
	ui5b hi = ui5b_hi(ra1) + ui5b_hi(r1) + ui5b_hi(r2) + r3;

	Ui6fromHiLo(hi, lo, z);
}

#endif


LOCALINLINEPROC mul64To128( ui6b a, ui6b b, ui6b *z0Ptr, ui6b *z1Ptr )
{
	ui5b aHigh, aLow, bHigh, bLow;
	ui6b z0, zMiddleA, zMiddleB, z1;

	aLow = a;
	aHigh = a>>32;
	bLow = b;
	bHigh = b>>32;

	Ui5to6Mul(aLow, bLow, &z1);
	Ui5to6Mul(aLow, bHigh, &zMiddleA);
	Ui5to6Mul(aHigh, bLow, &zMiddleB);
	Ui5to6Mul(aHigh, bHigh, &z0);

	zMiddleA += zMiddleB;
	z0 += ( ( (ui6b) ( zMiddleA < zMiddleB ) )<<32 ) + ( zMiddleA>>32 );
	zMiddleA <<= 32;
	z1 += zMiddleA;
	z0 += ( z1 < zMiddleA );
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Multiplies the 128-bit value formed by concatenating `a0' and `a1' by
| `b' to obtain a 192-bit product.  The product is broken into three 64-bit
| pieces which are stored at the locations pointed to by `z0Ptr', `z1Ptr', and
| `z2Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC
 mul128By64To192(
	 ui6b a0,
	 ui6b a1,
	 ui6b b,
	 ui6b *z0Ptr,
	 ui6b *z1Ptr,
	 ui6b *z2Ptr
 )
{
	ui6b z0, z1, z2, more1;

	mul64To128( a1, b, &z1, &z2 );
	mul64To128( a0, b, &z0, &more1 );
	add128( z0, more1, 0, z1, &z0, &z1 );
	*z2Ptr = z2;
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Multiplies the 128-bit value formed by concatenating `a0' and `a1' to the
| 128-bit value formed by concatenating `b0' and `b1' to obtain a 256-bit
| product.  The product is broken into four 64-bit pieces which are stored at
| the locations pointed to by `z0Ptr', `z1Ptr', `z2Ptr', and `z3Ptr'.
*----------------------------------------------------------------------------*/

LOCALINLINEPROC
 mul128To256(
	 ui6b a0,
	 ui6b a1,
	 ui6b b0,
	 ui6b b1,
	 ui6b *z0Ptr,
	 ui6b *z1Ptr,
	 ui6b *z2Ptr,
	 ui6b *z3Ptr
 )
{
	ui6b z0, z1, z2, z3;
	ui6b more1, more2;

	mul64To128( a1, b1, &z2, &z3 );
	mul64To128( a1, b0, &z1, &more2 );
	add128( z1, more2, 0, z2, &z1, &z2 );
	mul64To128( a0, b0, &z0, &more1 );
	add128( z0, more1, 0, z1, &z0, &z1 );
	mul64To128( a0, b1, &more1, &more2 );
	add128( more1, more2, 0, z2, &more1, &z2 );
	add128( z0, z1, 0, more1, &z0, &z1 );
	*z3Ptr = z3;
	*z2Ptr = z2;
	*z1Ptr = z1;
	*z0Ptr = z0;

}

/*----------------------------------------------------------------------------
| Returns an approximation to the 64-bit integer quotient obtained by dividing
| `b' into the 128-bit value formed by concatenating `a0' and `a1'.  The
| divisor `b' must be at least 2^63.  If q is the exact quotient truncated
| toward zero, the approximation returned lies between q and q + 2 inclusive.
| If the exact quotient q is larger than 64 bits, the maximum positive 64-bit
| unsigned integer is returned.
*----------------------------------------------------------------------------*/

#ifndef HaveUi6Div
#define HaveUi6Div 0
#endif

#if HaveUi6Div
#define Ui6Div(x, y) ((x) / (y))
#else
/*
	Assuming other 64 bit operations available,
	like compare, subtract, shift.
*/
LOCALFUNC ui6b Ui6Div(ui6b num, ui6b den)
{
	ui6b bit = 1;
	ui6b res = 0;

	while ((den < num) && bit && ! (den & (LIT64(1) << 63))) {
		den <<= 1;
		bit <<= 1;
	}

	while (bit) {
		if (num >= den) {
			num -= den;
			res |= bit;
		}
		bit >>= 1;
		den >>= 1;
	}

	return res;
}
#endif

LOCALFUNC ui6b estimateDiv128To64( ui6b a0, ui6b a1, ui6b b )
{
	ui6b b0, b1;
	ui6b rem0, rem1, term0, term1;
	ui6b z;

	if ( b <= a0 ) return LIT64( 0xFFFFFFFFFFFFFFFF );
	b0 = b>>32;
	z = ( b0<<32 <= a0 ) ? LIT64( 0xFFFFFFFF00000000 ) : Ui6Div(a0, b0) << 32;
	mul64To128( b, z, &term0, &term1 );
	sub128( a0, a1, term0, term1, &rem0, &rem1 );
	while ( ( (si6b) rem0 ) < 0 ) {
		z -= LIT64( 0x100000000 );
		b1 = b<<32;
		add128( rem0, rem1, b0, b1, &rem0, &rem1 );
	}
	rem0 = ( rem0<<32 ) | ( rem1>>32 );
	z |= ( b0<<32 <= rem0 ) ? 0xFFFFFFFF : Ui6Div(rem0, b0);
	return z;

}

/*----------------------------------------------------------------------------
| Returns an approximation to the square root of the 32-bit significand given
| by `a'.  Considered as an integer, `a' must be at least 2^31.  If bit 0 of
| `aExp' (the least significant bit) is 1, the integer returned approximates
| 2^31*sqrt(`a'/2^31), where `a' is considered an integer.  If bit 0 of `aExp'
| is 0, the integer returned approximates 2^31*sqrt(`a'/2^30).  In either
| case, the approximation returned lies strictly within +/-2 of the exact
| value.
*----------------------------------------------------------------------------*/

LOCALFUNC ui5b estimateSqrt32( si4r aExp, ui5b a )
{
	static const ui4b sqrtOddAdjustments[] = {
		0x0004, 0x0022, 0x005D, 0x00B1, 0x011D, 0x019F, 0x0236, 0x02E0,
		0x039C, 0x0468, 0x0545, 0x0631, 0x072B, 0x0832, 0x0946, 0x0A67
	};
	static const ui4b sqrtEvenAdjustments[] = {
		0x0A2D, 0x08AF, 0x075A, 0x0629, 0x051A, 0x0429, 0x0356, 0x029E,
		0x0200, 0x0179, 0x0109, 0x00AF, 0x0068, 0x0034, 0x0012, 0x0002
	};
	si3r index;
	ui5b z;

	index = ( a>>27 ) & 15;
	if ( aExp & 1 ) {
		z = 0x4000 + ( a>>17 ) - sqrtOddAdjustments[ index ];
		z = ( ( a / z )<<14 ) + ( z<<15 );
		a >>= 1;
	}
	else {
		z = 0x8000 + ( a>>17 ) - sqrtEvenAdjustments[ index ];
		z = a / z + z;
		z = ( 0x20000 <= z ) ? 0xFFFF8000 : ( z<<15 );
		if ( z <= a ) return (ui5b) ( ( (si5b) a )>>1 );
	}
	return ( (ui5b) Ui6Div( ( ( (ui6b) a )<<31 ), z ) ) + ( z>>1 );

}

/*----------------------------------------------------------------------------
| Returns the number of leading 0 bits before the most-significant 1 bit of
| `a'.  If `a' is zero, 32 is returned.
*----------------------------------------------------------------------------*/

LOCALFUNC si3r countLeadingZeros32( ui5b a )
{
	static const si3r countLeadingZerosHigh[] = {
		8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	si3r shiftCount;

	shiftCount = 0;
	if ( a < 0x10000 ) {
		shiftCount += 16;
		a <<= 16;
	}
	if ( a < 0x1000000 ) {
		shiftCount += 8;
		a <<= 8;
	}
	shiftCount += countLeadingZerosHigh[ a>>24 ];
	return shiftCount;

}

/*----------------------------------------------------------------------------
| Returns the number of leading 0 bits before the most-significant 1 bit of
| `a'.  If `a' is zero, 64 is returned.
*----------------------------------------------------------------------------*/

LOCALFUNC si3r countLeadingZeros64( ui6b a )
{
	si3r shiftCount;

	shiftCount = 0;
	if ( a < ( (ui6b) 1 )<<32 ) {
		shiftCount += 32;
	}
	else {
		a >>= 32;
	}
	shiftCount += countLeadingZeros32( a );
	return shiftCount;

}

/*----------------------------------------------------------------------------
| Returns 1 if the 128-bit value formed by concatenating `a0' and `a1'
| is equal to the 128-bit value formed by concatenating `b0' and `b1'.
| Otherwise, returns 0.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag eq128( ui6b a0, ui6b a1, ui6b b0, ui6b b1 )
{

	return ( a0 == b0 ) && ( a1 == b1 );

}

/*----------------------------------------------------------------------------
| Returns 1 if the 128-bit value formed by concatenating `a0' and `a1' is less
| than or equal to the 128-bit value formed by concatenating `b0' and `b1'.
| Otherwise, returns 0.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag le128( ui6b a0, ui6b a1, ui6b b0, ui6b b1 )
{

	return ( a0 < b0 ) || ( ( a0 == b0 ) && ( a1 <= b1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the 128-bit value formed by concatenating `a0' and `a1' is less
| than the 128-bit value formed by concatenating `b0' and `b1'.  Otherwise,
| returns 0.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag lt128( ui6b a0, ui6b a1, ui6b b0, ui6b b1 )
{

	return ( a0 < b0 ) || ( ( a0 == b0 ) && ( a1 < b1 ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the 128-bit value formed by concatenating `a0' and `a1' is
| not equal to the 128-bit value formed by concatenating `b0' and `b1'.
| Otherwise, returns 0.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALINLINEFUNC flag ne128( ui6b a0, ui6b a1, ui6b b0, ui6b b1 )
{

	return ( a0 != b0 ) || ( a1 != b1 );

}
#endif

/* ----- end from original file "softfloat-macros" ----- */

/*----------------------------------------------------------------------------
| Functions and definitions to determine:  (1) whether tininess for underflow
| is detected before or after rounding by default, (2) what (if anything)
| happens when exceptions are raised, (3) how signaling NaNs are distinguished
| from quiet NaNs, (4) the default generated quiet NaNs, and (5) how NaNs
| are propagated from function inputs to output.  These details are target-
| specific.
*----------------------------------------------------------------------------*/

/* ----- from original file "softfloat-specialize" ----- */

/*============================================================================

This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b.

["original SoftFloat Copyright notice" went here, included near top of this file.]

=============================================================================*/

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point underflow tininess-detection mode.
*----------------------------------------------------------------------------*/
enum {
	float_tininess_after_rounding  = 0,
	float_tininess_before_rounding = 1
};

/*----------------------------------------------------------------------------
| Underflow tininess-detection mode, statically initialized to default value.
| (The declaration in `softfloat.h' must match the `si3r' type here.)
*----------------------------------------------------------------------------*/
LOCALVAR si3r float_detect_tininess = float_tininess_after_rounding;

/*----------------------------------------------------------------------------
| Routine to raise any or all of the software IEC/IEEE floating-point
| exception flags.
*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
| Raises the exceptions specified by `flags'.  Floating-point traps can be
| defined here if desired.  It is currently not possible for such a trap
| to substitute a result value.  If traps are not implemented, this routine
| should be simply `float_exception_flags |= flags;'.
*----------------------------------------------------------------------------*/

LOCALFUNC void float_raise( si3r flags )
{

	float_exception_flags |= flags;

}

/*----------------------------------------------------------------------------
| Internal canonical NaN format.
*----------------------------------------------------------------------------*/
typedef struct {
	flag sign;
	ui6b high, low;
} commonNaNT;

/*----------------------------------------------------------------------------
| The pattern for a default generated extended double-precision NaN.  The
| `high' and `low' values hold the most- and least-significant bits,
| respectively.
*----------------------------------------------------------------------------*/
#define floatx80_default_nan_high 0xFFFF
#define floatx80_default_nan_low  LIT64( 0xC000000000000000 )

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is a
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

LOCALFUNC flag floatx80_is_nan( floatx80 a )
{

	return ( ( a.high & 0x7FFF ) == 0x7FFF ) && (ui6b) ( a.low<<1 );

}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is a
| signaling NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

LOCALFUNC flag floatx80_is_signaling_nan( floatx80 a )
{
	ui6b aLow;

	aLow = a.low & ~ LIT64( 0x4000000000000000 );
	return
		   ( ( a.high & 0x7FFF ) == 0x7FFF )
		&& (ui6b) ( aLow<<1 )
		&& ( a.low == aLow );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point NaN `a' to the canonical NaN format.  If `a' is a signaling NaN, the
| invalid exception is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC commonNaNT floatx80ToCommonNaN( floatx80 a )
{
	commonNaNT z;

	if ( floatx80_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
	z.sign = a.high>>15;
	z.low = 0;
	z.high = a.low<<1;
	return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the extended
| double-precision floating-point format.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 commonNaNToFloatx80( commonNaNT a )
{
	floatx80 z;

	z.low = LIT64( 0xC000000000000000 ) | ( a.high>>1 );
	z.high = ( ( (ui4b) a.sign )<<15 ) | 0x7FFF;
	return z;

}

/*----------------------------------------------------------------------------
| Takes two extended double-precision floating-point values `a' and `b', one
| of which is a NaN, and returns the appropriate NaN result.  If either `a' or
| `b' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 propagateFloatx80NaN( floatx80 a, floatx80 b )
{
	flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

	aIsNaN = floatx80_is_nan( a );
	aIsSignalingNaN = floatx80_is_signaling_nan( a );
	bIsNaN = floatx80_is_nan( b );
	bIsSignalingNaN = floatx80_is_signaling_nan( b );
	a.low |= LIT64( 0xC000000000000000 );
	b.low |= LIT64( 0xC000000000000000 );
	if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_invalid );
	if ( aIsSignalingNaN ) {
		if ( bIsSignalingNaN ) goto returnLargerSignificand;
		return bIsNaN ? b : a;
	}
	else if ( aIsNaN ) {
		if ( bIsSignalingNaN | ! bIsNaN ) return a;
 returnLargerSignificand:
		if ( a.low < b.low ) return b;
		if ( b.low < a.low ) return a;
		return ( a.high < b.high ) ? a : b;
	}
	else {
		return b;
	}

}

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| The pattern for a default generated quadruple-precision NaN.  The `high' and
| `low' values hold the most- and least-significant bits, respectively.
*----------------------------------------------------------------------------*/
#define float128_default_nan_high LIT64( 0xFFFF800000000000 )
#define float128_default_nan_low  LIT64( 0x0000000000000000 )

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

LOCALFUNC flag float128_is_nan( float128 a )
{

	return
		   ( LIT64( 0xFFFE000000000000 ) <= (ui6b) ( a.high<<1 ) )
		&& ( a.low || ( a.high & LIT64( 0x0000FFFFFFFFFFFF ) ) );

}

/*----------------------------------------------------------------------------
| Returns 1 if the quadruple-precision floating-point value `a' is a
| signaling NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

LOCALFUNC flag float128_is_signaling_nan( float128 a )
{

	return
		   ( ( ( a.high>>47 ) & 0xFFFF ) == 0xFFFE )
		&& ( a.low || ( a.high & LIT64( 0x00007FFFFFFFFFFF ) ) );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC commonNaNT float128ToCommonNaN( float128 a )
{
	commonNaNT z;

	if ( float128_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
	z.sign = a.high>>63;
	shortShift128Left( a.high, a.low, 16, &z.high, &z.low );
	return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the quadruple-
| precision floating-point format.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 commonNaNToFloat128( commonNaNT a )
{
	float128 z;

	shift128Right( a.high, a.low, 16, &z.high, &z.low );
	z.high |= ( ( (ui6b) a.sign )<<63 ) | LIT64( 0x7FFF800000000000 );
	return z;

}

/*----------------------------------------------------------------------------
| Takes two quadruple-precision floating-point values `a' and `b', one of
| which is a NaN, and returns the appropriate NaN result.  If either `a' or
| `b' is a signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 propagateFloat128NaN( float128 a, float128 b )
{
	flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

	aIsNaN = float128_is_nan( a );
	aIsSignalingNaN = float128_is_signaling_nan( a );
	bIsNaN = float128_is_nan( b );
	bIsSignalingNaN = float128_is_signaling_nan( b );
	a.high |= LIT64( 0x0000800000000000 );
	b.high |= LIT64( 0x0000800000000000 );
	if ( aIsSignalingNaN | bIsSignalingNaN ) float_raise( float_flag_invalid );
	if ( aIsSignalingNaN ) {
		if ( bIsSignalingNaN ) goto returnLargerSignificand;
		return bIsNaN ? b : a;
	}
	else if ( aIsNaN ) {
		if ( bIsSignalingNaN | ! bIsNaN ) return a;
 returnLargerSignificand:
		if ( lt128( a.high<<1, a.low, b.high<<1, b.low ) ) return b;
		if ( lt128( b.high<<1, b.low, a.high<<1, a.low ) ) return a;
		return ( a.high < b.high ) ? a : b;
	}
	else {
		return b;
	}

}

#endif

/* ----- end from original file "softfloat-specialize" ----- */

/* ----- from original file "softfloat.c" ----- */


/*============================================================================

This C source file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

["original SoftFloat Copyright notice" went here, included near top of this file.]

=============================================================================*/

/*----------------------------------------------------------------------------
| Takes a 64-bit fixed-point value `absZ' with binary point between bits 6
| and 7, and returns the properly rounded 32-bit integer corresponding to the
| input.  If `zSign' is 1, the input is negated before being converted to an
| integer.  Bit 63 of `absZ' must be zero.  Ordinarily, the fixed-point input
| is simply rounded to an integer, with the inexact exception raised if the
| input cannot be represented exactly as an integer.  However, if the fixed-
| point input is too large, the invalid exception is raised and the largest
| positive or negative integer is returned.
*----------------------------------------------------------------------------*/

LOCALFUNC si5r roundAndPackInt32( flag zSign, ui6b absZ )
{
	si3r roundingMode;
	flag roundNearestEven;
	si3r roundIncrement, roundBits;
	si5r z;

	roundingMode = float_rounding_mode;
	roundNearestEven = ( roundingMode == float_round_nearest_even );
	roundIncrement = 0x40;
	if ( ! roundNearestEven ) {
		if ( roundingMode == float_round_to_zero ) {
			roundIncrement = 0;
		}
		else {
			roundIncrement = 0x7F;
			if ( zSign ) {
				if ( roundingMode == float_round_up ) roundIncrement = 0;
			}
			else {
				if ( roundingMode == float_round_down ) roundIncrement = 0;
			}
		}
	}
	roundBits = absZ & 0x7F;
	absZ = ( absZ + roundIncrement )>>7;
	absZ &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
	z = absZ;
	if ( zSign ) z = - z;
	if ( ( absZ>>32 ) || ( z && ( ( z < 0 ) ^ zSign ) ) ) {
		float_raise( float_flag_invalid );
		return zSign ? (si5b) 0x80000000 : 0x7FFFFFFF;
	}
	if ( roundBits ) float_exception_flags |= float_flag_inexact;
	return z;

}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC ui6b extractFloatx80Frac( floatx80 a )
{

	return a.low;

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the extended double-precision floating-point
| value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC si5r extractFloatx80Exp( floatx80 a )
{

	return a.high & 0x7FFF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the extended double-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag extractFloatx80Sign( floatx80 a )
{

	return a.high>>15;

}

/*----------------------------------------------------------------------------
| Normalizes the subnormal extended double-precision floating-point value
| represented by the denormalized significand `aSig'.  The normalized exponent
| and significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

LOCALPROC
 normalizeFloatx80Subnormal( ui6b aSig, si5r *zExpPtr, ui6b *zSigPtr )
{
	si3r shiftCount;

	shiftCount = countLeadingZeros64( aSig );
	*zSigPtr = aSig<<shiftCount;
	*zExpPtr = 1 - shiftCount;

}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into an
| extended double-precision floating-point value, returning the result.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC floatx80 packFloatx80( flag zSign, si5r zExp, ui6b zSig )
{
	floatx80 z;

	z.low = zSig;
	z.high = ( ( (ui4b) zSign )<<15 ) + zExp;
	return z;

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and extended significand formed by the concatenation of `zSig0' and `zSig1',
| and returns the proper extended double-precision floating-point value
| corresponding to the abstract input.  Ordinarily, the abstract value is
| rounded and packed into the extended double-precision format, with the
| inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal extended
| double-precision floating-point number.
|     If `roundingPrecision' is 32 or 64, the result is rounded to the same
| number of bits as single or double precision, respectively.  Otherwise, the
| result is rounded to the full precision of the extended double-precision
| format.
|     The input significand must be normalized or smaller.  If the input
| significand is not normalized, `zExp' must be 0; in that case, the result
| returned is a subnormal number, and it must not require rounding.  The
| handling of underflow and overflow follows the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80
 roundAndPackFloatx80(
	 si3r roundingPrecision, flag zSign, si5r zExp, ui6b zSig0, ui6b zSig1
 )
{
	si3r roundingMode;
	flag roundNearestEven, increment, isTiny;
	si6r roundIncrement, roundMask, roundBits;

	roundingMode = float_rounding_mode;
	roundNearestEven = ( roundingMode == float_round_nearest_even );
	if ( roundingPrecision == 80 ) goto precision80;
	if ( roundingPrecision == 64 ) {
		roundIncrement = LIT64( 0x0000000000000400 );
		roundMask = LIT64( 0x00000000000007FF );
	}
	else if ( roundingPrecision == 32 ) {
		roundIncrement = LIT64( 0x0000008000000000 );
		roundMask = LIT64( 0x000000FFFFFFFFFF );
	}
	else {
		goto precision80;
	}
	zSig0 |= ( zSig1 != 0 );
	if ( ! roundNearestEven ) {
		if ( roundingMode == float_round_to_zero ) {
			roundIncrement = 0;
		}
		else {
			roundIncrement = roundMask;
			if ( zSign ) {
				if ( roundingMode == float_round_up ) roundIncrement = 0;
			}
			else {
				if ( roundingMode == float_round_down ) roundIncrement = 0;
			}
		}
	}
	roundBits = zSig0 & roundMask;
	if ( 0x7FFD <= (ui5b) ( zExp - 1 ) ) {
		if (    ( 0x7FFE < zExp )
			 || ( ( zExp == 0x7FFE ) && ( zSig0 + roundIncrement < zSig0 ) )
		   ) {
			goto overflow;
		}
		if ( zExp <= 0 ) {
			isTiny =
				   ( float_detect_tininess == float_tininess_before_rounding )
				|| ( zExp < 0 )
				|| ( zSig0 <= zSig0 + roundIncrement );
			shift64RightJamming( zSig0, 1 - zExp, &zSig0 );
			zExp = 0;
			roundBits = zSig0 & roundMask;
			if ( isTiny && roundBits ) float_raise( float_flag_underflow );
			if ( roundBits ) float_exception_flags |= float_flag_inexact;
			zSig0 += roundIncrement;
			if ( (si6b) zSig0 < 0 ) zExp = 1;
			roundIncrement = roundMask + 1;
			if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
				roundMask |= roundIncrement;
			}
			zSig0 &= ~ roundMask;
			return packFloatx80( zSign, zExp, zSig0 );
		}
	}
	if ( roundBits ) float_exception_flags |= float_flag_inexact;
	zSig0 += roundIncrement;
	if ( zSig0 < roundIncrement ) {
		++zExp;
		zSig0 = LIT64( 0x8000000000000000 );
	}
	roundIncrement = roundMask + 1;
	if ( roundNearestEven && ( roundBits<<1 == roundIncrement ) ) {
		roundMask |= roundIncrement;
	}
	zSig0 &= ~ roundMask;
	if ( zSig0 == 0 ) zExp = 0;
	return packFloatx80( zSign, zExp, zSig0 );
 precision80:
	increment = ( (si6b) zSig1 < 0 );
	if ( ! roundNearestEven ) {
		if ( roundingMode == float_round_to_zero ) {
			increment = 0;
		}
		else {
			if ( zSign ) {
				increment = ( roundingMode == float_round_down ) && zSig1;
			}
			else {
				increment = ( roundingMode == float_round_up ) && zSig1;
			}
		}
	}
	if ( 0x7FFD <= (ui5b) ( zExp - 1 ) ) {
		if (    ( 0x7FFE < zExp )
			 || (    ( zExp == 0x7FFE )
				  && ( zSig0 == LIT64( 0xFFFFFFFFFFFFFFFF ) )
				  && increment
				)
		   ) {
			roundMask = 0;
 overflow:
			float_raise( float_flag_overflow | float_flag_inexact );
			if (    ( roundingMode == float_round_to_zero )
				 || ( zSign && ( roundingMode == float_round_up ) )
				 || ( ! zSign && ( roundingMode == float_round_down ) )
			   ) {
				return packFloatx80( zSign, 0x7FFE, ~ roundMask );
			}
			return packFloatx80( zSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
		}
		if ( zExp <= 0 ) {
			isTiny =
				   ( float_detect_tininess == float_tininess_before_rounding )
				|| ( zExp < 0 )
				|| ! increment
				|| ( zSig0 < LIT64( 0xFFFFFFFFFFFFFFFF ) );
			shift64ExtraRightJamming( zSig0, zSig1, 1 - zExp, &zSig0, &zSig1 );
			zExp = 0;
			if ( isTiny && zSig1 ) float_raise( float_flag_underflow );
			if ( zSig1 ) float_exception_flags |= float_flag_inexact;
			if ( roundNearestEven ) {
				increment = ( (si6b) zSig1 < 0 );
			}
			else {
				if ( zSign ) {
					increment = ( roundingMode == float_round_down ) && zSig1;
				}
				else {
					increment = ( roundingMode == float_round_up ) && zSig1;
				}
			}
			if ( increment ) {
				++zSig0;
				zSig0 &=
					~ ( ( (ui6b) ( zSig1<<1 ) == 0 ) & roundNearestEven );
				if ( (si6b) zSig0 < 0 ) zExp = 1;
			}
			return packFloatx80( zSign, zExp, zSig0 );
		}
	}
	if ( zSig1 ) float_exception_flags |= float_flag_inexact;
	if ( increment ) {
		++zSig0;
		if ( zSig0 == 0 ) {
			++zExp;
			zSig0 = LIT64( 0x8000000000000000 );
		}
		else {
			zSig0 &= ~ ( ( (ui6b) ( zSig1<<1 ) == 0 ) & roundNearestEven );
		}
	}
	else {
		if ( zSig0 == 0 ) zExp = 0;
	}
	return packFloatx80( zSign, zExp, zSig0 );

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent
| `zExp', and significand formed by the concatenation of `zSig0' and `zSig1',
| and returns the proper extended double-precision floating-point value
| corresponding to the abstract input.  This routine is just like
| `roundAndPackFloatx80' except that the input significand does not have to be
| normalized.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80
 normalizeRoundAndPackFloatx80(
	 si3r roundingPrecision, flag zSign, si5r zExp, ui6b zSig0, ui6b zSig1
 )
{
	si3r shiftCount;

	if ( zSig0 == 0 ) {
		zSig0 = zSig1;
		zSig1 = 0;
		zExp -= 64;
	}
	shiftCount = countLeadingZeros64( zSig0 );
	shortShift128Left( zSig0, zSig1, shiftCount, &zSig0, &zSig1 );
	zExp -= shiftCount;
	return
		roundAndPackFloatx80( roundingPrecision, zSign, zExp, zSig0, zSig1 );

}

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the least-significant 64 fraction bits of the quadruple-precision
| floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC ui6b extractFloat128Frac1( float128 a )
{

	return a.low;

}

/*----------------------------------------------------------------------------
| Returns the most-significant 48 fraction bits of the quadruple-precision
| floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC ui6b extractFloat128Frac0( float128 a )
{

	return a.high & LIT64( 0x0000FFFFFFFFFFFF );

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the quadruple-precision floating-point value
| `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC si5r extractFloat128Exp( float128 a )
{

	return ( a.high>>48 ) & 0x7FFF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the quadruple-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag extractFloat128Sign( float128 a )
{

	return a.high>>63;

}

/*----------------------------------------------------------------------------
| Normalizes the subnormal quadruple-precision floating-point value
| represented by the denormalized significand formed by the concatenation of
| `aSig0' and `aSig1'.  The normalized exponent is stored at the location
| pointed to by `zExpPtr'.  The most significant 49 bits of the normalized
| significand are stored at the location pointed to by `zSig0Ptr', and the
| least significant 64 bits of the normalized significand are stored at the
| location pointed to by `zSig1Ptr'.
*----------------------------------------------------------------------------*/

LOCALPROC
 normalizeFloat128Subnormal(
	 ui6b aSig0,
	 ui6b aSig1,
	 si5r *zExpPtr,
	 ui6b *zSig0Ptr,
	 ui6b *zSig1Ptr
 )
{
	si3r shiftCount;

	if ( aSig0 == 0 ) {
		shiftCount = countLeadingZeros64( aSig1 ) - 15;
		if ( shiftCount < 0 ) {
			*zSig0Ptr = aSig1>>( - shiftCount );
			*zSig1Ptr = aSig1<<( shiftCount & 63 );
		}
		else {
			*zSig0Ptr = aSig1<<shiftCount;
			*zSig1Ptr = 0;
		}
		*zExpPtr = - shiftCount - 63;
	}
	else {
		shiftCount = countLeadingZeros64( aSig0 ) - 15;
		shortShift128Left( aSig0, aSig1, shiftCount, zSig0Ptr, zSig1Ptr );
		*zExpPtr = 1 - shiftCount;
	}

}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', the exponent `zExp', and the significand formed
| by the concatenation of `zSig0' and `zSig1' into a quadruple-precision
| floating-point value, returning the result.  After being shifted into the
| proper positions, the three fields `zSign', `zExp', and `zSig0' are simply
| added together to form the most significant 32 bits of the result.  This
| means that any integer portion of `zSig0' will be added into the exponent.
| Since a properly normalized significand will have an integer portion equal
| to 1, the `zExp' input should be 1 less than the desired result exponent
| whenever `zSig0' and `zSig1' concatenated form a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC float128
 packFloat128( flag zSign, si5r zExp, ui6b zSig0, ui6b zSig1 )
{
	float128 z;

	z.low = zSig1;
	z.high = ( ( (ui6b) zSign )<<63 ) + ( ( (ui6b) zExp )<<48 ) + zSig0;
	return z;

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and extended significand formed by the concatenation of `zSig0', `zSig1',
| and `zSig2', and returns the proper quadruple-precision floating-point value
| corresponding to the abstract input.  Ordinarily, the abstract value is
| simply rounded and packed into the quadruple-precision format, with the
| inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal quadruple-
| precision floating-point number.
|     The input significand must be normalized or smaller.  If the input
| significand is not normalized, `zExp' must be 0; in that case, the result
| returned is a subnormal number, and it must not require rounding.  In the
| usual case that the input significand is normalized, `zExp' must be 1 less
| than the ``true'' floating-point exponent.  The handling of underflow and
| overflow follows the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128
 roundAndPackFloat128(
	 flag zSign, si5r zExp, ui6b zSig0, ui6b zSig1, ui6b zSig2 )
{
	si3r roundingMode;
	flag roundNearestEven, increment, isTiny;

	roundingMode = float_rounding_mode;
	roundNearestEven = ( roundingMode == float_round_nearest_even );
	increment = ( (si6b) zSig2 < 0 );
	if ( ! roundNearestEven ) {
		if ( roundingMode == float_round_to_zero ) {
			increment = 0;
		}
		else {
			if ( zSign ) {
				increment = ( roundingMode == float_round_down ) && zSig2;
			}
			else {
				increment = ( roundingMode == float_round_up ) && zSig2;
			}
		}
	}
	if ( 0x7FFD <= (ui5b) zExp ) {
		if (    ( 0x7FFD < zExp )
			 || (    ( zExp == 0x7FFD )
				  && eq128(
						 LIT64( 0x0001FFFFFFFFFFFF ),
						 LIT64( 0xFFFFFFFFFFFFFFFF ),
						 zSig0,
						 zSig1
					 )
				  && increment
				)
		   ) {
			float_raise( float_flag_overflow | float_flag_inexact );
			if (    ( roundingMode == float_round_to_zero )
				 || ( zSign && ( roundingMode == float_round_up ) )
				 || ( ! zSign && ( roundingMode == float_round_down ) )
			   ) {
				return
					packFloat128(
						zSign,
						0x7FFE,
						LIT64( 0x0000FFFFFFFFFFFF ),
						LIT64( 0xFFFFFFFFFFFFFFFF )
					);
			}
			return packFloat128( zSign, 0x7FFF, 0, 0 );
		}
		if ( zExp < 0 ) {
			isTiny =
				   ( float_detect_tininess == float_tininess_before_rounding )
				|| ( zExp < -1 )
				|| ! increment
				|| lt128(
					   zSig0,
					   zSig1,
					   LIT64( 0x0001FFFFFFFFFFFF ),
					   LIT64( 0xFFFFFFFFFFFFFFFF )
				   );
			shift128ExtraRightJamming(
				zSig0, zSig1, zSig2, - zExp, &zSig0, &zSig1, &zSig2 );
			zExp = 0;
			if ( isTiny && zSig2 ) float_raise( float_flag_underflow );
			if ( roundNearestEven ) {
				increment = ( (si6b) zSig2 < 0 );
			}
			else {
				if ( zSign ) {
					increment = ( roundingMode == float_round_down ) && zSig2;
				}
				else {
					increment = ( roundingMode == float_round_up ) && zSig2;
				}
			}
		}
	}
	if ( zSig2 ) float_exception_flags |= float_flag_inexact;
	if ( increment ) {
		add128( zSig0, zSig1, 0, 1, &zSig0, &zSig1 );
		zSig1 &= ~ ( ( zSig2 + zSig2 == 0 ) & roundNearestEven );
	}
	else {
		if ( ( zSig0 | zSig1 ) == 0 ) zExp = 0;
	}
	return packFloat128( zSign, zExp, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand formed by the concatenation of `zSig0' and `zSig1', and
| returns the proper quadruple-precision floating-point value corresponding
| to the abstract input.  This routine is just like `roundAndPackFloat128'
| except that the input significand has fewer bits and does not have to be
| normalized.  In all cases, `zExp' must be 1 less than the ``true'' floating-
| point exponent.
*----------------------------------------------------------------------------*/

LOCALFUNC float128
 normalizeRoundAndPackFloat128(
	 flag zSign, si5r zExp, ui6b zSig0, ui6b zSig1 )
{
	si3r shiftCount;
	ui6b zSig2;

	if ( zSig0 == 0 ) {
		zSig0 = zSig1;
		zSig1 = 0;
		zExp -= 64;
	}
	shiftCount = countLeadingZeros64( zSig0 ) - 15;
	if ( 0 <= shiftCount ) {
		zSig2 = 0;
		shortShift128Left( zSig0, zSig1, shiftCount, &zSig0, &zSig1 );
	}
	else {
		shift128ExtraRightJamming(
			zSig0, zSig1, 0, - shiftCount, &zSig0, &zSig1, &zSig2 );
	}
	zExp -= shiftCount;
	return roundAndPackFloat128( zSign, zExp, zSig0, zSig1, zSig2 );

}

#endif

/*----------------------------------------------------------------------------
| Returns the result of converting the 32-bit two's complement integer `a'
| to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 int32_to_floatx80( si5r a )
{
	flag zSign;
	ui5r absA;
	si3r shiftCount;
	ui6b zSig;

	if ( a == 0 ) return packFloatx80( 0, 0, 0 );
	zSign = ( a < 0 );
	absA = zSign ? - a : a;
	shiftCount = countLeadingZeros32( absA ) + 32;
	zSig = absA;
	return packFloatx80( zSign, 0x403E - shiftCount, zSig<<shiftCount );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 32-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic---which means in particular that the conversion
| is rounded according to the current rounding mode.  If `a' is a NaN, the
| largest positive integer is returned.  Otherwise, if the conversion
| overflows, the largest integer with the same sign as `a' is returned.
*----------------------------------------------------------------------------*/

LOCALFUNC si5r floatx80_to_int32( floatx80 a )
{
	flag aSign;
	si5r aExp, shiftCount;
	ui6b aSig;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	if ( ( aExp == 0x7FFF ) && (ui6b) ( aSig<<1 ) ) aSign = 0;
	shiftCount = 0x4037 - aExp;
	if ( shiftCount <= 0 ) shiftCount = 1;
	shift64RightJamming( aSig, shiftCount, &aSig );
	return roundAndPackInt32( aSign, aSig );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 32-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic, except that the conversion is always rounded
| toward zero.  If `a' is a NaN, the largest positive integer is returned.
| Otherwise, if the conversion overflows, the largest integer with the same
| sign as `a' is returned.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC si5r floatx80_to_int32_round_to_zero( floatx80 a )
{
	flag aSign;
	si5r aExp, shiftCount;
	ui6b aSig, savedASig;
	si5r z;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	if ( 0x401E < aExp ) {
		if ( ( aExp == 0x7FFF ) && (ui6b) ( aSig<<1 ) ) aSign = 0;
		goto invalid;
	}
	else if ( aExp < 0x3FFF ) {
		if ( aExp || aSig ) float_exception_flags |= float_flag_inexact;
		return 0;
	}
	shiftCount = 0x403E - aExp;
	savedASig = aSig;
	aSig >>= shiftCount;
	z = aSig;
	if ( aSign ) z = - z;
	if ( ( z < 0 ) ^ aSign ) {
 invalid:
		float_raise( float_flag_invalid );
		return aSign ? (si5b) 0x80000000 : 0x7FFFFFFF;
	}
	if ( ( aSig<<shiftCount ) != savedASig ) {
		float_exception_flags |= float_flag_inexact;
	}
	return z;

}
#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the quadruple-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 floatx80_to_float128( floatx80 a )
{
	flag aSign;
	si4r aExp;
	ui6b aSig, zSig0, zSig1;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	if ( ( aExp == 0x7FFF ) && (ui6b) ( aSig<<1 ) ) {
		return commonNaNToFloat128( floatx80ToCommonNaN( a ) );
	}
	shift128Right( aSig<<1, 0, 16, &zSig0, &zSig1 );
	return packFloat128( aSign, aExp, zSig0, zSig1 );

}

#endif

/*----------------------------------------------------------------------------
| Rounds the extended double-precision floating-point value `a' to an integer,
| and returns the result as an extended quadruple-precision floating-point
| value.  The operation is performed according to the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_round_to_int( floatx80 a )
{
	flag aSign;
	si5r aExp;
	ui6b lastBitMask, roundBitsMask;
	si3r roundingMode;
	floatx80 z;

	aExp = extractFloatx80Exp( a );
	if ( 0x403E <= aExp ) {
		if ( ( aExp == 0x7FFF ) && (ui6b) ( extractFloatx80Frac( a )<<1 ) ) {
			return propagateFloatx80NaN( a, a );
		}
		return a;
	}
	if ( aExp < 0x3FFF ) {
		if (    ( aExp == 0 )
			 && ( (ui6b) ( extractFloatx80Frac( a )<<1 ) == 0 ) ) {
			return a;
		}
		float_exception_flags |= float_flag_inexact;
		aSign = extractFloatx80Sign( a );
		switch ( float_rounding_mode ) {
		 case float_round_nearest_even:
			if ( ( aExp == 0x3FFE ) && (ui6b) ( extractFloatx80Frac( a )<<1 )
			   ) {
				return
					packFloatx80( aSign, 0x3FFF, LIT64( 0x8000000000000000 ) );
			}
			break;
		 case float_round_down:
			return
				  aSign ?
					  packFloatx80( 1, 0x3FFF, LIT64( 0x8000000000000000 ) )
				: packFloatx80( 0, 0, 0 );
		 case float_round_up:
			return
				  aSign ? packFloatx80( 1, 0, 0 )
				: packFloatx80( 0, 0x3FFF, LIT64( 0x8000000000000000 ) );
		}
		return packFloatx80( aSign, 0, 0 );
	}
	lastBitMask = 1;
	lastBitMask <<= 0x403E - aExp;
	roundBitsMask = lastBitMask - 1;
	z = a;
	roundingMode = float_rounding_mode;
	if ( roundingMode == float_round_nearest_even ) {
		z.low += lastBitMask>>1;
		if ( ( z.low & roundBitsMask ) == 0 ) z.low &= ~ lastBitMask;
	}
	else if ( roundingMode != float_round_to_zero ) {
		if ( extractFloatx80Sign( z ) ^ ( roundingMode == float_round_up ) ) {
			z.low += roundBitsMask;
		}
	}
	z.low &= ~ roundBitsMask;
	if ( z.low == 0 ) {
		++z.high;
		z.low = LIT64( 0x8000000000000000 );
	}
	if ( z.low != a.low ) float_exception_flags |= float_flag_inexact;
	return z;

}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the extended double-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the sum is
| negated before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 addFloatx80Sigs( floatx80 a, floatx80 b, flag zSign )
{
	si5r aExp, bExp, zExp;
	ui6b aSig, bSig, zSig0, zSig1;
	si5r expDiff;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	expDiff = aExp - bExp;
	if ( 0 < expDiff ) {
		if ( aExp == 0x7FFF ) {
			if ( (ui6b) ( aSig<<1 ) ) return propagateFloatx80NaN( a, b );
			return a;
		}
		if ( bExp == 0 ) --expDiff;
		shift64ExtraRightJamming( bSig, 0, expDiff, &bSig, &zSig1 );
		zExp = aExp;
	}
	else if ( expDiff < 0 ) {
		if ( bExp == 0x7FFF ) {
			if ( (ui6b) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b );
			return packFloatx80( zSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
		}
		if ( aExp == 0 ) ++expDiff;
		shift64ExtraRightJamming( aSig, 0, - expDiff, &aSig, &zSig1 );
		zExp = bExp;
	}
	else {
		if ( aExp == 0x7FFF ) {
			if ( (ui6b) ( ( aSig | bSig )<<1 ) ) {
				return propagateFloatx80NaN( a, b );
			}
			return a;
		}
		zSig1 = 0;
		zSig0 = aSig + bSig;
		if ( aExp == 0 ) {
			normalizeFloatx80Subnormal( zSig0, &zExp, &zSig0 );
			goto roundAndPack;
		}
		zExp = aExp;
		goto shiftRight1;
	}
	zSig0 = aSig + bSig;
	if ( (si6b) zSig0 < 0 ) goto roundAndPack;
 shiftRight1:
	shift64ExtraRightJamming( zSig0, zSig1, 1, &zSig0, &zSig1 );
	zSig0 |= LIT64( 0x8000000000000000 );
	++zExp;
 roundAndPack:
	return
		roundAndPackFloatx80(
			floatx80_rounding_precision, zSign, zExp, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the extended
| double-precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 subFloatx80Sigs( floatx80 a, floatx80 b, flag zSign )
{
	si5r aExp, bExp, zExp;
	ui6b aSig, bSig, zSig0, zSig1;
	si5r expDiff;
	floatx80 z;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	expDiff = aExp - bExp;
	if ( 0 < expDiff ) goto aExpBigger;
	if ( expDiff < 0 ) goto bExpBigger;
	if ( aExp == 0x7FFF ) {
		if ( (ui6b) ( ( aSig | bSig )<<1 ) ) {
			return propagateFloatx80NaN( a, b );
		}
		float_raise( float_flag_invalid );
		z.low = floatx80_default_nan_low;
		z.high = floatx80_default_nan_high;
		return z;
	}
	if ( aExp == 0 ) {
		aExp = 1;
		bExp = 1;
	}
	zSig1 = 0;
	if ( bSig < aSig ) goto aBigger;
	if ( aSig < bSig ) goto bBigger;
	return packFloatx80( float_rounding_mode == float_round_down, 0, 0 );
 bExpBigger:
	if ( bExp == 0x7FFF ) {
		if ( (ui6b) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b );
		return packFloatx80( zSign ^ 1, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( aExp == 0 ) ++expDiff;
	shift128RightJamming( aSig, 0, - expDiff, &aSig, &zSig1 );
 bBigger:
	sub128( bSig, 0, aSig, zSig1, &zSig0, &zSig1 );
	zExp = bExp;
	zSign ^= 1;
	goto normalizeRoundAndPack;
 aExpBigger:
	if ( aExp == 0x7FFF ) {
		if ( (ui6b) ( aSig<<1 ) ) return propagateFloatx80NaN( a, b );
		return a;
	}
	if ( bExp == 0 ) --expDiff;
	shift128RightJamming( bSig, 0, expDiff, &bSig, &zSig1 );
 aBigger:
	sub128( aSig, 0, bSig, zSig1, &zSig0, &zSig1 );
	zExp = aExp;
 normalizeRoundAndPack:
	return
		normalizeRoundAndPackFloatx80(
			floatx80_rounding_precision, zSign, zExp, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Returns the result of adding the extended double-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_add( floatx80 a, floatx80 b )
{
	flag aSign, bSign;

	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign == bSign ) {
		return addFloatx80Sigs( a, b, aSign );
	}
	else {
		return subFloatx80Sigs( a, b, aSign );
	}

}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the extended double-precision floating-
| point values `a' and `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_sub( floatx80 a, floatx80 b )
{
	flag aSign, bSign;

	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign == bSign ) {
		return subFloatx80Sigs( a, b, aSign );
	}
	else {
		return addFloatx80Sigs( a, b, aSign );
	}

}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the extended double-precision floating-
| point values `a' and `b'.  The operation is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_mul( floatx80 a, floatx80 b )
{
	flag aSign, bSign, zSign;
	si5r aExp, bExp, zExp;
	ui6b aSig, bSig, zSig0, zSig1;
	floatx80 z;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	bSign = extractFloatx80Sign( b );
	zSign = aSign ^ bSign;
	if ( aExp == 0x7FFF ) {
		if (    (ui6b) ( aSig<<1 )
			 || ( ( bExp == 0x7FFF ) && (ui6b) ( bSig<<1 ) ) ) {
			return propagateFloatx80NaN( a, b );
		}
		if ( ( bExp | bSig ) == 0 ) goto invalid;
		return packFloatx80( zSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( bExp == 0x7FFF ) {
		if ( (ui6b) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b );
		if ( ( aExp | aSig ) == 0 ) {
 invalid:
			float_raise( float_flag_invalid );
			z.low = floatx80_default_nan_low;
			z.high = floatx80_default_nan_high;
			return z;
		}
		return packFloatx80( zSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( aExp == 0 ) {
		if ( aSig == 0 ) return packFloatx80( zSign, 0, 0 );
		normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
	}
	if ( bExp == 0 ) {
		if ( bSig == 0 ) return packFloatx80( zSign, 0, 0 );
		normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
	}
	zExp = aExp + bExp - 0x3FFE;
	mul64To128( aSig, bSig, &zSig0, &zSig1 );
	if ( 0 < (si6b) zSig0 ) {
		shortShift128Left( zSig0, zSig1, 1, &zSig0, &zSig1 );
		--zExp;
	}
	return
		roundAndPackFloatx80(
			floatx80_rounding_precision, zSign, zExp, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Returns the result of dividing the extended double-precision floating-point
| value `a' by the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_div( floatx80 a, floatx80 b )
{
	flag aSign, bSign, zSign;
	si5r aExp, bExp, zExp;
	ui6b aSig, bSig, zSig0, zSig1;
	ui6b rem0, rem1, rem2, term0, term1, term2;
	floatx80 z;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	bSign = extractFloatx80Sign( b );
	zSign = aSign ^ bSign;
	if ( aExp == 0x7FFF ) {
		if ( (ui6b) ( aSig<<1 ) ) return propagateFloatx80NaN( a, b );
		if ( bExp == 0x7FFF ) {
			if ( (ui6b) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b );
			goto invalid;
		}
		return packFloatx80( zSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( bExp == 0x7FFF ) {
		if ( (ui6b) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b );
		return packFloatx80( zSign, 0, 0 );
	}
	if ( bExp == 0 ) {
		if ( bSig == 0 ) {
			if ( ( aExp | aSig ) == 0 ) {
 invalid:
				float_raise( float_flag_invalid );
				z.low = floatx80_default_nan_low;
				z.high = floatx80_default_nan_high;
				return z;
			}
			float_raise( float_flag_divbyzero );
			return packFloatx80( zSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
		}
		normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
	}
	if ( aExp == 0 ) {
		if ( aSig == 0 ) return packFloatx80( zSign, 0, 0 );
		normalizeFloatx80Subnormal( aSig, &aExp, &aSig );
	}
	if ( aSig == 0 ) {
		/*
			added by pcp. this invalid input seems to
			cause hang in estimateDiv128To64. should
			validate inputs generally.
		*/
		return packFloatx80( zSign, 0, 0 );
	}
	if ( (si6b) bSig >= 0 ) {
		/*
			added by pcp. another check.
			invalid input, most significant bit should be set?
		*/
		return packFloatx80( zSign, 0, 0 );
	}
	zExp = aExp - bExp + 0x3FFE;
	rem1 = 0;
	if ( bSig <= aSig ) {
		shift128Right( aSig, 0, 1, &aSig, &rem1 );
		++zExp;
	}
	zSig0 = estimateDiv128To64( aSig, rem1, bSig );
	mul64To128( bSig, zSig0, &term0, &term1 );
	sub128( aSig, rem1, term0, term1, &rem0, &rem1 );
	while ( (si6b) rem0 < 0 ) {
		--zSig0;
		add128( rem0, rem1, 0, bSig, &rem0, &rem1 );
	}
	zSig1 = estimateDiv128To64( rem1, 0, bSig );
	if ( (ui6b) ( zSig1<<1 ) <= 8 ) {
		mul64To128( bSig, zSig1, &term1, &term2 );
		sub128( rem1, 0, term1, term2, &rem1, &rem2 );
		while ( (si6b) rem1 < 0 ) {
			--zSig1;
			add128( rem1, rem2, 0, bSig, &rem1, &rem2 );
		}
		zSig1 |= ( ( rem1 | rem2 ) != 0 );
	}
	return
		roundAndPackFloatx80(
			floatx80_rounding_precision, zSign, zExp, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Returns the remainder of the extended double-precision floating-point value
| `a' with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_rem( floatx80 a, floatx80 b )
{
	flag aSign, /* bSign, */ zSign;
	si5r aExp, bExp, expDiff;
	ui6b aSig0, aSig1, bSig;
	ui6b q, term0, term1, alternateASig0, alternateASig1;
	floatx80 z;

	aSig0 = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	bSig = extractFloatx80Frac( b );
	bExp = extractFloatx80Exp( b );
	/*
		not used. should it be?
		bSign = extractFloatx80Sign( b );
	*/
	if ( aExp == 0x7FFF ) {
		if (    (ui6b) ( aSig0<<1 )
			 || ( ( bExp == 0x7FFF ) && (ui6b) ( bSig<<1 ) ) ) {
			return propagateFloatx80NaN( a, b );
		}
		goto invalid;
	}
	if ( bExp == 0x7FFF ) {
		if ( (ui6b) ( bSig<<1 ) ) return propagateFloatx80NaN( a, b );
		return a;
	}
	if ( bExp == 0 ) {
		if ( bSig == 0 ) {
 invalid:
			float_raise( float_flag_invalid );
			z.low = floatx80_default_nan_low;
			z.high = floatx80_default_nan_high;
			return z;
		}
		normalizeFloatx80Subnormal( bSig, &bExp, &bSig );
	}
	if ( aExp == 0 ) {
		if ( (ui6b) ( aSig0<<1 ) == 0 ) return a;
		normalizeFloatx80Subnormal( aSig0, &aExp, &aSig0 );
	}
	bSig |= LIT64( 0x8000000000000000 );
	zSign = aSign;
	expDiff = aExp - bExp;
	aSig1 = 0;
	if ( expDiff < 0 ) {
		if ( expDiff < -1 ) return a;
		shift128Right( aSig0, 0, 1, &aSig0, &aSig1 );
		expDiff = 0;
	}
	q = ( bSig <= aSig0 );
	if ( q ) aSig0 -= bSig;
	expDiff -= 64;
	while ( 0 < expDiff ) {
		q = estimateDiv128To64( aSig0, aSig1, bSig );
		q = ( 2 < q ) ? q - 2 : 0;
		mul64To128( bSig, q, &term0, &term1 );
		sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
		shortShift128Left( aSig0, aSig1, 62, &aSig0, &aSig1 );
		expDiff -= 62;
	}
	expDiff += 64;
	if ( 0 < expDiff ) {
		q = estimateDiv128To64( aSig0, aSig1, bSig );
		q = ( 2 < q ) ? q - 2 : 0;
		q >>= 64 - expDiff;
		mul64To128( bSig, q<<( 64 - expDiff ), &term0, &term1 );
		sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
		shortShift128Left( 0, bSig, 64 - expDiff, &term0, &term1 );
		while ( le128( term0, term1, aSig0, aSig1 ) ) {
			++q;
			sub128( aSig0, aSig1, term0, term1, &aSig0, &aSig1 );
		}
	}
	else {
		term1 = 0;
		term0 = bSig;
	}
	sub128( term0, term1, aSig0, aSig1, &alternateASig0, &alternateASig1 );
	if (    lt128( alternateASig0, alternateASig1, aSig0, aSig1 )
		 || (    eq128( alternateASig0, alternateASig1, aSig0, aSig1 )
			  && ( q & 1 ) )
	   ) {
		aSig0 = alternateASig0;
		aSig1 = alternateASig1;
		zSign = ! zSign;
	}
	return
		normalizeRoundAndPackFloatx80(
			80, zSign, bExp + expDiff, aSig0, aSig1 );

}

/*----------------------------------------------------------------------------
| Returns the square root of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_sqrt( floatx80 a )
{
	flag aSign;
	si5r aExp, zExp;
	ui6b aSig0, aSig1, zSig0, zSig1, doubleZSig0;
	ui6b rem0, rem1, rem2, rem3, term0, term1, term2, term3;
	floatx80 z;

	aSig0 = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	if ( aExp == 0x7FFF ) {
		if ( (ui6b) ( aSig0<<1 ) ) return propagateFloatx80NaN( a, a );
		if ( ! aSign ) return a;
		goto invalid;
	}
	if ( aSign ) {
		if ( ( aExp | aSig0 ) == 0 ) return a;
 invalid:
		float_raise( float_flag_invalid );
		z.low = floatx80_default_nan_low;
		z.high = floatx80_default_nan_high;
		return z;
	}
	if ( aExp == 0 ) {
		if ( aSig0 == 0 ) return packFloatx80( 0, 0, 0 );
		normalizeFloatx80Subnormal( aSig0, &aExp, &aSig0 );
	}
	zExp = ( ( aExp - 0x3FFF )>>1 ) + 0x3FFF;
	zSig0 = estimateSqrt32( aExp, aSig0>>32 );
	shift128Right( aSig0, 0, 2 + ( aExp & 1 ), &aSig0, &aSig1 );
	zSig0 = estimateDiv128To64( aSig0, aSig1, zSig0<<32 ) + ( zSig0<<30 );
	doubleZSig0 = zSig0<<1;
	mul64To128( zSig0, zSig0, &term0, &term1 );
	sub128( aSig0, aSig1, term0, term1, &rem0, &rem1 );
	while ( (si6b) rem0 < 0 ) {
		--zSig0;
		doubleZSig0 -= 2;
		add128( rem0, rem1, zSig0>>63, doubleZSig0 | 1, &rem0, &rem1 );
	}
	zSig1 = estimateDiv128To64( rem1, 0, doubleZSig0 );
	if ( ( zSig1 & LIT64( 0x3FFFFFFFFFFFFFFF ) ) <= 5 ) {
		if ( zSig1 == 0 ) zSig1 = 1;
		mul64To128( doubleZSig0, zSig1, &term1, &term2 );
		sub128( rem1, 0, term1, term2, &rem1, &rem2 );
		mul64To128( zSig1, zSig1, &term2, &term3 );
		sub192( rem1, rem2, 0, 0, term2, term3, &rem1, &rem2, &rem3 );
		while ( (si6b) rem1 < 0 ) {
			--zSig1;
			shortShift128Left( 0, zSig1, 1, &term2, &term3 );
			term3 |= 1;
			term2 |= doubleZSig0;
			add192( rem1, rem2, rem3, 0, term2, term3, &rem1, &rem2, &rem3 );
		}
		zSig1 |= ( ( rem1 | rem2 | rem3 ) != 0 );
	}
	shortShift128Left( 0, zSig1, 1, &zSig0, &zSig1 );
	zSig0 |= doubleZSig0;
	return
		roundAndPackFloatx80(
			floatx80_rounding_precision, 0, zExp, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| equal to the corresponding value `b', and 0 otherwise.  The comparison is
| performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC flag floatx80_eq( floatx80 a, floatx80 b )
{

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( a )<<1 ) )
		 || (    ( extractFloatx80Exp( b ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( b )<<1 ) )
	   ) {
		if (    floatx80_is_signaling_nan( a )
			 || floatx80_is_signaling_nan( b ) ) {
			float_raise( float_flag_invalid );
		}
		return 0;
	}
	return
		   ( a.low == b.low )
		&& (    ( a.high == b.high )
			 || (    ( a.low == 0 )
				  && ( (ui4b) ( ( a.high | b.high )<<1 ) == 0 ) )
		   );

}
#endif

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than or equal to the corresponding value `b', and 0 otherwise.  The
| comparison is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC flag floatx80_le( floatx80 a, floatx80 b )
{
	flag aSign, bSign;

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( a )<<1 ) )
		 || (    ( extractFloatx80Exp( b ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( b )<<1 ) )
	   ) {
		float_raise( float_flag_invalid );
		return 0;
	}
	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign != bSign ) {
		return
			   aSign
			|| (    ( ( (ui4b) ( ( a.high | b.high )<<1 ) ) | a.low | b.low )
				 == 0 );
	}
	return
		  aSign ? le128( b.high, b.low, a.high, a.low )
		: le128( a.high, a.low, b.high, b.low );

}
#endif

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is
| less than the corresponding value `b', and 0 otherwise.  The comparison
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC flag floatx80_lt( floatx80 a, floatx80 b )
{
	flag aSign, bSign;

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( a )<<1 ) )
		 || (    ( extractFloatx80Exp( b ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( b )<<1 ) )
	   ) {
		float_raise( float_flag_invalid );
		return 0;
	}
	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign != bSign ) {
		return
			   aSign
			&& (    ( ( (ui4b) ( ( a.high | b.high )<<1 ) ) | a.low | b.low )
				 != 0 );
	}
	return
		  aSign ? lt128( b.high, b.low, a.high, a.low )
		: lt128( a.high, a.low, b.high, b.low );

}
#endif

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is equal
| to the corresponding value `b', and 0 otherwise.  The invalid exception is
| raised if either operand is a NaN.  Otherwise, the comparison is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC flag floatx80_eq_signaling( floatx80 a, floatx80 b )
{

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( a )<<1 ) )
		 || (    ( extractFloatx80Exp( b ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( b )<<1 ) )
	   ) {
		float_raise( float_flag_invalid );
		return 0;
	}
	return
		   ( a.low == b.low )
		&& (    ( a.high == b.high )
			 || (    ( a.low == 0 )
				  && ( (ui4b) ( ( a.high | b.high )<<1 ) == 0 ) )
		   );

}
#endif

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is less
| than or equal to the corresponding value `b', and 0 otherwise.  Quiet NaNs
| do not cause an exception.  Otherwise, the comparison is performed according
| to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC flag floatx80_le_quiet( floatx80 a, floatx80 b )
{
	flag aSign, bSign;

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( a )<<1 ) )
		 || (    ( extractFloatx80Exp( b ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( b )<<1 ) )
	   ) {
		if (    floatx80_is_signaling_nan( a )
			 || floatx80_is_signaling_nan( b ) ) {
			float_raise( float_flag_invalid );
		}
		return 0;
	}
	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign != bSign ) {
		return
			   aSign
			|| (    ( ( (ui4b) ( ( a.high | b.high )<<1 ) ) | a.low | b.low )
				 == 0 );
	}
	return
		  aSign ? le128( b.high, b.low, a.high, a.low )
		: le128( a.high, a.low, b.high, b.low );

}
#endif

/*----------------------------------------------------------------------------
| Returns 1 if the extended double-precision floating-point value `a' is less
| than the corresponding value `b', and 0 otherwise.  Quiet NaNs do not cause
| an exception.  Otherwise, the comparison is performed according to the
| IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC flag floatx80_lt_quiet( floatx80 a, floatx80 b )
{
	flag aSign, bSign;

	if (    (    ( extractFloatx80Exp( a ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( a )<<1 ) )
		 || (    ( extractFloatx80Exp( b ) == 0x7FFF )
			  && (ui6b) ( extractFloatx80Frac( b )<<1 ) )
	   ) {
		if (    floatx80_is_signaling_nan( a )
			 || floatx80_is_signaling_nan( b ) ) {
			float_raise( float_flag_invalid );
		}
		return 0;
	}
	aSign = extractFloatx80Sign( a );
	bSign = extractFloatx80Sign( b );
	if ( aSign != bSign ) {
		return
			   aSign
			&& (    ( ( (ui4b) ( ( a.high | b.high )<<1 ) ) | a.low | b.low )
				 != 0 );
	}
	return
		  aSign ? lt128( b.high, b.low, a.high, a.low )
		: lt128( a.high, a.low, b.high, b.low );

}
#endif

#ifdef FLOAT128

/*----------------------------------------------------------------------------
| Returns the result of converting the quadruple-precision floating-point
| value `a' to the extended double-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 float128_to_floatx80( float128 a )
{
	flag aSign;
	si5r aExp;
	ui6b aSig0, aSig1;

	aSig1 = extractFloat128Frac1( a );
	aSig0 = extractFloat128Frac0( a );
	aExp = extractFloat128Exp( a );
	aSign = extractFloat128Sign( a );
	if ( aExp == 0x7FFF ) {
		if ( aSig0 | aSig1 ) {
			return commonNaNToFloatx80( float128ToCommonNaN( a ) );
		}
		return packFloatx80( aSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( aExp == 0 ) {
		if ( ( aSig0 | aSig1 ) == 0 ) return packFloatx80( aSign, 0, 0 );
		normalizeFloat128Subnormal( aSig0, aSig1, &aExp, &aSig0, &aSig1 );
	}
	else {
		aSig0 |= LIT64( 0x0001000000000000 );
	}
	shortShift128Left( aSig0, aSig1, 15, &aSig0, &aSig1 );
	return roundAndPackFloatx80( 80, aSign, aExp, aSig0, aSig1 );

}

/*----------------------------------------------------------------------------
| Returns the result of adding the absolute values of the quadruple-precision
| floating-point values `a' and `b'.  If `zSign' is 1, the sum is negated
| before being returned.  `zSign' is ignored if the result is a NaN.
| The addition is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 addFloat128Sigs( float128 a, float128 b, flag zSign )
{
	si5r aExp, bExp, zExp;
	ui6b aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2;
	si5r expDiff;

	aSig1 = extractFloat128Frac1( a );
	aSig0 = extractFloat128Frac0( a );
	aExp = extractFloat128Exp( a );
	bSig1 = extractFloat128Frac1( b );
	bSig0 = extractFloat128Frac0( b );
	bExp = extractFloat128Exp( b );
	expDiff = aExp - bExp;
	if ( 0 < expDiff ) {
		if ( aExp == 0x7FFF ) {
			if ( aSig0 | aSig1 ) return propagateFloat128NaN( a, b );
			return a;
		}
		if ( bExp == 0 ) {
			--expDiff;
		}
		else {
			bSig0 |= LIT64( 0x0001000000000000 );
		}
		shift128ExtraRightJamming(
			bSig0, bSig1, 0, expDiff, &bSig0, &bSig1, &zSig2 );
		zExp = aExp;
	}
	else if ( expDiff < 0 ) {
		if ( bExp == 0x7FFF ) {
			if ( bSig0 | bSig1 ) return propagateFloat128NaN( a, b );
			return packFloat128( zSign, 0x7FFF, 0, 0 );
		}
		if ( aExp == 0 ) {
			++expDiff;
		}
		else {
			aSig0 |= LIT64( 0x0001000000000000 );
		}
		shift128ExtraRightJamming(
			aSig0, aSig1, 0, - expDiff, &aSig0, &aSig1, &zSig2 );
		zExp = bExp;
	}
	else {
		if ( aExp == 0x7FFF ) {
			if ( aSig0 | aSig1 | bSig0 | bSig1 ) {
				return propagateFloat128NaN( a, b );
			}
			return a;
		}
		add128( aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1 );
		if ( aExp == 0 ) return packFloat128( zSign, 0, zSig0, zSig1 );
		zSig2 = 0;
		zSig0 |= LIT64( 0x0002000000000000 );
		zExp = aExp;
		goto shiftRight1;
	}
	aSig0 |= LIT64( 0x0001000000000000 );
	add128( aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1 );
	--zExp;
	if ( zSig0 < LIT64( 0x0002000000000000 ) ) goto roundAndPack;
	++zExp;
 shiftRight1:
	shift128ExtraRightJamming(
		zSig0, zSig1, zSig2, 1, &zSig0, &zSig1, &zSig2 );
 roundAndPack:
	return roundAndPackFloat128( zSign, zExp, zSig0, zSig1, zSig2 );

}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the absolute values of the quadruple-
| precision floating-point values `a' and `b'.  If `zSign' is 1, the
| difference is negated before being returned.  `zSign' is ignored if the
| result is a NaN.  The subtraction is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 subFloat128Sigs( float128 a, float128 b, flag zSign )
{
	si5r aExp, bExp, zExp;
	ui6b aSig0, aSig1, bSig0, bSig1, zSig0, zSig1;
	si5r expDiff;
	float128 z;

	aSig1 = extractFloat128Frac1( a );
	aSig0 = extractFloat128Frac0( a );
	aExp = extractFloat128Exp( a );
	bSig1 = extractFloat128Frac1( b );
	bSig0 = extractFloat128Frac0( b );
	bExp = extractFloat128Exp( b );
	expDiff = aExp - bExp;
	shortShift128Left( aSig0, aSig1, 14, &aSig0, &aSig1 );
	shortShift128Left( bSig0, bSig1, 14, &bSig0, &bSig1 );
	if ( 0 < expDiff ) goto aExpBigger;
	if ( expDiff < 0 ) goto bExpBigger;
	if ( aExp == 0x7FFF ) {
		if ( aSig0 | aSig1 | bSig0 | bSig1 ) {
			return propagateFloat128NaN( a, b );
		}
		float_raise( float_flag_invalid );
		z.low = float128_default_nan_low;
		z.high = float128_default_nan_high;
		return z;
	}
	if ( aExp == 0 ) {
		aExp = 1;
		bExp = 1;
	}
	if ( bSig0 < aSig0 ) goto aBigger;
	if ( aSig0 < bSig0 ) goto bBigger;
	if ( bSig1 < aSig1 ) goto aBigger;
	if ( aSig1 < bSig1 ) goto bBigger;
	return packFloat128( float_rounding_mode == float_round_down, 0, 0, 0 );
 bExpBigger:
	if ( bExp == 0x7FFF ) {
		if ( bSig0 | bSig1 ) return propagateFloat128NaN( a, b );
		return packFloat128( zSign ^ 1, 0x7FFF, 0, 0 );
	}
	if ( aExp == 0 ) {
		++expDiff;
	}
	else {
		aSig0 |= LIT64( 0x4000000000000000 );
	}
	shift128RightJamming( aSig0, aSig1, - expDiff, &aSig0, &aSig1 );
	bSig0 |= LIT64( 0x4000000000000000 );
 bBigger:
	sub128( bSig0, bSig1, aSig0, aSig1, &zSig0, &zSig1 );
	zExp = bExp;
	zSign ^= 1;
	goto normalizeRoundAndPack;
 aExpBigger:
	if ( aExp == 0x7FFF ) {
		if ( aSig0 | aSig1 ) return propagateFloat128NaN( a, b );
		return a;
	}
	if ( bExp == 0 ) {
		--expDiff;
	}
	else {
		bSig0 |= LIT64( 0x4000000000000000 );
	}
	shift128RightJamming( bSig0, bSig1, expDiff, &bSig0, &bSig1 );
	aSig0 |= LIT64( 0x4000000000000000 );
 aBigger:
	sub128( aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1 );
	zExp = aExp;
 normalizeRoundAndPack:
	--zExp;
	return normalizeRoundAndPackFloat128( zSign, zExp - 14, zSig0, zSig1 );

}

/*----------------------------------------------------------------------------
| Returns the result of adding the quadruple-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 float128_add( float128 a, float128 b )
{
	flag aSign, bSign;

	aSign = extractFloat128Sign( a );
	bSign = extractFloat128Sign( b );
	if ( aSign == bSign ) {
		return addFloat128Sigs( a, b, aSign );
	}
	else {
		return subFloat128Sigs( a, b, aSign );
	}

}

/*----------------------------------------------------------------------------
| Returns the result of subtracting the quadruple-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 float128_sub( float128 a, float128 b )
{
	flag aSign, bSign;

	aSign = extractFloat128Sign( a );
	bSign = extractFloat128Sign( b );
	if ( aSign == bSign ) {
		return subFloat128Sigs( a, b, aSign );
	}
	else {
		return addFloat128Sigs( a, b, aSign );
	}

}

/*----------------------------------------------------------------------------
| Returns the result of multiplying the quadruple-precision floating-point
| values `a' and `b'.  The operation is performed according to the IEC/IEEE
| Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 float128_mul( float128 a, float128 b )
{
	flag aSign, bSign, zSign;
	si5r aExp, bExp, zExp;
	ui6b aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2, zSig3;
	float128 z;

	aSig1 = extractFloat128Frac1( a );
	aSig0 = extractFloat128Frac0( a );
	aExp = extractFloat128Exp( a );
	aSign = extractFloat128Sign( a );
	bSig1 = extractFloat128Frac1( b );
	bSig0 = extractFloat128Frac0( b );
	bExp = extractFloat128Exp( b );
	bSign = extractFloat128Sign( b );
	zSign = aSign ^ bSign;
	if ( aExp == 0x7FFF ) {
		if (    ( aSig0 | aSig1 )
			 || ( ( bExp == 0x7FFF ) && ( bSig0 | bSig1 ) ) ) {
			return propagateFloat128NaN( a, b );
		}
		if ( ( bExp | bSig0 | bSig1 ) == 0 ) goto invalid;
		return packFloat128( zSign, 0x7FFF, 0, 0 );
	}
	if ( bExp == 0x7FFF ) {
		if ( bSig0 | bSig1 ) return propagateFloat128NaN( a, b );
		if ( ( aExp | aSig0 | aSig1 ) == 0 ) {
 invalid:
			float_raise( float_flag_invalid );
			z.low = float128_default_nan_low;
			z.high = float128_default_nan_high;
			return z;
		}
		return packFloat128( zSign, 0x7FFF, 0, 0 );
	}
	if ( aExp == 0 ) {
		if ( ( aSig0 | aSig1 ) == 0 ) return packFloat128( zSign, 0, 0, 0 );
		normalizeFloat128Subnormal( aSig0, aSig1, &aExp, &aSig0, &aSig1 );
	}
	if ( bExp == 0 ) {
		if ( ( bSig0 | bSig1 ) == 0 ) return packFloat128( zSign, 0, 0, 0 );
		normalizeFloat128Subnormal( bSig0, bSig1, &bExp, &bSig0, &bSig1 );
	}
	zExp = aExp + bExp - 0x4000;
	aSig0 |= LIT64( 0x0001000000000000 );
	shortShift128Left( bSig0, bSig1, 16, &bSig0, &bSig1 );
	mul128To256( aSig0, aSig1, bSig0, bSig1, &zSig0, &zSig1, &zSig2, &zSig3 );
	add128( zSig0, zSig1, aSig0, aSig1, &zSig0, &zSig1 );
	zSig2 |= ( zSig3 != 0 );
	if ( LIT64( 0x0002000000000000 ) <= zSig0 ) {
		shift128ExtraRightJamming(
			zSig0, zSig1, zSig2, 1, &zSig0, &zSig1, &zSig2 );
		++zExp;
	}
	return roundAndPackFloat128( zSign, zExp, zSig0, zSig1, zSig2 );

}

/*----------------------------------------------------------------------------
| Returns the result of dividing the quadruple-precision floating-point value
| `a' by the corresponding value `b'.  The operation is performed according to
| the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float128 float128_div( float128 a, float128 b )
{
	flag aSign, bSign, zSign;
	si5r aExp, bExp, zExp;
	ui6b aSig0, aSig1, bSig0, bSig1, zSig0, zSig1, zSig2;
	ui6b rem0, rem1, rem2, rem3, term0, term1, term2, term3;
	float128 z;

	aSig1 = extractFloat128Frac1( a );
	aSig0 = extractFloat128Frac0( a );
	aExp = extractFloat128Exp( a );
	aSign = extractFloat128Sign( a );
	bSig1 = extractFloat128Frac1( b );
	bSig0 = extractFloat128Frac0( b );
	bExp = extractFloat128Exp( b );
	bSign = extractFloat128Sign( b );
	zSign = aSign ^ bSign;
	if ( aExp == 0x7FFF ) {
		if ( aSig0 | aSig1 ) return propagateFloat128NaN( a, b );
		if ( bExp == 0x7FFF ) {
			if ( bSig0 | bSig1 ) return propagateFloat128NaN( a, b );
			goto invalid;
		}
		return packFloat128( zSign, 0x7FFF, 0, 0 );
	}
	if ( bExp == 0x7FFF ) {
		if ( bSig0 | bSig1 ) return propagateFloat128NaN( a, b );
		return packFloat128( zSign, 0, 0, 0 );
	}
	if ( bExp == 0 ) {
		if ( ( bSig0 | bSig1 ) == 0 ) {
			if ( ( aExp | aSig0 | aSig1 ) == 0 ) {
 invalid:
				float_raise( float_flag_invalid );
				z.low = float128_default_nan_low;
				z.high = float128_default_nan_high;
				return z;
			}
			float_raise( float_flag_divbyzero );
			return packFloat128( zSign, 0x7FFF, 0, 0 );
		}
		normalizeFloat128Subnormal( bSig0, bSig1, &bExp, &bSig0, &bSig1 );
	}
	if ( aExp == 0 ) {
		if ( ( aSig0 | aSig1 ) == 0 ) return packFloat128( zSign, 0, 0, 0 );
		normalizeFloat128Subnormal( aSig0, aSig1, &aExp, &aSig0, &aSig1 );
	}
	zExp = aExp - bExp + 0x3FFD;
	shortShift128Left(
		aSig0 | LIT64( 0x0001000000000000 ), aSig1, 15, &aSig0, &aSig1 );
	shortShift128Left(
		bSig0 | LIT64( 0x0001000000000000 ), bSig1, 15, &bSig0, &bSig1 );
	if ( le128( bSig0, bSig1, aSig0, aSig1 ) ) {
		shift128Right( aSig0, aSig1, 1, &aSig0, &aSig1 );
		++zExp;
	}
	zSig0 = estimateDiv128To64( aSig0, aSig1, bSig0 );
	mul128By64To192( bSig0, bSig1, zSig0, &term0, &term1, &term2 );
	sub192( aSig0, aSig1, 0, term0, term1, term2, &rem0, &rem1, &rem2 );
	while ( (si6b) rem0 < 0 ) {
		--zSig0;
		add192( rem0, rem1, rem2, 0, bSig0, bSig1, &rem0, &rem1, &rem2 );
	}
	zSig1 = estimateDiv128To64( rem1, rem2, bSig0 );
	if ( ( zSig1 & 0x3FFF ) <= 4 ) {
		mul128By64To192( bSig0, bSig1, zSig1, &term1, &term2, &term3 );
		sub192( rem1, rem2, 0, term1, term2, term3, &rem1, &rem2, &rem3 );
		while ( (si6b) rem1 < 0 ) {
			--zSig1;
			add192( rem1, rem2, rem3, 0, bSig0, bSig1, &rem1, &rem2, &rem3 );
		}
		zSig1 |= ( ( rem1 | rem2 | rem3 ) != 0 );
	}
	shift128ExtraRightJamming( zSig0, zSig1, 0, 15, &zSig0, &zSig1, &zSig2 );
	return roundAndPackFloat128( zSign, zExp, zSig0, zSig1, zSig2 );

}

#endif

typedef unsigned int float32;

/*----------------------------------------------------------------------------
| Normalizes the subnormal single-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

LOCALPROC
 normalizeFloat32Subnormal( ui5b aSig, si4r *zExpPtr, ui5b *zSigPtr )
{
	si3r shiftCount;

	shiftCount = countLeadingZeros32( aSig ) - 8;
	*zSigPtr = aSig<<shiftCount;
	*zExpPtr = 1 - shiftCount;

}

/*----------------------------------------------------------------------------
| Returns the fraction bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC ui5b extractFloat32Frac( float32 a )
{

	return a & 0x007FFFFF;

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC si4r extractFloat32Exp( float32 a )
{

	return ( a>>23 ) & 0xFF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the single-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag extractFloat32Sign( float32 a )
{

	return a>>31;

}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

LOCALFUNC flag float32_is_signaling_nan( float32 a )
{

	return ( ( ( a>>22 ) & 0x1FF ) == 0x1FE ) && ( a & 0x003FFFFF );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC commonNaNT float32ToCommonNaN( float32 a )
{
	commonNaNT z;

	if ( float32_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
	z.sign = a>>31;
	z.low = 0;
	z.high = ( (ui6b) a )<<41;
	return z;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 float32_to_floatx80( float32 a )
{
	flag aSign;
	si4r aExp;
	ui5b aSig;

	aSig = extractFloat32Frac( a );
	aExp = extractFloat32Exp( a );
	aSign = extractFloat32Sign( a );
	if ( aExp == 0xFF ) {
		if ( aSig ) return commonNaNToFloatx80( float32ToCommonNaN( a ) );
		return packFloatx80( aSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( aExp == 0 ) {
		if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
		normalizeFloat32Subnormal( aSig, &aExp, &aSig );
	}
	aSig |= 0x00800000;
	return packFloatx80( aSign, aExp + 0x3F80, ( (ui6b) aSig )<<40 );

}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| single-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC float32 packFloat32( flag zSign, si4r zExp, ui5b zSig )
{

	return ( ( (ui5b) zSign )<<31 ) + ( ( (ui5b) zExp )<<23 ) + zSig;

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper single-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the single-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded to
| a subnormal number, and the underflow and inexact exceptions are raised if
| the abstract input cannot be represented exactly as a subnormal single-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 30
| and 29, which is 7 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float32 roundAndPackFloat32( flag zSign, si4r zExp, ui5b zSig )
{
	si3r roundingMode;
	flag roundNearestEven;
	si3r roundIncrement, roundBits;
	flag isTiny;

	roundingMode = float_rounding_mode;
	roundNearestEven = ( roundingMode == float_round_nearest_even );
	roundIncrement = 0x40;
	if ( ! roundNearestEven ) {
		if ( roundingMode == float_round_to_zero ) {
			roundIncrement = 0;
		}
		else {
			roundIncrement = 0x7F;
			if ( zSign ) {
				if ( roundingMode == float_round_up ) roundIncrement = 0;
			}
			else {
				if ( roundingMode == float_round_down ) roundIncrement = 0;
			}
		}
	}
	roundBits = zSig & 0x7F;
	if ( 0xFD <= (ui4b) zExp ) {
		if (    ( 0xFD < zExp )
			 || (    ( zExp == 0xFD )
				  && ( (si5b) ( zSig + roundIncrement ) < 0 ) )
		   ) {
			float_raise( float_flag_overflow | float_flag_inexact );
			return packFloat32( zSign, 0xFF, 0 ) - ( roundIncrement == 0 );
		}
		if ( zExp < 0 ) {
			isTiny =
				   ( float_detect_tininess == float_tininess_before_rounding )
				|| ( zExp < -1 )
				|| ( zSig + roundIncrement < 0x80000000 );
			shift32RightJamming( zSig, - zExp, &zSig );
			zExp = 0;
			roundBits = zSig & 0x7F;
			if ( isTiny && roundBits ) float_raise( float_flag_underflow );
		}
	}
	if ( roundBits ) float_exception_flags |= float_flag_inexact;
	zSig = ( zSig + roundIncrement )>>7;
	zSig &= ~ ( ( ( roundBits ^ 0x40 ) == 0 ) & roundNearestEven );
	if ( zSig == 0 ) zExp = 0;
	return packFloat32( zSign, zExp, zSig );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the single-
| precision floating-point format.
*----------------------------------------------------------------------------*/

LOCALFUNC float32 commonNaNToFloat32( commonNaNT a )
{

	return ( ( (ui5b) a.sign )<<31 ) | 0x7FC00000 | ( a.high>>41 );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the single-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float32 floatx80_to_float32( floatx80 a )
{
	flag aSign;
	si5r aExp;
	ui6b aSig;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	if ( aExp == 0x7FFF ) {
		if ( (ui6b) ( aSig<<1 ) ) {
			return commonNaNToFloat32( floatx80ToCommonNaN( a ) );
		}
		return packFloat32( aSign, 0xFF, 0 );
	}
	shift64RightJamming( aSig, 33, &aSig );
	if ( aExp || aSig ) aExp -= 0x3F81;
	return roundAndPackFloat32( aSign, aExp, aSig );

}


typedef ui6b float64;

/*----------------------------------------------------------------------------
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC ui6b extractFloat64Frac( float64 a )
{

	return a & LIT64( 0x000FFFFFFFFFFFFF );

}

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC si4r extractFloat64Exp( float64 a )
{

	return ( a>>52 ) & 0x7FF;

}

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC flag extractFloat64Sign( float64 a )
{

	return a>>63;

}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

LOCALFUNC flag float64_is_signaling_nan( float64 a )
{

	return
		   ( ( ( a>>51 ) & 0xFFF ) == 0xFFE )
		&& ( a & LIT64( 0x0007FFFFFFFFFFFF ) );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC commonNaNT float64ToCommonNaN( float64 a )
{
	commonNaNT z;

	if ( float64_is_signaling_nan( a ) ) float_raise( float_flag_invalid );
	z.sign = a>>63;
	z.low = 0;
	z.high = a<<12;
	return z;

}

/*----------------------------------------------------------------------------
| Normalizes the subnormal double-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/

LOCALPROC
 normalizeFloat64Subnormal( ui6b aSig, si4r *zExpPtr, ui6b *zSigPtr )
{
	si3r shiftCount;

	shiftCount = countLeadingZeros64( aSig ) - 11;
	*zSigPtr = aSig<<shiftCount;
	*zExpPtr = 1 - shiftCount;

}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point value
| `a' to the extended double-precision floating-point format.  The conversion
| is performed according to the IEC/IEEE Standard for Binary Floating-Point
| Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 float64_to_floatx80( float64 a )
{
	flag aSign;
	si4r aExp;
	ui6b aSig;

	aSig = extractFloat64Frac( a );
	aExp = extractFloat64Exp( a );
	aSign = extractFloat64Sign( a );
	if ( aExp == 0x7FF ) {
		if ( aSig ) return commonNaNToFloatx80( float64ToCommonNaN( a ) );
		return packFloatx80( aSign, 0x7FFF, LIT64( 0x8000000000000000 ) );
	}
	if ( aExp == 0 ) {
		if ( aSig == 0 ) return packFloatx80( aSign, 0, 0 );
		normalizeFloat64Subnormal( aSig, &aExp, &aSig );
	}
	return
		packFloatx80(
			aSign, aExp + 0x3C00, ( aSig | LIT64( 0x0010000000000000 ) )<<11 );

}

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| double-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/

LOCALINLINEFUNC float64 packFloat64( flag zSign, si4r zExp, ui6b zSig )
{

	return ( ( (ui6b) zSign )<<63 ) + ( ( (ui6b) zExp )<<52 ) + zSig;

}

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the double-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded
| to a subnormal number, and the underflow and inexact exceptions are raised
| if the abstract input cannot be represented exactly as a subnormal double-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 62
| and 61, which is 10 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float64 roundAndPackFloat64( flag zSign, si4r zExp, ui6b zSig )
{
	si3r roundingMode;
	flag roundNearestEven;
	si4r roundIncrement, roundBits;
	flag isTiny;

	roundingMode = float_rounding_mode;
	roundNearestEven = ( roundingMode == float_round_nearest_even );
	roundIncrement = 0x200;
	if ( ! roundNearestEven ) {
		if ( roundingMode == float_round_to_zero ) {
			roundIncrement = 0;
		}
		else {
			roundIncrement = 0x3FF;
			if ( zSign ) {
				if ( roundingMode == float_round_up ) roundIncrement = 0;
			}
			else {
				if ( roundingMode == float_round_down ) roundIncrement = 0;
			}
		}
	}
	roundBits = zSig & 0x3FF;
	if ( 0x7FD <= (ui4b) zExp ) {
		if (    ( 0x7FD < zExp )
			 || (    ( zExp == 0x7FD )
				  && ( (si6b) ( zSig + roundIncrement ) < 0 ) )
		   ) {
			float_raise( float_flag_overflow | float_flag_inexact );
			return packFloat64( zSign, 0x7FF, 0 ) - ( roundIncrement == 0 );
		}
		if ( zExp < 0 ) {
			isTiny =
				   ( float_detect_tininess == float_tininess_before_rounding )
				|| ( zExp < -1 )
				|| ( zSig + roundIncrement < LIT64( 0x8000000000000000 ) );
			shift64RightJamming( zSig, - zExp, &zSig );
			zExp = 0;
			roundBits = zSig & 0x3FF;
			if ( isTiny && roundBits ) float_raise( float_flag_underflow );
		}
	}
	if ( roundBits ) float_exception_flags |= float_flag_inexact;
	zSig = ( zSig + roundIncrement )>>10;
	zSig &= ~ ( ( ( roundBits ^ 0x200 ) == 0 ) & roundNearestEven );
	if ( zSig == 0 ) zExp = 0;
	return packFloat64( zSign, zExp, zSig );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the double-
| precision floating-point format.
*----------------------------------------------------------------------------*/

LOCALFUNC float64 commonNaNToFloat64( commonNaNT a )
{

	return
		  ( ( (ui6b) a.sign )<<63 )
		| LIT64( 0x7FF8000000000000 )
		| ( a.high>>12 );

}

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the double-precision floating-point format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC float64 floatx80_to_float64( floatx80 a )
{
	flag aSign;
	si5r aExp;
	ui6b aSig, zSig;

	aSig = extractFloatx80Frac( a );
	aExp = extractFloatx80Exp( a );
	aSign = extractFloatx80Sign( a );
	if ( aExp == 0x7FFF ) {
		if ( (ui6b) ( aSig<<1 ) ) {
			return commonNaNToFloat64( floatx80ToCommonNaN( a ) );
		}
		return packFloat64( aSign, 0x7FF, 0 );
	}
	shift64RightJamming( aSig, 1, &zSig );
	if ( aExp || aSig ) aExp -= 0x3C01;
	return roundAndPackFloat64( aSign, aExp, zSig );

}

/* ----- end from original file "softfloat.c" ----- */

typedef si4r Bit16s;
typedef si5r Bit32s;
typedef si6r Bit64s;

typedef ui5r Bit32u;
typedef ui6r Bit64u;

#define int16_indefinite 0x8000

/*----------------------------------------------------------------------------
| Takes extended double-precision floating-point  NaN  `a' and returns the
| appropriate NaN result. If `a' is a signaling NaN, the invalid exception
| is raised.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 propagateOneFloatx80NaN(floatx80 *a)
{
	if (floatx80_is_signaling_nan(*a))
		float_raise(float_flag_invalid);

	a->low |= LIT64(0xC000000000000000);

	return *a;
}

#define float_flag_denormal 0x02

#define floatx80_default_nan_exp 0xFFFF
#define floatx80_default_nan_fraction LIT64(0xC000000000000000)

#define packFloatx80m(zSign, zExp, zSig) {(zSig), ((zSign) << 15) + (zExp) }

/*----------------------------------------------------------------------------
| Packs two 64-bit precision integers into into the quadruple-precision
| floating-point value, returning the result.
*----------------------------------------------------------------------------*/

#define packFloat2x128m(zHi, zLo) {(zLo), (zHi)}
#define PACK_FLOAT_128(hi,lo) packFloat2x128m(LIT64(hi),LIT64(lo))

static const floatx80 floatx80_default_nan =
	packFloatx80m(0, floatx80_default_nan_exp, floatx80_default_nan_fraction);

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point ordering relations
*----------------------------------------------------------------------------*/
enum {
	float_relation_less      = -1,
	float_relation_equal     =  0,
	float_relation_greater   =  1,
	float_relation_unordered =  2
};

#define EXP_BIAS 0x3FFF


/*----------------------------------------------------------------------------
| Returns the result of multiplying the extended double-precision floating-
| point value `a' and quadruple-precision floating point value `b'. The
| operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_mul128(floatx80 a, float128 b)
{
	Bit32s aExp, bExp, zExp;
	Bit64u aSig, bSig0, bSig1, zSig0, zSig1, zSig2;
	int aSign, bSign, zSign;

	// handle unsupported extended double-precision floating encodings
	if (0 /* floatx80_is_unsupported(a) */)
	{
 invalid:
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}

	aSig = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	aSign = extractFloatx80Sign(a);
	bSig0 = extractFloat128Frac0(b);
	bSig1 = extractFloat128Frac1(b);
	bExp = extractFloat128Exp(b);
	bSign = extractFloat128Sign(b);

	zSign = aSign ^ bSign;

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig<<1)
			 || ((bExp == 0x7FFF) && (bSig0 | bSig1)))
		{
			floatx80 r = commonNaNToFloatx80(float128ToCommonNaN(b));
			return propagateFloatx80NaN(a, r);
		}
		if (bExp == 0) {
			if ((bSig0 | bSig1) == 0) goto invalid;
			float_raise(float_flag_denormal);
		}
		return packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
	}
	if (bExp == 0x7FFF) {
		if (bSig0 | bSig1) {
			floatx80 r = commonNaNToFloatx80(float128ToCommonNaN(b));
			return propagateFloatx80NaN(a, r);
		}
		if (aExp == 0) {
			if (aSig == 0) goto invalid;
			float_raise(float_flag_denormal);
		}
		return packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
	}
	if (aExp == 0) {
		if (aSig == 0) {
			if ((bExp == 0) && (bSig0 | bSig1)) float_raise(float_flag_denormal);
			return packFloatx80(zSign, 0, 0);
		}
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
	}
	if (bExp == 0) {
		if ((bSig0 | bSig1) == 0) return packFloatx80(zSign, 0, 0);
		float_raise(float_flag_denormal);
		normalizeFloat128Subnormal(bSig0, bSig1, &bExp, &bSig0, &bSig1);
	}
	else bSig0 |= LIT64(0x0001000000000000);

	zExp = aExp + bExp - 0x3FFE;
	shortShift128Left(bSig0, bSig1, 15, &bSig0, &bSig1);
	mul128By64To192(bSig0, bSig1, aSig, &zSig0, &zSig1, &zSig2);
	if (0 < (Bit64s) zSig0) {
		shortShift128Left(zSig0, zSig1, 1, &zSig0, &zSig1);
		--zExp;
	}
	return
		roundAndPackFloatx80(floatx80_rounding_precision,
			 zSign, zExp, zSig0, zSig1);
}

/* ----- from original file "softfloatx80.h" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/


/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point class.
*----------------------------------------------------------------------------*/
typedef enum {
	float_zero,
	float_NaN,
	float_negative_inf,
	float_positive_inf,
	float_denormal,
	float_normalized
} float_class_t;


/*-----------------------------------------------------------------------------
| Calculates the absolute value of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#if 0
inline floatx80 floatx80_abs(floatx80 *reg)
{
	reg->high &= 0x7FFF;
	return *reg;
}
#endif

/*-----------------------------------------------------------------------------
| Changes the sign of the extended double-precision floating-point value 'a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_chs(floatx80 *x)
{
	x->high ^= 0x8000;
	return *x;
}

/* ----- end from original file "softfloatx80.h" ----- */

/* ----- from original file "softfloatx80.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 16-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic - which means in particular that the conversion
| is rounded according to the current rounding mode. If `a' is a NaN or the
| conversion overflows, the integer indefinite value is returned.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC Bit16s floatx80_to_int16(floatx80 a)
{
#if 0
   if (floatx80_is_unsupported(a))
   {
		float_raise(float_flag_invalid);
		return int16_indefinite;
   }
#endif

   Bit32s v32 = floatx80_to_int32(a);

   if ((v32 > 32767) || (v32 < -32768)) {
		float_exception_flags = float_flag_invalid; // throw way other flags
		return int16_indefinite;
   }

   return (Bit16s) v32;
}
#endif

/*----------------------------------------------------------------------------
| Returns the result of converting the extended double-precision floating-
| point value `a' to the 16-bit two's complement integer format.  The
| conversion is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic, except that the conversion is always rounded
| toward zero.  If `a' is a NaN or the conversion overflows, the integer
| indefinite value is returned.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC Bit16s floatx80_to_int16_round_to_zero(floatx80 a)
{
#if 0
   if (floatx80_is_unsupported(a))
   {
		float_raise(float_flag_invalid);
		return int16_indefinite;
   }
#endif

   Bit32s v32 = floatx80_to_int32_round_to_zero(a);

   if ((v32 > 32767) || (v32 < -32768)) {
		float_exception_flags = float_flag_invalid; // throw way other flags
		return int16_indefinite;
   }

   return (Bit16s) v32;
}
#endif

/*----------------------------------------------------------------------------
| Separate the source extended double-precision floating point value `a'
| into its exponent and significand, store the significant back to the
| 'a' and return the exponent. The operation performed is a superset of
| the IEC/IEEE recommended logb(x) function.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_extract(floatx80 *a)
{
	Bit64u aSig = extractFloatx80Frac(*a);
	Bit32s aExp = extractFloatx80Exp(*a);
	int   aSign = extractFloatx80Sign(*a);

#if 0
	if (floatx80_is_unsupported(*a))
	{
		float_raise(float_flag_invalid);
		*a = floatx80_default_nan;
		return *a;
	}
#endif

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig<<1))
		{
			*a = propagateOneFloatx80NaN(a);
			return *a;
		}
		return packFloatx80(0, 0x7FFF, LIT64(0x8000000000000000));
	}
	if (aExp == 0)
	{
		if (aSig == 0) {
			float_raise(float_flag_divbyzero);
			*a = packFloatx80(aSign, 0, 0);
			return packFloatx80(1, 0x7FFF, LIT64(0x8000000000000000));
		}
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
	}

	a->high = (aSign << 15) + 0x3FFF;
	a->low = aSig;
	return int32_to_floatx80(aExp - 0x3FFF);
}

/*----------------------------------------------------------------------------
| Scales extended double-precision floating-point value in operand `a' by
| value `b'. The function truncates the value in the second operand 'b' to
| an integral value and adds that value to the exponent of the operand 'a'.
| The operation performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_scale(floatx80 a, floatx80 b)
{
	int shiftCount;
	Bit32s scale;

	// handle unsupported extended double-precision floating encodings
#if 0
	if (floatx80_is_unsupported(a) || floatx80_is_unsupported(b))
	{
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}
#endif

	Bit64u aSig = extractFloatx80Frac(a);
	Bit32s aExp = extractFloatx80Exp(a);
	int aSign = extractFloatx80Sign(a);
	Bit64u bSig = extractFloatx80Frac(b);
	Bit32s bExp = extractFloatx80Exp(b);
	int bSign = extractFloatx80Sign(b);

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig<<1) || ((bExp == 0x7FFF) && (Bit64u) (bSig<<1)))
		{
			return propagateFloatx80NaN(a, b);
		}
		if ((bExp == 0x7FFF) && bSign) {
			float_raise(float_flag_invalid);
			return floatx80_default_nan;
		}
		if (bSig && (bExp == 0)) float_raise(float_flag_denormal);
		return a;
	}
	if (bExp == 0x7FFF) {
		if ((Bit64u) (bSig<<1)) return propagateFloatx80NaN(a, b);
		if ((aExp | aSig) == 0) {
			if (! bSign) {
				float_raise(float_flag_invalid);
				return floatx80_default_nan;
			}
			return a;
		}
		if (aSig && (aExp == 0)) float_raise(float_flag_denormal);
		if (bSign) return packFloatx80(aSign, 0, 0);
		return packFloatx80(aSign, 0x7FFF, LIT64(0x8000000000000000));
	}
	if (aExp == 0) {
		if (bSig && (bExp == 0)) float_raise(float_flag_denormal);
		if (aSig == 0) return a;
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
		if (bExp < 0x3FFF)
			return normalizeRoundAndPackFloatx80(80, aSign, aExp, aSig, 0);
	}
	if (bExp == 0) {
		if (bSig == 0) return a;
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
	}

	if (bExp > 0x400E) {
		/* generate appropriate overflow/underflow */
		return roundAndPackFloatx80(80, aSign,
						  bSign ? -0x3FFF : 0x7FFF, aSig, 0);
	}

	if (bExp < 0x3FFF) return a;

	shiftCount = 0x403E - bExp;
	bSig >>= shiftCount;
	scale = (Bit32s) bSig;
	if (bSign) scale = -scale; /* -32768..32767 */
	return
		roundAndPackFloatx80(80, aSign, aExp+scale, aSig, 0);
}

/*----------------------------------------------------------------------------
| Determine extended-precision floating-point number class.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC float_class_t floatx80_class(floatx80 a)
{
   Bit32s aExp = extractFloatx80Exp(a);
   Bit64u aSig = extractFloatx80Frac(a);

   if(aExp == 0) {
	   if (aSig == 0)
		   return float_zero;

	   /* denormal or pseudo-denormal */
	   return float_denormal;
   }

   /* valid numbers have the MS bit set */
   if (!(aSig & LIT64(0x8000000000000000)))
	   return float_NaN; /* report unsupported as NaNs */

   if(aExp == 0x7fff) {
	   int aSign = extractFloatx80Sign(a);

	   if (((Bit64u) (aSig<< 1)) == 0)
		   return (aSign) ? float_negative_inf : float_positive_inf;

	   return float_NaN;
   }

   return float_normalized;
}
#endif

/*----------------------------------------------------------------------------
| Compare  between  two extended precision  floating  point  numbers. Returns
| 'float_relation_equal'  if the operands are equal, 'float_relation_less' if
| the    value    'a'   is   less   than   the   corresponding   value   `b',
| 'float_relation_greater' if the value 'a' is greater than the corresponding
| value `b', or 'float_relation_unordered' otherwise.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC int floatx80_compare(floatx80 a, floatx80 b)
{
	int aSign;
	int bSign;
	Bit64u aSig;
	Bit32s aExp;
	Bit64u bSig;
	Bit32s bExp;
	int less_than;
	float_class_t aClass = floatx80_class(a);
	float_class_t bClass = floatx80_class(b);

	if (aClass == float_NaN || bClass == float_NaN)
	{
		float_raise(float_flag_invalid);
		return float_relation_unordered;
	}

	if (aClass == float_denormal || bClass == float_denormal)
	{
		float_raise(float_flag_denormal);
	}

	aSign = extractFloatx80Sign(a);
	bSign = extractFloatx80Sign(b);

	if (aClass == float_zero) {
		if (bClass == float_zero) return float_relation_equal;
		return bSign ? float_relation_greater : float_relation_less;
	}

	if (bClass == float_zero || aSign != bSign) {
		return aSign ? float_relation_less : float_relation_greater;
	}

	aSig = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	bSig = extractFloatx80Frac(b);
	bExp = extractFloatx80Exp(b);

	if (aClass == float_denormal)
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);

	if (bClass == float_denormal)
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);

	if (aExp == bExp && aSig == bSig)
		return float_relation_equal;

	less_than =
		aSign ? ((bExp < aExp) || ((bExp == aExp) && (bSig < aSig)))
			  : ((aExp < bExp) || ((aExp == bExp) && (aSig < bSig)));

	if (less_than) return float_relation_less;

	return float_relation_greater;
}
#endif

/*----------------------------------------------------------------------------
| Compare  between  two extended precision  floating  point  numbers. Returns
| 'float_relation_equal'  if the operands are equal, 'float_relation_less' if
| the    value    'a'   is   less   than   the   corresponding   value   `b',
| 'float_relation_greater' if the value 'a' is greater than the corresponding
| value `b', or 'float_relation_unordered' otherwise. Quiet NaNs do not cause
| an exception.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC int floatx80_compare_quiet(floatx80 a, floatx80 b)
{
	int aSign;
	int bSign;
	Bit64u aSig;
	Bit32s aExp;
	Bit64u bSig;
	Bit32s bExp;
	int less_than;
	float_class_t aClass = floatx80_class(a);
	float_class_t bClass = floatx80_class(b);

	if (aClass == float_NaN || bClass == float_NaN)
	{
#if 0
		if (floatx80_is_unsupported(a) || floatx80_is_unsupported(b))
			float_raise(float_flag_invalid);
#endif

		if (floatx80_is_signaling_nan(a) || floatx80_is_signaling_nan(b))
			float_raise(float_flag_invalid);

		return float_relation_unordered;
	}

	if (aClass == float_denormal || bClass == float_denormal)
	{
		float_raise(float_flag_denormal);
	}

	aSign = extractFloatx80Sign(a);
	bSign = extractFloatx80Sign(b);

	if (aClass == float_zero) {
		if (bClass == float_zero) return float_relation_equal;
		return bSign ? float_relation_greater : float_relation_less;
	}

	if (bClass == float_zero || aSign != bSign) {
		return aSign ? float_relation_less : float_relation_greater;
	}

	aSig = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	bSig = extractFloatx80Frac(b);
	bExp = extractFloatx80Exp(b);

	if (aClass == float_denormal)
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);

	if (bClass == float_denormal)
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);

	if (aExp == bExp && aSig == bSig)
		return float_relation_equal;

	less_than =
		aSign ? ((bExp < aExp) || ((bExp == aExp) && (bSig < aSig)))
			  : ((aExp < bExp) || ((aExp == bExp) && (aSig < bSig)));

	if (less_than) return float_relation_less;
	return float_relation_greater;
}
#endif

/* ----- end from original file "softfloatx80.cc" ----- */

/* ----- from original file "fprem.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

#define USE_estimateDiv128To64

/* executes single exponent reduction cycle */
LOCALFUNC Bit64u remainder_kernel(Bit64u aSig0, Bit64u bSig, int expDiff, Bit64u *zSig0, Bit64u *zSig1)
{
	Bit64u term0, term1;
	Bit64u aSig1 = 0;
	Bit64u q;

	shortShift128Left(aSig1, aSig0, expDiff, &aSig1, &aSig0);
	q = estimateDiv128To64(aSig1, aSig0, bSig);
	mul64To128(bSig, q, &term0, &term1);
	sub128(aSig1, aSig0, term0, term1, zSig1, zSig0);
	while ((Bit64s)(*zSig1) < 0) {
		--q;
		add128(*zSig1, *zSig0, 0, bSig, zSig1, zSig0);
	}
	return q;
}

LOCALFUNC floatx80 do_fprem(floatx80 a, floatx80 b, Bit64u *q, int rounding_mode)
{
	Bit32s aExp, bExp, zExp, expDiff;
	Bit64u aSig0, aSig1, bSig;
	int aSign;
	*q = 0;

#if 0
	// handle unsupported extended double-precision floating encodings
	if (floatx80_is_unsupported(a) || floatx80_is_unsupported(b))
	{
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}
#endif

	aSig0 = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	aSign = extractFloatx80Sign(a);
	bSig = extractFloatx80Frac(b);
	bExp = extractFloatx80Exp(b);

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig0<<1) || ((bExp == 0x7FFF) && (Bit64u) (bSig<<1))) {
			return propagateFloatx80NaN(a, b);
		}
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}
	if (bExp == 0x7FFF) {
		if ((Bit64u) (bSig<<1)) return propagateFloatx80NaN(a, b);
		if (aExp == 0 && aSig0) {
			float_raise(float_flag_denormal);
			normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
			return (a.low & LIT64(0x8000000000000000)) ?
					packFloatx80(aSign, aExp, aSig0) : a;
		}
		return a;
	}
	if (bExp == 0) {
		if (bSig == 0) {
			float_raise(float_flag_invalid);
			return floatx80_default_nan;
		}
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
	}
	if (aExp == 0) {
		if (aSig0 == 0) return a;
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
	}
	expDiff = aExp - bExp;
	aSig1 = 0;

	if (expDiff >= 64) {
		int n = (expDiff & 0x1f) | 0x20;
		remainder_kernel(aSig0, bSig, n, &aSig0, &aSig1);
		zExp = aExp - n;
		*q = (Bit64u) -1;
	}
	else {
		zExp = bExp;

		if (expDiff < 0) {
			if (expDiff < -1)
			   return (a.low & LIT64(0x8000000000000000)) ?
					packFloatx80(aSign, aExp, aSig0) : a;
			shift128Right(aSig0, 0, 1, &aSig0, &aSig1);
			expDiff = 0;
		}

		if (expDiff > 0) {
			*q = remainder_kernel(aSig0, bSig, expDiff, &aSig0, &aSig1);
		}
		else {
			if (bSig <= aSig0) {
			   aSig0 -= bSig;
			   *q = 1;
			}
		}

		if (rounding_mode == float_round_nearest_even)
		{
			Bit64u term0, term1;
			shift128Right(bSig, 0, 1, &term0, &term1);

			if (! lt128(aSig0, aSig1, term0, term1))
			{
			   int lt = lt128(term0, term1, aSig0, aSig1);
			   int eq = eq128(aSig0, aSig1, term0, term1);

			   if ((eq && (*q & 1)) || lt) {
				  aSign = !aSign;
				  ++*q;
			   }
			   if (lt) sub128(bSig, 0, aSig0, aSig1, &aSig0, &aSig1);
			}
		}
	}

	return normalizeRoundAndPackFloatx80(80, aSign, zExp, aSig0, aSig1);
}

/*----------------------------------------------------------------------------
| Returns the remainder of the extended double-precision floating-point value
| `a' with respect to the corresponding value `b'.  The operation is performed
| according to the IEC/IEEE Standard for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

#if cIncludeFPUUnused
LOCALFUNC floatx80 floatx80_ieee754_remainder(floatx80 a, floatx80 b, Bit64u *q)
{
	return do_fprem(a, b, q, float_round_nearest_even);
}
#endif

/*----------------------------------------------------------------------------
| Returns the remainder of the extended double-precision floating-point value
| `a' with  respect to  the corresponding value `b'. Unlike previous function
| the  function  does not compute  the remainder  specified  in  the IEC/IEEE
| Standard  for Binary  Floating-Point  Arithmetic.  This  function  operates
| differently  from the  previous  function in  the way  that it  rounds  the
| quotient of 'a' divided by 'b' to an integer.
*----------------------------------------------------------------------------*/

LOCALFUNC floatx80 floatx80_remainder(floatx80 a, floatx80 b, Bit64u *q)
{
	return do_fprem(a, b, q, float_round_to_zero);
}

/* ----- end from original file "fprem.cc" ----- */

/* ----- from original file "fpu_constant.h" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

// Pentium CPU uses only 68-bit precision M_PI approximation
// #define BETTER_THAN_PENTIUM

//////////////////////////////
// PI, PI/2, PI/4 constants
//////////////////////////////

#define FLOATX80_PI_EXP  (0x4000)

// 128-bit PI fraction
#ifdef BETTER_THAN_PENTIUM
#define FLOAT_PI_HI (LIT64(0xc90fdaa22168c234))
#define FLOAT_PI_LO (LIT64(0xc4c6628b80dc1cd1))
#else
#define FLOAT_PI_HI (LIT64(0xc90fdaa22168c234))
#define FLOAT_PI_LO (LIT64(0xC000000000000000))
#endif

#define FLOATX80_PI2_EXP  (0x3FFF)
#define FLOATX80_PI4_EXP  (0x3FFE)

//////////////////////////////
// 3PI/4 constant
//////////////////////////////

#define FLOATX80_3PI4_EXP (0x4000)

// 128-bit 3PI/4 fraction
#ifdef BETTER_THAN_PENTIUM
#define FLOAT_3PI4_HI (LIT64(0x96cbe3f9990e91a7))
#define FLOAT_3PI4_LO (LIT64(0x9394c9e8a0a5159c))
#else
#define FLOAT_3PI4_HI (LIT64(0x96cbe3f9990e91a7))
#define FLOAT_3PI4_LO (LIT64(0x9000000000000000))
#endif

//////////////////////////////
// 1/LN2 constant
//////////////////////////////

#define FLOAT_LN2INV_EXP  (0x3FFF)

// 128-bit 1/LN2 fraction
#ifdef BETTER_THAN_PENTIUM
#define FLOAT_LN2INV_HI (LIT64(0xb8aa3b295c17f0bb))
#define FLOAT_LN2INV_LO (LIT64(0xbe87fed0691d3e89))
#else
#define FLOAT_LN2INV_HI (LIT64(0xb8aa3b295c17f0bb))
#define FLOAT_LN2INV_LO (LIT64(0xC000000000000000))
#endif

/* ----- end from original file "fpu_constant.h" ----- */

/* ----- from original file "poly.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

//                            2         3         4               n
// f(x) ~ C + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//         0    1         2         3         4               n
//
//          --       2k                --        2k+1
//   p(x) = >  C  * x           q(x) = >  C   * x
//          --  2k                     --  2k+1
//
//   f(x) ~ [ p(x) + x * q(x) ]
//

LOCALFUNC float128 EvalPoly(float128 x, float128 *arr, unsigned n)
{
	float128 r2;
	float128 x2 = float128_mul(x, x);
	unsigned i;

#if 0
	assert(n > 1);
#endif

	float128 r1 = arr[--n];
	i = n;
	while(i >= 2) {
		r1 = float128_mul(r1, x2);
		i -= 2;
		r1 = float128_add(r1, arr[i]);
	}
	if (i) r1 = float128_mul(r1, x);

	r2 = arr[--n];
	i = n;
	while(i >= 2) {
		r2 = float128_mul(r2, x2);
		i -= 2;
		r2 = float128_add(r2, arr[i]);
	}
	if (i) r2 = float128_mul(r2, x);

	return float128_add(r1, r2);
}

//                  2         4         6         8               2n
// f(x) ~ C + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//         0    1         2         3         4               n
//
//          --       4k                --        4k+2
//   p(x) = >  C  * x           q(x) = >  C   * x
//          --  2k                     --  2k+1
//
//                    2
//   f(x) ~ [ p(x) + x * q(x) ]
//

LOCALFUNC float128 EvenPoly(float128 x, float128 *arr, unsigned n)
{
	 return EvalPoly(float128_mul(x, x), arr, n);
}

//                        3         5         7         9               2n+1
// f(x) ~ (C * x) + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//          0         1         2         3         4               n
//                        2         4         6         8               2n
//      = x * [ C + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//               0    1         2         3         4               n
//
//          --       4k                --        4k+2
//   p(x) = >  C  * x           q(x) = >  C   * x
//          --  2k                     --  2k+1
//
//                        2
//   f(x) ~ x * [ p(x) + x * q(x) ]
//

LOCALFUNC float128 OddPoly(float128 x, float128 *arr, unsigned n)
{
	 return float128_mul(x, EvenPoly(x, arr, n));
}

/* ----- end from original file "poly.cc" ----- */

/* ----- from original file "fyl2x.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

static const floatx80 floatx80_one =
	packFloatx80m(0, 0x3fff, LIT64(0x8000000000000000));

static const float128 float128_one =
	packFloat2x128m(LIT64(0x3fff000000000000), LIT64(0x0000000000000000));
static const float128 float128_two =
	packFloat2x128m(LIT64(0x4000000000000000), LIT64(0x0000000000000000));

static const float128 float128_ln2inv2 =
	packFloat2x128m(LIT64(0x400071547652b82f), LIT64(0xe1777d0ffda0d23a));

#define SQRT2_HALF_SIG 	LIT64(0xb504f333f9de6484)

#define L2_ARR_SIZE 9

static float128 ln_arr[L2_ARR_SIZE] =
{
	PACK_FLOAT_128(0x3fff000000000000, 0x0000000000000000), /*  1 */
	PACK_FLOAT_128(0x3ffd555555555555, 0x5555555555555555), /*  3 */
	PACK_FLOAT_128(0x3ffc999999999999, 0x999999999999999a), /*  5 */
	PACK_FLOAT_128(0x3ffc249249249249, 0x2492492492492492), /*  7 */
	PACK_FLOAT_128(0x3ffbc71c71c71c71, 0xc71c71c71c71c71c), /*  9 */
	PACK_FLOAT_128(0x3ffb745d1745d174, 0x5d1745d1745d1746), /* 11 */
	PACK_FLOAT_128(0x3ffb3b13b13b13b1, 0x3b13b13b13b13b14), /* 13 */
	PACK_FLOAT_128(0x3ffb111111111111, 0x1111111111111111), /* 15 */
	PACK_FLOAT_128(0x3ffae1e1e1e1e1e1, 0xe1e1e1e1e1e1e1e2)  /* 17 */
};

LOCALFUNC float128 poly_ln(float128 x1)
{
/*
	//
	//                     3     5     7     9     11     13     15
	//        1+u         u     u     u     u     u      u      u
	// 1/2 ln ---  ~ u + --- + --- + --- + --- + ---- + ---- + ---- =
	//        1-u         3     5     7     9     11     13     15
	//
	//                     2     4     6     8     10     12     14
	//                    u     u     u     u     u      u      u
	//       = u * [ 1 + --- + --- + --- + --- + ---- + ---- + ---- ] =
	//                    3     5     7     9     11     13     15
	//
	//           3                          3
	//          --       4k                --        4k+2
	//   p(u) = >  C  * u           q(u) = >  C   * u
	//          --  2k                     --  2k+1
	//          k=0                        k=0
	//
	//          1+u                 2
	//   1/2 ln --- ~ u * [ p(u) + u * q(u) ]
	//          1-u
	//
*/
	return OddPoly(x1, ln_arr, L2_ARR_SIZE);
}

/* required sqrt(2)/2 < x < sqrt(2) */
LOCALFUNC float128 poly_l2(float128 x)
{
	/* using float128 for approximation */
	float128 x_p1 = float128_add(x, float128_one);
	float128 x_m1 = float128_sub(x, float128_one);
	x = float128_div(x_m1, x_p1);
	x = poly_ln(x);
	x = float128_mul(x, float128_ln2inv2);
	return x;
}

LOCALFUNC float128 poly_l2p1(float128 x)
{
	/* using float128 for approximation */
	float128 x_p2 = float128_add(x, float128_two);
	x = float128_div(x, x_p2);
	x = poly_ln(x);
	x = float128_mul(x, float128_ln2inv2);
	return x;
}

// =================================================
// FYL2X                   Compute y * log (x)
//                                        2
// =================================================

//
// Uses the following identities:
//
// 1. ----------------------------------------------------------
//              ln(x)
//   log (x) = -------,  ln (x*y) = ln(x) + ln(y)
//      2       ln(2)
//
// 2. ----------------------------------------------------------
//                1+u             x-1
//   ln (x) = ln -----, when u = -----
//                1-u             x+1
//
// 3. ----------------------------------------------------------
//                        3     5     7           2n+1
//       1+u             u     u     u           u
//   ln ----- = 2 [ u + --- + --- + --- + ... + ------ + ... ]
//       1-u             3     5     7           2n+1
//

LOCALFUNC floatx80 fyl2x(floatx80 a, floatx80 b)
{
	int aSign;
	int bSign;
	Bit64u aSig;
	Bit32s aExp;
	Bit64u bSig;
	Bit32s bExp;
	int zSign;
	int ExpDiff;
	Bit64u zSig0, zSig1;
	float128 x;

	// handle unsupported extended double-precision floating encodings
	if (0 /* floatx80_is_unsupported(a) */) {
invalid:
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}

	aSig = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	aSign = extractFloatx80Sign(a);
	bSig = extractFloatx80Frac(b);
	bExp = extractFloatx80Exp(b);
	bSign = extractFloatx80Sign(b);

	zSign = bSign ^ 1;

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig<<1)
			 || ((bExp == 0x7FFF) && (Bit64u) (bSig<<1)))
		{
			return propagateFloatx80NaN(a, b);
		}
		if (aSign) goto invalid;
		else {
			if (bExp == 0) {
				if (bSig == 0) goto invalid;
				float_raise(float_flag_denormal);
			}
			return packFloatx80(bSign, 0x7FFF, LIT64(0x8000000000000000));
		}
	}
	if (bExp == 0x7FFF)
	{
		if ((Bit64u) (bSig<<1)) return propagateFloatx80NaN(a, b);
		if (aSign && (Bit64u)(aExp | aSig)) goto invalid;
		if (aSig && (aExp == 0))
			float_raise(float_flag_denormal);
		if (aExp < 0x3FFF) {
			return packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
		}
		if (aExp == 0x3FFF && ((Bit64u) (aSig<<1) == 0)) goto invalid;
		return packFloatx80(bSign, 0x7FFF, LIT64(0x8000000000000000));
	}
	if (aExp == 0) {
		if (aSig == 0) {
			if ((bExp | bSig) == 0) goto invalid;
			float_raise(float_flag_divbyzero);
			return packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
		}
		if (aSign) goto invalid;
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
	}
	if (aSign) goto invalid;
	if (bExp == 0) {
		if (bSig == 0) {
			if (aExp < 0x3FFF) return packFloatx80(zSign, 0, 0);
			return packFloatx80(bSign, 0, 0);
		}
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
	}
	if (aExp == 0x3FFF && ((Bit64u) (aSig<<1) == 0))
		return packFloatx80(bSign, 0, 0);

	float_raise(float_flag_inexact);

	ExpDiff = aExp - 0x3FFF;
	aExp = 0;
	if (aSig >= SQRT2_HALF_SIG) {
		ExpDiff++;
		aExp--;
	}

	/* ******************************** */
	/* using float128 for approximation */
	/* ******************************** */

	shift128Right(aSig<<1, 0, 16, &zSig0, &zSig1);
	x = packFloat128(0, aExp+0x3FFF, zSig0, zSig1);
	x = poly_l2(x);
	x = float128_add(x, floatx80_to_float128(int32_to_floatx80(ExpDiff)));
	return floatx80_mul128(b, x);
}

// =================================================
// FYL2XP1                 Compute y * log (x + 1)
//                                        2
// =================================================

//
// Uses the following identities:
//
// 1. ----------------------------------------------------------
//              ln(x)
//   log (x) = -------
//      2       ln(2)
//
// 2. ----------------------------------------------------------
//                  1+u              x
//   ln (x+1) = ln -----, when u = -----
//                  1-u             x+2
//
// 3. ----------------------------------------------------------
//                        3     5     7           2n+1
//       1+u             u     u     u           u
//   ln ----- = 2 [ u + --- + --- + --- + ... + ------ + ... ]
//       1-u             3     5     7           2n+1
//

LOCALFUNC floatx80 fyl2xp1(floatx80 a, floatx80 b)
{
	Bit32s aExp, bExp;
	Bit64u aSig, bSig, zSig0, zSig1, zSig2;
	int aSign, bSign;
	int zSign;
	float128 x;

	// handle unsupported extended double-precision floating encodings
	if (0 /* floatx80_is_unsupported(a) */) {
invalid:
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}

	aSig = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	aSign = extractFloatx80Sign(a);
	bSig = extractFloatx80Frac(b);
	bExp = extractFloatx80Exp(b);
	bSign = extractFloatx80Sign(b);
	zSign = aSign ^ bSign;

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig<<1)
			 || ((bExp == 0x7FFF) && (Bit64u) (bSig<<1)))
		{
			return propagateFloatx80NaN(a, b);
		}
		if (aSign) goto invalid;
		else {
			if (bExp == 0) {
				if (bSig == 0) goto invalid;
				float_raise(float_flag_denormal);
			}
			return packFloatx80(bSign, 0x7FFF, LIT64(0x8000000000000000));
		}
	}
	if (bExp == 0x7FFF)
	{
		if ((Bit64u) (bSig<<1))
			return propagateFloatx80NaN(a, b);

		if (aExp == 0) {
			if (aSig == 0) goto invalid;
			float_raise(float_flag_denormal);
		}

		return packFloatx80(zSign, 0x7FFF, LIT64(0x8000000000000000));
	}
	if (aExp == 0) {
		if (aSig == 0) {
			if (bSig && (bExp == 0)) float_raise(float_flag_denormal);
			return packFloatx80(zSign, 0, 0);
		}
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
	}
	if (bExp == 0) {
		if (bSig == 0) return packFloatx80(zSign, 0, 0);
		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
	}

	float_raise(float_flag_inexact);

	if (aSign && aExp >= 0x3FFF)
		return a;

	if (aExp >= 0x3FFC) // big argument
	{
		return fyl2x(floatx80_add(a, floatx80_one), b);
	}

	// handle tiny argument
	if (aExp < EXP_BIAS-70)
	{
		// first order approximation, return (a*b)/ln(2)
		Bit32s zExp = aExp + FLOAT_LN2INV_EXP - 0x3FFE;

	mul128By64To192(FLOAT_LN2INV_HI, FLOAT_LN2INV_LO, aSig, &zSig0, &zSig1, &zSig2);
		if (0 < (Bit64s) zSig0) {
			shortShift128Left(zSig0, zSig1, 1, &zSig0, &zSig1);
			--zExp;
		}

		zExp = zExp + bExp - 0x3FFE;
	mul128By64To192(zSig0, zSig1, bSig, &zSig0, &zSig1, &zSig2);
		if (0 < (Bit64s) zSig0) {
			shortShift128Left(zSig0, zSig1, 1, &zSig0, &zSig1);
			--zExp;
		}

		return
			roundAndPackFloatx80(80, aSign ^ bSign, zExp, zSig0, zSig1);
	}

	/* ******************************** */
	/* using float128 for approximation */
	/* ******************************** */

	shift128Right(aSig<<1, 0, 16, &zSig0, &zSig1);
	x = packFloat128(aSign, aExp, zSig0, zSig1);
	x = poly_l2p1(x);
	return floatx80_mul128(b, x);
}

/* ----- end from original file "fyl2x.cc" ----- */

/* ----- from original file "f2xm1.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

static const floatx80 floatx80_negone  = packFloatx80m(1, 0x3fff, LIT64(0x8000000000000000));
static const floatx80 floatx80_neghalf = packFloatx80m(1, 0x3ffe, LIT64(0x8000000000000000));
static const float128 float128_ln2     =
	packFloat2x128m(LIT64(0x3ffe62e42fefa39e), LIT64(0xf35793c7673007e6));

#define LN2_SIG        LIT64(0xb17217f7d1cf79ac)

#define EXP_ARR_SIZE 15

static float128 exp_arr[EXP_ARR_SIZE] =
{
	PACK_FLOAT_128(0x3fff000000000000, 0x0000000000000000), /*  1 */
	PACK_FLOAT_128(0x3ffe000000000000, 0x0000000000000000), /*  2 */
	PACK_FLOAT_128(0x3ffc555555555555, 0x5555555555555555), /*  3 */
	PACK_FLOAT_128(0x3ffa555555555555, 0x5555555555555555), /*  4 */
	PACK_FLOAT_128(0x3ff8111111111111, 0x1111111111111111), /*  5 */
	PACK_FLOAT_128(0x3ff56c16c16c16c1, 0x6c16c16c16c16c17), /*  6 */
	PACK_FLOAT_128(0x3ff2a01a01a01a01, 0xa01a01a01a01a01a), /*  7 */
	PACK_FLOAT_128(0x3fefa01a01a01a01, 0xa01a01a01a01a01a), /*  8 */
	PACK_FLOAT_128(0x3fec71de3a556c73, 0x38faac1c88e50017), /*  9 */
	PACK_FLOAT_128(0x3fe927e4fb7789f5, 0xc72ef016d3ea6679), /* 10 */
	PACK_FLOAT_128(0x3fe5ae64567f544e, 0x38fe747e4b837dc7), /* 11 */
	PACK_FLOAT_128(0x3fe21eed8eff8d89, 0x7b544da987acfe85), /* 12 */
	PACK_FLOAT_128(0x3fde6124613a86d0, 0x97ca38331d23af68), /* 13 */
	PACK_FLOAT_128(0x3fda93974a8c07c9, 0xd20badf145dfa3e5), /* 14 */
	PACK_FLOAT_128(0x3fd6ae7f3e733b81, 0xf11d8656b0ee8cb0)  /* 15 */
};

/* required -1 < x < 1 */
LOCALFUNC float128 poly_exp(float128 x)
{
/*
	//               2     3     4     5     6     7     8     9
	//  x           x     x     x     x     x     x     x     x
	// e - 1 ~ x + --- + --- + --- + --- + --- + --- + --- + --- + ...
	//              2!    3!    4!    5!    6!    7!    8!    9!
	//
	//                     2     3     4     5     6     7     8
	//              x     x     x     x     x     x     x     x
	//   = x [ 1 + --- + --- + --- + --- + --- + --- + --- + --- + ... ]
	//              2!    3!    4!    5!    6!    7!    8!    9!
	//
	//           8                          8
	//          --       2k                --        2k+1
	//   p(x) = >  C  * x           q(x) = >  C   * x
	//          --  2k                     --  2k+1
	//          k=0                        k=0
	//
	//    x
	//   e  - 1 ~ x * [ p(x) + x * q(x) ]
	//
*/
	float128 t = EvalPoly(x, exp_arr, EXP_ARR_SIZE);
	return float128_mul(t, x);
}

// =================================================
//                                  x
// FX2P1                   Compute 2  - 1
// =================================================

//
// Uses the following identities:
//
// 1. ----------------------------------------------------------
//      x    x*ln(2)
//     2  = e
//
// 2. ----------------------------------------------------------
//                      2     3     4     5           n
//      x        x     x     x     x     x           x
//     e  = 1 + --- + --- + --- + --- + --- + ... + --- + ...
//               1!    2!    3!    4!    5!          n!
//

LOCALFUNC floatx80 f2xm1(floatx80 a)
{
	Bit64u zSig0, zSig1;
	float128 x;

#if 0
	// handle unsupported extended double-precision floating encodings
	if (floatx80_is_unsupported(a))
	{
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}
#endif

	Bit64u aSig = extractFloatx80Frac(a);
	Bit32s aExp = extractFloatx80Exp(a);
	int aSign = extractFloatx80Sign(a);

	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig<<1))
			return propagateOneFloatx80NaN(&a);

		return (aSign) ? floatx80_negone : a;
	}

	if (aExp == 0) {
		if (aSig == 0) return a;
		float_raise(float_flag_denormal | float_flag_inexact);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);

	tiny_argument:
		mul64To128(aSig, LN2_SIG, &zSig0, &zSig1);
		if (0 < (Bit64s) zSig0) {
			shortShift128Left(zSig0, zSig1, 1, &zSig0, &zSig1);
			--aExp;
		}
		return
			roundAndPackFloatx80(80, aSign, aExp, zSig0, zSig1);
	}

	float_raise(float_flag_inexact);

	if (aExp < 0x3FFF)
	{
		if (aExp < EXP_BIAS-68)
			goto tiny_argument;

		/* ******************************** */
		/* using float128 for approximation */
		/* ******************************** */

		x = floatx80_to_float128(a);
		x = float128_mul(x, float128_ln2);
		x = poly_exp(x);
		return float128_to_floatx80(x);
	}
	else
	{
		if ((a.high == 0xBFFF) && (! (aSig<<1)))
		   return floatx80_neghalf;

		return a;
	}
}

/* ----- end from original file "f2xm1.cc" ----- */

/* ----- from original file "fsincos.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

#define USE_estimateDiv128To64

#if 0
static const floatx80 floatx80_one = packFloatx80m(0, 0x3fff, LIT64(0x8000000000000000));
#endif

/* reduce trigonometric function argument using 128-bit precision
   M_PI approximation */
LOCALFUNC Bit64u argument_reduction_kernel(Bit64u aSig0, int Exp, Bit64u *zSig0, Bit64u *zSig1)
{
	Bit64u term0, term1, term2;
	Bit64u aSig1 = 0;
	Bit64u q;

	shortShift128Left(aSig1, aSig0, Exp, &aSig1, &aSig0);
	q = estimateDiv128To64(aSig1, aSig0, FLOAT_PI_HI);
	mul128By64To192(FLOAT_PI_HI, FLOAT_PI_LO, q, &term0, &term1, &term2);
	sub128(aSig1, aSig0, term0, term1, zSig1, zSig0);
	while ((Bit64s)(*zSig1) < 0) {
		--q;
		add192(*zSig1, *zSig0, term2, 0, FLOAT_PI_HI, FLOAT_PI_LO, zSig1, zSig0, &term2);
	}
	*zSig1 = term2;
	return q;
}

LOCALFUNC int reduce_trig_arg(int expDiff, int *zSign, Bit64u *aSig0, Bit64u *aSig1)
{
	Bit64u term0, term1, q = 0;

	if (expDiff < 0) {
		shift128Right(*aSig0, 0, 1, aSig0, aSig1);
		expDiff = 0;
	}
	if (expDiff > 0) {
		q = argument_reduction_kernel(*aSig0, expDiff, aSig0, aSig1);
	}
	else {
		if (FLOAT_PI_HI <= *aSig0) {
			*aSig0 -= FLOAT_PI_HI;
			q = 1;
		}
	}

	shift128Right(FLOAT_PI_HI, FLOAT_PI_LO, 1, &term0, &term1);
	if (! lt128(*aSig0, *aSig1, term0, term1))
	{
		int lt = lt128(term0, term1, *aSig0, *aSig1);
		int eq = eq128(*aSig0, *aSig1, term0, term1);

		if ((eq && (q & 1)) || lt) {
			*zSign = !*zSign;
			++q;
		}
		if (lt) sub128(FLOAT_PI_HI, FLOAT_PI_LO, *aSig0, *aSig1, aSig0, aSig1);
	}

	return (int)(q & 3);
}

#define SIN_ARR_SIZE 9
#define COS_ARR_SIZE 9

static float128 sin_arr[SIN_ARR_SIZE] =
{
	PACK_FLOAT_128(0x3fff000000000000, 0x0000000000000000), /*  1 */
	PACK_FLOAT_128(0xbffc555555555555, 0x5555555555555555), /*  3 */
	PACK_FLOAT_128(0x3ff8111111111111, 0x1111111111111111), /*  5 */
	PACK_FLOAT_128(0xbff2a01a01a01a01, 0xa01a01a01a01a01a), /*  7 */
	PACK_FLOAT_128(0x3fec71de3a556c73, 0x38faac1c88e50017), /*  9 */
	PACK_FLOAT_128(0xbfe5ae64567f544e, 0x38fe747e4b837dc7), /* 11 */
	PACK_FLOAT_128(0x3fde6124613a86d0, 0x97ca38331d23af68), /* 13 */
	PACK_FLOAT_128(0xbfd6ae7f3e733b81, 0xf11d8656b0ee8cb0), /* 15 */
	PACK_FLOAT_128(0x3fce952c77030ad4, 0xa6b2605197771b00)  /* 17 */
};

static float128 cos_arr[COS_ARR_SIZE] =
{
	PACK_FLOAT_128(0x3fff000000000000, 0x0000000000000000), /*  0 */
	PACK_FLOAT_128(0xbffe000000000000, 0x0000000000000000), /*  2 */
	PACK_FLOAT_128(0x3ffa555555555555, 0x5555555555555555), /*  4 */
	PACK_FLOAT_128(0xbff56c16c16c16c1, 0x6c16c16c16c16c17), /*  6 */
	PACK_FLOAT_128(0x3fefa01a01a01a01, 0xa01a01a01a01a01a), /*  8 */
	PACK_FLOAT_128(0xbfe927e4fb7789f5, 0xc72ef016d3ea6679), /* 10 */
	PACK_FLOAT_128(0x3fe21eed8eff8d89, 0x7b544da987acfe85), /* 12 */
	PACK_FLOAT_128(0xbfda93974a8c07c9, 0xd20badf145dfa3e5), /* 14 */
	PACK_FLOAT_128(0x3fd2ae7f3e733b81, 0xf11d8656b0ee8cb0)  /* 16 */
};

/* 0 <= x <= pi/4 */
LOCALINLINEFUNC float128 poly_sin(float128 x)
{
	//                 3     5     7     9     11     13     15
	//                x     x     x     x     x      x      x
	// sin (x) ~ x - --- + --- - --- + --- - ---- + ---- - ---- =
	//                3!    5!    7!    9!    11!    13!    15!
	//
	//                 2     4     6     8     10     12     14
	//                x     x     x     x     x      x      x
	//   = x * [ 1 - --- + --- - --- + --- - ---- + ---- - ---- ] =
	//                3!    5!    7!    9!    11!    13!    15!
	//
	//           3                          3
	//          --       4k                --        4k+2
	//   p(x) = >  C  * x   > 0     q(x) = >  C   * x     < 0
	//          --  2k                     --  2k+1
	//          k=0                        k=0
	//
	//                          2
	//   sin(x) ~ x * [ p(x) + x * q(x) ]
	//

	return OddPoly(x, sin_arr, SIN_ARR_SIZE);
}

/* 0 <= x <= pi/4 */
LOCALINLINEFUNC float128 poly_cos(float128 x)
{
	//                 2     4     6     8     10     12     14
	//                x     x     x     x     x      x      x
	// cos (x) ~ 1 - --- + --- - --- + --- - ---- + ---- - ----
	//                2!    4!    6!    8!    10!    12!    14!
	//
	//           3                          3
	//          --       4k                --        4k+2
	//   p(x) = >  C  * x   > 0     q(x) = >  C   * x     < 0
	//          --  2k                     --  2k+1
	//          k=0                        k=0
	//
	//                      2
	//   cos(x) ~ [ p(x) + x * q(x) ]
	//

	return EvenPoly(x, cos_arr, COS_ARR_SIZE);
}

LOCALINLINEPROC sincos_invalid(floatx80 *sin_a, floatx80 *cos_a, floatx80 a)
{
	if (sin_a) *sin_a = a;
	if (cos_a) *cos_a = a;
}

LOCALINLINEPROC sincos_tiny_argument(floatx80 *sin_a, floatx80 *cos_a, floatx80 a)
{
	if (sin_a) *sin_a = a;
	if (cos_a) *cos_a = floatx80_one;
}

LOCALFUNC floatx80 sincos_approximation(int neg, float128 r, Bit64u quotient)
{
	floatx80 result;

	if (quotient & 0x1) {
		r = poly_cos(r);
		neg = 0;
	} else  {
		r = poly_sin(r);
	}

	result = float128_to_floatx80(r);
	if (quotient & 0x2)
		neg = ! neg;

	if (neg)
		floatx80_chs(&result);

	return result;
}

// =================================================
// FSINCOS               Compute sin(x) and cos(x)
// =================================================

//
// Uses the following identities:
// ----------------------------------------------------------
//
//  sin(-x) = -sin(x)
//  cos(-x) =  cos(x)
//
//  sin(x+y) = sin(x)*cos(y)+cos(x)*sin(y)
//  cos(x+y) = sin(x)*sin(y)+cos(x)*cos(y)
//
//  sin(x+ pi/2)  =  cos(x)
//  sin(x+ pi)    = -sin(x)
//  sin(x+3pi/2)  = -cos(x)
//  sin(x+2pi)    =  sin(x)
//

LOCALFUNC int fsincos(floatx80 a, floatx80 *sin_a, floatx80 *cos_a)
{
	float128 r;
	Bit64u aSig0, aSig1 = 0;
	Bit32s aExp, zExp, expDiff;
	int aSign, zSign;
	int q = 0;

#if 0
	// handle unsupported extended double-precision floating encodings
	if (floatx80_is_unsupported(a))
	{
		goto invalid;
	}
#endif

	aSig0 = extractFloatx80Frac(a);
	aExp = extractFloatx80Exp(a);
	aSign = extractFloatx80Sign(a);

	/* invalid argument */
	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig0<<1)) {
			sincos_invalid(sin_a, cos_a, propagateOneFloatx80NaN(&a));
			return 0;
		}

#if 0
	invalid:
#endif
		float_raise(float_flag_invalid);
		sincos_invalid(sin_a, cos_a, floatx80_default_nan);
		return 0;
	}

	if (aExp == 0) {
		if (aSig0 == 0) {
			sincos_tiny_argument(sin_a, cos_a, a);
			return 0;
		}

		float_raise(float_flag_denormal);

		/* handle pseudo denormals */
		if (! (aSig0 & LIT64(0x8000000000000000)))
		{
			float_raise(float_flag_inexact);
			if (sin_a)
				float_raise(float_flag_underflow);
			sincos_tiny_argument(sin_a, cos_a, a);
			return 0;
		}

		normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
	}

	zSign = aSign;
	zExp = EXP_BIAS;
	expDiff = aExp - zExp;

	/* argument is out-of-range */
	if (expDiff >= 63)
		return -1;

	float_raise(float_flag_inexact);

	if (expDiff < -1) {    // doesn't require reduction
		if (expDiff <= -68) {
			a = packFloatx80(aSign, aExp, aSig0);
			sincos_tiny_argument(sin_a, cos_a, a);
			return 0;
		}
		zExp = aExp;
	}
	else {
		q = reduce_trig_arg(expDiff, &zSign, &aSig0, &aSig1);
	}

	/* **************************** */
	/* argument reduction completed */
	/* **************************** */

	/* using float128 for approximation */
	r = normalizeRoundAndPackFloat128(0, zExp-0x10, aSig0, aSig1);

	if (aSign) q = -q;
	if (sin_a) *sin_a = sincos_approximation(zSign, r,   q);
	if (cos_a) *cos_a = sincos_approximation(zSign, r, q+1);

	return 0;
}

// =================================================
// FPTAN                 Compute tan(x)
// =================================================

//
// Uses the following identities:
//
// 1. ----------------------------------------------------------
//
//  sin(-x) = -sin(x)
//  cos(-x) =  cos(x)
//
//  sin(x+y) = sin(x)*cos(y)+cos(x)*sin(y)
//  cos(x+y) = sin(x)*sin(y)+cos(x)*cos(y)
//
//  sin(x+ pi/2)  =  cos(x)
//  sin(x+ pi)    = -sin(x)
//  sin(x+3pi/2)  = -cos(x)
//  sin(x+2pi)    =  sin(x)
//
// 2. ----------------------------------------------------------
//
//           sin(x)
//  tan(x) = ------
//           cos(x)
//

LOCALFUNC int ftan(floatx80 *a)
{
	float128 r;
	float128 sin_r;
	float128 cos_r;
	Bit64u aSig0, aSig1 = 0;
	Bit32s aExp, zExp, expDiff;
	int aSign, zSign;
	int q = 0;

#if 0
	// handle unsupported extended double-precision floating encodings
	if (floatx80_is_unsupported(*a))
	{
		goto invalid;
	}
#endif

	aSig0 = extractFloatx80Frac(*a);
	aExp = extractFloatx80Exp(*a);
	aSign = extractFloatx80Sign(*a);

	/* invalid argument */
	if (aExp == 0x7FFF) {
		if ((Bit64u) (aSig0<<1))
		{
			*a = propagateOneFloatx80NaN(a);
			return 0;
		}

#if 0
	invalid:
#endif
		float_raise(float_flag_invalid);
		*a = floatx80_default_nan;
		return 0;
	}

	if (aExp == 0) {
		if (aSig0 == 0) return 0;
		float_raise(float_flag_denormal);
		/* handle pseudo denormals */
		if (! (aSig0 & LIT64(0x8000000000000000)))
		{
			float_raise(float_flag_inexact | float_flag_underflow);
			return 0;
		}
		normalizeFloatx80Subnormal(aSig0, &aExp, &aSig0);
	}

	zSign = aSign;
	zExp = EXP_BIAS;
	expDiff = aExp - zExp;

	/* argument is out-of-range */
	if (expDiff >= 63)
		return -1;

	float_raise(float_flag_inexact);

	if (expDiff < -1) {    // doesn't require reduction
		if (expDiff <= -68) {
			*a = packFloatx80(aSign, aExp, aSig0);
			return 0;
		}
		zExp = aExp;
	}
	else {
		q = reduce_trig_arg(expDiff, &zSign, &aSig0, &aSig1);
	}

	/* **************************** */
	/* argument reduction completed */
	/* **************************** */

	/* using float128 for approximation */
	r = normalizeRoundAndPackFloat128(0, zExp-0x10, aSig0, aSig1);

	sin_r = poly_sin(r);
	cos_r = poly_cos(r);

	if (q & 0x1) {
		r = float128_div(cos_r, sin_r);
		zSign = ! zSign;
	} else {
		r = float128_div(sin_r, cos_r);
	}

	*a = float128_to_floatx80(r);
	if (zSign)
		floatx80_chs(a);

	return 0;
}

/* ----- end from original file "fsincos.cc" ----- */

/* ----- from original file "fpatan.cc" ----- */

/*
	["original Stanislav Shwartsman Copyright notice" went here, included near top of this file.]
*/

#define FPATAN_ARR_SIZE 11

#if 0
static const float128 float128_one =
		packFloat2x128m(LIT64(0x3fff000000000000), LIT64(0x0000000000000000));
#endif
static const float128 float128_sqrt3 =
		packFloat2x128m(LIT64(0x3fffbb67ae8584ca), LIT64(0xa73b25742d7078b8));
static const floatx80 floatx80_pi  =
		packFloatx80m(0, 0x4000, LIT64(0xc90fdaa22168c235));

static const float128 float128_pi2 =
		packFloat2x128m(LIT64(0x3fff921fb54442d1), LIT64(0x8469898CC5170416));
static const float128 float128_pi4 =
		packFloat2x128m(LIT64(0x3ffe921fb54442d1), LIT64(0x8469898CC5170416));
static const float128 float128_pi6 =
		packFloat2x128m(LIT64(0x3ffe0c152382d736), LIT64(0x58465BB32E0F580F));

static float128 atan_arr[FPATAN_ARR_SIZE] =
{
	PACK_FLOAT_128(0x3fff000000000000, 0x0000000000000000), /*  1 */
	PACK_FLOAT_128(0xbffd555555555555, 0x5555555555555555), /*  3 */
	PACK_FLOAT_128(0x3ffc999999999999, 0x999999999999999a), /*  5 */
	PACK_FLOAT_128(0xbffc249249249249, 0x2492492492492492), /*  7 */
	PACK_FLOAT_128(0x3ffbc71c71c71c71, 0xc71c71c71c71c71c), /*  9 */
	PACK_FLOAT_128(0xbffb745d1745d174, 0x5d1745d1745d1746), /* 11 */
	PACK_FLOAT_128(0x3ffb3b13b13b13b1, 0x3b13b13b13b13b14), /* 13 */
	PACK_FLOAT_128(0xbffb111111111111, 0x1111111111111111), /* 15 */
	PACK_FLOAT_128(0x3ffae1e1e1e1e1e1, 0xe1e1e1e1e1e1e1e2), /* 17 */
	PACK_FLOAT_128(0xbffaaf286bca1af2, 0x86bca1af286bca1b), /* 19 */
	PACK_FLOAT_128(0x3ffa861861861861, 0x8618618618618618)  /* 21 */
};

/* |x| < 1/4 */
LOCALFUNC float128 poly_atan(float128 x1)
{
/*
	//                 3     5     7     9     11     13     15     17
	//                x     x     x     x     x      x      x      x
	// atan(x) ~ x - --- + --- - --- + --- - ---- + ---- - ---- + ----
	//                3     5     7     9     11     13     15     17
	//
	//                 2     4     6     8     10     12     14     16
	//                x     x     x     x     x      x      x      x
	//   = x * [ 1 - --- + --- - --- + --- - ---- + ---- - ---- + ---- ]
	//                3     5     7     9     11     13     15     17
	//
	//           5                          5
	//          --       4k                --        4k+2
	//   p(x) = >  C  * x           q(x) = >  C   * x
	//          --  2k                     --  2k+1
	//          k=0                        k=0
	//
	//                            2
	//    atan(x) ~ x * [ p(x) + x * q(x) ]
	//
*/
	return OddPoly(x1, atan_arr, FPATAN_ARR_SIZE);
}

// =================================================
// FPATAN                  Compute y * log (x)
//                                        2
// =================================================

//
// Uses the following identities:
//
// 1. ----------------------------------------------------------
//
//   atan(-x) = -atan(x)
//
// 2. ----------------------------------------------------------
//
//                             x + y
//   atan(x) + atan(y) = atan -------, xy < 1
//                             1-xy
//
//                             x + y
//   atan(x) + atan(y) = atan ------- + PI, x > 0, xy > 1
//                             1-xy
//
//                             x + y
//   atan(x) + atan(y) = atan ------- - PI, x < 0, xy > 1
//                             1-xy
//
// 3. ----------------------------------------------------------
//
//   atan(x) = atan(INF) + atan(- 1/x)
//
//                           x-1
//   atan(x) = PI/4 + atan( ----- )
//                           x+1
//
//                           x * sqrt(3) - 1
//   atan(x) = PI/6 + atan( ----------------- )
//                             x + sqrt(3)
//
// 4. ----------------------------------------------------------
//                   3     5     7     9                 2n+1
//                  x     x     x     x              n  x
//   atan(x) = x - --- + --- - --- + --- - ... + (-1)  ------ + ...
//                  3     5     7     9                 2n+1
//

LOCALFUNC floatx80 fpatan(floatx80 a, floatx80 b)
{
	float128 a128;
	float128 b128;
	float128 x;
	int swap, add_pi6, add_pi4;
	Bit32s xExp;
	floatx80 result;
	int rSign;

	// handle unsupported extended double-precision floating encodings
#if 0
	if (floatx80_is_unsupported(a)) {
		float_raise(float_flag_invalid);
		return floatx80_default_nan;
	}
#endif

	Bit64u aSig = extractFloatx80Frac(a);
	Bit32s aExp = extractFloatx80Exp(a);
	int aSign = extractFloatx80Sign(a);
	Bit64u bSig = extractFloatx80Frac(b);
	Bit32s bExp = extractFloatx80Exp(b);
	int bSign = extractFloatx80Sign(b);

	int zSign = aSign ^ bSign;

	if (bExp == 0x7FFF)
	{
		if ((Bit64u) (bSig<<1))
			return propagateFloatx80NaN(a, b);

		if (aExp == 0x7FFF) {
			if ((Bit64u) (aSig<<1))
				return propagateFloatx80NaN(a, b);

			if (aSign) {   /* return 3PI/4 */
				return roundAndPackFloatx80(80, bSign,
						FLOATX80_3PI4_EXP, FLOAT_3PI4_HI, FLOAT_3PI4_LO);
			}
			else {         /* return  PI/4 */
				return roundAndPackFloatx80(80, bSign,
						FLOATX80_PI4_EXP, FLOAT_PI_HI, FLOAT_PI_LO);
			}
		}

		if (aSig && (aExp == 0))
			float_raise(float_flag_denormal);

		/* return PI/2 */
		return roundAndPackFloatx80(80, bSign, FLOATX80_PI2_EXP, FLOAT_PI_HI, FLOAT_PI_LO);
	}
	if (aExp == 0x7FFF)
	{
		if ((Bit64u) (aSig<<1))
			return propagateFloatx80NaN(a, b);

		if (bSig && (bExp == 0))
			float_raise(float_flag_denormal);

return_PI_or_ZERO:

		if (aSign) {   /* return PI */
			return roundAndPackFloatx80(80, bSign, FLOATX80_PI_EXP, FLOAT_PI_HI, FLOAT_PI_LO);
		} else {       /* return  0 */
			return packFloatx80(bSign, 0, 0);
		}
	}
	if (bExp == 0)
	{
		if (bSig == 0) {
			 if (aSig && (aExp == 0)) float_raise(float_flag_denormal);
			 goto return_PI_or_ZERO;
		}

		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(bSig, &bExp, &bSig);
	}
	if (aExp == 0)
	{
		if (aSig == 0)   /* return PI/2 */
			return roundAndPackFloatx80(80, bSign, FLOATX80_PI2_EXP, FLOAT_PI_HI, FLOAT_PI_LO);

		float_raise(float_flag_denormal);
		normalizeFloatx80Subnormal(aSig, &aExp, &aSig);
	}

	float_raise(float_flag_inexact);

	/* |a| = |b| ==> return PI/4 */
	if (aSig == bSig && aExp == bExp)
		return roundAndPackFloatx80(80, bSign, FLOATX80_PI4_EXP, FLOAT_PI_HI, FLOAT_PI_LO);

	/* ******************************** */
	/* using float128 for approximation */
	/* ******************************** */

	a128 = normalizeRoundAndPackFloat128(0, aExp-0x10, aSig, 0);
	b128 = normalizeRoundAndPackFloat128(0, bExp-0x10, bSig, 0);
	swap = 0;
	add_pi6 = 0;
	add_pi4 = 0;

	if (aExp > bExp || (aExp == bExp && aSig > bSig))
	{
		x = float128_div(b128, a128);
	}
	else {
		x = float128_div(a128, b128);
		swap = 1;
	}

	xExp = extractFloat128Exp(x);

	if (xExp <= EXP_BIAS-40)
		goto approximation_completed;

	if (x.high >= LIT64(0x3ffe800000000000))        // 3/4 < x < 1
	{
		/*
		arctan(x) = arctan((x-1)/(x+1)) + pi/4
		*/
		float128 t1 = float128_sub(x, float128_one);
		float128 t2 = float128_add(x, float128_one);
		x = float128_div(t1, t2);
		add_pi4 = 1;
	}
	else
	{
		/* argument correction */
		if (xExp >= 0x3FFD)                     // 1/4 < x < 3/4
		{
			/*
			arctan(x) = arctan((x*sqrt(3)-1)/(x+sqrt(3))) + pi/6
			*/
			float128 t1 = float128_mul(x, float128_sqrt3);
			float128 t2 = float128_add(x, float128_sqrt3);
			x = float128_sub(t1, float128_one);
			x = float128_div(x, t2);
			add_pi6 = 1;
		}
	}

	x = poly_atan(x);
	if (add_pi6) x = float128_add(x, float128_pi6);
	if (add_pi4) x = float128_add(x, float128_pi4);

approximation_completed:
	if (swap) x = float128_sub(float128_pi2, x);
	result = float128_to_floatx80(x);
	if (zSign) floatx80_chs(&result);
	rSign = extractFloatx80Sign(result);
	if (!bSign && rSign)
		return floatx80_add(result, floatx80_pi);
	if (bSign && !rSign)
		return floatx80_sub(result, floatx80_pi);
	return result;
}

/* ----- end from original file "fpatan.cc" ----- */

/* end soft float stuff */

typedef floatx80 myfpr;

LOCALPROC myfp_FromExtendedFormat(myfpr *r, ui4r v2, ui5r v1, ui5r v0)
{
	r->high = v2;
	r->low = (((ui6b)v1) << 32) | (v0 & 0xFFFFFFFF);
}

LOCALPROC myfp_ToExtendedFormat(myfpr *dd, ui4r *v2, ui5r *v1, ui5r *v0)
{
	*v0 = ((ui6b) dd->low) & 0xFFFFFFFF;
	*v1 = (((ui6b) dd->low) >> 32) & 0xFFFFFFFF;
	*v2 = dd->high;
}

LOCALPROC myfp_FromDoubleFormat(myfpr *r, ui5r v1, ui5r v0)
{
	float64 t = (float64)((((ui6b)v1) << 32) | (v0 & 0xFFFFFFFF));

	*r = float64_to_floatx80(t);
}

LOCALPROC myfp_ToDoubleFormat(myfpr *dd, ui5r *v1, ui5r *v0)
{
	float64 t = floatx80_to_float64(*dd);

	*v0 = ((ui6b) t) & 0xFFFFFFFF;
	*v1 = (((ui6b) t) >> 32) & 0xFFFFFFFF;
}

LOCALPROC myfp_FromSingleFormat(myfpr *r, ui5r x)
{
	*r = float32_to_floatx80(x);
}

LOCALFUNC ui5r myfp_ToSingleFormat(myfpr *ff)
{
	return floatx80_to_float32(*ff);
}

LOCALPROC myfp_FromLong(myfpr *r, ui5r x)
{
	*r = int32_to_floatx80( x );
}

LOCALFUNC ui5r myfp_ToLong(myfpr *x)
{
	return floatx80_to_int32( *x );
}

LOCALFUNC blnr myfp_IsNan(myfpr *x)
{
	return floatx80_is_nan(*x);
}

LOCALFUNC blnr myfp_IsInf(myfpr *x)
{
	return ( ( x->high & 0x7FFF ) == 0x7FFF ) && (0 == ((ui6b) ( x->low<<1 )));
}

LOCALFUNC blnr myfp_IsZero(myfpr *x)
{
	return ( ( x->high & 0x7FFF ) == 0x0000 ) && (0 == ((ui6b) ( x->low<<1 )));
}

LOCALFUNC blnr myfp_IsNeg(myfpr *x)
{
	return ( ( x->high & 0x8000 ) != 0x0000 );
}

LOCALPROC myfp_Add(myfpr *r, const myfpr *a, const myfpr *b)
{
	*r = floatx80_add(*a, *b);
}

LOCALPROC myfp_Sub(myfpr *r, const myfpr *a, const myfpr *b)
{
	*r = floatx80_sub(*a, *b);
}

LOCALPROC myfp_Mul(myfpr *r, const myfpr *a, const myfpr *b)
{
	*r = floatx80_mul(*a, *b);
}

LOCALPROC myfp_Div(myfpr *r, const myfpr *a, const myfpr *b)
{
	*r = floatx80_div(*a, *b);
}

LOCALPROC myfp_Rem(myfpr *r, const myfpr *a, const myfpr *b)
{
	*r = floatx80_rem(*a, *b);
}

LOCALPROC myfp_Sqrt(myfpr *r, myfpr *x)
{
	*r = floatx80_sqrt(*x);
}

LOCALPROC myfp_Mod(myfpr *r, myfpr *a, myfpr *b)
{
	Bit64u q;
	*r = floatx80_remainder(*a, *b, &q);
		/* should save low byte of q */
}

LOCALPROC myfp_Scale(myfpr *r, myfpr *a, myfpr *b)
{
	*r = floatx80_scale(*a, *b);
}

LOCALPROC myfp_GetMan(myfpr *r, myfpr *x)
{
	*r = *x;
	(void) floatx80_extract(r);
}

LOCALPROC myfp_GetExp(myfpr *r, myfpr *x)
{
	floatx80 t0 = *x;
	*r = floatx80_extract(&t0);
}

LOCALPROC myfp_floor(myfpr *r, myfpr *x)
{
	si3r SaveRoundingMode = float_rounding_mode;

	float_rounding_mode = float_round_down;
	*r = floatx80_round_to_int(*x);
	float_rounding_mode = SaveRoundingMode;
}

LOCALPROC myfp_IntRZ(myfpr *r, myfpr *x)
{
	si3r SaveRoundingMode = float_rounding_mode;

	float_rounding_mode = float_round_to_zero;
	*r = floatx80_round_to_int(*x);
	float_rounding_mode = SaveRoundingMode;
}

LOCALPROC myfp_Int(myfpr *r, myfpr *x)
{
	*r = floatx80_round_to_int(*x);
}

LOCALPROC myfp_RoundToSingle(myfpr *r, myfpr *x)
{
	float32 t0 = floatx80_to_float32(*x);

	*r = float32_to_floatx80(t0);
}

LOCALPROC myfp_RoundToDouble(myfpr *r, myfpr *x)
{
	float64 t0 = floatx80_to_float64(*x);

	*r = float64_to_floatx80(t0);
}

LOCALPROC myfp_Abs(myfpr *r, myfpr *x)
{
	*r = *x;
	r->high &= 0x7FFF;
}

LOCALPROC myfp_Neg(myfpr *r, myfpr *x)
{
	*r = *x;
	r->high ^= 0x8000;
}

LOCALPROC myfp_TwoToX(myfpr *r, myfpr *x)
{
	floatx80 t2;
	floatx80 t3;
	floatx80 t4;
	floatx80 t5;
	myfp_floor(&t2, x);
	t3 = floatx80_sub(*x, t2);
	t4 = f2xm1(t3);
	t5 = floatx80_add(t4, floatx80_one);
	*r = floatx80_scale(t5, t2);
}

LOCALPROC myfp_TenToX(myfpr *r, myfpr *x)
{
	floatx80 t1;
	const floatx80 t = /* 1.0 / log(2.0) */
		packFloatx80m(0, 0x3fff, LIT64(0xb8aa3b295c17f0bc));
	const floatx80 t2 = /* log(10.0) */
		packFloatx80m(0, 0x4000, LIT64(0x935d8dddaaa8ac17));
	t1 = floatx80_mul(floatx80_mul(*x, t), t2);
	myfp_TwoToX(r, &t1);
}

LOCALPROC myfp_EToX(myfpr *r, myfpr *x)
{
	floatx80 t1;
	const floatx80 t = /* 1.0 / log(2.0) */
		packFloatx80m(0, 0x3fff, LIT64(0xb8aa3b295c17f0bc));
	t1 = floatx80_mul(*x, t);
	myfp_TwoToX(r, &t1);
}

LOCALPROC myfp_EToXM1(myfpr *r, myfpr *x)
{
	floatx80 t1;
	floatx80 t2;
	floatx80 t3;
	floatx80 t4;
	floatx80 t5;
	floatx80 t6;
	const floatx80 t = /* 1.0 / log(2.0) */
		packFloatx80m(0, 0x3fff, LIT64(0xb8aa3b295c17f0bc));
	t1 = floatx80_mul(*x, t);
	myfp_floor(&t2, &t1);
	t3 = floatx80_sub(t1, t2);
	t4 = f2xm1(t3);
	if (myfp_IsZero(&t2)) {
		*r = t4;
	} else {
		t5 = floatx80_add(t4, floatx80_one);
		t6 = floatx80_scale(t5, t2);
		*r = floatx80_sub(t6, floatx80_one);
	}
}

LOCALPROC myfp_Log2(myfpr *r, myfpr *x)
{
	*r = fyl2x(*x, floatx80_one);
}

LOCALPROC myfp_LogN(myfpr *r, myfpr *x)
{
	const floatx80 t = /* log(2.0) */
		packFloatx80m(0, 0x3ffe, LIT64(0xb17217f7d1cf79ac));
	*r = fyl2x(*x, t);
}

LOCALPROC myfp_Log10(myfpr *r, myfpr *x)
{
	const floatx80 t = /* log10(2.0) = ln(2) / ln(10), unknown accuracy */
		packFloatx80m(0, 0x3ffd, LIT64(0x9a209a84fbcff798));
	*r = fyl2x(*x, t);
}

LOCALPROC myfp_LogNP1(myfpr *r, myfpr *x)
{
	const floatx80 t = /* log(2.0) */
		packFloatx80m(0, 0x3ffe, LIT64(0xb17217f7d1cf79ac));
	*r = fyl2xp1(*x, t);
}

LOCALPROC myfp_Sin(myfpr *r, myfpr *x)
{
	(void) fsincos(*x, r, 0);
}

LOCALPROC myfp_Cos(myfpr *r, myfpr *x)
{
	(void) fsincos(*x, 0, r);
}

LOCALPROC myfp_Tan(myfpr *r, myfpr *x)
{
	*r = *x;
	(void) ftan(r);
}

LOCALPROC myfp_ATan(myfpr *r, myfpr *x)
{
	*r = fpatan(floatx80_one, *x);
}

LOCALPROC myfp_ASin(myfpr *r, myfpr *x)
{
	floatx80 x2 = floatx80_mul(*x, *x);
	floatx80 mx2 = floatx80_sub(floatx80_one, x2);
	floatx80 cx = floatx80_sqrt(mx2);

	*r = fpatan(cx, *x);
}

LOCALPROC myfp_ACos(myfpr *r, myfpr *x)
{
	floatx80 x2 = floatx80_mul(*x, *x);
	floatx80 mx2 = floatx80_sub(floatx80_one, x2);
	floatx80 cx = floatx80_sqrt(mx2);

	*r = fpatan(*x, cx);
}

static const floatx80 floatx80_zero =
	packFloatx80m(0, 0x0000, LIT64(0x0000000000000000));

static const floatx80 floatx80_Two =
	packFloatx80m(0, 0x4000, LIT64(0x8000000000000000));

static const floatx80 floatx80_Ten =
	packFloatx80m(0, 0x4002, LIT64(0xa000000000000000));

LOCALPROC myfp_Sinh(myfpr *r, myfpr *x)
{
	myfpr ex;
	myfpr nx;
	myfpr enx;
	myfpr t1;

	myfp_EToX(&ex, x);
	myfp_Neg(&nx, x);
	myfp_EToX(&enx, &nx);
	myfp_Sub(&t1, &ex, &enx);
	myfp_Div(r, &t1, &floatx80_Two);
}

LOCALPROC myfp_Cosh(myfpr *r, myfpr *x)
{
	myfpr ex;
	myfpr nx;
	myfpr enx;
	myfpr t1;

	myfp_EToX(&ex, x);
	myfp_Neg(&nx, x);
	myfp_EToX(&enx, &nx);
	myfp_Add(&t1, &ex, &enx);
	myfp_Div(r, &t1, &floatx80_Two);
}

LOCALPROC myfp_Tanh(myfpr *r, myfpr *x)
{
	myfpr x2;
	myfpr ex2;
	myfpr ex2m1;
	myfpr ex2p1;

	myfp_Mul(&x2, x, &floatx80_Two);
	myfp_EToX(&ex2, &x2);
	myfp_Sub(&ex2m1, &ex2, &floatx80_one);
	myfp_Add(&ex2p1, &ex2, &floatx80_one);
	myfp_Div(r, &ex2m1, &ex2p1);
}

LOCALPROC myfp_ATanh(myfpr *r, myfpr *x)
{
	myfpr onepx;
	myfpr onemx;
	myfpr dv;
	myfpr ldv;

	myfp_Add(&onepx, x, &floatx80_one);
	myfp_Sub(&onemx, x, &floatx80_one);
	myfp_Div(&dv, &onepx, &onemx);
	myfp_LogN(&ldv, &dv);
	myfp_Div(r, &ldv, &floatx80_Two);
}

LOCALPROC myfp_SinCos(myfpr *r_sin, myfpr *r_cos, myfpr *source)
{
	(void) fsincos(*source, r_sin, r_cos);
}

LOCALFUNC blnr myfp_getCR(myfpr *r, ui4b opmode)
{
	switch (opmode) {
		case 0x00:
			*r = floatx80_pi; /* M_PI */
			break;
		case 0x0B:
			{
				const floatx80 t =
					packFloatx80m(0, 0x3ffd, LIT64(0x9a209a84fbcff798));
				*r = t; /* log10(2.0) = ln(2) / ln(10), unknown accuracy */
			}
			break;
		case 0x0C:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4000, LIT64(0xadf85458a2bb4a9b));
				*r = t; /* exp(1.0), unknown accuracy */
			}
			break;
		case 0x0D:
			{
				const floatx80 t =
					packFloatx80m(0, 0x3fff, LIT64(0xb8aa3b295c17f0bc));
				*r = t; /* 1.0 / log(2.0) */
			}
			break;
		case 0x0E:
			{
				const floatx80 t =
					packFloatx80m(0, 0x3ffd, LIT64(0xde5bd8a937287195));
				*r = t; /* 1.0 / log(10.0), unknown accuracy */
			}
			break;
		case 0x0F:
			*r = floatx80_zero; /* 0.0 */
			break;
		case 0x30:
			{
				const floatx80 t =
					packFloatx80m(0, 0x3ffe, LIT64(0xb17217f7d1cf79ac));
				*r = t; /* log(2.0) */
			}
			break;
		case 0x31:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4000, LIT64(0x935d8dddaaa8ac17));
				*r = t; /* log(10.0) */
			}
			break;
		case 0x32:
			*r = floatx80_one; /* 1.0 */
			break;
		case 0x33:
			*r = floatx80_Ten; /* 10.0 */
			break;
		case 0x34:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4005, LIT64(0xc800000000000000));
				*r = t; /* 100.0 */
			}
			break;
		case 0x35:
			{
				const floatx80 t =
					packFloatx80m(0, 0x400c, LIT64(0x9c40000000000000));
				*r = t; /* 10000.0 */
			}
			break;
		case 0x36:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4019, LIT64(0xbebc200000000000));
				*r = t; /* 1.0e8 */
			}
			break;
		case 0x37:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4034, LIT64(0x8e1bc9bf04000000));
				*r = t; /* 1.0e16 */
			}
			break;
		case 0x38:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4069, LIT64(0x9dc5ada82b70b59e));
				*r = t; /* 1.0e32 */
			}
			break;
		case 0x39:
			{
				const floatx80 t =
					packFloatx80m(0, 0x40d3, LIT64(0xc2781f49ffcfa6d5));
				*r = t; /* 1.0e64 */
			}
			break;
		case 0x3A:
			{
				const floatx80 t =
					packFloatx80m(0, 0x41a8, LIT64(0x93ba47c980e98ce0));
				*r = t; /* 1.0e128 */
			}
			break;
		case 0x3B:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4351, LIT64(0xaa7eebfb9df9de8e));
				*r = t; /* 1.0e256 */
			}
			break;
		case 0x3C:
			{
				const floatx80 t =
					packFloatx80m(0, 0x46a3, LIT64(0xe319a0aea60e91c7));
				*r = t; /* 1.0e512 */
			}
			break;
		case 0x3D:
			{
				const floatx80 t =
					packFloatx80m(0, 0x4d48, LIT64(0xc976758681750c17));
				*r = t; /* 1.0e1024 */
			}
			break;
		case 0x3E:
			{
				const floatx80 t =
					packFloatx80m(0, 0x5a92, LIT64(0x9e8b3b5dc53d5de5));
				*r = t; /* 1.0e2048 */
			}
			break;
		case 0x3F:
			{
				const floatx80 t =
					packFloatx80m(0, 0x7525, LIT64(0xc46052028a20979b));
				*r = t; /* 1.0e4096 */
			}
			break;
		default:
			return falseblnr;
	}
	return trueblnr;
}

/* Floating point control register */

LOCALPROC myfp_SetFPCR(ui5r v)
{
	switch ((v >> 4) & 0x03) {
		case 0:
			float_rounding_mode = float_round_nearest_even;
			break;
		case 1:
			float_rounding_mode = float_round_to_zero;
			break;
		case 2:
			float_rounding_mode = float_round_down;
			break;
		case 3:
			float_rounding_mode = float_round_up;
			break;
	}
	switch ((v >> 6) & 0x03) {
		case 0:
			floatx80_rounding_precision = 80;
			break;
		case 1:
			floatx80_rounding_precision = 32;
			break;
		case 2:
			floatx80_rounding_precision = 64;
			break;
		case 3:
			ReportAbnormalID(0x0201,
				"Bad rounding precision in myfp_SetFPCR");
			floatx80_rounding_precision = 80;
			break;
	}
	if (0 != (v & 0xF)) {
		ReportAbnormalID(0x0202,
			"Reserved bits not zero in myfp_SetFPCR");
	}
}

LOCALFUNC ui5r myfp_GetFPCR(void)
{
	ui5r v = 0;

	switch (float_rounding_mode) {
		case float_round_nearest_even:
			/* v |= (0 << 4); */
			break;
		case float_round_to_zero:
			v |= (1 << 4);
			break;
		case float_round_down:
			v |= (2 << 4);
			break;
		case float_round_up:
			v |= (3 << 4);
			break;
	}

	if (80 == floatx80_rounding_precision) {
		/* v |= (0 << 6); */
	} else if (32 == floatx80_rounding_precision) {
		v |= (1 << 6);
	} else if (64 == floatx80_rounding_precision) {
		v |= (2 << 6);
	} else {
		ReportAbnormalID(0x0203,
			"Bad rounding precision in myfp_GetFPCR");
	}

	return v;
}

LOCALVAR struct myfp_envStruct
{
	ui5r FPSR;  /* Floating point status register */
} myfp_env;

LOCALPROC myfp_SetFPSR(ui5r v)
{
	myfp_env.FPSR = v;
}

LOCALFUNC ui5r myfp_GetFPSR(void)
{
	return myfp_env.FPSR;
}

LOCALFUNC ui3r myfp_GetConditionCodeByte(void)
{
	return (myfp_env.FPSR >> 24) & 0x0F;
}

LOCALPROC myfp_SetConditionCodeByte(ui3r v)
{
	myfp_env.FPSR = ((myfp_env.FPSR & 0x00FFFFFF)
		| (v << 24));
}

LOCALPROC myfp_SetConditionCodeByteFromResult(myfpr *result)
{
	/* Set condition codes here based on result */

	int c_nan  = myfp_IsNan(result) ? 1 : 0;
	int c_inf  = myfp_IsInf(result) ? 1 : 0;
	int c_zero = myfp_IsZero(result) ? 1 : 0;
	int c_neg  = myfp_IsNeg(result) ? 1 : 0;

	myfp_SetConditionCodeByte(c_nan
		| (c_inf  << 1)
		| (c_zero << 2)
		| (c_neg  << 3));
}
