/*
	GLOBGLUE.h

	Copyright (C) 2003 Bernd Schmidt, Philip Cummins, Paul C. Pratt

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

#ifdef GLOBGLUE_H
#error "header already included"
#else
#define GLOBGLUE_H
#endif


#define kEmMd_Twig43      0
#define kEmMd_Twiggy      1
#define kEmMd_128K        2
#define kEmMd_512Ke       3
#define kEmMd_Plus        4
#define kEmMd_SE          5
#define kEmMd_SEFDHD      6
#define kEmMd_Classic     7
#define kEmMd_PB100       8
#define kEmMd_II          9
#define kEmMd_IIx        10

#define RAMSafetyMarginFudge 4

#define kRAM_Size (kRAMa_Size + kRAMb_Size)
EXPORTVAR(ui3p, RAM)
	/*
		allocated by MYOSGLUE to be at least
			kRAM_Size + RAMSafetyMarginFudge
		bytes. Because of shortcuts taken in GLOBGLUE.c, it is in theory
		possible for the emulator to write up to 3 bytes past kRAM_Size.
	*/

#if EmVidCard
EXPORTVAR(ui3p, VidROM)
#endif

#if IncludeVidMem
EXPORTVAR(ui3p, VidMem)
#endif

EXPORTPROC MemOverlay_ChangeNtfy(void);

#if (CurEmMd == kEmMd_II) || (CurEmMd == kEmMd_IIx)
EXPORTPROC Addr32_ChangeNtfy(void);
#endif

/*
	representation of pointer into memory of emulated computer.
*/
typedef ui5b CPTR;

/*
	mapping of address space to real memory
*/

EXPORTFUNC ui3p get_real_address0(ui5b L, blnr WritableMem, CPTR addr,
	ui5b *actL);

/*
	memory access routines that can use when have address
	that is known to be in RAM (and that is in the first
	copy of the ram, not the duplicates, i.e. < kRAM_Size).
*/

#ifndef ln2mtb

#define get_ram_byte(addr) do_get_mem_byte((addr) + RAM)
#define get_ram_word(addr) do_get_mem_word((addr) + RAM)
#define get_ram_long(addr) do_get_mem_long((addr) + RAM)

#define put_ram_byte(addr, b) do_put_mem_byte((addr) + RAM, (b))
#define put_ram_word(addr, w) do_put_mem_word((addr) + RAM, (w))
#define put_ram_long(addr, l) do_put_mem_long((addr) + RAM, (l))

#else

#define get_ram_byte get_vm_byte
#define get_ram_word get_vm_word
#define get_ram_long get_vm_long

#define put_ram_byte put_vm_byte
#define put_ram_word put_vm_word
#define put_ram_long put_vm_long

#endif

#define get_ram_address(addr) ((addr) + RAM)

/*
	accessing addresses that don't map to
	real memory, i.e. memory mapped devices
*/

EXPORTFUNC blnr AddrSpac_Init(void);


#define ui5r_FromSByte(x) ((ui5r)(si5r)(si3b)(ui3b)(x))
#define ui5r_FromSWord(x) ((ui5r)(si5r)(si4b)(ui4b)(x))
#define ui5r_FromSLong(x) ((ui5r)(si5r)(si5b)(ui5b)(x))

#define ui5r_FromUByte(x) ((ui5r)(ui3b)(x))
#define ui5r_FromUWord(x) ((ui5r)(ui4b)(x))
#define ui5r_FromULong(x) ((ui5r)(ui5b)(x))


#if WantDisasm
EXPORTPROC dbglog_StartLine(void);
#else
#define dbglog_StartLine()
#endif

#if dbglog_HAVE
EXPORTPROC dbglog_WriteMemArrow(blnr WriteMem);

EXPORTPROC dbglog_WriteNote(char *s);
EXPORTPROC dbglog_WriteSetBool(char *s, blnr v);
EXPORTPROC dbglog_AddrAccess(char *s,
	ui5r Data, blnr WriteMem, ui5r addr);
EXPORTPROC dbglog_Access(char *s, ui5r Data, blnr WriteMem);
#endif

#if ! WantAbnormalReports
#define ReportAbnormalID(id, s)
#else
#if dbglog_HAVE
#define ReportAbnormalID DoReportAbnormalID
#else
#define ReportAbnormalID(id, s) DoReportAbnormalID(id)
#endif
EXPORTPROC DoReportAbnormalID(ui4r id
#if dbglog_HAVE
	, char *s
#endif
	);
#endif /* WantAbnormalReports */

EXPORTPROC VIAorSCCinterruptChngNtfy(void);

EXPORTVAR(blnr, InterruptButton)
EXPORTPROC SetInterruptButton(blnr v);

enum {
	kICT_SubTick,
#if EmClassicKbrd
	kICT_Kybd_ReceiveCommand,
	kICT_Kybd_ReceiveEndCommand,
#endif
#if EmADB
	kICT_ADB_NewState,
#endif
#if EmPMU
	kICT_PMU_Task,
#endif
	kICT_VIA1_Timer1Check,
	kICT_VIA1_Timer2Check,
#if EmVIA2
	kICT_VIA2_Timer1Check,
	kICT_VIA2_Timer2Check,
#endif

	kNumICTs
};

EXPORTPROC ICT_add(int taskid, ui5b n);

#define iCountt ui5b
EXPORTFUNC iCountt GetCuriCount(void);
EXPORTPROC ICT_Zap(void);

EXPORTVAR(uimr, ICTactive)
EXPORTVAR(iCountt, ICTwhen[kNumICTs])
EXPORTVAR(iCountt, NextiCount)

EXPORTVAR(ui3b, Wires[kNumWires])

#define kLn2CycleScale 6
#define kCycleScale (1 << kLn2CycleScale)

#if WantCycByPriOp
#define RdAvgXtraCyc /* 0 */ (kCycleScale + kCycleScale / 4)
#define WrAvgXtraCyc /* 0 */ (kCycleScale + kCycleScale / 4)
#endif

#define kNumSubTicks 16


#define HaveMasterMyEvtQLock EmClassicKbrd
#if HaveMasterMyEvtQLock
EXPORTVAR(ui4r, MasterMyEvtQLock)
#endif
EXPORTFUNC blnr FindKeyEvent(int *VirtualKey, blnr *KeyDown);


/* minivmac extensions */

#define ExtnDat_checkval 0
#define ExtnDat_extension 2
#define ExtnDat_commnd 4
#define ExtnDat_result 6
#define ExtnDat_params 8

#define kCmndVersion 0
#define ExtnDat_version 8

enum {
	kExtnFindExtn, /* must be first */

	kExtnDisk,
	kExtnSony,
#if EmVidCard
	kExtnVideo,
#endif
#if IncludeExtnPbufs
	kExtnParamBuffers,
#endif
#if IncludeExtnHostTextClipExchange
	kExtnHostTextClipExchange,
#endif

	kNumExtns
};

#define kcom_callcheck 0x5B17

EXPORTVAR(ui5r, my_disk_icon_addr)

EXPORTPROC Memory_Reset(void);

EXPORTPROC Extn_Reset(void);

EXPORTPROC customreset(void);

struct ATTer {
	struct ATTer *Next;
	ui5r cmpmask;
	ui5r cmpvalu;
	ui5r Access;
	ui5r usemask; /* Should be one less than a power of two. */
	ui3p usebase;
	ui3r MMDV;
	ui3r Ntfy;
	ui4r Pad0;
	ui5r Pad1; /* make 32 byte structure, on 32 bit systems */
};
typedef struct ATTer ATTer;
typedef ATTer *ATTep;

#define kATTA_readreadybit 0
#define kATTA_writereadybit 1
#define kATTA_mmdvbit 2
#define kATTA_ntfybit 3

#define kATTA_readwritereadymask \
	((1 << kATTA_readreadybit) | (1 << kATTA_writereadybit))
#define kATTA_readreadymask (1 << kATTA_readreadybit)
#define kATTA_writereadymask (1 << kATTA_writereadybit)
#define kATTA_mmdvmask (1 << kATTA_mmdvbit)
#define kATTA_ntfymask (1 << kATTA_ntfybit)

EXPORTFUNC ui5b MMDV_Access(ATTep p, ui5b Data,
	blnr WriteMem, blnr ByteSize, CPTR addr);
EXPORTFUNC blnr MemAccessNtfy(ATTep pT);
