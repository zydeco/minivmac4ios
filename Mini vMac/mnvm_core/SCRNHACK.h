/*
	SCRNHACK.h

	Copyright (C) 2007 Paul C. Pratt

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
	SCReeN Hack

	Patch ROM to support other screen sizes.
*/


#if CurEmMd <= kEmMd_128K
	do_put_mem_long(112 + ROM, kVidMem_Base);
	do_put_mem_long(260 + ROM, kVidMem_Base);
	do_put_mem_long(292 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 + 9) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 24))
			/ 8);

	/* sad mac, error code */
	do_put_mem_word(330 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(342 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(350 + ROM, vMacScreenWidth / 4 * 3 - 1);
	/* sad mac, blink pixels */
	do_put_mem_word(358 + ROM, vMacScreenWidth - 4);

	do_put_mem_word(456 + ROM,
		(vMacScreenHeight * vMacScreenWidth / 32) - 1 + 32);

	/* screen setup, main */
	{
		pto = 862 + ROM;
		do_put_mem_word(pto, 0x4EB9); /* JSR */
		pto += 2;
		do_put_mem_long(pto, kROM_Base + (patchp - ROM));
		pto += 4;

		do_put_mem_word(patchp, 0x21FC); /* MOVE.L */
		patchp += 2;
		do_put_mem_long(patchp, kVidMem_Base); /* kVidMem_Base */
		patchp += 4;
		do_put_mem_word(patchp, 0x0824); /* (ScrnBase) */
		patchp += 2;
		do_put_mem_word(patchp, 0x4E75); /* RTS */
		patchp += 2;
	}
	do_put_mem_word(892 + ROM, vMacScreenHeight - 1);
	do_put_mem_word(894 + ROM, vMacScreenWidth - 1);

	/* blink floppy, disk icon */
	do_put_mem_long(1388 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 25) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 16))
			/ 8);
	/* blink floppy, question mark */
	do_put_mem_long(1406 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 10) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);

	/* blink floppy and sadmac, position */
	do_put_mem_word(1966 + ROM, vMacScreenWidth / 8 - 4);
	do_put_mem_word(1982 + ROM, vMacScreenWidth / 8);
	/* sad mac, mac icon */
	do_put_mem_long(2008 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 25) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 16))
			/ 8);
	/* sad mac, frown */
	do_put_mem_long(2020 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 19) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);
	do_put_mem_word(2052 + ROM, vMacScreenWidth / 8 - 2);

	/* cursor handling */
#if vMacScreenWidth >= 1024
	pto = 3448 + ROM;
	do_put_mem_word(pto, 0x4EB9); /* JSR */
	pto += 2;
	do_put_mem_long(pto, kROM_Base + (patchp - ROM));
	pto += 4;

	do_put_mem_word(patchp, 0x41F8); /* Lea.L     (CrsrSave),A0 */
	patchp += 2;
	do_put_mem_word(patchp, 0x088C);
	patchp += 2;
	do_put_mem_word(patchp, 0x203C); /* MOVE.L #$x,D0 */
	patchp += 2;
	do_put_mem_long(patchp, (vMacScreenWidth / 8));
	patchp += 4;
	do_put_mem_word(patchp, 0x4E75); /* RTS */
	patchp += 2;
#else
	do_put_mem_word(3452 + ROM, 0x7000 + (vMacScreenWidth / 8));
#endif
	do_put_mem_word(3572 + ROM, vMacScreenWidth - 32);
	do_put_mem_word(3578 + ROM, vMacScreenWidth - 32);
	do_put_mem_word(3610 + ROM, vMacScreenHeight - 16);
	do_put_mem_word(3616 + ROM, vMacScreenHeight);
#if vMacScreenWidth >= 1024
	pto = 3646 + ROM;
	do_put_mem_word(pto, 0x4EB9); /* JSR */
	pto += 2;
	do_put_mem_long(pto, kROM_Base + (patchp - ROM));
	pto += 4;

	do_put_mem_word(patchp, 0x2A3C); /* MOVE.L #$x,D5 */
	patchp += 2;
	do_put_mem_long(patchp, (vMacScreenWidth / 8));
	patchp += 4;
	do_put_mem_word(patchp, 0xC2C5); /* MulU      D5,D1 */
	patchp += 2;
	do_put_mem_word(patchp, 0xD3C1); /* AddA.L    D1,A1 */
	patchp += 2;
	do_put_mem_word(patchp, 0x4E75); /* RTS */
	patchp += 2;
#else
	do_put_mem_word(3646 + ROM, 0x7A00 + (vMacScreenWidth / 8));
#endif

	/* set up screen bitmap */
	do_put_mem_word(3832 + ROM, vMacScreenHeight);
	do_put_mem_word(3838 + ROM, vMacScreenWidth);
	/* do_put_mem_word(7810 + ROM, vMacScreenHeight); */

#elif CurEmMd <= kEmMd_Plus

	do_put_mem_long(138 + ROM, kVidMem_Base);
	do_put_mem_long(326 + ROM, kVidMem_Base);
	do_put_mem_long(356 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 + 9) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 24))
			/ 8);

	/* sad mac, error code */
	do_put_mem_word(392 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(404 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(412 + ROM, vMacScreenWidth / 4 * 3 - 1);
	/* sad mac, blink pixels */
	do_put_mem_long(420 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 + 17) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);

	do_put_mem_word(494 + ROM,
		(vMacScreenHeight * vMacScreenWidth / 32) - 1);

	/* screen setup, main */
	{
		pto = 1132 + ROM;
		do_put_mem_word(pto, 0x4EB9); /* JSR */
		pto += 2;
		do_put_mem_long(pto, kROM_Base + (patchp - ROM));
		pto += 4;

		do_put_mem_word(patchp, 0x21FC); /* MOVE.L */
		patchp += 2;
		do_put_mem_long(patchp, kVidMem_Base); /* kVidMem_Base */
		patchp += 4;
		do_put_mem_word(patchp, 0x0824); /* (ScrnBase) */
		patchp += 2;
		do_put_mem_word(patchp, 0x4E75); /* RTS */
		patchp += 2;
	}
	do_put_mem_word(1140 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(1172 + ROM, vMacScreenHeight);
	do_put_mem_word(1176 + ROM, vMacScreenWidth);

	/* blink floppy, disk icon */
	do_put_mem_long(2016 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 25) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 16))
			/ 8);
	/* blink floppy, question mark */
	do_put_mem_long(2034 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 10) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);

	do_put_mem_word(2574 + ROM, vMacScreenHeight);
	do_put_mem_word(2576 + ROM, vMacScreenWidth);

	/* blink floppy and sadmac, position */
	do_put_mem_word(3810 + ROM, vMacScreenWidth / 8 - 4);
	do_put_mem_word(3826 + ROM, vMacScreenWidth / 8);
	/* sad mac, mac icon */
	do_put_mem_long(3852 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 25) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 16))
			/ 8);
	/* sad mac, frown */
	do_put_mem_long(3864 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 19) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);
	do_put_mem_word(3894 + ROM, vMacScreenWidth / 8 - 2);

	/* cursor handling */
#if vMacScreenWidth >= 1024
	pto = 7372 + ROM;
	do_put_mem_word(pto, 0x4EB9); /* JSR */
	pto += 2;
	do_put_mem_long(pto, kROM_Base + (patchp - ROM));
	pto += 4;

	do_put_mem_word(patchp, 0x41F8); /* Lea.L     (CrsrSave), A0 */
	patchp += 2;
	do_put_mem_word(patchp, 0x088C);
	patchp += 2;
	do_put_mem_word(patchp, 0x203C); /* MOVE.L #$x, D0 */
	patchp += 2;
	do_put_mem_long(patchp, (vMacScreenWidth / 8));
	patchp += 4;
	do_put_mem_word(patchp, 0x4E75); /* RTS */
	patchp += 2;
#else
	do_put_mem_word(7376 + ROM, 0x7000 + (vMacScreenWidth / 8));
#endif
	do_put_mem_word(7496 + ROM, vMacScreenWidth - 32);
	do_put_mem_word(7502 + ROM, vMacScreenWidth - 32);
	do_put_mem_word(7534 + ROM, vMacScreenHeight - 16);
	do_put_mem_word(7540 + ROM, vMacScreenHeight);
#if vMacScreenWidth >= 1024
	pto = 7570 + ROM;
	do_put_mem_word(pto, 0x4EB9); /* JSR */
	pto += 2;
	do_put_mem_long(pto, kROM_Base + (patchp - ROM));
	pto += 4;

	do_put_mem_word(patchp, 0x2A3C); /* MOVE.L #$x,D5 */
	patchp += 2;
	do_put_mem_long(patchp, (vMacScreenWidth / 8));
	patchp += 4;
	do_put_mem_word(patchp, 0xC2C5); /* MulU      D5,D1 */
	patchp += 2;
	do_put_mem_word(patchp, 0xD3C1); /* AddA.L    D1,A1 */
	patchp += 2;
	do_put_mem_word(patchp, 0x4E75); /* RTS */
	patchp += 2;
#else
	do_put_mem_word(7570 + ROM, 0x7A00 + (vMacScreenWidth / 8));
#endif

	/* set up screen bitmap */
	do_put_mem_word(7784 + ROM, vMacScreenHeight);
	do_put_mem_word(7790 + ROM, vMacScreenWidth);
	do_put_mem_word(7810 + ROM, vMacScreenHeight);

#if 0
	/*
		Haven't got these working. Alert outlines ok, but
		not contents. Perhaps global position of contents
		stored in system resource file.
	*/

	/* perhaps switch disk alert */
	do_put_mem_word(10936 + ROM, vMacScreenHeight / 2 - 91);
	do_put_mem_word(10938 + ROM, vMacScreenWidth / 2 - 136);
	do_put_mem_word(10944 + ROM, vMacScreenHeight / 2 - 19);
	do_put_mem_word(10946 + ROM, vMacScreenWidth / 2 + 149);

	do_put_mem_word(11008 + ROM, ?);
	do_put_mem_word(11010 + ROM, ?);

	/* DSAlertRect */
	do_put_mem_word(4952 + ROM, vMacScreenHeight / 2 - 107);
	do_put_mem_word(4954 + ROM, vMacScreenWidth / 2 - 236);
	do_put_mem_word(4958 + ROM, vMacScreenHeight / 2 + 19);
	do_put_mem_word(4960 + ROM, vMacScreenWidth / 2 + 236);

	do_put_mem_word(5212 + ROM, vMacScreenHeight / 2 - 101);
	do_put_mem_word(5214 + ROM, vMacScreenWidth / 2 - 218);
#endif

#elif CurEmMd <= kEmMd_Classic

	/* screen setup, main */
	{
		pto = 1482 + ROM;
		do_put_mem_word(pto, 0x4EB9); /* JSR */
		pto += 2;
		do_put_mem_long(pto, kROM_Base + (patchp - ROM));
		pto += 4;

		do_put_mem_word(patchp, 0x21FC); /* MOVE.L */
		patchp += 2;
		do_put_mem_long(patchp, kVidMem_Base); /* kVidMem_Base */
		patchp += 4;
		do_put_mem_word(patchp, 0x0824); /* (ScrnBase) */
		patchp += 2;
		do_put_mem_word(patchp, 0x4E75); /* RTS */
		patchp += 2;
	}
	do_put_mem_word(1490 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(1546 + ROM, vMacScreenHeight);
	do_put_mem_word(1550 + ROM, vMacScreenWidth);

	do_put_mem_word(2252 + ROM, vMacScreenHeight);
	do_put_mem_word(2254 + ROM, vMacScreenWidth);

	/* blink floppy, disk icon */
	do_put_mem_long(3916 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 25) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 16))
			/ 8);
	/* blink floppy, question mark */
	do_put_mem_long(3934 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 10) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);

	do_put_mem_long(4258 + ROM, kVidMem_Base);
	do_put_mem_word(4264 + ROM, vMacScreenHeight);
	do_put_mem_word(4268 + ROM, vMacScreenWidth);
	do_put_mem_word(4272 + ROM, vMacScreenWidth / 8);
	do_put_mem_long(4276 + ROM, vMacScreenNumBytes);

	/* sad mac, mac icon */
	do_put_mem_long(4490 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 25) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 16))
			/ 8);
	/* sad mac, frown */
	do_put_mem_long(4504 + ROM, kVidMem_Base
		+ (((vMacScreenHeight / 4) * 2 - 19) * vMacScreenWidth
			+ (vMacScreenWidth / 2 - 8))
			/ 8);
	do_put_mem_word(4528 + ROM, vMacScreenWidth / 8);
	/* blink floppy and sadmac, position */
	do_put_mem_word(4568 + ROM, vMacScreenWidth / 8);
	do_put_mem_word(4586 + ROM, vMacScreenWidth / 8);

	/* cursor handling */
#if vMacScreenWidth >= 1024
	pto = 101886 + ROM;
	do_put_mem_word(pto, 0x4EB9); /* JSR */
	pto += 2;
	do_put_mem_long(pto, kROM_Base + (patchp - ROM));
	pto += 4;

	do_put_mem_word(patchp, 0x41F8); /* Lea.L     (CrsrSave),A0 */
	patchp += 2;
	do_put_mem_word(patchp, 0x088C);
	patchp += 2;
	do_put_mem_word(patchp, 0x203C); /* MOVE.L #$x,D0 */
	patchp += 2;
	do_put_mem_long(patchp, (vMacScreenWidth / 8));
	patchp += 4;
	do_put_mem_word(patchp, 0x4E75); /* RTS */
	patchp += 2;
#else
	do_put_mem_word(101890 + ROM, 0x7000 + (vMacScreenWidth / 8));
#endif
	do_put_mem_word(102010 + ROM, vMacScreenWidth - 32);
	do_put_mem_word(102016 + ROM, vMacScreenWidth - 32);
	do_put_mem_word(102048 + ROM, vMacScreenHeight - 16);
	do_put_mem_word(102054 + ROM, vMacScreenHeight);
#if vMacScreenWidth >= 1024
	pto = 102084 + ROM;
	do_put_mem_word(pto, 0x4EB9); /* JSR */
	pto += 2;
	do_put_mem_long(pto, kROM_Base + (patchp - ROM));
	pto += 4;

	do_put_mem_word(patchp, 0x2A3C); /* MOVE.L #$x, D5 */
	patchp += 2;
	do_put_mem_long(patchp, (vMacScreenWidth / 8));
	patchp += 4;
	do_put_mem_word(patchp, 0xC2C5); /* MulU      D5, D1 */
	patchp += 2;
	do_put_mem_word(patchp, 0xD3C1); /* AddA.L    D1, A1 */
	patchp += 2;
	do_put_mem_word(patchp, 0x4E75); /* RTS */
	patchp += 2;
#else
	do_put_mem_word(102084 + ROM, 0x7A00 + (vMacScreenWidth / 8));
#endif

	/* set up screen bitmap */
	do_put_mem_word(102298 + ROM, vMacScreenHeight);
	do_put_mem_word(102304 + ROM, vMacScreenWidth);
	do_put_mem_word(102324 + ROM, vMacScreenHeight);

#endif
