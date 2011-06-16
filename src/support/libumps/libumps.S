/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include "umps/regdef.h"
#include "umps/arch.h"
#include "umps/cp0.h"
#include "umps/bios_defs.h"

#define casopc movn

/*
 * Helper macros
 */

#define LEAF_FUNC(func)				\
	.globl	func;				\
	.type	func, @function;		\
	.ent	func;                           \
func:	.frame	$sp, 0, $ra;                    \
	.mask   0x00000000,0;                   \
	.fmask  0x00000000,0

#define END_LEAF_FUNC(func)			\
	.end	func;				\
	.size	func, . - func

#define DEFINE_CP0_GETTER(suffix, reg)          \
LEAF_FUNC(get ## suffix);                       \
	.set noreorder;                         \
	.set nomacro;                           \
	nop;                                    \
	mfc0	$v0, reg;                       \
	nop;                                    \
	.set reorder;                           \
	.set macro;                             \
	jr	$ra;                            \
END_LEAF_FUNC(get ## suffix)

#define DEFINE_CP0_SETTER(suffix, reg)          \
LEAF_FUNC(set ## suffix);                       \
	.set	noreorder;                      \
	.set	nomacro;                        \
	nop;                                    \
	mtc0	$a0, reg;                       \
	nop;                                    \
	mfc0	$v0, reg;                       \
	nop;                                    \
	.set	reorder;                        \
	.set	macro;                          \
	jr	$ra;                            \
END_LEAF_FUNC(set ## suffix)


	/* We don't want abicalls unconditionally. */
#ifdef ABICALLS
	.abicalls
#endif

	/*
	 * Code start
	 */

	.text
	.set noat
	.align 2


/*
 * This function cause a system call trap.
 * System call code is in $a0, return value in $v0.
 * It is programmer's task to load the return value into
 * register, and to set PC correctly for returning _after_
 * syscall
 */
LEAF_FUNC(SYSCALL)
	.set noreorder
	.set nomacro
	nop
	syscall
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(SYSCALL)

/*
 * Define CP0 register accessors.
 *
 * Each accessor is of the form: u32 get<REG_NAME>(void)
 */
DEFINE_CP0_GETTER(INDEX, $CP0_Index)
DEFINE_CP0_GETTER(RANDOM, $CP0_Random)
DEFINE_CP0_GETTER(ENTRYLO, $CP0_EntryLo)
DEFINE_CP0_GETTER(BADVADDR, $CP0_BadVAddr)
DEFINE_CP0_GETTER(TIMER, $CP0_Timer)
DEFINE_CP0_GETTER(ENTRYHI, $CP0_EntryHi)
DEFINE_CP0_GETTER(STATUS, $CP0_Status)
DEFINE_CP0_GETTER(CAUSE, $CP0_Cause)
DEFINE_CP0_GETTER(EPC, $CP0_EPC)
DEFINE_CP0_GETTER(PRID, $CP0_PRID)

/*
 * CP0 setter functions
 * Each setter is of the form: u32 set<REG_NAME>(u32 value)
 * Each function returns the updated register value.
 */
DEFINE_CP0_SETTER(INDEX, $CP0_Index)
DEFINE_CP0_SETTER(ENTRYLO, $CP0_EntryLo)
DEFINE_CP0_SETTER(TIMER, $CP0_Timer)
DEFINE_CP0_SETTER(ENTRYHI, $CP0_EntryHi)
DEFINE_CP0_SETTER(STATUS, $CP0_Status)
DEFINE_CP0_SETTER(CAUSE, $CP0_Cause)

/*
 * TLBWR instruction wrapper
 */
LEAF_FUNC(TLBWR)
	.set noreorder
	.set nomacro
	nop
	tlbwr
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(TLBWR)

/*
 * TLBWI instruction wrapper
 */
LEAF_FUNC(TLBWI)
	.set noreorder
	.set nomacro
	nop
	tlbwi
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(TLBWI)

/*
 * TLBP instruction wrapper
 */
LEAF_FUNC(TLBP)
	.set noreorder
	.set nomacro
	nop
	tlbp
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(TLBP)

/*
 * TLBR instruction wrapper
 * Reads TLB entry at INDEX and returns result in ENTRYHI and ENTRYLO
 */
LEAF_FUNC(TLBR)
	.set noreorder
	.set nomacro
	nop
	tlbr
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(TLBR)

/*
 * TLBCLR instruction wrapper
 */
LEAF_FUNC(TLBCLR)
 	.set noreorder
 	.set nomacro
 	nop
 	mtc0	$a0, $4
 	nop
 	.set reorder
 	.set macro
 	jr	$ra
END_LEAF_FUNC(TLBCLR)

/*
 * WAIT instruction wrapper
 */
LEAF_FUNC(WAIT)
	.set	noreorder
	.set	nomacro
	nop
	.set	mips32
	wait
	.set	mips0
	nop
	.set	reorder
	.set	macro
	jr	$ra
END_LEAF_FUNC(WAIT)

/*
 * CAS instruction wrapper
 */
LEAF_FUNC(CAS)
	.set	mips32
	casopc	$a2, $a0, $a1
	.set	mips0
	move	$v0, $a2
	jr	$ra
END_LEAF_FUNC(CAS)

/*
 * FORK
 *
 * SYNOPSIS:
 * void FORK(unsigned int entryhi, unsigned FORK
 *           int status, unsigned int pc, state_t *statep)
 *
 * This function loads a processor state from memory and start 
 * executing it. It changes processor state completely, and it is
 * NOT an atomic operation (see interface for description).
 */
LEAF_FUNC(FORK)
	/* first registers loaded are HI and LO */
	lw	$1, 132($a3)
	mthi	$1
	lw	$1, 136($a3)
	mtlo	$1
	
	lw	$1, 16($a3)

	/* register $v0 itself is not loaded */

	lw	$v1, 24($a3)

	/* registers $a0..$a3 are not loaded too */

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

	/* $k0 and $k1 are not saved so they are not loaded too */

	lw	$gp, 116($a3)
	lw	$sp, 120($a3)
	lw	$fp, 124($a3)
	lw	$ra, 128($a3)
	/* all processor registers loaded (almost) */

	/* load CAUSE from memory to $a3 (a3) */
	lw	$a3, 4($a3)

	/* move $a0 to $v0 to use $a0 as EXEC call parameter */
	move 	$v0, $a0
	
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_FORK
	break
	nop
	.set reorder
	.set macro

	jr	$ra
END_LEAF_FUNC(FORK)

/*
 * STST
 *
 * SYNOPSIS:
 * void STST(state t *statep)
 *
 * This function will save processor status to memory block pointed by
 * register $a0 (a0), and return PC value of instruction immediately
 * following the call as return value in $v0.
 * PC field itself is intentionally left at 0 value
 */
LEAF_FUNC(STST)
	.set noat
	sw	$1, 16($a0)
	sw	$v0, 20($a0)

	/* gets CAUSE register and stores it */
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

	/* $k0 and $k1 are not saved */

	sw	$gp, 116($a0)
	sw	$sp, 120($a0)
	sw	$fp, 124($a0)
	sw	$ra, 128($a0)
	mfhi	$a1
	sw	$a1, 132($a0)
	mflo	$a1
	sw	$a1, 136($a0)
	/* all processor registers saved */
	
	/* gets EntryHI and stores it */
	mfc0	$v0, $CP0_EntryHi
	sw	$v0, 0($a0)
	
	/* and now saves STATUS register and zeroes PC */
	mfc0	$v0, $CP0_Status
	sw	$v0, 8($a0)
	sw	$0, 12($a0)

	/* reloads $v1 and $a1 registers */
	lw	$v1, 24($a0)
	lw 	$a1, 32($a0)

	/* sets $v0 to return value */
	move	$v0, $ra

	jr	$ra
END_LEAF_FUNC(STST)

/*
 * HALT
 */
LEAF_FUNC(HALT)
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_HALT
	break
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(HALT)

/*
 * PANIC
 */
LEAF_FUNC(PANIC)
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_PANIC
	break
	nop
	.set reorder
	.set macro
	jr	$ra
END_LEAF_FUNC(PANIC)

/*
 * LDST
 *
 * SYNOPSIS:
 *     void LDST(state t *statep)
 *
 * This function forces the complete reload of processor state from
 * vector state area pointed by argument in $a0 (a0): it works only in
 * kernel mode. There is  no real return: $a1 is used as BIOS
 * argument, but it is reloaded too.
 */
LEAF_FUNC(LDST)
	move	$a1, $a0
	
	.set noreorder
	.set nomacro
	li	$a0, BIOS_SRV_LDST
	break
	nop
	.set reorder
	.set macro

	jr	$ra
END_LEAF_FUNC(LDST)

/*
 * INITCPU
 *
 * SYNOPSIS:
 *     void INITCPU(u32 cpu_id, state t *start_state, state_t *state_areas)
 */
LEAF_FUNC(INITCPU)
	li	$t0, MCTL_BOOT_PC
	li	$t1, 0x00000100
	sw	$t1, 0($t0)

	/* Compute cpu scratch area base address in the ROM reserved
	   frame ($t0) */
	li	$t0, BIOS_CPU_SCRATCH_RAM_SIZE
	mult	$t0, $a0
	mflo	$t0
	li	$t1, BIOS_LEGACY_SCRATCH_BASE
	subu	$t0, $t1, $t0

	/* Stage initial state & old/new state area array addresses at
	   the beginning of the cpu scratch area in the RRF */
	sw	$a1, 4($t0)
	sw	$a2, 8($t0)

	.set	noreorder
	li	$t0, MCTL_RESET_CPU
	sw	$a0, 0($t0)
	.set	reorder

	jr	$ra
END_LEAF_FUNC(INITCPU)
