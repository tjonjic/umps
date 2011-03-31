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

#ifndef UMPS_DISASSEMBLE_H
#define UMPS_DISASSEMBLE_H

// This function decodes and returns the opcode type for an instruction
unsigned int OpType(Word instr);

// This function sign-extends an halfword quantity to a full word for
// 2-complement arithmetics
SWord SignExtImm(Word instr);

// This function detects invalid formats for register type instructions,
// checking if fields are set appropriately to zero where needed.
// Returns FALSE if instruction is valid, and a non-zero value otherwise
bool InvalidRegInstr(Word instr);

// This function maps the MIPS R2/3000 CP0 register codes to simulator internal
// codes and returns TRUE if the register is implemented, FALSE otherwise
bool ValidCP0Reg(unsigned int regnum, unsigned int * cpnum);

// This function returns main processor's register name indexed by position
const char * RegName(unsigned int index);

// This function returns CP0 register name indexed by position
const char * CP0RegName(unsigned int index);

// this function returns the pointer to a static buffer which contains
// the instruction translation into readable form

const char * StrInstr(Word instr);

const char* InstructionMnemonic(Word ins);

#endif // UMPS_DISASSEMBLE_H
