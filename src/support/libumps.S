/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include "regdef.h"
#include "h/bios_srv.h"
#include "umps/arch_asm.h"

#if 0
#define LEAF_FUNC(func)				\
	.globl	func;				\
	.type	func, @function;		\
	.end	func				\
func:	.frame	$fp, 0, $ra			\
	.mask   0x00000000,0			\
	.fmask  0x00000000,0

#define END_LEAF_FUNC(func)			\
	jr	$ra				\
	.end	func				\
	.size	func, . - func
#endif

	/* Ugh, we don't really want abicalls unconditionally. */
	.abicalls

	# Code start

	.text
	.set noat
	.align 2


	# this function cause a system call trap
	# system call code is in $a0 (a0) register
	# return value in $v0 (v0) register
	# it is programmer's task to load the return value into 
 	# state register, and to set PC correctly for returning _after_
	# syscall

	.globl	SYSCALL
	.type	SYSCALL, @function
	.ent 	SYSCALL

SYSCALL:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	syscall
	nop
	.set reorder
	.set macro
	jr	$ra
	
	.end SYSCALL

LendSYSCALL:
	.size	SYSCALL, LendSYSCALL - SYSCALL
		

	# This function returns the CP0 INDEX register 
	# return value goes back  thru register $v0 (v0)

	.globl	getINDEX
	.type	getINDEX, @function
	.ent 	getINDEX
	
getINDEX:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_Index
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getINDEX

LendgetINDEX:
	.size	getINDEX, LendgetINDEX - getINDEX


	# This function returns the CP0 RANDOM register 
	# return value goes back thru register $v0

	.globl	getRANDOM
	.type	getRANDOM, @function
	.ent 	getRANDOM
	
getRANDOM:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_Random
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getRANDOM

LendgetRANDOM:
	.size	getRANDOM, LendgetRANDOM - getRANDOM



	# This function returns the CP0 ENTRYLO register 
	# return value goes back  thru register $v0 (v0)

	.globl	getENTRYLO
	.type	getENTRYLO, @function
	.ent 	getENTRYLO
	
getENTRYLO:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_EntryLo
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getENTRYLO

LendgetENTRYLO:
	.size	getENTRYLO, LendgetENTRYLO - getENTRYLO



	# This function returns the CP0 BADVADDR register 
	# return value goes back  thru register $v0

	.globl	getBADVADDR
	.type	getBADVADDR, @function
	.ent 	getBADVADDR
	
getBADVADDR:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_BadVAddr
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getBADVADDR

LendgetBADVADDR:
	.size	getBADVADDR, LendgetBADVADDR - getBADVADDR



	# This function returns the CP0 ENTRYHI register 
	# return value goes back  thru register $v0 (v0)

	.globl	getENTRYHI
	.type	getENTRYHI, @function
	.ent 	getENTRYHI
	
getENTRYHI:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_EntryHi
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getENTRYHI

LendgetENTRYHI:
	.size	getENTRYHI, LendgetENTRYHI - getENTRYHI



	# This function returns the CP0 STATUS register 
	# return value goes back  thru register $v0 (v0)

	.globl	getSTATUS
	.type	getSTATUS, @function
	.ent 	getSTATUS
	
getSTATUS:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_Status
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getSTATUS

LendgetSTATUS:
	.size	getSTATUS, LendgetSTATUS - getSTATUS


	# This function returns the CP0 CAUSE register 
	# return value goes back  thru register $v0 (v0)

	.globl	getCAUSE
	.type	getCAUSE, @function
	.ent 	getCAUSE
	
getCAUSE:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_Cause
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getCAUSE

LendgetCAUSE:
	.size	getCAUSE, LendgetCAUSE - getCAUSE



	# This function returns the CP0 EPC register 
	# return value goes back  thru register $v0 (v0)

	.globl	getEPC
	.type	getEPC, @function
	.ent 	getEPC
	
getEPC:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_EPC
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getEPC

LendgetEPC:
	.size	getEPC, LendgetEPC - getEPC


	# This function returns the CP0 PRID register 
	# return value goes back  thru register $v0

	.globl	getPRID
	.type	getPRID, @function
	.ent 	getPRID
	
getPRID:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mfc0	$v0, $CP0_PRID
	nop
	.set reorder
	.set macro
	jr	$ra

	.end getPRID

LendgetPRID:
	.size	getPRID, LendgetPRID - getPRID


	# This function returns the CP0 INDEX register after write 
	# argument comes thru register $a0 (a0)
	# return value goes back  thru register $v0 (v0)

	.globl	setINDEX
	.type	setINDEX, @function
	.ent 	setINDEX
	
setINDEX:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mtc0	$a0, $CP0_Index
	nop
	mfc0	$v0, $CP0_Index
	nop
	.set reorder
	.set macro
	jr	$ra

	.end setINDEX

LendsetINDEX:
	.size	setINDEX, LendsetINDEX - setINDEX


	# This function returns the CP0 ENTRYLO register after write
	# argument comes thru register $a0 (a0)
	# return value goes back  thru register $v0

	.globl	setENTRYLO
	.type	setENTRYLO, @function
	.ent 	setENTRYLO
	
setENTRYLO:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mtc0	$a0, $CP0_EntryLo
	nop
	mfc0	$v0, $CP0_EntryLo
	nop
	.set reorder
	.set macro
	jr	$ra

	.end setENTRYLO

LendsetENTRYLO:
	.size	setENTRYLO, LendsetENTRYLO - setENTRYLO



	# This function returns the CP0 ENTRYHI register after write
	# argument goes thru register $a0 (a0)
	# return value goes back  thru register $v0 (v0)

	.globl	setENTRYHI
	.type	setENTRYHI, @function
	.ent 	setENTRYHI
	
setENTRYHI:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mtc0	$a0, $CP0_EntryHi
	nop
	mfc0	$v0, $CP0_EntryHi
	nop
	.set reorder
	.set macro
	jr	$ra

	.end setENTRYHI

LendsetENTRYHI:
	.size	setENTRYHI, LendsetENTRYHI - setENTRYHI



	# This function returns the CP0 STATUS register after write
	# argument goes thru register $a0 (a0)
	# return value goes back  thru register $v0 (v0)

	.globl	setSTATUS
	.type	setSTATUS, @function
	.ent 	setSTATUS
	
setSTATUS:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mtc0	$a0, $CP0_Status
	nop
	mfc0	$v0, $CP0_Status
	nop
	.set reorder
	.set macro
	jr	$ra

	.end setSTATUS

LendsetSTATUS:
	.size	setSTATUS, LendsetSTATUS - setSTATUS


	# This function returns the CP0 CAUSE register after write
	# argument goes thru register $a0 (a0)
	# return value goes back  thru register $v0 (v0)

	.globl	setCAUSE
	.type	setCAUSE, @function
	.ent 	setCAUSE
	
setCAUSE:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	mtc0	$a0, $CP0_Cause
	nop
	mfc0	$v0, $CP0_Cause
	nop
	.set reorder
	.set macro
	jr	$ra

	.end setCAUSE

LendsetCAUSE:
	.size	setCAUSE, LendsetCAUSE - setCAUSE


	# This function write in a random TLB position
	# actual ENTRYHI, ENTRYLO registers

	.globl	TLBWR
	.type	TLBWR, @function
	.ent 	TLBWR
	
TLBWR:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0

	
	.set noreorder
	.set nomacro
	nop
	tlbwr
	nop
	.set reorder
	.set macro

	jr	$ra

	.end TLBWR

LendTLBWR:
	.size	TLBWR, LendTLBWR - TLBWR


	
	# This function write at INDEX TLB position 
  	# actual ENTRYHI and ENTRYLO

	.globl	TLBWI
	.type	TLBWI, @function
	.ent 	TLBWI
	
TLBWI:
	
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	tlbwi
	nop
	.set reorder
	.set macro

	jr	$ra

	.end TLBWI

LendTLBWI:
	.size	TLBWI, LendTLBWI - TLBWI


	# This function probes TLB for matching 
	# arguments ENTRYHI, ENTRYLO

	.globl	TLBP
	.type	TLBP, @function
	.ent 	TLBP
	
TLBP:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	tlbp
	nop
	.set reorder
	.set macro

	jr	$ra

	.end TLBP

LendTLBP:
	.size	TLBP, LendTLBP - TLBP



	# This function reads TLB entry at INDEX and returns in ENTRYHI and ENTRYLO

	.globl	TLBR
	.type	TLBR, @function
	.ent 	TLBR
	
TLBR:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	nop
	tlbr
	nop
	.set reorder
	.set macro
	jr	$ra

	.end TLBR

LendTLBR:
	.size	TLBR, LendTLBR - TLBR


	# This function issues a TLBCLR instruction
 	# no arguments or return values
 
 	.globl	TLBCLR
 	.type	TLBCLR, @function
 	.ent 	TLBCLR
 	
TLBCLR:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
 	
 	.set noreorder
 	.set nomacro
 	nop
 	mtc0	$a0, $4
 	nop
 	.set reorder
 	.set macro
 	jr	$ra
 
 	.end TLBCLR
 
LendTLBCLR:
 	.size	TLBCLR, LendTLBCLR - TLBCLR
 


	# This function loads a processor state from memory and start 
	# executing it. It changes processor state completely, and it is
	# NOT an atomic operation (see interface for description)

	.globl	FORK
	.type	FORK, @function
	.ent 	FORK
	
FORK:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0

	# first registers loaded are HI and LO
	lw	$1, 132($a3)
	mthi	$1
	lw	$1, 136($a3)
	mtlo	$1
	
	lw	$1, 16($a3)

	# register $v0 itself is not loaded

	lw	$v1, 24($a3)

	#registers $a0..$a3 are not loaded too

	lw	$t0, 44($a3)
	lw	$t1, 48($a3)
	lw	$t2, 52($a3)
	lw	$t3, 56($a3)
	lw	$t4, 60($a3)
	lw	$t5, 64($a3)
	lw	$t6, 68($a3)
	lw	$t7, 72($a3)
	lw	$s0, 76($a3)
	lw	$s1, 80($a3)
	lw	$s2, 84($a3)
	lw	$s3, 88($a3)
	lw	$s4, 92($a3)
	lw	$s5, 96($a3)
	lw	$s6, 100($a3)
	lw	$s7, 104($a3)
	lw	$t8, 108($a3)
	lw	$t9, 112($a3)

	# $k0 and $k1 are not saved so they are not loaded too

	lw	$gp, 116($a3)
	lw	$sp, 120($a3)
	lw	$fp, 124($a3)
	lw	$ra, 128($a3)
	# all processor registers loaded (almost)

	# load CAUSE from memory to $a3 (a3)
	lw	$a3, 4($a3)

	# move $a0 to $v0 to use $a0 as EXEC call parameter
	move 	$v0, $a0
	
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_FORK
	break
	nop
	.set reorder
	.set macro


	jr	$ra
	
	.end FORK

LendFORK:
	.size	FORK, LendFORK - FORK


	# This function will save processor status to memory block pointed by
	# register $a0 (a0), and return PC value of instruction immediately
	# following the call as return value in $v0.
	# PC field itself is intentionally leaved at 0 value

	.globl	STST
	.type	STST, @function
	.ent 	STST
	
STST:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noat
	sw	$1, 16($a0)
	sw	$v0, 20($a0)

	# gets CAUSE register and stores it
	mfc0	$v0, $CP0_Cause
	sw 	$v0, 4($a0)

	sw	$v1, 24($a0)
	sw	$a0, 28($a0)
	sw	$a1, 32($a0)
	sw	$a2, 36($a0)
	sw	$a3, 40($a0)
	sw	$t0, 44($a0)
	sw	$t1, 48($a0)
	sw	$t2, 52($a0)
	sw	$t3, 56($a0)
	sw	$t4, 60($a0)
	sw	$t5, 64($a0)
	sw	$t6, 68($a0)
	sw	$t7, 72($a0)
	sw	$s0, 76($a0)
	sw	$s1, 80($a0)
	sw	$s2, 84($a0)
	sw	$s3, 88($a0)
	sw	$s4, 92($a0)
	sw	$s5, 96($a0)
	sw	$s6, 100($a0)
	sw	$s7, 104($a0)
	sw	$t8, 108($a0)
	sw	$t9, 112($a0)

	# $k0 and $k1 are not saved

	sw	$gp, 116($a0)
	sw	$sp, 120($a0)
	sw	$fp, 124($a0)
	sw	$ra, 128($a0)
	mfhi	$a1
	sw	$a1, 132($a0)
	mflo	$a1
	sw	$a1, 136($a0)
	# all processor registers saved
	
	# gets EntryHI and stores it 
	mfc0	$v0, $CP0_EntryHi
	sw	$v0, 0($a0)
	

	# and now saves STATUS register and zeroes PC
	mfc0	$v0, $CP0_Status
	sw	$v0, 8($a0)
	sw	$0, 12($a0)

	# reloads $v1 and $a1 registers

	lw	$v1, 24($a0)
	lw 	$a1, 32($a0)

	# sets $v0 to return value

	move	$v0, $ra

	jr	$ra

	.end STST

LendSTST:
	.size	STST, LendSTST - STST


	.globl	HALT
	.type	HALT, @function
	.ent 	HALT
	
HALT:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_HALT
	break
	nop
	.set reorder
	.set macro


	jr	$ra

	.end HALT

LendHALT:
	.size	HALT, LendHALT - HALT


	.globl	PANIC
	.type	PANIC, @function
	.ent 	PANIC
	
PANIC:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_PANIC
	break
	nop
	.set reorder
	.set macro


	jr	$ra

	.end PANIC

LendPANIC:
	.size	PANIC, LendPANIC - PANIC


	# This function forces the complete reload of processor state from
	# vector state area pointed by argument in $a0 (a0): it works only in
	# kernel mode. There is  no real return: $a1 is used as BIOS
	# argument, but it is reloaded too.

	.globl	LDST
	.type	LDST, @function
	.ent 	LDST
	
LDST:
	.frame  $fp,0,$ra
	.mask   0x00000000,0
	.fmask  0x00000000,0
	
	move	$a1, $a0
	
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_LDST
	break
	nop
	.set reorder
	.set macro


	jr	$ra

	.end LDST

LendLDST:
	.size	LDST, LendLDST - LDST


	.globl	STARTCPU
	.type	STARTCPU, @function
	.ent	STARTCPU
STARTCPU:
	.frame	$fp, 0, $ra
	.mask	0x00000000, 0
	.fmask	0x00000000, 0

	.set	noreorder

	li	$t0, MPCONF_BOOT_PC
	li	$t1, 0x00000100
	sw	$t1, 0($t0)

	li	$t0, MPCONF_BOOT_ARG
	sw	$a1, 0($t0)

	li	$t0, MPCONF_RESET
	sw	$a0, 0($t0)

	jr	$ra
	nop

	.set	reorder

	.end	STARTCPU
	.size	STARTCPU, . - STARTCPU
