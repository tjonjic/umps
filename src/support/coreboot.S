/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include "regdef.h"
#include "umps/arch_asm.h"

	.abicalls

 # GCC automatic generated header
 
 # GNU C 2.7.2 [AL 1.1, MM 40] Linux/MIPSEL compiled by GNU C

 # Cc1 defaults:
 # -mgas -mabicalls

 # Cc1 arguments (-G value = 0, Cpu = 3000, ISA = 1):
 # -quiet -dumpbase -version -o
 # 

 	# Constants

	STATUSMASK 	= 0xFDBFFFF3
	BIOSVECTPAGE 	= 0x20000000
	BIOSPANIC	= 0x00000108
	VECTAREANUM	= 8
	VECTPCPOS	= 12
	VECTSIZE	= 140
	PROGVSTART	= 0x20001004

	PADDINGTLB	= 0x100 - 8
	PADDINGEXC	= 0x80 - 16

	#Code start

	.text
	.align	2
	.globl	coreboot
	.type	coreboot,@function
	.ent	coreboot

coreboot:
	.frame	$fp,0,$ra
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set noat

	.set noreorder
	.set nomacro

	# boot starts here 
	j	LCoreBootStart
	nop

	.space PADDINGTLB

	# TLB miss should not happen during boot phase: jumps to BIOS PANIC
	lui	$k0, 0x0000
	ori	$k0, $k0, 0x108
	jr	$k0
	nop

	.space PADDINGEXC

	# no other exceptions should happen during boot phase:
	# jumps to BIOS PANIC
	lui	$k0, 0x0000
	ori	$k0, $k0, 0x108
	jr	$k0
	nop

	.set reorder
	.set macro

LCoreBootStart:	
	# mapping all exception vectors to BIOS code
	# this is done resetting bit 22 in STATUS CP0 register
	# we also assure that kernel will start in kernel mode, with
	# interrupts and VM disabled: this is done zeroing bits 3, 2 and 25
	mfc0	$a0, $CP0_Status
	li	$a1, STATUSMASK   
	and	$a0, $a0, $a1
	mtc0	$a0, $CP0_Status
	
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
	addiu	$a0, VECTSIZE
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
	
	.end coreboot

LcorebootEnd:
	.size	 coreboot, LcorebootEnd - coreboot
