/*
	SCCEMDEV.c

	Copyright (C) 2012 Philip Cummins, Weston Pawlowski,
		Michael Fort, Paul C. Pratt

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
	Serial Communications Controller EMulated DEVice

	additions for LocalTalk networking support
		Copyright 2011-2012, Michael Fort
		enabled with "EmLocalTalk"

	-- original description: --

	Emulates the Z8530 SCC found in the Mac Plus.
		But only the minimum amount needed to emulate
		normal operation in a Mac Plus with nothing
		connected to the serial ports.
		(and not even that much is complete yet)

	This code adapted from "SCC.c" in vMac by Philip Cummins.
	With additional code by Weston Pawlowski from the Windows
	port of vMac.

	Further information was found in the
	"Zilog SCC/ESCC User's Manual".
*/

#ifndef AllFiles
#include "SYSDEPNS.h"

#include "MYOSGLUE.h"
#include "EMCONFIG.h"
#include "GLOBGLUE.h"
#endif

#include "SCCEMDEV.h"

/*
	ReportAbnormalID unused 0x0721, 0x0722, 0x074D - 0x07FF
*/

#define SCC_dolog (dbglog_HAVE && 0)
#define SCC_TrackMore 0

/* Just to make things a little easier */
#define Bit0 1
#define Bit1 2
#define Bit2 4
#define Bit3 8
#define Bit4 16
#define Bit5 32
#define Bit6 64
#define Bit7 128

/* SCC Interrupts */
#define SCC_A_Rx       8 /* Rx Char Available */
#define SCC_A_Rx_Spec  7 /* Rx Special Condition */
#define SCC_A_Tx_Empty 6 /* Tx Buffer Empty */
#define SCC_A_Ext      5 /* External/Status Change */
#define SCC_B_Rx       4 /* Rx Char Available */
#define SCC_B_Rx_Spec  3 /* Rx Special Condition */
#define SCC_B_Tx_Empty 2 /* Tx Buffer Empty */
#define SCC_B_Ext      1 /* External/Status Change */

typedef struct {
	blnr TxEnable;
	blnr RxEnable;
	blnr TxIE; /* Transmit Interrupt Enable */
	blnr TxUnderrun;
	blnr SyncHunt;
	blnr TxIP; /* Transmit Interrupt Pending */
#if EmLocalTalk
	ui3r RxBuff;
#endif
#if EmLocalTalk
	/* otherwise TxBufferEmpty always true */
	/*
		though should behave as went false
		for an instant when write to transmit buffer
	*/
	blnr TxBufferEmpty;
#endif
#if EmLocalTalk || SCC_TrackMore
	blnr ExtIE;
#endif
#if SCC_TrackMore
	blnr WaitRqstEnbl;
#endif
#if SCC_TrackMore
	blnr WaitRqstSlct;
#endif
#if SCC_TrackMore
	blnr WaitRqstRT;
#endif
#if SCC_TrackMore
	blnr PrtySpclCond;
#endif
#if SCC_TrackMore
	blnr PrtyEnable;
#endif
#if SCC_TrackMore
	blnr PrtyEven;
#endif
#if SCC_TrackMore
	blnr RxCRCEnbl;
#endif
#if SCC_TrackMore
	blnr TxCRCEnbl;
#endif
#if SCC_TrackMore
	blnr RTSctrl;
#endif
#if SCC_TrackMore
	blnr SndBrkCtrl;
#endif
#if SCC_TrackMore
	blnr DTRctrl;
#endif
#if EmLocalTalk || SCC_TrackMore
	blnr AddrSrchMd;
#endif
#if SCC_TrackMore
	blnr SyncChrLdInhb;
#endif
#if SCC_TrackMore
	ui3r ClockRate;
#endif
#if SCC_TrackMore
	ui3r DataEncoding;
#endif
#if SCC_TrackMore
	ui3r TRxCsrc;
#endif
#if SCC_TrackMore
	ui3r TClkSlct;
#endif
#if SCC_TrackMore
	ui3r RClkSlct;
#endif
#if SCC_TrackMore
	ui3r RBitsPerChar;
#endif
#if SCC_TrackMore
	ui3r TBitsPerChar;
#endif
#if EmLocalTalk || SCC_TrackMore
	ui3r RxIntMode;
#endif
#if EmLocalTalk || SCC_TrackMore
	blnr FirstChar;
#endif
#if EmLocalTalk || SCC_TrackMore
	ui3r SyncMode;
#endif
#if SCC_TrackMore
	ui3r StopBits;
#endif
#if 0 /* AllSent always true */
	blnr AllSent;
#endif
#if 0 /* CTS always false */
	blnr CTS; /* input pin, unattached, so false? */
#endif
#if 0 /* DCD always false */
	blnr DCD; /* Data Carrier Detect */
		/*
			input pin for mouse interrupts. but since
			not emulating mouse this way, leave false.
		*/
#endif
#if EmLocalTalk
	/* otherwise RxChrAvail always false */
	blnr RxChrAvail;
#endif
#if 0 /* RxOverrun always false */
	blnr RxOverrun;
#endif
#if 0 /* CRCFramingErr always false */
	blnr CRCFramingErr;
#endif
#if EmLocalTalk
	/* otherwise EndOfFrame always false */
	blnr EndOfFrame;
#endif
#if 0 /* ParityErr always false */
	blnr ParityErr;
#endif
#if 0 /* ZeroCount always false */
	blnr ZeroCount;
#endif
#if 0 /* BreakAbort always false */
	blnr BreakAbort;
#endif
#if 0 /* SyncHuntIE usually false */
	blnr SyncHuntIE;
#endif
#if SCC_TrackMore /* don't care about CTS_IE */
	blnr CTS_IE;
#endif
#if SCC_TrackMore
	blnr CRCPreset;
#endif
#if SCC_TrackMore
	blnr BRGEnbl;
#endif
#if 0 /* don't care about DCD_IE, always true */
	blnr DCD_IE;
#endif
#if SCC_TrackMore /* don't care about BreakAbortIE */
	blnr BreakAbortIE;
#endif
#if SCC_TrackMore /* don't care about Baud */
	ui3r BaudLo;
	ui3r BaudHi;
#endif
} Channel_Ty;

typedef struct {
	Channel_Ty a[2]; /* 0 = channel A, 1 = channel B */
	int SCC_Interrupt_Type;
	int PointerBits;
	ui3b InterruptVector;
	blnr MIE; /* master interrupt enable */
#if SCC_TrackMore
	blnr NoVectorSlct;
#endif
#if 0 /* StatusHiLo always false */
	blnr StatusHiLo;
#endif
} SCC_Ty;

LOCALVAR SCC_Ty SCC;

#if 0
LOCALVAR int ReadPrint;
LOCALVAR int ReadModem;
#endif

#if EmLocalTalk
static int rx_data_offset = 0;
	/* when data pending, this is used */
#endif

EXPORTFUNC blnr SCC_InterruptsEnabled(void)
{
	return SCC.MIE;
}

/* ---- */

/* Function used to update the interrupt state of the SCC */
LOCALPROC CheckSCCInterruptFlag(void)
{
#if 0 /* ReceiveAInterrupt always false */
	blnr ReceiveAInterrupt = falseblnr
		/*
			also dependeds on WR1, bits 3 and 4, but
			this doesn't change that it's all false
		*/
#if EmLocalTalk
		/* otherwise RxChrAvail always false */
		| SCC.a[0].RxChrAvail
#endif
#if 0 /* RxOverrun always false */
		| SCC.a[0].RxOverrun
#endif
#if 0 /* CRCFramingErr always false */
		| SCC.a[0].CRCFramingErr
#endif
#if EmLocalTalk
		/* otherwise EndOfFrame always false */
		| SCC.a[0].EndOfFrame
#endif
#if 0 /* ParityErr always false */
		| SCC.a[0].ParityErr
#endif
		;
#endif
#if 0
	blnr TransmitAInterrupt = SCC.a[0].TxBufferEmpty;
	/*
		but probably looking for transitions not
		current value
	*/
#endif
#if 0
	blnr ExtStatusAInterrupt = 0
#if 0 /* ZeroCount always false */
		| SCC.a[0].ZeroCount
#endif
		/* probably want transition for these, not value */
#if 0 /* DCD always false */
		| SCC.a[0].DCD /* DCD IE always true */
#endif
#if 0 /* CTS always false */
		| SCC.a[0].CTS /* would depend on CTS_IE */
#endif
		| SCC.a[0].SyncHunt /* SyncHuntIE usually false */
		| SCC.a[0].TxUnderrun /* Tx underrun/EOM IE always false */
#if 0 /* BreakAbort always false */
		| SCC.a[0].BreakAbort
#endif
		;
#endif
	ui3b NewSCCInterruptRequest;

#if EmLocalTalk
	blnr ReceiveBInterrupt = falseblnr;
	blnr RxSpclBInterrupt = falseblnr
		/* otherwise EndOfFrame always false */
		| SCC.a[1].EndOfFrame
		;
#endif

#if EmLocalTalk
	switch (SCC.a[1].RxIntMode) {
		case 0:
			/* disabled */
			RxSpclBInterrupt = falseblnr;
			break;
		case 1:
			/* Rx INT on 1st char or special condition */
			if (SCC.a[1].RxChrAvail && SCC.a[1].FirstChar) {
				ReceiveBInterrupt = trueblnr;
			}
			break;
		case 2:
			/* INT on all Rx char or special condition */
			if (SCC.a[1].RxChrAvail) {
				ReceiveBInterrupt = trueblnr;
			}
			break;
		case 3:
			/* Rx INT on special condition only */
			break;
	}
#endif

	/* Master Interrupt Enable */
	if (! SCC.MIE) {
		SCC.SCC_Interrupt_Type = 0;
	} else
#if 0
	/* External Interrupt Enable */
	if (SCC.a[1].ExtIE) {
		/* DCD Interrupt Enable */
		if (SCC.a[1].DCD_IE && 0) { /* dcd unchanged */
			SCC.SCC_Interrupt_Type = ??;
		}
	}
#endif
	if (SCC.a[0].TxIP && SCC.a[0].TxIE) {
		SCC.SCC_Interrupt_Type = SCC_A_Tx_Empty;
	} else
#if EmLocalTalk
	if (ReceiveBInterrupt) {
		SCC.SCC_Interrupt_Type = SCC_B_Rx;
	} else
	if (RxSpclBInterrupt) {
		SCC.SCC_Interrupt_Type = SCC_B_Rx_Spec;
	} else
#endif
	if (SCC.a[1].TxIP && SCC.a[1].TxIE) {
		SCC.SCC_Interrupt_Type = SCC_B_Tx_Empty;
	} else
	{
		SCC.SCC_Interrupt_Type = 0;
	}

	NewSCCInterruptRequest = (SCC.SCC_Interrupt_Type != 0) ? 1 : 0;
	if (NewSCCInterruptRequest != SCCInterruptRequest) {
#if SCC_dolog
		dbglog_WriteSetBool("SCCInterruptRequest change",
			NewSCCInterruptRequest);

		dbglog_StartLine();
		dbglog_writeCStr("SCC.SCC_Interrupt_Type <- ");
		dbglog_writeHex(SCC.SCC_Interrupt_Type);
		dbglog_writeReturn();
#endif
		SCCInterruptRequest = NewSCCInterruptRequest;
#ifdef SCCinterruptChngNtfy
		SCCinterruptChngNtfy();
#endif
	}
}

LOCALPROC SCC_InitChannel(int chan)
{
	/* anything not done by ResetChannel */

	SCC.a[chan].SyncHunt = trueblnr;
#if 0 /* DCD always false */
	SCC.a[chan].DCD = falseblnr; /* input pin, reset doesn't change */
#endif
#if 0 /* CTS always false */
	SCC.a[chan].CTS = falseblnr; /* input pin, reset doesn't change */
#endif
#if 0 /* AllSent always true */
	SCC.a[chan].AllSent = trueblnr;
#endif
#if SCC_TrackMore /* don't care about Baud */
	SCC.a[chan].BaudLo = 0;
	SCC.a[chan].BaudHi = 0;
#endif
#if 0 /* BreakAbort always false */
	SCC.a[chan].BreakAbort = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].BRGEnbl = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].TRxCsrc = 0;
#endif
#if SCC_TrackMore
	SCC.a[chan].TClkSlct = 1;
#endif
#if SCC_TrackMore
	SCC.a[chan].RClkSlct = 0;
#endif
}

LOCALPROC SCC_ResetChannel(int chan)
{
/* RR 0 */
#if EmLocalTalk
	SCC.a[chan].RxBuff = 0;
#endif
#if EmLocalTalk
	/* otherwise RxChrAvail always false */
	SCC.a[chan].RxChrAvail = falseblnr;
#endif
#if 0 /* ZeroCount always false */
	SCC.a[chan].ZeroCount = falseblnr;
#endif
#if EmLocalTalk
	/* otherwise TxBufferEmpty always true */
	SCC.a[chan].TxBufferEmpty = trueblnr;
#endif
	SCC.a[chan].TxUnderrun = trueblnr;
/* RR 1 */
#if 0 /* ParityErr always false */
	SCC.a[chan].ParityErr = falseblnr;
#endif
#if 0 /* RxOverrun always false */
	SCC.a[chan].RxOverrun = falseblnr;
#endif
#if 0 /* CRCFramingErr always false */
	SCC.a[chan].CRCFramingErr = falseblnr;
#endif
#if EmLocalTalk
	/* otherwise EndOfFrame always false */
	SCC.a[chan].EndOfFrame = falseblnr;
#endif
/* RR 3 */
#if EmLocalTalk || SCC_TrackMore
	SCC.a[chan].ExtIE = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].RxCRCEnbl = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].TxCRCEnbl = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].RTSctrl = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].SndBrkCtrl = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].DTRctrl = falseblnr;
#endif
#if EmLocalTalk || SCC_TrackMore
	SCC.a[chan].AddrSrchMd = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].SyncChrLdInhb = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].WaitRqstEnbl = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].WaitRqstSlct = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].WaitRqstRT = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].PrtySpclCond = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].PrtyEnable = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].PrtyEven = falseblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].ClockRate = 0;
#endif
#if SCC_TrackMore
	SCC.a[chan].DataEncoding = 0;
#endif
#if SCC_TrackMore
	SCC.a[chan].RBitsPerChar = 0;
#endif
#if SCC_TrackMore
	SCC.a[chan].TBitsPerChar = 0;
#endif
#if EmLocalTalk || SCC_TrackMore
	SCC.a[chan].RxIntMode = 0;
#endif
#if EmLocalTalk || SCC_TrackMore
	SCC.a[chan].FirstChar = falseblnr;
#endif
#if EmLocalTalk || SCC_TrackMore
	SCC.a[chan].SyncMode = 0;
#endif
#if SCC_TrackMore
	SCC.a[chan].StopBits = 0;
#endif
#if SCC_TrackMore
	SCC.NoVectorSlct = falseblnr;
#endif
	SCC.a[chan].TxIP = falseblnr;

	SCC.a[chan].TxEnable = falseblnr;
	SCC.a[chan].RxEnable = falseblnr;
	SCC.a[chan].TxIE = falseblnr;

#if 0 /* don't care about DCD_IE, always true */
	SCC.a[chan].DCD_IE = trueblnr;
#endif
#if SCC_TrackMore /* don't care about CTS_IE */
	SCC.a[chan].CTS_IE = trueblnr;
#endif
#if SCC_TrackMore
	SCC.a[chan].CRCPreset = falseblnr;
#endif
#if 0 /* SyncHuntIE usually false */
	SCC.a[chan].SyncHuntIE = trueblnr;
#endif
#if SCC_TrackMore /* don't care about BreakAbortIE */
	SCC.a[chan].BreakAbortIE = trueblnr;
#endif

	SCC.PointerBits = 0;

#if 0
	if (chan != 0) {
		ReadPrint = 0;
	} else {
		ReadModem = 0;
	}
#endif
}

GLOBALPROC SCC_Reset(void)
{
	SCCwaitrq = 1;

	SCC.SCC_Interrupt_Type = 0;

	SCCInterruptRequest = 0;
	SCC.PointerBits = 0;
	SCC.MIE = falseblnr;
	SCC.InterruptVector = 0;
#if 0 /* StatusHiLo always false */
	SCC.StatusHiLo = falseblnr;
#endif

	SCC_InitChannel(1);
	SCC_InitChannel(0);

	SCC_ResetChannel(1);
	SCC_ResetChannel(0);
}


#if EmLocalTalk

LOCALVAR blnr CTSpacketPending = falseblnr;
LOCALVAR ui3r CTSpacketRxDA;
LOCALVAR ui3r CTSpacketRxSA;

/*
	Function used when all the tx data is sent to the SCC as indicated
	by resetting the TX underrun/EOM latch.  If the transmit packet is
	a unicast RTS LAPD packet, we fake the corresponding CTS LAPD
	packet.  This is okay because it is only a collision avoidance
	mechanism and the Ethernet device itself and BPF automatically
	handle collision detection and retransmission.  Besides this is
	what a standard AppleTalk (LocalTalk to EtherTalk) bridge does.
*/
LOCALPROC process_transmit(void)
{
	/* Check for LLAP packets, which we won't send */
	if (LT_TxBuffSz == 3) {
		/*
			We will automatically and immediately acknowledge
			any non-broadcast RTS packets
		*/
		if ((LT_TxBuffer[0] != 0xFF) && (LT_TxBuffer[2] == 0x84)) {
#if SCC_dolog
			dbglog_WriteNote("SCC LLAP packet in process_transmit");
#endif
			if (CTSpacketPending) {
				ReportAbnormalID(0x0701,
					"Already CTSpacketPending in process_transmit");
			} else {
				CTSpacketRxDA = LT_TxBuffer[1]; /* rx da = tx sa */
				CTSpacketRxSA = LT_TxBuffer[0]; /* rx sa = tx da */
				CTSpacketPending = trueblnr;
			}
		}
	} else {
		LT_TransmitPacket();
	}
}

LOCALPROC SCC_TxBuffPut(ui3r Data)
{
	/* Buffer the data in the transmit buffer */
	if (LT_TxBuffSz < LT_TxBfMxSz) {
		LT_TxBuffer[LT_TxBuffSz] = Data;
		++LT_TxBuffSz;
	}
}

LOCALVAR ui3b MyCTSBuffer[4];

LOCALPROC GetCTSpacket(void)
{
	/* Get a single buffer worth of packets at a time */
	ui3p device_buffer = MyCTSBuffer;

#if SCC_dolog
	dbglog_WriteNote("SCC receiving CTS packet");
#endif
	/* Create the fake response from the other node */
	device_buffer[0] = CTSpacketRxDA;
	device_buffer[1] = CTSpacketRxSA;
	device_buffer[2] = 0x85;          /* llap cts */

	/* Start the receiver */
	LT_RxBuffer = device_buffer;
	LT_RxBuffSz = 3;

	CTSpacketPending = falseblnr;
}

/*
	This function is called once all the normal packet bytes have been
	received.
*/
LOCALPROC rx_complete(void)
{
	if (SCC.a[1].EndOfFrame) {
		ReportAbnormalID(0x0702, "EndOfFrame true in rx_complete");
	}
	if (! SCC.a[1].RxChrAvail) {
		ReportAbnormalID(0x0703, "RxChrAvail false in rx_complete");
	}
	if (SCC.a[1].SyncHunt) {
		ReportAbnormalID(0x0704, "SyncHunt true in rx_complete");
	}

	/*
		Need to wait for rx_eof_pending (end of frame) to clear before
		preparing the next packet for receive.
	*/
	LT_RxBuffer = nullpr;

	SCC.a[1].EndOfFrame = trueblnr;
}

LOCALPROC SCC_RxBuffAdvance(void)
{
	ui3r value;

	/*
		From the manual:
		"If status is checked, it must be done before the data is read,
		because the act of reading the data pops both the data and
		error FIFOs."
	*/

	if (nullpr == LT_RxBuffer) {
		value = 0x7E;
		SCC.a[1].RxChrAvail = falseblnr;
	} else {
		if (rx_data_offset < LT_RxBuffSz) {
			value = LT_RxBuffer[rx_data_offset];
		} else {
			ui5r i = rx_data_offset - LT_RxBuffSz;

			/* if i==0 in first byte of CRC, have not got EOF yet */
			if (i == 1) {
				rx_complete();
			}

			value = 0;
		}
		++rx_data_offset;
	}

	SCC.a[1].RxBuff = value;
}

/* LLAP/SDLC address */
LOCALVAR ui3b my_node_address = 0;

LOCALPROC GetNextPacketForMe(void)
{
	unsigned char dst;
	unsigned char src;

label_retry:
	LT_ReceivePacket();

	if (nullpr != LT_RxBuffer) {
#if SCC_dolog
		dbglog_WriteNote("SCC receiving packet from BPF");
#endif

		/* Is this packet destined for me? */
		dst = LT_RxBuffer[0];
		src = LT_RxBuffer[1];
		if (src == my_node_address) {
#if SCC_dolog
			dbglog_WriteNote("SCC ignore packet from myself");
#endif
			LT_RxBuffer = nullpr;
			goto label_retry;
		} else if ((dst == my_node_address)
			|| (dst == 0xFF)
			|| ! SCC.a[1].AddrSrchMd)
		{
			/* ok */
		} else {
#if SCC_dolog
			dbglog_WriteNote("SCC ignore packet not for me");
#endif
			LT_RxBuffer = nullpr;
			goto label_retry;
		}
	}
}

/*
	External function, called periodically, to poll for any new LTOE
	packets. Any new packets are queued into the packet receipt queue.
*/
GLOBALPROC LocalTalkTick(void)
{
	if (SCC.a[1].RxEnable
		&& (! SCC.a[1].RxChrAvail))
	{
		if (nullpr != LT_RxBuffer) {
#if SCC_dolog
			dbglog_WriteNote("SCC recover abandoned packet");
#endif
		} else {
			if (CTSpacketPending)  {
				GetCTSpacket();
			} else {
				GetNextPacketForMe();
			}
		}

		if (nullpr != LT_RxBuffer) {
			rx_data_offset  = 0;
			SCC.a[1].EndOfFrame = falseblnr;
			SCC.a[1].RxChrAvail = trueblnr;
			SCC.a[1].SyncHunt = falseblnr;

			SCC_RxBuffAdvance();
			/* We can update the rx interrupt if enabled */
			CheckSCCInterruptFlag();
		}
	}
}

#endif


#if 0
LOCALPROC SCC_Interrupt(int Type)
{
	if (SCC.MIE) { /* Master Interrupt Enable */

		if (Type > SCC.SCC_Interrupt_Type) {
			SCC.SCC_Interrupt_Type = Type;
		}

		CheckSCCInterruptFlag();
	}
}
#endif

#if 0
LOCALPROC SCC_Int(void)
{
	/* This should be called at regular intervals */

	/* Turn off Sync/Hunt Mode */
	if (SCC.a[0].SyncHunt) {
		SCC.a[0].SyncHunt = falseblnr;

#ifdef _SCC_Debug2
		vMac_Message("SCC_Int: Disable Sync/Hunt on A");
#endif

#if 0 /* SyncHuntIE usually false */
		if (SCC.a[0].SyncHuntIE) {
			SCC_Interrupt(SCC_A_Ext);
		}
#endif
	}
	if (SCC.a[1].SyncHunt) {
		SCC.a[1].SyncHunt = falseblnr;

#ifdef _SCC_Debug2
		vMac_Message("SCC_Int: Disable Sync/Hunt on B");
#endif

#if 0 /* SyncHuntIE usually false */
		if (SCC.a[1].SyncHuntIE) {
			SCC_Interrupt(SCC_B_Ext);
		}
#endif
	}

#if 0
	/* Check for incoming data */
	if (ModemPort)
	{
		if (! SCC.a[0].RxEnable) { /* Rx Disabled */
			ReadModem = 0;
		}

		if ((ModemBytes > 0) && (ModemCount > ModemBytes - 1))
		{
			SCC.a[0].RxChrAvail = falseblnr;
			ReadModem = ModemBytes = ModemCount = 0;
		}

		if (ReadModem) {
			ReadModem = 2;

			SCC.a[0].RxChrAvail = trueblnr;

			if (SCC.a[0].WR[0] & Bit5
				&& ! (SCC.a[0].WR[0] & (Bit4 | Bit3)))
			{
				/* Int on next Rx char */
				SCC_Interrupt(SCC_A_Rx);
			} else if (SCC.a[0].WR[1] & Bit3
				&& ! (SCC.a[0].WR[1] & Bit4))
			{
				/* Int on first Rx char */
				SCC_Interrupt(SCC_A_Rx);
			} else if (SCC.a[0].WR[1] & Bit4
				&& ! (SCC.a[0].WR[1] & Bit3))
			{
				/* Int on all Rx chars */
				SCC_Interrupt(SCC_A_Rx);
			}
		}
	}
	if (PrintPort) {
		if (! SCC.a[1].RxEnable) {
			/* Rx Disabled */
			ReadPrint = 0;
		}

		if ((PrintBytes > 0) && (PrintCount > PrintBytes - 1)) {
			SCC.a[1].RxChrAvail = falseblnr;
			ReadPrint = PrintBytes = PrintCount = 0;
		}

		if (ReadPrint) {
			ReadPrint = 2;

			SCC.a[1].RxChrAvail = trueblnr;

			if (SCC.a[1].WR[0] & Bit5
				&& ! (SCC.a[1].WR[0] & (Bit4 | Bit3)))
			{
				/* Int on next Rx char */
				SCC_Interrupt(SCC_B_Rx);
			} else if (SCC.a[1].WR[1] & Bit3
				&& ! (SCC.a[1].WR[1] & Bit4))
			{
				/* Int on first Rx char */
				SCC_Interrupt(SCC_B_Rx);
			} else if (SCC.a[1].WR[1] & Bit4
				&& ! (SCC.a[1].WR[1] & Bit3))
			{
				/* Int on all Rx chars */
				SCC_Interrupt(SCC_B_Rx);
			}
		}
	}
#endif
}
#endif

#if SCC_dolog
LOCALPROC SCC_DbgLogChanStartLine(int chan)
{
	dbglog_StartLine();
	dbglog_writeCStr("SCC chan(");
	if (chan) {
		dbglog_writeCStr("B");
	} else {
		dbglog_writeCStr("A");
	}
	/* dbglog_writeHex(chan); */
	dbglog_writeCStr(")");
}
#endif

LOCALFUNC ui3r SCC_GetRR0(int chan)
{
	/* happens on boot always */

	return 0
#if 0 /* BreakAbort always false */
		| (SCC.a[chan].BreakAbort ? (1 << 7) : 0)
#endif
		| (SCC.a[chan].TxUnderrun ? (1 << 6) : 0)
#if 0 /* CTS always false */
		| (SCC.a[chan].CTS ? (1 << 5) : 0)
#endif
		| (SCC.a[chan].SyncHunt ? (1 << 4) : 0)
#if 0 /* DCD always false */
		| (SCC.a[chan].DCD ? (1 << 3) : 0)
#endif
#if EmLocalTalk
		| (SCC.a[chan].TxBufferEmpty ? (1 << 2) : 0)
#else
		/* otherwise TxBufferEmpty always true */
		| (1 << 2)
#endif
#if 0 /* ZeroCount always false */
		| (SCC.a[chan].ZeroCount ? (1 << 1) : 0)
#endif
#if EmLocalTalk
		/* otherwise RxChrAvail always false */
		| (SCC.a[chan].RxChrAvail ? (1 << 0) : 0)
#endif
		;
}

LOCALFUNC ui3r SCC_GetRR1(int chan)
{
	/* happens in MacCheck */

	ui3r value;
#if ! EmLocalTalk
	UnusedParam(chan);
#endif

	value = Bit2 | Bit1
#if 0 /* AllSent always true */
		| (SCC.a[chan].AllSent ? (1 << 0) : 0)
#else
		| Bit0
#endif
#if 0 /* ParityErr always false */
		| (SCC.a[chan].ParityErr ? (1 << 4) : 0)
#endif
#if 0 /* RxOverrun always false */
		| (SCC.a[chan].RxOverrun ? (1 << 5) : 0)
#endif
#if 0 /* CRCFramingErr always false */
		| (SCC.a[chan].CRCFramingErr ? (1 << 6) : 0)
#endif
#if EmLocalTalk
		/* otherwise EndOfFrame always false */
		| (SCC.a[chan].EndOfFrame ? (1 << 7) : 0)
#endif
		;

	return value;
}

LOCALFUNC ui3r SCC_GetRR2(int chan)
{
	/* happens in MacCheck */
	/* happens in Print to ImageWriter */

	ui3r value = SCC.InterruptVector;

	if (chan != 0) { /* B Channel */
#if 0 /* StatusHiLo always false */
		if (SCC.StatusHiLo) {
			/* Status High */
			value = value
				& (Bit0 | Bit1 | Bit2 | Bit3 | Bit7);

			ReportAbnormalID(0x0705, "Status high/low");
			switch (SCC.SCC_Interrupt_Type) {
				case SCC_A_Rx:
					value |= Bit4 | Bit5;
					break;

				case SCC_A_Rx_Spec:
					value |= Bit4 | Bit5 | Bit6;
					break;

				case SCC_A_Tx_Empty:
					value |= Bit4;
					break;

				case SCC_A_Ext:
					value |= Bit4 | Bit6;
					break;

				case SCC_B_Rx:
					value |= Bit5;
					break;

				case SCC_B_Rx_Spec:
					value |= Bit5 | Bit6;
					break;

				case SCC_B_Tx_Empty:
					value |= 0;
					break;

				case SCC_B_Ext:
					value |= Bit6;
					break;

				default:
					value |= Bit5 | Bit6;
					break;
			}
		} else
#endif
		{
			/* Status Low */
			value = value
				& (Bit0 | Bit4 | Bit5 | Bit6 | Bit7);

			switch (SCC.SCC_Interrupt_Type) {
				case SCC_A_Rx:
					value |= Bit3 | Bit2;
					break;

				case SCC_A_Rx_Spec:
					value |= Bit3 | Bit2 | Bit1;
					break;

				case SCC_A_Tx_Empty:
					value |= Bit3;
					break;

				case SCC_A_Ext:
					value |= Bit3 | Bit1;
					break;

				case SCC_B_Rx:
					value |= Bit2;
					break;

				case SCC_B_Rx_Spec:
					value |= Bit2 | Bit1;
					break;

				case SCC_B_Tx_Empty:
					value |= 0;
					break;

				case SCC_B_Ext:
					value |= Bit1;
					break;

				default:
					value |= Bit2 | Bit1;
					break;
			}
		}

		/* SCC.SCC_Interrupt_Type = 0; */
	}

	return value;
}

LOCALFUNC ui3r SCC_GetRR3(int chan)
{
	ui3r value = 0;

	UnusedParam(chan);
	ReportAbnormalID(0x0706, "RR 3");

#if 0
	if (chan == 0) {
		value = 0
			| (SCC.a[1].TxIP ? (1 << 1) : 0)
			| (SCC.a[0].TxIP ? (1 << 4) : 0)
			;
	} else {
		value = 0;
	}
#endif

	return value;
}

LOCALFUNC ui3r SCC_GetRR8(int chan)
{
	ui3r value = 0;

	/* Receive Buffer */
	/* happens on boot with appletalk on */
	if (SCC.a[chan].RxEnable) {
#if EmLocalTalk
		if (0 != chan) {
			/*
				Check the receive state, handling a complete rx
				if necessary
			*/
			value = SCC.a[1].RxBuff;
			SCC.a[1].FirstChar = falseblnr;
			SCC_RxBuffAdvance();
		} else {
			value = 0x7E;
		}
#else
		/* Rx Enable */
#if (CurEmMd >= kEmMd_SE) && (CurEmMd <= kEmMd_IIx)
		/* don't report */
#else
		ReportAbnormalID(0x0707, "read rr8 when RxEnable");
#endif

		/* Input 1 byte from Modem Port/Printer into Data */
#endif
	} else {
		/* happens on boot with appletalk on */
	}

	return value;
}

LOCALFUNC ui3r SCC_GetRR10(int chan)
{
	/* happens on boot with appletalk on */

	ui3r value = 0;
	UnusedParam(chan);

#if 0 && EmLocalTalk
	value = 2;
#endif

	return value;
}

LOCALFUNC ui3r SCC_GetRR12(int chan)
{
	ui3r value = 0;

#if ! SCC_TrackMore
	UnusedParam(chan);
#endif
	ReportAbnormalID(0x0708, "RR 12");

#if SCC_TrackMore /* don't care about Baud */
	value = SCC.a[chan].BaudLo;
#endif

	return value;
}

LOCALFUNC ui3r SCC_GetRR13(int chan)
{
	ui3r value = 0;

#if ! SCC_TrackMore
	UnusedParam(chan);
#endif
	ReportAbnormalID(0x0709, "RR 13");

#if SCC_TrackMore /* don't care about Baud */
	value = SCC.a[chan].BaudHi;
#endif

	return value;
}

LOCALFUNC ui3r SCC_GetRR15(int chan)
{
	ui3r value = 0;

	UnusedParam(chan);
	ReportAbnormalID(0x070A, "RR 15");

#if 0
	value = 0
#if 0 /* don't care about DCD_IE, always true */
		| (SCC.a[chan].DCD_IE ? Bit3 : 0)
#else
		| Bit3
#endif
#if 0 /* SyncHuntIE usually false */
		| (SCC.a[chan].SyncHuntIE ? Bit4 : 0)
#endif
#if SCC_TrackMore /* don't care about CTS_IE */
		| (SCC.a[chan].CTS_IE ? Bit5 : 0)
#endif
#if SCC_TrackMore /* don't care about BreakAbortIE */
		| (SCC.a[chan].BreakAbortIE ? Bit7 : 0)
#endif
		;
#endif

	return value;
}

#if SCC_dolog
LOCALPROC SCC_DbgLogChanCmnd(int chan, char *s)
{
	SCC_DbgLogChanStartLine(chan);
	dbglog_writeCStr(" ");
	dbglog_writeCStr(s);
	dbglog_writeReturn();
}
#endif

#if SCC_dolog
LOCALPROC SCC_DbgLogChanChngBit(int chan, char *s, blnr v)
{
	SCC_DbgLogChanStartLine(chan);
	dbglog_writeCStr(" ");
	dbglog_writeCStr(s);
	dbglog_writeCStr(" <- ");
	if (v) {
		dbglog_writeCStr("1");
	} else {
		dbglog_writeCStr("0");
	}
	dbglog_writeReturn();
}
#endif

LOCALPROC SCC_PutWR0(ui3r Data, int chan)
/*
	"CRC initialize, initialization commands for the various modes,
	Register Pointers"
*/
{
	switch ((Data >> 6) & 3) {
		case 1:
			ReportAbnormalID(0x070B, "Reset Rx CRC Checker");
			break;
		case 2:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "Reset Tx CRC Generator");
#endif
			/* happens on boot with appletalk on */
			break;
		case 3:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan,
				"Reset Tx Underrun/EOM Latch");
#endif
			/* happens on boot with appletalk on */
#if EmLocalTalk
			/*
				This is the indication we are done transmitting
				data for the current packet.
			*/
			process_transmit();
#endif
#if 0 /* It seems to work better without this */
			if (SCC.a[chan].TxEnable) {
				/* Tx Enabled */
				SCC.a[chan].TxUnderrun = falseblnr;

				if (SCC.a[chan].WR[10] & Bit2) {
					/* Abort/Flag on Underrun */
					/* Send Abort */
					SCC.a[chan].TxUnderrun = trueblnr;
#if 0 /* TxBufferEmpty always true */
					SCC.a[chan].TxBufferEmpty = trueblnr;
#endif

					/* Send Flag */
				}
			}
#endif
			break;
		case 0:
		default:
			/* Null Code */
			break;
	}
	SCC.PointerBits = Data & 0x07;
	switch ((Data >> 3) & 7) {
		case 1: /* Point High */
			SCC.PointerBits |= 8;
			break;
		case 2:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "Reset Ext/Status Ints");
#endif
			/* happens on boot always */
			SCC.a[chan].SyncHunt = falseblnr;
#if 0 /* only in sync mode */
			SCC.a[chan].TxUnderrun = falseblnr;
#endif
#if 0 /* ZeroCount always false */
			SCC.a[chan].ZeroCount = falseblnr;
#endif
#if 0 /* BreakAbort always false */
			SCC.a[chan].BreakAbort = falseblnr;
#endif
			break;
		case 3:
			ReportAbnormalID(0x070C, "Send Abort (SDLC)");
#if EmLocalTalk
			SCC.a[chan].TxBufferEmpty = trueblnr;
#endif
#if 0
			SCC.a[chan].TxUnderrun = trueblnr;
#if 0 /* TxBufferEmpty always true */
			SCC.a[chan].TxBufferEmpty = trueblnr;
#endif
#endif
			break;
		case 4:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan,
				"Enable Int on next Rx char");
#endif
#if EmLocalTalk || SCC_TrackMore
			SCC.a[chan].FirstChar = trueblnr;
#endif
			/* happens in MacCheck */
			break;
		case 5:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "Reset Tx Int Pending");
#endif
			/* happens in MacCheck */
			/* happens in Print to ImageWriter */
			SCC.a[chan].TxIP = falseblnr;
			CheckSCCInterruptFlag();
			break;
		case 6:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "Error Reset");
#endif
			/* happens on boot with appletalk on */
#if EmLocalTalk
			SCC.a[chan].EndOfFrame = falseblnr;
#endif
#if 0 /* ParityErr always false */
			SCC.a[chan].ParityErr = falseblnr;
#endif
#if 0 /* RxOverrun always false */
			SCC.a[chan].RxOverrun = falseblnr;
#endif
#if 0 /* CRCFramingErr always false */
			SCC.a[chan].CRCFramingErr = falseblnr;
#endif
			break;
		case 7:
			/* happens in "Network Watch" program (Cayman Systems) */
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "Reset Highest IUS");
#endif
			break;
		case 0:
		default:
			/* Null Code */
			break;
	}
}

LOCALPROC SCC_PutWR1(ui3r Data, int chan)
/*
	"Transmit/Receive interrupt and data transfer mode definition"
*/
{
#if EmLocalTalk || SCC_TrackMore
	{
		blnr NewExtIE = (Data & Bit0) != 0;
		if (SCC.a[chan].ExtIE != NewExtIE) {
			SCC.a[chan].ExtIE = NewExtIE;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan, "EXT INT Enable",
				NewExtIE);
#endif
			/*
				set to 1 on start up, set to 0 in MacCheck
				and in Print to ImageWriter
			*/
		}
	}
#endif

	{
		blnr NewTxIE = (Data & Bit1) != 0;
		if (SCC.a[chan].TxIE != NewTxIE) {
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan, "Tx Int Enable",
				NewTxIE);
#endif
			/* happens in MacCheck */
			/* happens in Print to ImageWriter */
			SCC.a[chan].TxIE = NewTxIE;
			CheckSCCInterruptFlag();
		}
	}

#if SCC_TrackMore
	{
		blnr NewPrtySpclCond = (Data & Bit2) != 0;
		if (SCC.a[chan].PrtySpclCond != NewPrtySpclCond) {
			SCC.a[chan].PrtySpclCond = NewPrtySpclCond;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Parity is special condition", NewPrtySpclCond);
#endif
			/*
				set to 1 in MacCheck
				and in Print to ImageWriter
			*/
		}
	}
#endif

#if EmLocalTalk || SCC_TrackMore
	{
		ui3r NewRxIntMode = (Data >> 3) & 3;
		if (SCC.a[chan].RxIntMode != NewRxIntMode) {
			SCC.a[chan].RxIntMode = NewRxIntMode;

			switch (NewRxIntMode) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan, "Rx INT Disable");
#endif
					/* happens on boot always */
					break;
				case 1:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Rx INT on 1st char"
						" or special condition");
#endif
					SCC.a[chan].FirstChar = trueblnr;
					/* happens on boot with appletalk on */
					break;
				case 2:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"INT on all Rx char"
						" or special condition");
#endif
					/* happens in MacCheck */
					/* happens in Print to ImageWriter */
					break;
				case 3:
					ReportAbnormalID(0x070D,
						"Rx INT on special condition only");
					break;
			}
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewWaitRqstRT = (Data & Bit5) != 0;
		if (SCC.a[chan].WaitRqstRT != NewWaitRqstRT) {
			SCC.a[chan].WaitRqstRT = NewWaitRqstRT;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Wait/DMA request on receive/transmit",
				NewWaitRqstRT);
#endif
			/* happens in MacCheck */
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewWaitRqstSlct = (Data & Bit6) != 0;
		if (SCC.a[chan].WaitRqstSlct != NewWaitRqstSlct) {
			SCC.a[chan].WaitRqstSlct = NewWaitRqstSlct;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Wait/DMA request function", NewWaitRqstSlct);
#endif
			/* happens in MacCheck */
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewWaitRqstEnbl = (Data & Bit7) != 0;
		if (SCC.a[chan].WaitRqstEnbl != NewWaitRqstEnbl) {
			SCC.a[chan].WaitRqstEnbl = NewWaitRqstEnbl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Wait/DMA request enable", NewWaitRqstEnbl);
#endif
			/* happens in MacCheck */
		}
	}
#endif
}

LOCALPROC SCC_PutWR2(ui3r Data, int chan)
/* "Interrupt Vector (accessed through either channel)" */
{
	/*
		Only 1 interrupt vector for the SCC, which
		is stored in channels A and B. B is modified
		when read.
	*/

	/* happens on boot always */

#if ! SCC_dolog
	UnusedParam(chan);
#endif

	if (SCC.InterruptVector != Data) {
#if SCC_dolog
		SCC_DbgLogChanStartLine(chan);
		dbglog_writeCStr(" InterruptVector <- ");
		dbglog_writeHex(Data);
		dbglog_writeReturn();
#endif
		SCC.InterruptVector = Data;
	}
	if ((Data & Bit0) != 0) { /* interrupt vector 0 */
		ReportAbnormalID(0x070E, "interrupt vector 0");
	}
	if ((Data & Bit1) != 0) { /* interrupt vector 1 */
		ReportAbnormalID(0x070F, "interrupt vector 1");
	}
	if ((Data & Bit2) != 0) { /* interrupt vector 2 */
		ReportAbnormalID(0x0710, "interrupt vector 2");
	}
	if ((Data & Bit3) != 0) { /* interrupt vector 3 */
		ReportAbnormalID(0x0711, "interrupt vector 3");
	}
	if ((Data & Bit4) != 0) { /* interrupt vector 4 */
		/* happens on boot with appletalk on */
	}
	if ((Data & Bit5) != 0) { /* interrupt vector 5 */
		/* happens on boot with appletalk on */
	}
	if ((Data & Bit6) != 0) { /* interrupt vector 6 */
		ReportAbnormalID(0x0712, "interrupt vector 6");
	}
	if ((Data & Bit7) != 0) { /* interrupt vector 7 */
		ReportAbnormalID(0x0713, "interrupt vector 7");
	}
}

LOCALPROC SCC_PutWR3(ui3r Data, int chan)
/* "Receive parameters and control" */
{
#if SCC_TrackMore
	{
		ui3r NewRBitsPerChar = (Data >> 6) & 3;
		if (SCC.a[chan].RBitsPerChar != NewRBitsPerChar) {
			SCC.a[chan].RBitsPerChar = NewRBitsPerChar;

			switch (NewRBitsPerChar) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Rx Bits/Character <- 5");
#endif
					break;
				case 1:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Rx Bits/Character <- 7");
#endif
					break;
				case 2:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Rx Bits/Character <- 6");
#endif
					break;
				case 3:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Rx Bits/Character <- 8");
#endif
					break;
			}
		}
	}
#endif

	if ((Data & Bit5) != 0) { /* Auto Enables */
		/*
			use DCD input as receiver enable,
			and set RTS output when transmit buffer empty
		*/
		ReportAbnormalID(0x0714, "Auto Enables");
	}

	if ((Data & Bit4) != 0) { /* Enter Hunt Mode */
#if SCC_dolog
		SCC_DbgLogChanCmnd(chan, "Enter Hunt Mode");
#endif
		/* happens on boot with appletalk on */
		if (! (SCC.a[chan].SyncHunt)) {
			SCC.a[chan].SyncHunt = trueblnr;

#if 0 /* SyncHuntIE usually false */
			if (SCC.a[chan].SyncHuntIE) {
				SCC_Interrupt((chan == 0)
					? SCC_A_Ext
					: SCC_B_Ext);
			}
#endif
		}
	}

#if SCC_TrackMore
	{
		blnr NewRxCRCEnbl = (Data & Bit3) != 0;
		if (SCC.a[chan].RxCRCEnbl != NewRxCRCEnbl) {
			SCC.a[chan].RxCRCEnbl = NewRxCRCEnbl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Rx CRC Enable", NewRxCRCEnbl);
#endif
			/* happens on boot with appletalk on */
		}
	}
#endif

#if EmLocalTalk || SCC_TrackMore
	{
		blnr NewAddrSrchMd = (Data & Bit2) != 0;
		if (SCC.a[chan].AddrSrchMd != NewAddrSrchMd) {
			SCC.a[chan].AddrSrchMd = NewAddrSrchMd;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Addr Search Mode (SDLC)", NewAddrSrchMd);
#endif
			/* happens on boot with appletalk on */
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewSyncChrLdInhb = (Data & Bit1) != 0;
		if (SCC.a[chan].SyncChrLdInhb != NewSyncChrLdInhb) {
			SCC.a[chan].SyncChrLdInhb = NewSyncChrLdInhb;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Sync Char Load Inhibit", NewSyncChrLdInhb);
#endif
			/* happens on boot with appletalk on */
		}
	}
#endif

	{
		blnr NewRxEnable = (Data & Bit0) != 0;
		if (SCC.a[chan].RxEnable != NewRxEnable) {
			SCC.a[chan].RxEnable = NewRxEnable;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Rx Enable", NewRxEnable);
#endif
			/* true on boot with appletalk on */
			/* true on Print to ImageWriter */

#if EmLocalTalk
			if (! NewRxEnable) {
#if SCC_dolog
				if ((0 != chan) && (nullpr != LT_RxBuffer)) {
					dbglog_WriteNote("SCC abandon packet");
				}
#endif

				/*
					Go back into the idle state if we were
					waiting for EOF
				*/
				SCC.a[chan].EndOfFrame = falseblnr;
				SCC.a[chan].RxChrAvail = falseblnr;
				SCC.a[chan].SyncHunt = trueblnr;
			} else {
				/* look for a packet */
				LocalTalkTick();
			}
#endif
		}
	}
}

LOCALPROC SCC_PutWR4(ui3r Data, int chan)
/* "Transmit/Receive miscellaneous parameters and modes" */
{
#if ! (EmLocalTalk || SCC_TrackMore)
	UnusedParam(Data);
	UnusedParam(chan);
#endif

#if SCC_TrackMore
	{
		blnr NewPrtyEnable = (Data & Bit0) != 0;
		if (SCC.a[chan].PrtyEnable != NewPrtyEnable) {
			SCC.a[chan].PrtyEnable = NewPrtyEnable;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Parity Enable", NewPrtyEnable);
#endif
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewPrtyEven = (Data & Bit1) != 0;
		if (SCC.a[chan].PrtyEven != NewPrtyEven) {
			SCC.a[chan].PrtyEven = NewPrtyEven;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Parity Enable", NewPrtyEven);
#endif
		}
	}
#endif

#if SCC_TrackMore
	{
		ui3r NewStopBits = (Data >> 2) & 3;
		if (SCC.a[chan].StopBits != NewStopBits) {
			SCC.a[chan].StopBits = NewStopBits;

			/* SCC_SetStopBits(chan, NewStopBits); */
			switch (NewStopBits) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Sync Modes Enable");
#endif
					break;
				case 1:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan, "1 Stop Bit");
#endif
					break;
				case 2:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan, "1 1/2 Stop Bits");
#endif
					break;
				case 3:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan, "2 Stop Bits");
#endif
					break;
			}
		}
	}
#endif

#if EmLocalTalk || SCC_TrackMore
	{
		ui3r NewSyncMode = (Data >> 4) & 3;
		if (SCC.a[chan].SyncMode != NewSyncMode) {
			SCC.a[chan].SyncMode = NewSyncMode;

			switch (NewSyncMode) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan, "8 bit sync char");
#endif
					/* happens on boot always */
					break;
				case 1:
					ReportAbnormalID(0x0715, "16 bit sync char");
					break;
				case 2:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan, "SDLC MODE");
#endif
					/* happens on boot with appletalk on */
#if EmLocalTalk
					SCC.a[chan].TxBufferEmpty = trueblnr;
#endif
					break;
				case 3:
					ReportAbnormalID(0x0716, "External sync mode");
					break;
			}
		}
	}
#endif

#if SCC_TrackMore
	{
		ui3r NewClockRate = (Data >> 6) & 3;
		if (SCC.a[chan].ClockRate != NewClockRate) {
			SCC.a[chan].ClockRate = NewClockRate;

			switch (NewClockRate) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Clock Rate <- X1");
#endif
					/* happens on boot with appletalk on */
					break;
				case 1:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Clock Rate <- X16");
#endif
					/* happens on boot always */
					break;
				case 2:
					ReportAbnormalID(0x0717, "Clock Rate <- X32");
					break;
				case 3:
					ReportAbnormalID(0x0718, "Clock Rate <- X64");
					break;
			}
		}
	}
#endif
}

LOCALPROC SCC_PutWR5(ui3r Data, int chan)
/* "Transmit parameters and controls" */
{
	/* happens on boot with appletalk on */
	/* happens in Print to ImageWriter */

#if SCC_TrackMore
	{
		blnr NewTxCRCEnbl = (Data & Bit0) != 0;
		if (SCC.a[chan].TxCRCEnbl != NewTxCRCEnbl) {
			SCC.a[chan].TxCRCEnbl = NewTxCRCEnbl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Tx CRC Enable", NewTxCRCEnbl);
#endif
			/* both values on boot with appletalk on */
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewRTSctrl = (Data & Bit1) != 0;
		if (SCC.a[chan].RTSctrl != NewRTSctrl) {
			SCC.a[chan].RTSctrl = NewRTSctrl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"RTS Control", NewRTSctrl);
#endif
			/* both values on boot with appletalk on */
			/*
				value of Request To Send output pin, when
				Auto Enable is off
			*/
		}
	}
#endif

	if ((Data & Bit2) != 0) { /* SDLC/CRC-16 */
		ReportAbnormalID(0x0719, "SDLC/CRC-16");
	}

	{
		blnr NewTxEnable = (Data & Bit3) != 0;
		if (SCC.a[chan].TxEnable != NewTxEnable) {
			SCC.a[chan].TxEnable = NewTxEnable;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Tx Enable", NewTxEnable);
#endif

			if (NewTxEnable) {
				/* happens on boot with appletalk on */
				/* happens in Print to ImageWriter */
#if EmLocalTalk
				LT_TxBuffSz = 0;
#endif
			} else {
#if EmLocalTalk
				SCC.a[chan].TxBufferEmpty = trueblnr;
#endif
			}
		}
	}

#if SCC_TrackMore
	{
		blnr NewSndBrkCtrl = (Data & Bit4) != 0;
		if (SCC.a[chan].SndBrkCtrl != NewSndBrkCtrl) {
			SCC.a[chan].SndBrkCtrl = NewSndBrkCtrl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Send Break Control", NewSndBrkCtrl);
#endif
			/* true in Print to LaserWriter 300 */
		}
	}
#endif

#if SCC_TrackMore
	{
		ui3r NewTBitsPerChar = (Data >> 5) & 3;
		if (SCC.a[chan].TBitsPerChar != NewTBitsPerChar) {
			SCC.a[chan].TBitsPerChar = NewTBitsPerChar;

			switch (NewTBitsPerChar) {
				case 0:
					ReportAbnormalID(0x071A, "Tx Bits/Character <- 5");
					break;
				case 1:
					ReportAbnormalID(0x071B, "Tx Bits/Character <- 7");
					break;
				case 2:
					ReportAbnormalID(0x071C, "Tx Bits/Character <- 6");
					break;
				case 3:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Tx Bits/Character <- 8");
#endif
					/* happens on boot with appletalk on */
					/* happens in Print to ImageWriter */
					break;
			}
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewDTRctrl = (Data & Bit7) != 0;
		if (SCC.a[chan].DTRctrl != NewDTRctrl) {
			SCC.a[chan].DTRctrl = NewDTRctrl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"Data Terminal Ready Control", NewDTRctrl);
#endif
			/* zero happens in MacCheck */
			/*
				value of Data Terminal Ready output pin,
				when WR14 D2 = 0 (DTR/request function)
			*/
		}
	}
#endif
}

LOCALPROC SCC_PutWR6(ui3r Data, int chan)
/* "Sync characters or SDLC address field" */
{
	/* happens on boot with appletalk on */

#if ! (EmLocalTalk || SCC_dolog)
	UnusedParam(Data);
#endif
#if ! SCC_dolog
	UnusedParam(chan);
#endif

#if SCC_dolog
	SCC_DbgLogChanStartLine(chan);
	dbglog_writeCStr(" Sync Char <- ");
	dbglog_writeHex(Data);
	dbglog_writeReturn();
#endif

#if EmLocalTalk
	if (0 != Data) {
		my_node_address = Data;
	}
#endif
}

LOCALPROC SCC_PutWR7(ui3r Data, int chan)
/* "Sync character or SDLC flag" */
{
	/* happens on boot with appletalk on */

#if ! SCC_TrackMore
	UnusedParam(Data);
	UnusedParam(chan);
#endif

#if SCC_TrackMore
	if (2 == SCC.a[chan].SyncMode) {
		if (0x7E != Data) {
			ReportAbnormalID(0x071D,
				"unexpect flag character for SDLC");
		}
	} else {
		ReportAbnormalID(0x071E, "WR7 and not SDLC");
	}
#endif
}

LOCALPROC SCC_PutWR8(ui3r Data, int chan)
/* "Transmit Buffer" */
{
	/* happens on boot with appletalk on */
	/* happens in Print to ImageWriter */

#if ! (EmLocalTalk || SCC_dolog)
	UnusedParam(Data);
#endif

#if SCC_dolog
	SCC_DbgLogChanStartLine(chan);
	dbglog_writeCStr(" Transmit Buffer");
	dbglog_writeCStr(" <- ");
	dbglog_writeHex(Data);
	dbglog_writeCStr(" '");
	dbglog_writeMacChar(Data);
	dbglog_writeCStr("'");
	dbglog_writeReturn();
#endif

	if (SCC.a[chan].TxEnable) { /* Tx Enable */
		/* Output (Data) to Modem(B) or Printer(A) Port */

		/* happens on boot with appletalk on */
#if EmLocalTalk
		if (chan != 0) {
			SCC_TxBuffPut(Data);
		}
#else
#if 0 /* TxBufferEmpty always true */
		SCC.a[chan].TxBufferEmpty = trueblnr;
#endif
		SCC.a[chan].TxUnderrun = trueblnr; /* underrun ? */
#endif

		SCC.a[chan].TxIP = trueblnr;
		CheckSCCInterruptFlag();
	} else {
		ReportAbnormalID(0x071F,
			"write when Transmit Buffer not Enabled");
#if 0 /* TxBufferEmpty always true */
		SCC.a[chan].TxBufferEmpty = falseblnr;
#endif
	}
}

LOCALPROC SCC_PutWR9(ui3r Data, int chan)
/*
	"Master interrupt control and reset
	(accessed through either channel)"
*/
{
	/* Only 1 WR9 in the SCC */

	UnusedParam(chan);

	if ((Data & Bit0) != 0) { /* VIS */
		ReportAbnormalID(0x0720, "VIS");
	}

#if SCC_TrackMore
	{
		blnr NewNoVectorSlct = (Data & Bit1) != 0;
		if (SCC.NoVectorSlct != NewNoVectorSlct) {
			SCC.NoVectorSlct = NewNoVectorSlct;
#if SCC_dolog
			dbglog_WriteSetBool("SCC No Vector select",
				NewNoVectorSlct);
#endif
			/* has both values on boot always */
		}
	}
#endif

	if ((Data & Bit2) != 0) { /* DLC */
		ReportAbnormalID(0x0723, "DLC");
	}

	{
		blnr NewMIE = (Data & Bit3) != 0;
			/* has both values on boot always */
		if (SCC.MIE != NewMIE) {
			SCC.MIE = NewMIE;
#if SCC_dolog
			dbglog_WriteSetBool("SCC Master Interrupt Enable",
				NewMIE);
#endif
			CheckSCCInterruptFlag();
		}
	}

#if 0 /* StatusHiLo always false */
	SCC.StatusHiLo = (Data & Bit4) != 0;
#else
	if ((Data & Bit4) != 0) { /* Status high/low */
		ReportAbnormalID(0x0724, "Status high/low");
	}
#endif
	if ((Data & Bit5) != 0) { /* WR9 b5 should be 0 */
		ReportAbnormalID(0x0725, "WR9 b5 should be 0");
	}

	switch ((Data >> 6) & 3) {
		case 1:
#if SCC_dolog
			SCC_DbgLogChanCmnd(1, "Channel Reset");
#endif
			/* happens on boot always */
			SCC_ResetChannel(1);
			CheckSCCInterruptFlag();
			break;
		case 2:
#if SCC_dolog
			SCC_DbgLogChanCmnd(0, "Channel Reset");
#endif
			/* happens on boot always */
			SCC_ResetChannel(0);
			CheckSCCInterruptFlag();
			break;
		case 3:
#if SCC_dolog
			dbglog_WriteNote("SCC Force Hardware Reset");
#endif
#if (CurEmMd >= kEmMd_SE) && (CurEmMd <= kEmMd_IIx)
			/* don't report */
#else
			ReportAbnormalID(0x0726, "SCC_Reset");
#endif
			SCC_Reset();
			CheckSCCInterruptFlag();
			break;
		case 0: /* No Reset */
		default:
			break;
	}
}

LOCALPROC SCC_PutWR10(ui3r Data, int chan)
/* "Miscellaneous transmitter/receiver control bits" */
{
	/* happens on boot with appletalk on */
	/* happens in Print to ImageWriter */

#if ! SCC_TrackMore
	UnusedParam(chan);
#endif

	if ((Data & Bit0) != 0) { /* 6 bit/8 bit sync */
		ReportAbnormalID(0x0727, "6 bit/8 bit sync");
	}
	if ((Data & Bit1) != 0) { /* loop mode */
		ReportAbnormalID(0x0728, "loop mode");
	}
	if ((Data & Bit2) != 0) { /* abort/flag on underrun */
		ReportAbnormalID(0x0729, "abort/flag on underrun");
	}
	if ((Data & Bit3) != 0) { /* mark/flag idle */
		ReportAbnormalID(0x072A, "mark/flag idle");
	}
	if ((Data & Bit4) != 0) { /* go active on poll */
		ReportAbnormalID(0x072B, "go active on poll");
	}

#if SCC_TrackMore
	{
		ui3r NewDataEncoding = (Data >> 5) & 3;
		if (SCC.a[chan].DataEncoding != NewDataEncoding) {
			SCC.a[chan].DataEncoding = NewDataEncoding;

			switch (NewDataEncoding) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Data Encoding <- NRZ");
#endif
					/* happens in MacCheck */
					/* happens in Print to ImageWriter */
					break;
				case 1:
					ReportAbnormalID(0x072C, "Data Encoding <- NRZI");
					break;
				case 2:
					ReportAbnormalID(0x072D, "Data Encoding <- FM1");
					break;
				case 3:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"Data Encoding <- FM0");
#endif
					/* happens on boot with appletalk on */
					break;
			}
		}
	}
#endif

#if SCC_TrackMore
	{
		blnr NewCRCPreset = (Data & Bit7) != 0;
		if (SCC.a[chan].CRCPreset != NewCRCPreset) {
			SCC.a[chan].CRCPreset = NewCRCPreset;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"CRC preset I/O", NewCRCPreset);
#endif
			/* false happens in MacCheck */
			/* true happens in Print to ImageWriter */
		}
	}
#endif
}

LOCALPROC SCC_PutWR11(ui3r Data, int chan)
/* "Clock mode control" */
{
	/* happens on boot with appletalk on */
	/* happens in Print to ImageWriter */
	/* happens in MacCheck */

#if ! SCC_TrackMore
	UnusedParam(chan);
#endif

#if SCC_TrackMore
	/* Transmit External Control Selection */
	{
		ui3r NewTRxCsrc = Data & 3;
		if (SCC.a[chan].TRxCsrc != NewTRxCsrc) {
			SCC.a[chan].TRxCsrc = NewTRxCsrc;

			switch (NewTRxCsrc) {
				case 0:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"TRxC OUT = XTAL output");
#endif
					/* happens on boot with appletalk on */
					/* happens in Print to ImageWriter */
					/* happens in MacCheck */
					break;
				case 1:
					ReportAbnormalID(0x072E,
						"TRxC OUT = transmit clock");
					break;
				case 2:
					ReportAbnormalID(0x072F,
						"TRxC OUT = BR generator output");
					break;
				case 3:
					ReportAbnormalID(0x0730, "TRxC OUT = dpll output");
					break;
			}
		}
	}
#endif

	if ((Data & Bit2) != 0) {
		ReportAbnormalID(0x0731, "TRxC O/I");
	}

#if SCC_TrackMore
	{
		ui3r NewTClkSlct = (Data >> 3) & 3;
		if (SCC.a[chan].TClkSlct != NewTClkSlct) {
			SCC.a[chan].TClkSlct = NewTClkSlct;

			switch (NewTClkSlct) {
				case 0:
					ReportAbnormalID(0x0732,
						"transmit clock = RTxC pin");
					break;
				case 1:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"transmit clock = TRxC pin");
#endif
					/* happens in Print to LaserWriter 300 */
					break;
				case 2:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"transmit clock = BR generator output");
#endif
					/* happens on boot with appletalk on */
					/* happens in Print to ImageWriter */
					/* happens in MacCheck */
					break;
				case 3:
					ReportAbnormalID(0x0733,
						"transmit clock = dpll output");
					break;
			}
		}
	}
#endif

#if SCC_TrackMore
	{
		ui3r NewRClkSlct = (Data >> 5) & 3;
		if (SCC.a[chan].RClkSlct != NewRClkSlct) {
			SCC.a[chan].RClkSlct = NewRClkSlct;

			switch (NewRClkSlct) {
				case 0:
					ReportAbnormalID(0x0734,
						"receive clock = RTxC pin");
					break;
				case 1:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"receive clock = TRxC pin");
#endif
					/* happens in Print to LaserWriter 300 */
					break;
				case 2:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"receive clock = BR generator output");
#endif
					/* happens in MacCheck */
					/* happens in Print to ImageWriter */
					break;
				case 3:
#if SCC_dolog
					SCC_DbgLogChanCmnd(chan,
						"receive clock = dpll output");
#endif
					/* happens on boot with appletalk on */
					break;
			}
		}
	}
#endif

	if ((Data & Bit7) != 0) {
		ReportAbnormalID(0x0735, "RTxC XTAL/NO XTAL");
	}
}

LOCALPROC SCC_PutWR12(ui3r Data, int chan)
/* "Lower byte of baud rate generator time constant" */
{
	/* happens on boot with appletalk on */
	/* happens in Print to ImageWriter */

#if ! SCC_TrackMore
	UnusedParam(Data);
	UnusedParam(chan);
#endif

#if SCC_TrackMore /* don't care about Baud */
	if (SCC.a[chan].BaudLo != Data) {
		SCC.a[chan].BaudLo = Data;

#if SCC_dolog
		SCC_DbgLogChanStartLine(chan);
		dbglog_writeCStr(" BaudLo <- ");
		dbglog_writeHex(Data);
		dbglog_writeReturn();
#endif
	}
#endif

#if 0
	SCC_SetBaud(chan,
		SCC.a[chan].BaudLo + (SCC.a[chan].BaudHi << 8));
		/* 380: BaudRate = 300   */
		/*  94: BaudRate = 1200  */
		/*  46: BaudRate = 2400  */
		/*  22: BaudRate = 4800  */
		/*  10: BaudRate = 9600  */
		/*   4: BaudRate = 19200 */
		/*   1: BaudRate = 38400 */
		/*   0: BaudRate = 57600 */
#endif
}

LOCALPROC SCC_PutWR13(ui3r Data, int chan)
/* "Upper byte of baud rate generator time constant" */
{
	/* happens on boot with appletalk on */
	/* happens in Print to ImageWriter */

#if ! SCC_TrackMore
	UnusedParam(Data);
	UnusedParam(chan);
#endif

#if SCC_TrackMore /* don't care about Baud */
	if (SCC.a[chan].BaudHi != Data) {
		SCC.a[chan].BaudHi = Data;

#if SCC_dolog
		SCC_DbgLogChanStartLine(chan);
		dbglog_writeCStr(" BaudHi <- ");
		dbglog_writeHex(Data);
		dbglog_writeReturn();
#endif
	}
#endif

#if 0
	SCC_SetBaud(chan,
		SCC.a[chan].BaudLo + (SCC.a[chan].BaudHi << 8));
#endif
}

LOCALPROC SCC_PutWR14(ui3r Data, int chan)
/* "Miscellaneous control bits" */
{
	/* happens on boot with appletalk on */

#if ! (SCC_TrackMore || SCC_dolog)
	UnusedParam(chan);
#endif

#if SCC_TrackMore
	{
		blnr NewBRGEnbl = (Data & Bit0) != 0;
		if (SCC.a[chan].BRGEnbl != NewBRGEnbl) {
			SCC.a[chan].BRGEnbl = NewBRGEnbl;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"BR generator enable", NewBRGEnbl);
#endif
			/* both values on boot with appletalk on */
			/* true happens in Print to ImageWriter */
		}
	}
#endif

	if ((Data & Bit1) != 0) { /* BR generator source */
		ReportAbnormalID(0x0736, "BR generator source");
	}
	if ((Data & Bit2) != 0) { /* DTR/request function */
		ReportAbnormalID(0x0737, "DTR/request function");
	}
	if ((Data & Bit3) != 0) { /* auto echo */
		ReportAbnormalID(0x0738, "auto echo");
	}
	if ((Data & Bit4) != 0) { /* local loopback */
		ReportAbnormalID(0x0739, "local loopback");
	}

	switch ((Data >> 5) & 7) {
		case 1:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "enter search mode");
#endif
			/* happens on boot with appletalk on */
			break;
		case 2:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "reset missing clock");
#endif
			/* happens on boot with appletalk on */
			/*
				should clear Bit 6 and Bit 7 of RR[10], but
				since these are never set, don't need
				to do anything
			*/
			break;
		case 3:
			ReportAbnormalID(0x073A, "disable dpll");
			/*
				should clear Bit 6 and Bit 7 of RR[10], but
				since these are never set, don't need
				to do anything
			*/
			break;
		case 4:
			ReportAbnormalID(0x073B, "set source = br generator");
			break;
		case 5:
			ReportAbnormalID(0x073C, "set source = RTxC");
			break;
		case 6:
#if SCC_dolog
			SCC_DbgLogChanCmnd(chan, "set FM mode");
#endif
			/* happens on boot with appletalk on */
			break;
		case 7:
			ReportAbnormalID(0x073D, "set NRZI mode");
			break;
		case 0: /* No Reset */
		default:
			break;
	}
}

LOCALPROC SCC_PutWR15(ui3r Data, int chan)
/* "External/Status interrupt control" */
{
	/* happens on boot always */

#if ! SCC_TrackMore
	UnusedParam(chan);
#endif

	if ((Data & Bit0) != 0) { /* WR15 b0 should be 0 */
		ReportAbnormalID(0x073E, "WR15 b0 should be 0");
	}
	if ((Data & Bit1) != 0) { /* zero count IE */
		ReportAbnormalID(0x073F, "zero count IE");
	}
	if ((Data & Bit2) != 0) { /* WR15 b2 should be 0 */
		ReportAbnormalID(0x0740, "WR15 b2 should be 0");
	}

#if 0 /* don't care about DCD_IE, always true */
	SCC.a[chan].DCD_IE = (Data & Bit3) != 0;
#else
	if ((Data & Bit3) == 0) { /* DCD_IE */
#if (CurEmMd >= kEmMd_SE) && (CurEmMd <= kEmMd_IIx)
		/* don't report */
#else
		ReportAbnormalID(0x0741, "not DCD IE");
#endif
	}
#endif

#if 0 /* SyncHuntIE usually false */
	SCC.a[chan].SyncHuntIE = (Data & Bit4) != 0;
#else
	if ((Data & Bit4) != 0) {
		/* SYNC/HUNT IE */
		ReportAbnormalID(0x0742, "SYNC/HUNT IE");
	}
#endif

#if SCC_TrackMore /* don't care about CTS_IE */
	{
		blnr NewCTS_IE = (Data & Bit5) != 0;
		if (SCC.a[chan].CTS_IE != NewCTS_IE) {
			SCC.a[chan].CTS_IE = NewCTS_IE;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"CTS IE", NewCTS_IE);
#endif
			/* happens in MacCheck */
			/* happens in Print to ImageWriter */
		}
	}
#endif

	if ((Data & Bit6) != 0) { /* Tx underrun/EOM IE */
		ReportAbnormalID(0x0743, "Tx underrun/EOM IE");
	}

#if SCC_TrackMore
	{
		blnr NewBreakAbortIE = (Data & Bit7) != 0;
		if (SCC.a[chan].BreakAbortIE != NewBreakAbortIE) {
			SCC.a[chan].BreakAbortIE = NewBreakAbortIE;
#if SCC_dolog
			SCC_DbgLogChanChngBit(chan,
				"BreakAbort IE", NewBreakAbortIE);
#endif
			/* happens in MacCheck */
			/* happens in Print to ImageWriter */
		}
	}
#endif
}

LOCALFUNC ui3r SCC_GetReg(int chan, ui3r SCC_Reg)
{
	ui3r value;

	switch (SCC_Reg) {
		case 0:
			value = SCC_GetRR0(chan);
			break;
		case 1:
			value = SCC_GetRR1(chan);
			break;
		case 2:
			value = SCC_GetRR2(chan);
			break;
		case 3:
			value = SCC_GetRR3(chan);
			break;
		case 4:
			ReportAbnormalID(0x0744, "RR 4"); /* same as RR0 */
			value = SCC_GetRR0(chan);
			break;
		case 5:
			ReportAbnormalID(0x0745, "RR 5"); /* same as RR1 */
			value = SCC_GetRR1(chan);
			break;
		case 6:
			ReportAbnormalID(0x0746, "RR 6"); /* same as RR2 */
			value = SCC_GetRR2(chan);
			break;
		case 7:
			ReportAbnormalID(0x0747, "RR 7"); /* same as RR3 */
			value = SCC_GetRR3(chan);
			break;
		case 8:
			value = SCC_GetRR8(chan);
			break;
		case 9:
			ReportAbnormalID(0x0748, "RR 9"); /* same as RR13 */
			value = SCC_GetRR13(chan);
			break;
		case 10:
			value = SCC_GetRR10(chan);
			break;
		case 11:
			ReportAbnormalID(0x0749, "RR 11"); /* same as RR15 */
			value = SCC_GetRR15(chan);
			break;
		case 12:
			value = SCC_GetRR12(chan);
			break;
		case 13:
			value = SCC_GetRR13(chan);
			break;
		case 14:
			ReportAbnormalID(0x074A, "RR 14");
			value = 0;
			break;
		case 15:
			value = SCC_GetRR15(chan);
			break;
		default:
			ReportAbnormalID(0x074B,
				"unexpected SCC_Reg in SCC_GetReg");
			value = 0;
			break;
	}

#if EmLocalTalk
	/*
		Always check to see if interrupt state changed after
		ANY register access
	*/
	CheckSCCInterruptFlag();
#endif

#if SCC_dolog
	SCC_DbgLogChanStartLine(chan);
	dbglog_writeCStr(" RR[");
	dbglog_writeHex(SCC_Reg);
	dbglog_writeCStr("] -> ");
	dbglog_writeHex(value);
	dbglog_writeReturn();
#endif

	return value;
}

LOCALPROC SCC_PutReg(ui3r Data, int chan, ui3r SCC_Reg)
{
#if SCC_dolog && 0
	SCC_DbgLogChanStartLine(chan);
	dbglog_writeCStr(" WR[");
	dbglog_writeHex(SCC_Reg);
	dbglog_writeCStr("] <- ");
	dbglog_writeHex(Data);
	dbglog_writeReturn();
#endif

	switch (SCC_Reg) {
		case 0:
			SCC_PutWR0(Data, chan);
			break;
		case 1:
			SCC_PutWR1(Data, chan);
			break;
		case 2:
			SCC_PutWR2(Data, chan);
			break;
		case 3:
			SCC_PutWR3(Data, chan);
			break;
		case 4:
			SCC_PutWR4(Data, chan);
			break;
		case 5:
			SCC_PutWR5(Data, chan);
			break;
		case 6:
			SCC_PutWR6(Data, chan);
			break;
		case 7:
			SCC_PutWR7(Data, chan);
			break;
		case 8:
			SCC_PutWR8(Data, chan);
			break;
		case 9:
			SCC_PutWR9(Data, chan);
			break;
		case 10:
			SCC_PutWR10(Data, chan);
			break;
		case 11:
			SCC_PutWR11(Data, chan);
			break;
		case 12:
			SCC_PutWR12(Data, chan);
			break;
		case 13:
			SCC_PutWR13(Data, chan);
			break;
		case 14:
			SCC_PutWR14(Data, chan);
			break;
		case 15:
			SCC_PutWR15(Data, chan);
			break;
		default:
			ReportAbnormalID(0x074C,
				"unexpected SCC_Reg in SCC_PutReg");
			break;
	}

#if EmLocalTalk
	/*
		Always check to see if interrupt state changed after ANY
		register access
	*/
	CheckSCCInterruptFlag();
#endif
}

GLOBALFUNC ui5b SCC_Access(ui5b Data, blnr WriteMem, CPTR addr)
{
#if EmLocalTalk
	/*
		Determine channel, data, and access type from address.  The bus
		for the 8350 is non-standard, so the Macintosh connects address
		bus lines to various signals on the 8350 as shown below. The
		68K will use the upper byte of the data bus for odd addresses,
		and the 8350 is only wired to the upper byte, therefore use
		only odd addresses or you risk resetting the 8350.

		68k   8350
		----- ------
		a1    a/b
		a2    d/c
		a21   wr/rd
	*/
#endif
	ui3b SCC_Reg;
	int chan = (~ addr) & 1; /* 0=modem, 1=printer */
	if (((addr >> 1) & 1) == 0) {
		/* Channel Control */
		SCC_Reg = SCC.PointerBits;
		SCC.PointerBits = 0;
	} else {
		/* Channel Data */
		SCC_Reg = 8;
	}
	if (WriteMem) {
		SCC_PutReg(Data, chan, SCC_Reg);
	} else {
		Data = SCC_GetReg(chan, SCC_Reg);
	}

	return Data;
}
