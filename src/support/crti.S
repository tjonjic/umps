 #$Id: crti.s,v 1.1 2004/04/13 14:11:58 morsiani Exp $
	
#include "regdef.h"

	.abicalls

 # GCC automatic generated header
 
 # GNU C 2.7.2 [AL 1.1, MM 40] Linux/MIPSEL compiled by GNU C

 # Cc1 defaults:
 # -mgas -mabicalls

 # Cc1 arguments (-G value = 0, Cpu = 3000, ISA = 1):
 # -quiet -dumpbase -version -o
 # 

	#this constant has to be set to a kernel-specific value
	TERMINATESYS = 0

	# remember that $sp has to be set by kernel too
	
 	# useful constants
	PROGVSTART	= 0x80000004
	GPSTARTADDR	= 0x800000A8
	
	# This is the standard __start function for generic program 
	# activation: it loads $t9 with starting address,
	# $gp with base value for $gp computations, and 
	# at main() function return calls kernel TERMINATE service  
	# (a SYSCALL with $v0 == TERMINATE)

	.text
	.align	2
	.globl	__start
	.type	__start,@function
	.ent	__start
	.extern main 

__start:
	.frame	$fp,16,$ra		# vars= 0, regs= 2/0, args= 0, extra= 8
	.mask	0x50000000,-4
	.fmask	0x00000000,0

	# $sp has to be set to correct value by kernel 
	li 	$t8, PROGVSTART
	lw	$t9, 0($t8)
	li  $t8, GPSTARTADDR
	lw	$gp, 0($t8)

	subu	$sp,$sp,16
	.cprestore 0
	sw	$fp,12($sp)
	sw	$gp,8($sp)
	move	$fp,$sp

	jal	main

	move	$sp,$fp	
	lw	$fp,12($sp)
	addu	$sp,$sp,16

	.set noreorder
	.set nomacro

	# calls kernel TERMINATE service
	li	$v0, TERMINATESYS 
	nop	
	syscall
	nop
	.set reorder
	.set macro	
	.end __start

L__startEnd:
	.size	__start, L__startEnd - __start



