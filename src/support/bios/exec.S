/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include "umps/regdef.h"
#include "umps/arch.h"
#include "umps/cp0.h"
#include "umps/bios_defs.h"

	/* Constants */

	ASIDMASK	= 0x00000FC0
	ASIDSTEP	= 0x00000040
	GBITONMASK	= 0x00000100
	VPNMASK		= 0xFFFFF000
	VECTSIZE	= 140

	/* NOP padding: 30 nops - string length (32 bytes) */
	GENEXC_PADDING 	= (30 * 4) - 32
	ENDSTRADDR	= 0x00000008
	PANICSTRADDR	= ENDSTRADDR + 16

	TERM0COMMAND	= DEV_REG_ADDR(IL_TERMINAL, 0) + 0xC
	BUSYCODE	= 3
	BYTELEN 	= 8
	PRINTCHR	= 2	

	PTEBUGFLAG  	= 0x20000FFC
	PTESTART	= 0x20000500
	PTESEGSIZE  	= 12
	PTESEGOFFS	= 30
	PTEMAGIC	= 0x2A
	PTEMAGICOFFS	= 24
	PTECNTOFFS	= 8
	UTLBSTACK	= 0x20000FF8

	BADPTE		= 13
	PTEMISS		= 14
	
	CAUSENEGMASK	= 0xFFFFFF83
	KUPSTATUSMASK	= 0x00000008
	SAFESTATUSMASK	= 0xFEFFFFFC


	/*
	 * Code start
	 */

	.text
	.align	2
	.globl	bios
	.type	bios,@function
	.ent	bios
bios:
	.frame	$fp,0,$k1
	.mask	0x00000000,0
	.fmask	0x00000000,0

	/*
	 * Tell gas not to use $at in pseudoop expansions
	 */
	.set noat

	/*
	 * 0x00000000 address
	 * This is the entry point for UTLB type exceptions.
	 */
	.set noreorder
	.set nomacro
	b	LUTLBHandler
	nop

EndStr:	
	.asciiz	"System halted \n"	
PanicStr:
	.asciiz "kernel panic()\n"

	.space	GENEXC_PADDING

	/*
	 * 0x00000080 address
	 * This is the entry point for general ("other", non-TLB) exceptions.
	 */
	b	LEXCHandler
	nop

	.space	0x80 - 0x8
	/*
	 * 0x00000100 address
	 * Secondary processor startup routine
	 */
	b	LInitSecondaryProcessor
	nop

	/*
	 * 0x00000108: panic()
	 * Print a message on terminal 0 and loop forever 
	 */

	.set	reorder
	.set	macro

LPanic:
	li	$a0, PANICSTRADDR - 1

LTermOp:
	li 	$a3, TERM0COMMAND
	li 	$t0, BUSYCODE

LWaitForReady:
	lw 	$a2, -4($a3)
	beq	$a2, $t0, LWaitForReady

LPrintLoop:
	addiu	$a0, 1
	lbu	$a1, 0($a0)

	# char 0 ends the string
	beq	$0, $a1, LInfinite
	
	#prepares PRINTCHAR command
	sll 	$a1, BYTELEN
	addi	$a1, PRINTCHR
	sw	$a1, 0($a3)

	b	LWaitForReady

LInfinite:
	b	LInfinite

LHalt:
	# this call prints a message on terminal 0 and loops forever 
	li	$a0, ENDSTRADDR - 1
	b 	LTermOp

/*
 * Get a cpu up and runing: initialize BIOS related structures and
 * load the supplied processor state.
 */
LInitSecondaryProcessor:
	/* Set up a convenient pointer to some scratch RAM space */
	li	$t0, BIOS_CPU_SCRATCH_RAM_SIZE
	mfc0	$t1, $CP0_PRID
	mult	$t0, $t1
	mflo	$t0
	li	$t1, BIOS_LEGACY_SCRATCH_BASE
	subu	$t0, $t1, $t0

	li	$t1, BIOS_SCRATCH_BASE
	sw	$t0, 0($t1)

	/* Initialize ptr to new/old areas */
	lw	$t1, 8($t0)
	li	$t2, BIOS_EXCVEC_BASE
	sw	$t1, 0($t2)

	/* Load a new state */
	lw	$k0, 4($t0)
	b	LLoadStart

/*
 * Handle TLBL/TLBS refill misses
 */
LUTLBHandler:
	/* clear PTEBUGFLAG for extra exceptions signaling */
	li	$k0, BIOS_SCRATCH_BASE
	lw	$k0, 0($k0)
	sw	$0, BIOS_SCRATCH_TLBEXC($k0)

	/* gets some register work space using bottom of BIOS page frame */
	li	$k0, BIOS_SCRATCH_BASE
	lw	$k0, 0($k0)
	sw	$a0, BIOS_SCRATCH_a0($k0)
	sw	$a1, BIOS_SCRATCH_a1($k0)
	sw	$a2, BIOS_SCRATCH_a2($k0)
	sw	$a3, BIOS_SCRATCH_a3($k0)

	/* save failed VPN + ASID into register $k1 */
	mfc0	$k1, $CP0_EntryHi

	/* compute ASID in $a3 */
	move $a3, $k1
	andi $a3, ASIDMASK

	/* gets PTE segment starting address in $a2 */
	li	$a2, PTESTART

LPTEScanLoop:
	blez	$a3, LPTEStartFound
	addi	$a2, PTESEGSIZE
	addi	$a3, -ASIDSTEP
	b	LPTEScanLoop
	
LPTEStartFound:
	/* PTE segment table for ASID is in $a2; look for correct segment */

	/* using $a1 for segment computation */
	move	$a1, $k1
	srl	$a1, PTESEGOFFS

	/* segments 0 and 1 address is in $a2 already */
	beq	$0, $a1, LPTESegmentFound
	addi	$a1, -1
	beq	$0, $a1, LPTESegmentFound

	/* increment $a2 for segment 2 */
	addi	$a2, 4
	addi    $a1, -1
	beq	$0, $a1, LPTESegmentFound

	/* then it's segment 3 */
	addi    $a2, 4

LPTESegmentFound:	
	
	/* In $a2 there is the PTE segment starting address;
	 * check if the PTE address is aligned and between RAMBASE and RAMTOP */
	lw	$a2, 0($a2)

	/* check alignment */
	move 	$a3, $a2
	andi 	$a3, 0x3
	bne 	$0, $a3, LBadPTE

	/* check RAMBASE */
	li 	$a3, BUS_REG_RAM_BASE
	lw 	$a3, 0($a3)
	subu	$a3, $a2, $a3
	bltz 	$a3, LBadPTE

	/* check RAMTOP */
	li 	$a3, BUS_REG_RAM_BASE
	lw 	$a3, 0($a3)
	li 	$a1, BUS_REG_RAM_SIZE
	lw 	$a1, 0($a1)
	add	$a3, $a3, $a1
	subu 	$a3, $a3, $a2
	blez 	$a3, LBadPTE

	/* now check if it is a valid PTE at $a2 */
	lw	$a3, 0($a2)
	li  	$a1, PTEMAGIC
	srl 	$a3, PTEMAGICOFFS
	bne	$a1, $a3, LBadPTE
	
	/* get PTE entry count in $a3 */
	lw  	$a3, 0($a2)
	/* clean up MAGIC tag */
	sll	$a3, PTECNTOFFS
	srl 	$a3, PTECNTOFFS

	/* check if the PTE ends over RAMTOP */

	/* get RAMTOP */
	li 	$a0, BUS_REG_RAM_BASE
	lw 	$a0, 0($a0)
	li 	$a1, BUS_REG_RAM_SIZE
	lw 	$a1, 0($a1)
	add	$a0, $a0, $a1

	/* subtract PTEBASE and tag  */
	subu 	$a0, $a0, $a2
	addi	$a0, -4

	/* shift #entries left for multiplying x 8 and get PTESIZE */
	sll 	$a3, 3

	/* Subtract PTESIZE and check */
	subu	$a0, $a3
	blez	$a0, LBadPTE

	/*
	 * The page table seems ok (proper alignment, within RAM
	 * boundaries, right magic marker).
	 *
	 * Now on to locating the PTE.
	 */

	/* Restore $a3 to EntryCNT */
	srl 	$a3, 3

        /*
	 * Now scans PTE up to $a3 entries, getting them in $a1 from $k0 address 
	 * if the VPN+ASID does not match, checks for G bit and, if on, tries
	 * the VPN-only match against $a0.
         */
	
	/* set $k0 to first TLB entry */
	move 	$k0, $a2
	addiu 	$k0, 4

	/* set $a2 to VPNMASK and $a0 to match VPN without ASID */
	li	$a2, VPNMASK
	and	$a0, $k1, $a2

LSearchLoop:
	lw 	$a1, 0($k0)
	beq	$k1, $a1, LTLBFound
	
	/* checks for G bit */
	lw	$a1, 4($k0)
	andi	$a1, GBITONMASK
	beq	$0, $a1, LEndLoop
	
	/* G bit is on: checks against $a0 */
	lw	$a1, 0($k0)
	and 	$a1, $a1, $a2
	beq	$a0, $a1, LTLBFound 

LEndLoop:	
	addiu	$k0, 8
	addi	$a3, -1
	beq 	$0, $a3, LPTEMiss
	b	LSearchLoop

LTLBFound:
	# $k1 == $a1 o G bit entry match
	# load TLB using $a1, restore proc status by putting 
	# $k1 in ENTRYHI again, reload registers and return thru $k0
	lw	$a1, 0($k0)
	mtc0	$a1, $CP0_EntryHi
	lw	$a1, 4($k0)
	mtc0	$a1, $CP0_EntryLo

	/* this is needed to assure that tlbwr works correctly, since
	   assembler does not see the data dependency between mtc0 and tlb
	   operations */
	.set noreorder
	.set nomacro
	nop
	tlbwr
	nop
	.set reorder
	.set macro

	/* restore ENTRYHI after TLB write (ASID could be different) */
	mtc0	$k1, $CP0_EntryHi
	
	/* reload registers from stack */
	li	$k0, BIOS_SCRATCH_BASE
	lw	$k0, 0($k0)
	lw	$a0, BIOS_SCRATCH_a0($k0)
	lw	$a1, BIOS_SCRATCH_a1($k0)
	lw	$a2, BIOS_SCRATCH_a2($k0)
	lw	$a3, BIOS_SCRATCH_a3($k0)

	/* get EPC from CP0 */
	mfc0	$k0, $CP0_EPC

	.set noreorder
	.set nomacro
	jr	$k0
	rfe
	.set reorder
	.set macro

LBadPTE:
	/* set PTEBUGFLAG to signal TLB exc handler the problem */
	li	$k0, BIOS_SCRATCH_BASE
	lw	$k0, 0($k0)
	li	$a0, BADPTE
	sw	$a0, BIOS_SCRATCH_TLBEXC($k0)
	b 	LTLBNotFound

LPTEMiss:
	/* set PTEBUGFLAG to signal TLB exc handler the problem and fall through */
	li	$k0, BIOS_SCRATCH_BASE
	lw	$k0, 0($k0)
	li	$a0, PTEMISS
	sw	$a0, BIOS_SCRATCH_TLBEXC($k0)

LTLBNotFound:
	/* table exausted and match not found; reload registers,
	   ENTRYHI (which is still in $k1), and fall thru to default exc handler */
	mtc0	$k1, $CP0_EntryHi
	li	$k0, BIOS_SCRATCH_BASE
	lw	$k0, 0($k0)
	lw	$a0, BIOS_SCRATCH_a0($k0)
	lw	$a1, BIOS_SCRATCH_a1($k0)
	lw	$a2, BIOS_SCRATCH_a2($k0)
	lw	$a3, BIOS_SCRATCH_a3($k0)


/*
 * General exception handler
 *
 * k0 and k1 registers are always available (never have live
 * values outside BIOS)
 */
LEXCHandler:
	/* Copy Cause.ExcCode to k0 and dispatch accordingly */
	mfc0	$k0, $CP0_Cause
	andi	$k0, $k0, CAUSE_EXCCODE_MASK
	srl	$k0, CAUSE_EXCCODE_BIT

	/* cause == 0 is interrupt */
	beq	$0, $k0, LIntHandler
	
	/* cause 1-3 is a TLB exception (cause = 2 or 3 and EPC in user
	   space means UTLBHandler has failed) */
	addi	$k0, -3
	blez	$k0, LTLBHandler
	
	/* cause 4-7 is a program trap */
	addi	$k0, -4
	blez	$k0, LPRGHandler

	/* cause 8 is syscall */
	addi	$k0, -1
	beq	$0, $k0, LSYSHandler

	/* cause 9 is break */
	addi 	$k0, -1
	beq	$0, $k0, LBreakHandler
	
	/* other causes are program trap */
	b	LPRGHandler

LIntHandler:
	/* If EPC address is in BIOS area, something in kernel has
	 * gone horribly wrong: eg. BIOS running with int unmasked (a big
	 * bug for BIOS). */
	mfc0	$k0, $CP0_EPC
	li	$k1, BIOS_LEGACY_EXCVEC_BASE
	subu	$k0, $k0, $k1
	bltz	$k0, LPanic
	/* Else:
	 * save processor state into old area and load new state:
	 * this is done calling a BIOS subroutine */
	li	$k1, 0
	b	LSOldLNewArea

LTLBHandler:
	/* If EPC address is in BIOS area, something in kernel has
	 * gone horribly wrong: eg. page table area corrupted and
	 * consequent UTLB handler error (or bug in BIOS). */
	mfc0	$k0, $CP0_EPC
	li	$k1, BIOS_LEGACY_EXCVEC_BASE
	subu	$k0, $k0, $k1
	bltz	$k0, LPanic	

	li	$k1, 2
	b	LSOldLNewArea

LPRGHandler:
	/* If EPC address is in BIOS area, something in kernel or in BIOS
	 * has gone horribly wrong (a BIOS bug probably). */
	mfc0	$k0, $CP0_EPC
	li	$k1, BIOS_LEGACY_EXCVEC_BASE
	subu	$k0, $k0, $k1
	bltz	$k0, LPanic

	li	$k1, 4
	b	LSOldLNewArea


LSYSHandler:
	/* If EPC address is in BIOS area, something in kernel or in BIOS
	 * has gone orribly wrong (a BIOS bug probably). */
	mfc0	$k0, $CP0_EPC
	li	$k1, BIOS_LEGACY_EXCVEC_BASE	
	subu	$k0, $k0, $k1
	bltz	$k0, LPanic

	li	$k1, 6
	b	LSOldLNewArea

LBreakHandler:
	/* If EPC address is in BIOS area, something in kernel or in BIOS
	 * has gone horribly wrong (a BIOS bug probably). */
	mfc0	$k0, $CP0_EPC
	li	$k1, BIOS_LEGACY_EXCVEC_BASE
	subu	$k0, $k0, $k1
	bltz	$k0, LPanic


	/*
	 * A BIOS service routine is requested:
	 * look into $a0 register for identification.
         */

	move	$k1, $a0

	/* any BREAK may be executed only in kernel mode
	   kernel mode of caller is set when KUP bit in STATUS mask is 0 */
	mfc0	$k0, $CP0_Status
	andi	$k0, $k0, KUPSTATUSMASK
	beq	$0, $k0, LisKernel
	b 	LSYSHandler

LisKernel:
	/* 0  is FORK(STATUS, EntryHI, PC, CAUSE) */
	beq 	$0, $k1, LFORK
	
	addi    $k1, -1
	
	/* 1 is LDST */
	beq 	$0, $k1, LLDST
	
	addi    $k1, -1

	/* 2 is PANIC routine */
	beq	$0, $k1, LPanic

	addi	$k1, -1
	
	/* 3 is HALT routine */
	beq     $0, $k1, LHalt

	/* any other break is passed up to SYS handler */
	b	LSYSHandler


LFORK:
	# $v0 is ENTRYHI, $a1 is STATUS, $a2 is new PC, $a3 is CAUSE
	
	# EntryHI loading
	mtc0	$v0, $CP0_EntryHi
	
	# STATUS preparation
	move 	$k1, $a1
	# this for BIOS safety: no KU, IE or VM bits on
	li	$k0, SAFESTATUSMASK
	and	$k1, $k1, $k0

	# STATUS loading
	mtc0	$k1, $CP0_Status
	
	# CAUSE loading
	mtc0 	$a3, $CP0_Cause

	# get new PC and jump
	move 	$k1, $a2
	
	.set noreorder
	.set nomacro
	jr	$k1
	rfe
	.set reorder
	.set macro


LLDST:
	# this means load from physical address in $a1
	move	$k0, $a1
	b	LLoadStart


/*
 * Save cpu state into the Old Area #k1 and load state from the
 * corresponding New Area.
 */
LSOldLNewArea:
	/* locate the needed area using $k0, then save the
	   registers and other info */
	li	$k0, BIOS_EXCVEC_BASE
	lw	$k0, 0($k0)

LComputeArea:
	blez	$k1, LSave
	addiu	$k0, VECTSIZE
	addi	$k1, -1
	b 	LComputeArea

LSave:	
	sw	$1, 16($k0)
	sw	$v0, 20($k0)
	sw	$v1, 24($k0)
	sw 	$a0, 28($k0)
	sw	$a1, 32($k0)
	sw	$a2, 36($k0)
	sw	$a3, 40($k0)
	sw	$t0, 44($k0)
	sw	$t1, 48($k0)
	sw	$t2, 52($k0)
	sw	$t3, 56($k0)
	sw	$t4, 60($k0)
	sw	$t5, 64($k0)
	sw	$t6, 68($k0)
	sw	$t7, 72($k0)
	sw	$s0, 76($k0)
	sw	$s1, 80($k0)
	sw	$s2, 84($k0)
	sw	$s3, 88($k0)
	sw	$s4, 92($k0)
	sw	$s5, 96($k0)
	sw	$s6, 100($k0)
	sw	$s7, 104($k0)
	sw	$t8, 108($k0)
	sw	$t9, 112($k0)

	/* $k0 and $k1 are not saved */

	sw	$gp, 116($k0)
	sw	$sp, 120($k0)
	sw	$fp, 124($k0)
	sw	$ra, 128($k0)
	mfhi	$k1
	sw	$k1, 132($k0)
	mflo	$k1
	sw	$k1, 136($k0)
	/* all processor registers saved */

	/* gets EntryHI and stores it */
	mfc0	$k1, $CP0_EntryHi
	sw	$k1, 0($k0)
	
	/* gets CAUSE register and stores it */
	mfc0    $k1, $CP0_Cause
	sw      $k1, 4($k0)
	
	/* and now save STATUS and EPC registers */
	mfc0	$k1, $CP0_Status
	sw	$k1, 8($k0)
	mfc0    $k1, $CP0_EPC
	sw      $k1, 12($k0)

	/* processor state saved: any register except $k0 and $k1 is available */

	/*
	 * BadPTE and PTEMiss handling
	 */

	li	$a3, BIOS_SCRATCH_BASE
	lw	$a3, 0($a3)
	lw	$a3, BIOS_SCRATCH_TLBEXC($a3)

	/* if $a3 == 0 => no special case */
	beq	$0, $a3, LEndPTEMgmt
	
	/* else overwrite Cause.ExcCode with correct value */
	
	/* load old CAUSE in $a2 */
	lw	$a2, 4($k0)
	/* clear it */
	li	$a1, CAUSENEGMASK
	and	$a2, $a2, $a1
	
	/* shift, add correct value, and re-shift CAUSE */
	srl	$a2, CAUSE_EXCCODE_BIT
	addu	$a2, $a2, $a3
	sll	$a2, CAUSE_EXCCODE_BIT
	
	/* store it back */
	sw	$a2, 4($k0)

	/* Clear PTEBUGFLAG */
	li	$a3, BIOS_SCRATCH_BASE
	lw	$a3, 0($a3)
	sw	$0, BIOS_SCRATCH_TLBEXC($a3)

LEndPTEMgmt:
	/* start loading */
	addiu	$k0, VECTSIZE

/*
 * Load CPU/CP0 registers from (state_t*) $k0
 */
LLoadStart:
	lw	$1, 16($k0)
	lw	$v0, 20($k0)
	lw	$v1, 24($k0)
	lw 	$a0, 28($k0)
	lw	$a1, 32($k0)
	lw	$a2, 36($k0)
	lw	$a3, 40($k0)
	lw	$t0, 44($k0)
	lw	$t1, 48($k0)
	lw	$t2, 52($k0)
	lw	$t3, 56($k0)
	lw	$t4, 60($k0)
	lw	$t5, 64($k0)
	lw	$t6, 68($k0)
	lw	$t7, 72($k0)
	lw	$s0, 76($k0)
	lw	$s1, 80($k0)
	lw	$s2, 84($k0)
	lw	$s3, 88($k0)
	lw	$s4, 92($k0)
	lw	$s5, 96($k0)
	lw	$s6, 100($k0)
	lw	$s7, 104($k0)
	lw	$t8, 108($k0)
	lw	$t9, 112($k0)

	/* $k0 and $k1 are not saved so they are not loaded too */

	lw	$gp, 116($k0)
	lw	$sp, 120($k0)
	lw	$fp, 124($k0)
	lw	$ra, 128($k0)

	lw	$k1, 132($k0)
	mthi	$k1
	lw	$k1, 136($k0)
	mtlo	$k1

	/* all processor registers loaded (almost) */

	/* storing new EntryHI into CP0 register */
	lw	$k1, 0($k0)
	mtc0	$k1, $CP0_EntryHi
	
	/* storing new CAUSE into CP0 register */
	lw	$k1, 4($k0)
	mtc0	$k1, $CP0_Cause

	/* now load STATUS register */
	lw	$k1, 8($k0)
	/* This is for avoiding trouble if STATUS has (erroneously)
	 * set bit 0/1. It would cause an immediate EXC trap or expose
	 * BIOS to interrupts. */
	srl 	$k1, 2
	sll	$k1, 2
	mtc0	$k1, $CP0_Status

	lw	$k1, 12($k0)

	/* load new PC and jump */
	.set noreorder
	.set nomacro
	jr	$k1
	rfe
	.set reorder
	.set macro
	
	.end	bios
	.size	bios, . - bios
