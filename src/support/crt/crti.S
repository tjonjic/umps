/* -*- mode: asm; tab-width: 8; indent-tabs-mode: t -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2011 Tomislav Jonjic
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
 * This module contains startup code for non-PIC o32 .aout
 * executables.
 *
 * PLEASE NOTE:
 *
 * - The stack pointer ($sp) must be initialized by the kernel!
 *
 * - The TERMINATE_SYSCALL (see below) must be set to the appropriate
 *   code, if the program is ever expected to return from main().
 */

#include <umps/regdef.h>
#include <umps/aout.h>

#define TEXT_VADDR 0x80000000

/* Set this to a suitable value */
#define TERMINATE_SYSCALL 0

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

	/* Initialize the $gp register. The initial gp value is found
	   in the .aout header (and also in the linker-defined `_gp' symbol). */
	li	$t0, TEXT_VADDR + (AOUT_HE_GP_VALUE << 2)
	lw	$gp, 0($t0)

	/* Call main */
	addiu	$sp, $sp, -16
	jal	main
	addiu	$sp, $sp, 16

	/* Invoke "terminate" syscall */
	li	$a0, TERMINATE_SYSCALL
	syscall

	/* Not reached! */

	.end	__start
	.size	__start, .-__start
