/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */

#include "umps/aout.h"
#include "umps/arch.h"
#include "regdef.h"
#include "h/bios_srv.h"

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
	li	$k0, AOUT_GP_VALUE
	sll	$k0, $k0, 2
	li	$k1, CORE_TEXT_VADDR
	addu	$k1, $k1, $k0
	lw	$gp, 0($k1)

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
