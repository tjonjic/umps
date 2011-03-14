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

/*
 * This header file contains the global constant & macro definitions.
 */

#include <config.h>

// general configuration constants
#define MPSFILETYPE	".umps"
#define AOUTFILETYPE	".aout.umps"
#define BIOSFILETYPE	".rom.umps"
#define COREFILETYPE	".core.umps"
#define STABFILETYPE	".stab.umps"

// maximum area size for trace ranges: a little more than 4KB
// to avoid troubles in browser refresh if area is too large
#define MAXTRACESIZE	(FRAMESIZE + 1)

/***************************************************************************/

// no more user-serviceable parts below this line

// some utility constants
#define	HIDDEN	static

#define	EOS	'\0'
#define EMPTYSTR	""
#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

// host specific constants
#ifdef WORDS_BIGENDIAN
#define BIGENDIANCPU	1
#else
#define BIGENDIANCPU	0
#endif

// hardware constants 

// physical memory page frame size (in words)
#define FRAMESIZE	1024

// KB per frame
#define FRAMEKB	4

// block device size in words
#define BLOCKSIZE	FRAMESIZE

// eth packet size
#define PACKETSIZE 1514

// DMA transfer time
#define DMATICKS	BLOCKSIZE

// miscellaneous MIPS alignment and size definitions needed by modules
// other by processor.cc

// number of ASIDs
#define MAXASID 64

// MIPS NOP instruction 
#define NOP	0UL

// word length in bytes, byte length in bits, sign masks, etc.
#define WORDLEN 4
#define BYTELEN	8
#define WORDSHIFT	2
#define MAXWORDVAL	0xFFFFFFFFUL
#define SIGNMASK	0x80000000UL
#define BYTEMASK	0x000000FFUL

// immediate/lower halfword part mask
#define IMMMASK	0x0000FFFFUL

// word alignment mask
#define ALIGNMASK	0x00000003UL

// halfword bit length 
#define HWORDLEN	16

// exception type constants (simulator internal coding)
#define NOEXCEPTION 	0
#define INTEXCEPTION	1
#define MODEXCEPTION	2
#define UTLBLEXCEPTION	3
#define TLBLEXCEPTION 	4
#define UTLBSEXCEPTION	5
#define TLBSEXCEPTION	6
#define ADELEXCEPTION	7
#define ADESEXCEPTION	8
#define DBEXCEPTION	9
#define IBEXCEPTION	10
#define SYSEXCEPTION	11
#define BPEXCEPTION		12
#define RIEXCEPTION	13
#define CPUEXCEPTION	14
#define OVEXCEPTION	15

// interrupt handling related constants

// timer interrupt line
#define TIMERINT	2

// device starting interrupt line
#define DEVINTBASE	3

// device register length
#define DEVREGLEN 	4

// interrupts available for registers 
#define DEVINTUSED 	5

// devices per interrupt line
#define DEVPERINT	8

// CAUSE register IP field starting bit  
#define IPMASKBASE	8

// segments base addresses
#define KSEG0BASE	0x00000000UL
#define KSEG0TOP	0x20000000UL
#define KUSEG2BASE	0x80000000UL

// bus memory mapping constants (BIOS/device registers/BOOT/RAM)
#define BIOSBASE	0x00000000UL
#define DEVBASE		0x10000000UL
#define BOOTBASE	0x1FC00000UL
#define RAMBASE		0x20000000UL

// Processor structure register numbers
#define CPUREGNUM	34
#define CPUGPRNUM	32
#define CP0REGNUM	9

// device type codes
#define NULLDEV	0
#define DISKDEV	1
#define TAPEDEV	2
#define ETHDEV 3
#define PRNTDEV 4	
#define TERMDEV 5

// interrupt line offset used for terminals 
// (lots of code must be modified if this changes)

#define TERMINT	4

// memory access types for brkpt/susp/trace ranges in watch.cc and appforms.cc
// modules
#define READWRITE 0x6
#define READ	0x4
#define WRITE	0x2
#define EXEC	0x1
#define EMPTY 	0x0

// some useful macros

// recognizes bad (unaligned) virtual address 
#define BADADDR(w)	((w & ALIGNMASK) != 0UL)

// returns the sign bit of a word
#define SIGNBIT(w)	(w & SIGNMASK)

// returns 1 if the two strings are equal, 0 otherwise
#define SAMESTRING(s,t)	(strcmp(s,t) == 0)

// returns 1 if a is in open-ended interval [b, c[, 0 otherwise
#define INBOUNDS(a,b,c)		(a >= b && a < c)  
