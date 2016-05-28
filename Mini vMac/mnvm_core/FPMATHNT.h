/*
	FPMATHNT.h

	Copyright (C) 2009 Ross Martin, Paul C. Pratt

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
	Floating Point MATH implemented with NaTive operations
*/

#include <stdio.h>
#include <math.h>

#define FPDEBUG if (0)
/* #define FPDEBUG */

typedef long double myfpr;

/*
	Only valid for a host that's an X386 processor

	68881 extended is 96 bits.  This is 1 bit sign, 15 bits exponent
	16 bits unused, 1 bit explicit integer bit of mantissa, and
	63 bits of normal mantissa

	Intel extended is packed into 128 bits.  It is 80 bits, with
	1 bit sign, 15 bits exponent, 1 bit explicit integer bit of
	mantissa, and 63 bits of mantissa.

	So for the most part these match.  Just have to match the correct
	bytes.
*/

#if BigEndianUnaligned
#define fldbitsi(i) (9 - (i))
#else
#define fldbitsi(i) (i)
#endif

LOCALPROC myfp_FromExtendedFormat(myfpr *r, ui4r v2, ui5r v1, ui5r v0)
{
	int ii;
	union {
		ui3b fbits[16];
		myfpr x;
	} extended_p, fixer;
	myfpr result;

	/* filler.  Don't really need, helped me debug */
	for (ii = 10; ii < 16; ii++) {
		extended_p.fbits[ii] = 0x77;
	}

	/* sign and exponent */
	*(ui4b *)&extended_p.fbits[fldbitsi(8)] = v2;

	/* mantissa */
	*(ui5b *)&extended_p.fbits[fldbitsi(4)] = v1;
	*(ui5b *)&extended_p.fbits[fldbitsi(0)] = v0;

	result = extended_p.x;


	FPDEBUG printf("read_long_double reads %lf (0x",
		(double)extended_p.x);
	FPDEBUG {
		int ii;

		for (ii = 12; ii >= 0; ii--) {
			printf("%02x", extended_p.fbits[ii]);
		}
	}



	/*
		Check for unusual condition where 68881 mantissa high bit
		isnt' set.
		This is an error for the x86 FPU, so I need to patch it.
	*/

	if ((extended_p.fbits[fldbitsi(7)] & 0x80) == 0x00) {
		/* High bit of mantissa isn't set */

		if (((extended_p.fbits[fldbitsi(9)] & 0x7F) == 0x7F)
			&& (extended_p.fbits[fldbitsi(8)] == 0xFF))
		{
			/* infinity, OK for mantissa high bit to not be set */
		} else if (((extended_p.fbits[fldbitsi(9)] & 0x7F) == 0x00)
			&& (extended_p.fbits[fldbitsi(8)] == 0x00))
		{
			/* zero, OK for mantissa high bit to not be set */
		} else {
			/*
				Not OK for mantissa high bit to not be set.
				Fix using the FPU itself
			*/

			extended_p.fbits[fldbitsi(7)] =
				extended_p.fbits[fldbitsi(7)] | 0x80;
					/* Force top bit to 1 */

			/*
				Create a second extended long with *only* the high bit
				of the mantissa set, identical sign and exponent.  Then
				subtract it off to remove the 1 bit we just forced in
				the top of the mantissa.
			*/

			/* sign and exponent */
			fixer.fbits[fldbitsi(9)] = extended_p.fbits[fldbitsi(9)];
			fixer.fbits[fldbitsi(8)] = extended_p.fbits[fldbitsi(8)];

			/* mantissa */
			fixer.fbits[fldbitsi(7)] = 0x80;
			fixer.fbits[fldbitsi(6)] = 0x00;
			fixer.fbits[fldbitsi(5)] = 0x00;
			fixer.fbits[fldbitsi(4)] = 0x00;
			fixer.fbits[fldbitsi(3)] = 0x00;
			fixer.fbits[fldbitsi(2)] = 0x00;
			fixer.fbits[fldbitsi(1)] = 0x00;
			fixer.fbits[fldbitsi(0)] = 0x00;

			result = extended_p.x - fixer.x;

			FPDEBUG
			{
				printf(
					"Fixing denormalized extended precision float\n");

				printf("XP:  0x");
				for (ii = 0; ii < 16; ii++) {
					printf("%02x", extended_p.fbits[ii]);
				}
				printf(" %lf\n", (double)extended_p.x);

				printf("FX:  0x");
				for (ii = 0; ii < 16; ii++) {
					printf("%02x", fixer.fbits[ii]);
				}
				printf(" %lf\n", (double)fixer.x);

				fixer.x = result;

				printf("Out: 0x");
				for (ii = 0; ii < 16; ii++) {
					printf("%02x", fixer.fbits[ii]);
				}
				printf(" %lf\n", (double)fixer.x);
			}
		}
	}

	/*
		printf("XP: 0x");
		for (ii = 0; ii < 16; ii++) {
			printf("%02x", extended_p.fbits[ii]);
		}
		printf(" %lf\n", (double)extended_p.x);

		extended_p.x = 1.5 + 7.0 / 1024 / 1024 / 1024 / 1024;

		printf("ZP: 0x");
		for(ii = 0; ii < 16; ii++) {
			printf("%02x", extended_p.fbits[ii]);
		}
		printf(" %lf\n", (double)extended_p.x);
	*/

	/*
		printf("read_long_double returns %lf\n", (double)extended_p.x);
	*/

	*r = result;
}


LOCALPROC myfp_ToExtendedFormat(myfpr *dd, ui4r *v2, ui5r *v1, ui5r *v0)
{
	union {
		ui3b fbits[16];
		myfpr x;
	} extended_p;

	extended_p.x = *dd;

	/* sign and exponent */
	*v2 = *(ui4b *)&extended_p.fbits[fldbitsi(8)];

	/* mantissa */
	*v1 = *(ui5b *)&extended_p.fbits[fldbitsi(4)];
	*v0 = *(ui5b *)&extended_p.fbits[fldbitsi(0)];
}


/*
	Only valid for a host that's an X386 processor
*/

#if BigEndianUnaligned
#define fdbitsi(i) (7 - (i))
#else
#define fdbitsi(i) (i)
#endif

LOCALPROC myfp_FromDoubleFormat(myfpr *r, ui5r v1, ui5r v0)
{
	union {
		ui3b fbits[8];
		double d;
	} double_p;

	*(ui5b *)&double_p.fbits[fdbitsi(4)] = v1;
	*(ui5b *)&double_p.fbits[fdbitsi(0)] = v0;

	*r = (myfpr)double_p.d;
}

LOCALPROC myfp_ToDoubleFormat(myfpr *dd, ui5r *v1, ui5r *v0)
{
	union {
		ui3b fbits[8];
		double d;
	} double_p;

	double_p.d = (double)*dd;

	*v1 = *(ui5b *)&double_p.fbits[fdbitsi(4)];
	*v0 = *(ui5b *)&double_p.fbits[fdbitsi(0)];
}


LOCALPROC myfp_FromSingleFormat(myfpr *r, ui5r x)
{
	union {
		ui5b fbits;
		float f;
	} single_p;

	single_p.fbits = x;

	*r = (myfpr)single_p.f;
}


LOCALFUNC ui5r myfp_ToSingleFormat(myfpr *ff)
{
	union {
		ui5b fbits;
		float f;
	} single_p;

	single_p.f = (float)*ff;

	return single_p.fbits;
}

LOCALPROC myfp_FromLong(myfpr *r, ui5r x)
{
	*r = (myfpr)(si5b)x;
}

LOCALFUNC ui5r myfp_ToLong(myfpr *x)
{
	return (ui5r)(si5r)(si5b)*x;
}

LOCALFUNC blnr myfp_IsNan(myfpr *x)
{
	return isnan(*x);
}

LOCALFUNC blnr myfp_IsInf(myfpr *x)
{
	return isinf(*x);
}

LOCALFUNC blnr myfp_IsZero(myfpr *x)
{
	return (*x == 0.0);
}

LOCALFUNC blnr myfp_IsNeg(myfpr *x)
{
	return (*x < 0.0);
}

#define myfp_Add(r, a, b) (*r) = ((*a) + (*b))
#define myfp_Sub(r, a, b) (*r) = ((*a) - (*b))
#define myfp_Mul(r, a, b) (*r) = ((*a) * (*b))
#define myfp_Div(r, a, b) (*r) = ((*a) / (*b))
#define myfp_Mod(r, a, b) (*r) = (fmod((*a), (*b)))
#define myfp_Rem(r, a, b) (*r) = (remainder((*a), (*b)))

LOCALPROC myfp_Scale(myfpr *r, myfpr *a, myfpr *b)
{
	myfpr fscaleval;
	fscaleval = (*b < 0.0) ? ceil(*b) : floor(*b);
	*r = *a * pow(2.0, fscaleval);
}

LOCALPROC myfp_GetMan(myfpr *r, myfpr *x)
{
	int expval;

	*r = frexp(*x, &expval) * 2.0;
}

LOCALPROC myfp_GetExp(myfpr *r, myfpr *x)
{
	int expval;

	(void) frexp(*x, &expval);
	*r = (myfpr) (expval - 1);
}

LOCALPROC myfp_IntRZ(myfpr *r, myfpr *x)
{
	*r = (*x < 0.0) ? ceil(*x) : floor(*x);
}

LOCALPROC myfp_Int(myfpr *r, myfpr *x)
{
	/* Should take the current rounding mode into account, don't */
	*r = floor(*x + 0.5);
}

#define myfp_RoundToSingle(r, x) (*r) = ((myfpr)(float)(*x))
#define myfp_RoundToDouble(r, x) (*r) = ((myfpr)(double)(*x))

#define myfp_Abs(r, x) (*r) = (((*x) < 0) ? - (*x) : (*x))
#define myfp_Neg(r, x) (*r) = (- (*x))
#define myfp_Sqrt(r, x) (*r) = (sqrt(*x))

#define myfp_TenToX(r, x) (*r) = (pow(10.0, *x))
#define myfp_TwoToX(r, x) (*r) = (pow(2.0, *x))
#define myfp_EToX(r, x) (*r) = (exp(*x))
#define myfp_EToXM1(r, x) (*r) = (exp(*x) - 1.0)
#define myfp_Log10(r, x) (*r) = (log10(*x))
#define myfp_Log2(r, x) (*r) = (log(*x) / log(2.0))
#define myfp_LogN(r, x) (*r) = (log(*x))
#define myfp_LogNP1(r, x) (*x) = (log(*x + 1.0))

#define myfp_Sin(r, x) (*r) = (sin(*x))
#define myfp_Cos(r, x) (*r) = (cos(*x))
#define myfp_Tan(r, x) (*r) = (tan(*x))

#define myfp_ASin(r, x) (*r) = (asin(*x))
#define myfp_ACos(r, x) (*r) = (acos(*x))
#define myfp_ATan(r, x) (*r) = (atan(*x))

#define myfp_Sinh(r, x) (*r) = (sinh(*x))
#define myfp_Cosh(r, x) (*r) = (cosh(*x))
#define myfp_Tanh(r, x) (*r) = (tanh(*x))

#define myfp_ATanh(r, x) (*r) = (atanh(*x))

LOCALPROC myfp_SinCos(myfpr *r_sin, myfpr *r_cos, myfpr *source)
{
	*r_sin = sin(*source);
	*r_cos = cos(*source);
}

LOCALFUNC blnr myfp_getCR(myfpr *r, ui4b opmode)
{
	myfpr v;

	switch (opmode) {
		case 0x00:
			v = M_PI;
			break;
		case 0x0B:
			v = log10(2.0);
			break;
		case 0x0C:
			v = exp(1.0);
			break;
		case 0x0D:
			v = 1.0 / log(2.0);
			break;
		case 0x0E:
			v = 1.0 / log(10.0);
			break;
		case 0x0F:
			v = 0.0;
			break;
		case 0x30:
			v = log(2.0);
			break;
		case 0x31:
			v = log(10.0);
			break;
		case 0x32:
			v = 1.0;
			break;
		case 0x33:
			v = 10.0;
			break;
		case 0x34:
			v = 100.0;
			break;
		case 0x35:
			v = 10000.0;
			break;
		case 0x36:
			v = 1.0e8;
			break;
		case 0x37:
			v = 1.0e16;
			break;
		case 0x38:
			v = 1.0e32;
			break;
		case 0x39:
			v = 1.0e64;
			break;
		case 0x3A:
			v = 1.0e128;
			break;
		case 0x3B:
			v = 1.0e256;
			break;
		case 0x3C:
			v = 1.0e512;
			break;
		case 0x3D:
			v = 1.0e1024;
			break;
		case 0x3E:
			v = 1.0e2048;
			break;
		case 0x3F:
			v = 1.0e4096;
			break;
		default:
			return falseblnr;
	}

	*r = v;
	return trueblnr;
}

LOCALVAR struct myfp_envStruct
{
	ui5r FPCR;  /* Floating point control register */
	ui5r FPSR;  /* Floating point status register */
} myfp_env;

LOCALPROC myfp_SetFPCR(ui5r v)
{
	myfp_env.FPCR = v;
}

LOCALPROC myfp_SetFPSR(ui5r v)
{
	myfp_env.FPSR = v;
}

LOCALFUNC ui5r myfp_GetFPCR(void)
{
	return myfp_env.FPCR;
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
