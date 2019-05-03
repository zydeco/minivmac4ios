/*
	SCSIEMDV.c

	Copyright (C) 2004 Philip Cummins, Paul C. Pratt

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
	Small Computer System Interface EMulated DeVice

	Emulates the SCSI found in the Mac Plus.

	This code adapted from "SCSI.c" in vMac by Philip Cummins.
*/

/* NCR5380 chip emulation by Yoav Shadmi, 1998 */

#ifndef AllFiles
#include "SYSDEPNS.h"

#include "ENDIANAC.h"
#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#include "MINEM68K.h"
#endif

#include "SCSIEMDV.h"

#define scsiRd   0x00
#define scsiWr   0x01

#define sCDR     0x00 /* current scsi data register  (r/o) */
#define sODR     0x00 /* output data register        (w/o) */
#define sICR     0x02 /* initiator command register  (r/w) */
#define sMR      0x04 /* mode register               (r/w) */
#define sTCR     0x06 /* target command register     (r/w) */
#define sCSR     0x08 /* current SCSI bus status     (r/o) */
#define sSER     0x08 /* select enable register      (w/o) */
#define sBSR     0x0A /* bus and status register     (r/o) */
#define sDMAtx   0x0A /* start DMA send              (w/o) */
#define sIDR     0x0C /* input data register         (r/o) */
#define sTDMArx  0x0C /* start DMA target receive    (w/o) */
#define sRESET   0x0E /* reset parity/interrupt      (r/o) */
#define sIDMArx  0x0E /* start DMA initiator receive (w/o) */

#define kSCSI_Size 0x00010

LOCALVAR ui3b SCSI[kSCSI_Size];

GLOBALPROC SCSI_Reset(void)
{
	int i;

	for (i = 0; i < kSCSI_Size; i++) {
		SCSI[i] = 0;
	}
}

LOCALPROC SCSI_BusReset(void)
{
	SCSI[scsiRd + sCDR] = 0;
	SCSI[scsiWr + sODR] = 0;
	SCSI[scsiRd + sICR] = 0x80;
	SCSI[scsiWr + sICR] &= 0x80;
	SCSI[scsiRd + sMR] &= 0x40;
	SCSI[scsiWr + sMR] &= 0x40;
	SCSI[scsiRd + sTCR] = 0;
	SCSI[scsiWr + sTCR] = 0;
	SCSI[scsiRd + sCSR] = 0x80;
	SCSI[scsiWr + sSER] = 0;
	SCSI[scsiRd + sBSR] = 0x10;
	SCSI[scsiWr + sDMAtx] = 0;
	SCSI[scsiRd + sIDR] = 0;
	SCSI[scsiWr + sTDMArx] = 0;
	SCSI[scsiRd + sRESET] = 0;
	SCSI[scsiWr + sIDMArx] = 0;
#if 0
	SCSI[scsiRd + sODR + dackWr] = 0;
	SCSI[scsiWr + sIDR + dackRd] = 0;
#endif

	/* The missing piece of the puzzle.. :) */
	put_ram_word(0xb22, get_ram_word(0xb22) | 0x8000);
}

LOCALPROC SCSI_Check(void)
{
	/*
		The arbitration select/reselect scenario
		[stub.. doesn't really work...]
	*/
	if ((SCSI[scsiWr + sODR] >> 7) == 1) {
		/* Check if the Mac tries to be an initiator */
		if ((SCSI[scsiWr + sMR] & 1) == 1) {
			/* the Mac set arbitration in progress */
			/*
				stub! tell the mac that there
				is arbitration in progress...
			*/
			SCSI[scsiRd + sICR] |= 0x40;
			/* ... that we didn't lose arbitration ... */
			SCSI[scsiRd + sICR] &= ~ 0x20;
			/*
				... and that there isn't a higher priority ID present...
			*/
			SCSI[scsiRd + sCDR] = 0x00;

			/*
				... the arbitration and selection/reselection is
				complete. the initiator tries to connect to the SCSI
				device, fails and returns after timeout.
			*/
		}
	}

	/* check the chip registers, AS SET BY THE CPU */
	if ((SCSI[scsiWr + sICR] >> 7) == 1) {
		/* Check Assert RST */
		SCSI_BusReset();
	} else {
		SCSI[scsiRd + sICR] &= ~ 0x80;
		SCSI[scsiRd + sCSR] &= ~ 0x80;
	}

	if ((SCSI[scsiWr + sICR] >> 2) == 1) {
		/* Check Assert SEL */
		SCSI[scsiRd + sCSR] |= 0x02;
		SCSI[scsiRd + sBSR] = 0x10;
	} else {
		SCSI[scsiRd + sCSR] &= ~ 0x02;
	}
}

GLOBALFUNC ui5b SCSI_Access(ui5b Data, blnr WriteMem, CPTR addr)
{
	if (addr < (kSCSI_Size / 2)) {
		addr *= 2;
		if (WriteMem) {
			SCSI[addr + 1] = Data;
			SCSI_Check();
		} else {
			Data = SCSI[addr];
		}
	}
	return Data;
}
