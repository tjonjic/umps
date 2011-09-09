/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include <umps/regdef.h>
#include <umps/arch.h>
#include <umps/aout.h>
#include <umps/bios_defs.h>

#define CORE_TEXT_VADDR 0x20001000

	.text
	.align	2

	.globl	__start
	.type	__start, @function
	.ent	__start
	.extern	main
__start:
	.frame	$sp, 16, $ra
	.mask	0x00000000, 0
	.fmask	0x00000000, 0

	/* computes stack bottom (last address of physical RAM)
	   using $k0 and $k1 (k0 and k1) */
	lw	$k0, BUS_REG_RAM_BASE
	lw	$k1, BUS_REG_RAM_SIZE
	addu	$k0, $k0, $k1
	addiu	$sp, $k0, -8

	/* Initialize $gp */
	lw	$gp, CORE_TEXT_VADDR + (AOUT_HE_GP_VALUE << 2)

	addiu	$sp, $sp, -16
	jal	main
	addiu	$sp, $sp, 16

	.set	noreorder
	.set	nomacro
	li 	$a0, BIOS_SRV_HALT
	nop
	break
	nop
	.set	reorder
	.set	macro

	.end	__start
	.size	__start, .-__start
