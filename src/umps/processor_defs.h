/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2004 Mauro Morsiani
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**************************************************************************** 
 *
 * This header file contains constant & macro definitions for processor.cc
 * and disassemble.cc program modules.
 *
 * All names, bit positions and masks follow MIPS register formats: see
 * documentation.
 *
 * Instruction opcodes too may be retrieved from appropriate documentation:
 * codes are in octal format for easier coding by programmer.
 *
 ****************************************************************************/


// virtual exception handling base address
#define BOOTEXCBASE	(BOOTBASE + 0x100UL)

// exception virtual offsets
#define TLBREFOFFS	0x000UL
#define OTHEREXCOFFS	0x080UL

// CPUEXCEPTION handling constant
#define COPEOFFSET	27

// used to decode TLBCLR instruction
#define CONTEXTREG    4

// and corresponding register index in Processor structure
enum {
    INDEX,
    RANDOM,
    ENTRYLO,
    BADVADDR,
    CP0REG_TIMER,
    ENTRYHI,
    STATUS,
    CAUSE,
    EPC,
    PRID
};

// STATUS and CAUSE interrupt area mask
#define INTMASK	0x0000FF00UL

// TLB EntryHI handling masks and constants 
#define VPNMASK	0xFFFFF000UL
#define ASIDMASK	0x00000FC0UL
#define ASIDOFFS	6
#define OFFSETMASK	(~(VPNMASK))


// TLB EntryLO bit positions and mask
#define GBITPOS	8
#define VBITPOS 9
#define DBITPOS 10
#define ENTRYLOMASK	0xFFFFFF00UL

// RANDOM and INDEX register handling constants
#define RNDIDXOFFS	8
#define RANDOMBASE	(1UL << RNDIDXOFFS)
#define RANDOMSTEP	(1UL << RNDIDXOFFS)


// STATUS register handling constants:

// at reset, are set to 1: cop0 (bit 28), BEV (bit 22)
// all other bits are set to 0 (interrupts disabled, etc.)
#define STATUSRESET	0x10400000UL

// this mask forbids changes to some bits (CU3->1 usable bits, all DS field
// except BEV bit, 0 fixed parts)
#define STATUSMASK      0x1F40FF3FUL

// STATUS kernel/user, interrupt enable, VM bit positions
#define KUOBITPOS	5
#define IEOBITPOS	4
#define KUCBITPOS	1
#define IECBITPOS	0
#define VMOBITPOS	26
#define VMCBITPOS	24

// instruction decode field masks and constants

#define OPTYPEMASK	0xE0000000UL
#define OPCODEMASK	0xFC000000UL
#define FUNCTMASK	0x0000003FUL
#define RSMASK	0x03E00000UL
#define RTMASK	0x001F0000UL
#define RDMASK	0x0000F800UL
#define SHAMTMASK	0x000007C0UL
#define PCUPMASK	0xF0000000UL
#define COPTYPEMASK	0x03E00000UL
#define COPCODEMASK 0x03FF0000UL
#define COPNUMMASK	0x0C000000UL
#define CALLMASK	0x03FFFFC0UL
#define IMMSIGNPOS	15
#define DWCOPBITPOS	28

#define LINKREG	31

#define OPCODEOFFS	26
#define RSOFFSET	21
#define RTOFFSET	16
#define RDOFFSET	11
#define SHAMTOFFS	6
#define COPTYPEOFFS	21
#define COPCODEOFFS	16


// instruction main opcodes

#define REGTYPE	077
#define IMMTYPE 010
#define COPTYPE	020
#define BRANCHTYPE	000
#define LOADTYPE	040
#define LOADCOPTYPE	060
#define STORECOPTYPE	070
#define STORETYPE	050

// REGTYPE (Special) function opcodes

#define SFN_ADD       040
#define SFN_ADDU      041
#define SFN_AND       044
#define SFN_BREAK     015
#define SFN_DIV       032
#define SFN_DIVU      033
#define SFN_JALR      011
#define SFN_JR        010
#define SFN_MFHI      020
#define SFN_MFLO      022
#define SFN_MTHI      021
#define SFN_MTLO      023
#define SFN_MULT      030
#define SFN_MULTU     031
#define SFN_NOR       047
#define SFN_OR        045
#define SFN_SLL       000
#define SFN_SLLV      004
#define SFN_SLT       052
#define SFN_SLTU      053
#define SFN_SRA       003
#define SFN_SRAV      007
#define SFN_SRL       002
#define SFN_SRLV      006
#define SFN_SUB       042
#define SFN_SUBU      043
#define SFN_SYSCALL   014
#define SFN_XOR       046
#define SFN_CAS       013


// IMMCOMPTYPE opcodes

#define ADDI	010
#define ADDIU	011
#define ANDI	014
#define LUI	017
#define ORI	015
#define SLTI	012
#define SLTIU	013
#define XORI 	016

// BRANCHTYPE opcodes

#define BEQ	004

#define BGL	001
#define BGEZ	001
#define BGEZAL	021
#define BLTZ	000
#define BLTZAL	020

#define BGTZ	007
#define BLEZ	006
#define BNE	005
#define J	002
#define JAL	003

// COPTYPE opcodes

// detects if instruction refers to CP0
#define COP0SEL	020

#define CO0	020
#define RFE	020
#define TLBP	010
#define TLBR	001
#define TLBWI	002
#define TLBWR	006
#define COFUN_WAIT 040

#define BC0	010
#define BC0F	0400
#define BC0T	0401

#define MFC0	0
#define MTC0	04

// LOADTYPE opcodes

#define	LB	040
#define LBU	044 
#define LH	041
#define LHU 045
#define LW	043
#define LWL 042
#define LWR 046

//STORETYPE opcodes

#define SB	050
#define SH	051
#define SW	053
#define SWL 052
#define SWR	056

// LOADCOPTYPE opcodes
#define LWC0 060

// STORECOPTYPE opcodes
#define SWC0 070

// byte sign extension mask
#define BYTESIGNMASK	0x0000007FUL

// useful macros

// extracts VPN from address
#define VPN(w)		((w & VPNMASK))

// extracts ASID from address
#define ASID(w)	((w & ASIDMASK))

// applies interrupt mask to STATUS/CAUSE register 
#define IM(r)		((r & INTMASK))

// aligns a virtual address (clears lowest bits)
#define ALIGN(va)	(va & ~(ALIGNMASK))

// computes physical address from virtual address and PFN field
#define PHADDR(va, pa)	((va & OFFSETMASK) | (pa & VPNMASK))

// detects which byte is referred by a LB/LBU/LWL/LWR/SB/SWL/SWR instruction
#define BYTEPOS(va)	(va & ALIGNMASK)

// detects which halfword is referred by a LH/LHU/SH instruction
#define HWORDPOS(va)	((va & ALIGNMASK) >> 1)

// instruction fields decoding macros
#define OPCODE(i)	((i & OPCODEMASK) >> OPCODEOFFS)
#define FUNCT(i)	(i & FUNCTMASK)
#define SHAMT(i)	((i & SHAMTMASK) >> SHAMTOFFS)
#define REGSHAMT(r)	(r & (SHAMTMASK >> SHAMTOFFS))
#define RS(i)	((i & RSMASK) >> RSOFFSET)
#define RT(i)	((i & RTMASK) >> RTOFFSET)
#define RD(i)	((i & RDMASK) >> RDOFFSET)

// coprocessor instructions decoding macros
#define COPOPCODE(i) 	((i & COPCODEMASK) >> COPCODEOFFS)
#define COPOPTYPE(i)	((i & COPTYPEMASK) >> COPTYPEOFFS)
#define COPNUM(i)	((i & COPNUMMASK) >> OPCODEOFFS)

// computes jump target from PC and J/JAL instruction itself 
#define JUMPTO(pc, i)	((pc & PCUPMASK) | ((i & ~(OPCODEMASK)) << WORDSHIFT))

// zero-extends a immediate (halfword) quantity
#define ZEXTIMM(i)	(i & IMMMASK)

// computes index value from INDEX register format:
// SIGNMASK is used to clear bit 31, which marks a failed TLB probe
#define RNDIDX(id)	((id & ~(SIGNMASK)) >> RNDIDXOFFS)

// decodes BREAK/SYSCALL argument inside instruction itself
#define CALLVAL(i)	((i & CALLMASK) >> SHAMTOFFS)
