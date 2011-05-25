/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include "regdef.h"
#include "umps/arch.h"
#include "umps/cp0.h"

 	/* Constants */

	STATUSMASK 	= 0xFDBFFFF3
	BIOSVECTPAGE 	= 0x20000000
	BIOSPANIC	= 0x00000088
	VECTAREANUM	= 8
	VECTPCPOS	= 12
	VECTSIZE	= 140
	PROGVSTART	= 0x20001004

	/*TERM0COMMAND	= 0x1000025C*/
	TERM0COMMAND	= 0x1000026C
	BYTELEN 	= 8
	PRINTCHR	= 2	
	DOTCHAR		= 46
	LFCHAR		= 10

	TAPE0BASE	= 0x100000D0
	TAPESTART	= 3
	READBLK		= 3
	
	ACK		= 1
	BUSYCODE	= 3
	READYCODE	= 1
	EOBCODE		= 2

	RAMBASE		= 0x20000000
	BLOCKSIZE	= 4096

	PADDINGTLB	= 0x100 - 8
	PADDINGEXC	= 0x80 - 16

	#Code start

	.text
	.align	2
	.globl	tapeboot
	.type	tapeboot,@function
	.ent	tapeboot

tapeboot:
	.frame	$fp,0,$ra
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set noat

	.set noreorder
	.set nomacro

	# boot starts here 
	j LTapeBootStart
	nop

	.space PADDINGTLB

	# TLB miss should not happen during boot phase: jumps to BIOS PANIC
	lui	$k0, 0x0000
	ori	$k0, $k0, 0x88
	jr	$k0
	nop

	.space PADDINGEXC

	# no other exceptions should happen during boot phase:
	# jumps to BIOS PANIC
	lui	$k0, 0x0000
	ori	$k0, $k0, 0x88
	jr	$k0
	nop	

	.set reorder
	.set macro

LTapeBootStart:	
	# mapping all exception vectors to BIOS code
	# this is done resetting bit 22 in STATUS CP0 register
	# we also assure that kernel will start in kernel mode, with
	# interrupts and VM disabled: this is done zeroing bits 3, 2 and 25
	mfc0	$a0, $CP0_Status
	li	$a1, STATUSMASK   
	and	$a0, $a0, $a1
	mtc0	$a0, $CP0_Status

	# loads the kernel from tape: for each block loaded a dot will be
	# printed on terminal 0 

	# initializations	

	li 	$v0, TAPE0BASE
	li 	$v1, TAPESTART
	lw	$a0, +12($v0)
	
	/* if tape is not loaded from start, BIOS panic() routine is called */
	beq	$v1, $a0, LStartLoad
	
	li	$k0, BIOSPANIC
	jr	$k0

LStartLoad:	

	/* else loading may start */

	# physical memory load index
	li	$k0, RAMBASE - BLOCKSIZE
	
	# tape READBLK command
	li	$k1, READBLK

	# terminal 0 COMMAND address and busy code
	li 	$a3, TERM0COMMAND
	li 	$t0, BUSYCODE

	# tape loading main loop 

LTapeLoadLoop:
	# stores physical loading address into TAPE0 DATA0 register
	# and starts read operation

	addi 	$k0, BLOCKSIZE

	sw 	$k0, +8($v0)
	sw 	$k1, +4($v0)
	
	li	$v1, BUSYCODE

LTapeWaitLoop:
	#loads STATUS register
	lw	$a0, +0($v0)
	beq	$v1, $a0, LTapeWaitLoop

	# tape STATUS ($a0) is no more BUSY: looking for errors
	li 	$v1, READYCODE

	beq	$a0, $v1, LContinueLoad

	li	$k0, BIOSPANIC
	jr	$k0

LContinueLoad:
	# all ok

LPrintDotWaitLoop:

	# awaits terminal 0 availability

	lw 	$a2, -4($a3)
	beq	$a2, $t0, LPrintDotWaitLoop
	
	#load ASCII value of '.' into register 
	li	$a1, DOTCHAR

	#prepares PRINTCHAR command
	sll 	$a1, BYTELEN
	addi	$a1, PRINTCHR
	sw	$a1, 0($a3)


	# examines tape DATA1 register to see if next block must be read:
	# load ends when an EOF or EOT is found, thus it continues when
	# an EOB is found

	lw 	$a0, +12($v0)
	li	$v1, EOBCODE

	beq	$v1, $a0, LTapeLoadLoop

	#tape load ends here: clearing tape interrupt

	li 	$k1, ACK
	sw	$k1, +4($v0)


LPrintLFWaitLoop:

	# awaits terminal 0 availability

	lw 	$a2, -4($a3)
	beq	$a2, $t0, LPrintLFWaitLoop
	
	#load ASCII value of LF into register 
	li	$a1, LFCHAR

	#prepares PRINTCHAR command
	sll 	$a1, BYTELEN
	addi	$a1, PRINTCHR
	sw	$a1, 0($a3)

	# awaits terminal 0 availability again

LPrintLoop2:
	lw 	$a2, -4($a3)
	beq	$a2, $t0, LPrintLoop2

	# clears pending interrupt on terminal 0

	li	$a1, ACK
	sw	$a1, 0($a3)
	
	# setting boot exception vectors into first page of RAM memory:
 	# at first, all exceptions cause a kernel panic()

	# load kernel panic() BIOS routine address into every PC field
	# of exception vectors

	# there are: 1 vector for interrupt, 1 for TLB memory management, 
	# 1 for syscall and break (passup from BIOS), 1 for program traps
	# (all remaining exceptions)
	# total: 4 vectors; 
	# each contains two areas: old and new; fill both PCs at start

	# every vector area contains:  
	# 1 for EntryHI, 1 for CAUSE, 
	# 1 for STATUS, and 1 for EPC (old area) or PC (new area)
	# 29 words for GPR ($0, k0 & k1 are 
	# excluded), 2 for HI & LO regs
	# total: 35 words of memory per area
	
	li 	$a0, BIOSVECTPAGE
	li 	$a1, BIOSPANIC # put here panic() address
	li 	$a2, VECTAREANUM
Lfor:	
	sw	$a1, VECTPCPOS($a0)
	addiu	$a0,VECTSIZE
	addi	$a2, -1
	bgtz	$a2, Lfor
	
	# finally, we set EntryHI and STATUS for kernel:
	# being EntryHI = 0, and STATUS good as it is now,
	# do not touch anything
	
	# and now let's start kernel: its starting address is 
	# in aout place for it
	li	$a0, PROGVSTART
	lw	$ra, 0($a0)

	.set noreorder
	.set nomacro
	jr	$ra
	rfe
	.set reorder
	.set macro	
	
	.end tapeboot

LtapebootEnd:
	.size	 tapeboot, LtapebootEnd - tapeboot
