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

/****************************************************************************
 *
 * This support module contains some functions used to convert MIPS
 * instructions in human-readable form. They are used by objdump utility 
 * and Watch class. 
 * It also contains some utility functions for instruction decoding and sign
 * extension of halfword quantities.
 * 
 ****************************************************************************/

#include <stdio.h>
#include <cstring>

#include <umps/const.h>
#include "umps/types.h"
#include "umps/processor_defs.h"

#include "umps/arch.h"

// register names
HIDDEN const char* const regName[CPUREGNUM] = {
    "00",
    "at",
    "v0",
    "v1",
    "a0",
    "a1",
    "a2",
    "a3",
    "t0",
    "t1",
    "t2",
    "t3",
    "t4",
    "t5",
    "t6",
    "t7",
    "s0",
    "s1",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "t8",
    "t9",
    "k0",
    "k1",
    "gp",
    "sp",
    "s8",
    "ra",
    "HI",
    "LO"
};

// CP0 register names
HIDDEN const char* const cp0RegName[CP0REGNUM] = {
    "Index",
    "Random",
    "EntryLo",
    "BadVAddr",
    "EntryHi",
    "Status",
    "Cause",
    "EPC",
    "PRId"
};

// instruction mnemonic names split by type: array is indexed by opcode
// (this explains the empty spaces)

// register instructions
HIDDEN const char* const regIName[] = {
    "sll",
    "",
    "srl",
    "sra",
    "sllv",
    "",
    "srlv",
    "srav",
    "jr",
    "jalr",
    "",
    "",
    "syscall",
    "break",
    "",
    "",
    "mfhi",
    "mthi",
    "mflo",
    "mtlo",
    "",
    "",
    "",
    "",
    "mult",
    "multu",
    "div",
    "divu",
    "",
    "",
    "",
    "",
    "add",
    "addu",
    "sub",
    "subu",
    "and",
    "or",
    "xor",
    "nor",
    "",
    "",
    "slt",
    "sltu",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "nop",
    "ri/ill (r-type)"
};

// position for NOP (since its opcode clashes with SLL) 
#define NOPINAME	62

// reserved instruction string position for regIName array
#define RIREGINAME	63


// immediate/branch/load/store instructions
HIDDEN const char* const iName[] = {
    "bltz",
    "bgez",
    "j",
    "jal",
    "beq",
    "bne",
    "blez",
    "bgtz",
    "addi",
    "addiu",
    "slti",
    "sltiu",
    "andi",
    "ori",
    "xori",
    "lui",
    "bltzal",
    "bgezal",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "lb",
    "lh",
    "lwl",
    "lw",
    "lbu",
    "lhu",
    "lwr",
    "",
    "sb",
    "sh",
    "swl",
    "sw",
    "",
    "",
    "swr",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "ri/ill (unknown)",
    "ri/ill (R4000)",
    "ri/ill (load/store)",
    "ri/ill (branch type)",
    "ri/ill (immediate)"
};

// string positions for illegal opcodes by type in previous array
#define RIIMMINAME		63
#define RIBRANCHINAME	62
#define RILSINAME		61
#define RIR40INAME		60
#define RIUNKINAME		59


// coprocessor instructions
HIDDEN const char* const cp0IName[] = {
    "mfc0",
    "tlbr",
    "tlbwi",
    "bc0f",
    "mtc0",
    "bc0t",
    "tlbwr",
    "rfe",
    "tlbp",
    "ri/ill/unimp"
};

// string positions for peculiar istructions (decoding for them is peculiar)
#define RFEINAME	7
#define BC0FINAME	3
#define BC0TINAME	5
#define CPUILLINAME	9


// static string buffer size and definition
#define STRBUFSIZE	64
HIDDEN char strbuf[STRBUFSIZE];


// utility functions for instruction decoding
HIDDEN void strRegInstr(Word instr);
HIDDEN void strImmInstr(Word instr);
HIDDEN void strBranchInstr(Word instr);
HIDDEN void strLSInstr(Word instr);
HIDDEN void strCopInstr(Word instr);


/****************************************************************************/
/* Definitions to be exported.                                              */
/****************************************************************************/

// This function decodes and returns the opcode type for an instruction
unsigned int OpType(Word instr)
{
	if (OPCODE(instr) == 0UL)
	// SPECIAL field is 0: istruction type is REGTYPE
		return(REGTYPE);
	else
		return((instr & OPTYPEMASK) >> OPCODEOFFS);
}

// This function sign-extends an halfword quantity to a full word for
// 2-complement arithmetics
SWord SignExtImm(Word instr)
{
	// computes sign bit value
	if (((instr >> IMMSIGNPOS) & 0x1UL))
		return(instr | ~(IMMMASK));
	else
		return(ZEXTIMM(instr));
}

// This function detects invalid formats for register type instructions,
// checking if fields are set appropriately to zero where needed.
// Returns FALSE if instruction is valid, and a non-zero value otherwise
bool InvalidRegInstr(Word instr)
{
	bool invalid = false;
	
	switch (FUNCT(instr))
	{
		case ADD:
		case ADDU:
		case AND:
		case NOR:
		case OR:
		case SLLV:
		case SLT:
		case SLTU:		
		case SRAV:
		case SRLV:
		case SUB:
		case SUBU:
		case XOR:
			invalid = SHAMT(instr);
			break;
		
		case SLL:
		case SRA:
		case SRL:
			invalid = RS(instr);
			break;
			
		case DIV:
		case DIVU:
		case MULT:
		case MULTU:
			invalid = SHAMT(instr) || RD(instr);
			break;			
		
		case JALR:
			invalid = SHAMT(instr) || RT(instr);
			break;

		case MFHI:
		case MFLO:
			invalid = SHAMT(instr) || RT(instr) || RS(instr);
			break;
			
		case JR:
		case MTHI:
		case MTLO:
			invalid = SHAMT(instr) || RT(instr) || RD(instr);
			break;
			
		case BREAK:
		case SYSCALL:
		default:
			break;
	}
	
	return(invalid);
}


// This function maps the MIPS R2/3000 CP0 register codes to simulator
// internal codes and returns TRUE if the register is implemented,
// FALSE otherwise
bool ValidCP0Reg(unsigned int regnum, unsigned int* cpnum)
{
    bool ret = true;

    switch(regnum) {
    case CP0_Index:
        *cpnum = INDEX;
        break;

    case CP0_Random:
        *cpnum = RANDOM;
        break;

    case CP0_EntryLo:	
        *cpnum = ENTRYLO;
        break;

    case CP0_BadVAddr:
        *cpnum = BADVADDR;
        break;

    case CP0_EntryHi:
        *cpnum = ENTRYHI;
        break;

    case CP0_Status:	
        *cpnum = STATUS;
        break;

    case CP0_Cause:
        *cpnum = CAUSE;
        break;

    case CP0_EPC:
        *cpnum = EPC;
        break;

    case CP0_PRID:
        *cpnum = PRID;
        break;

    default:
        // invalid register
        ret = false;
        *cpnum = PRID;
        // this way, bad function use doesn't touch critical CP0 reg
        break;
    }

    return ret;
}

// This function returns main processor's register name indexed by position
const char* RegName(unsigned int index)
{
    if (index < CPUREGNUM)
        return regName[index];
    else 
        return EMPTYSTR;
}

// This function returns CP0 register name indexed by position
const char* CP0RegName(unsigned int index)
{
    if (index < CP0REGNUM)
        return cp0RegName[index];
    else
        return "";
}

// this function returns the pointer to a static buffer which contains
// the instruction translation into readable form 
const char* StrInstr(Word instr)
{
    switch(OpType(instr)) {
    case REGTYPE:
        strRegInstr(instr);
        break;

    case IMMTYPE:	
        strImmInstr(instr);
        break;

    case BRANCHTYPE:
        strBranchInstr(instr);
        break;					

    case LOADCOPTYPE:
    case STORECOPTYPE:
    case COPTYPE:
        strCopInstr(instr);
        break;

    case LOADTYPE:
    case STORETYPE:
        strLSInstr(instr);
        break;					

    default:
        // unknown instruction (generic)
        sprintf(strbuf, "%s", iName[RIUNKINAME]);
        break;
    }

    return strbuf;
}


/****************************************************************************/
/* Definitions strictly local to the module.                                */
/****************************************************************************/

// This function decodes and puts into static buffer the human
// readable form for MIPS register type instructions
HIDDEN void strRegInstr(Word instr)
{
    if (InvalidRegInstr(instr))
        sprintf(strbuf, "%s", regIName[RIREGINAME]);
    else {
        // instruction format is correct		
        switch(FUNCT(instr)) {
        case ADD:
        case ADDU:
        case AND:
        case NOR:
        case OR:
        case SLLV:
        case SLT:
        case SLTU:		
        case SRAV:
        case SRLV:
        case SUB:
        case SUBU:
        case XOR:
            sprintf(strbuf, "%s\t$%s, $%s, $%s",
                    regIName[FUNCT(instr)],
                    regName[RD(instr)],
                    regName[RS(instr)],
                    regName[RT(instr)]);
            break;

        case SLL:
        case SRA:
        case SRL:
            if (instr == NOP)
                sprintf(strbuf, "%s", regIName[NOPINAME]);
            else
                sprintf(strbuf, "%s\t$%s, $%s, %.2lu",
                        regIName[FUNCT(instr)],
                        regName[RD(instr)],
                        regName[RT(instr)],
                        SHAMT(instr));
            break;

        case DIV:
        case DIVU:
        case MULT:
        case MULTU:
            sprintf(strbuf, "%s\t$%s, $%s",
                    regIName[FUNCT(instr)], regName[RS(instr)], regName[RT(instr)]); 
            break;

        case JALR:
            sprintf(strbuf, "%s\t$%s, $%s",
                    regIName[FUNCT(instr)], regName[RD(instr)], regName[RS(instr)]);
            break;

        case MFHI:
        case MFLO:
            sprintf(strbuf, "%s\t$%s", regIName[FUNCT(instr)], regName[RD(instr)]);
            break;

        case JR:
        case MTHI:
        case MTLO:
            sprintf(strbuf, "%s\t$%s", regIName[FUNCT(instr)], regName[RS(instr)]);
            break;

        case BREAK:
        case SYSCALL:
            sprintf(strbuf, "%s\t(%lu)", regIName[FUNCT(instr)], CALLVAL(instr));
            break;

        default:
            // unknown instruction
            sprintf(strbuf, "%s", regIName[RIREGINAME]);
            break;
        }
    }
}


// This function decodes and puts into static buffer the human readable form
// for MIPS immediate type instructions
HIDDEN void strImmInstr(Word instr)
{
    switch(OPCODE(instr)) {
    case ADDI:
    case ADDIU:
    case SLTI:
    case SLTIU:
        sprintf(strbuf, "%s\t$%s, $%s, %+ld",
                iName[OPCODE(instr)],
                regName[RT(instr)],
                regName[RS(instr)],
                SignExtImm(instr));
        break;

    case ANDI:
    case ORI:
    case XORI:
        sprintf(strbuf, "%s\t$%s, $%s, 0x%.8lX",
                iName[OPCODE(instr)],
                regName[RT(instr)],
                regName[RS(instr)],
                ZEXTIMM(instr));
        break;

    case LUI:
        if (!RS(instr))
            sprintf(strbuf, "%s\t$%s, 0x%.4lX",
                    iName[OPCODE(instr)], regName[RT(instr)], ZEXTIMM(instr));
        else
            // instruction is ill-formed
            sprintf(strbuf, "%s", iName[RIIMMINAME]);
        break;

    default:
        sprintf(strbuf, "%s", iName[RIIMMINAME]);
        break;
    }
}


// This function decodes and puts into static buffer the human readable form
// for MIPS branch type instructions
HIDDEN void strBranchInstr(Word instr)
{
    switch (OPCODE(instr)) {
    case BEQ:
    case BNE:
        sprintf(strbuf, "%s\t$%s, $%s, %+ld",
                iName[OPCODE(instr)],
                regName[RS(instr)],
                regName[RT(instr)],
                SignExtImm(instr) << WORDSHIFT);
        break;

    case BGL:
        // uses RT field to choose which branch type is requested
        switch (RT(instr))
        {
        case BGEZ:
        case BGEZAL:
        case BLTZ:
        case BLTZAL:
            sprintf(strbuf, "%s\t$%s, %+ld",
                    iName[RT(instr)],
                    regName[RS(instr)],
                    SignExtImm(instr) << WORDSHIFT);
            break;			

        default:
            // unknown instruction of this type
            sprintf(strbuf, "%s", iName[RIBRANCHINAME]);
            break;
        }
        break;

    case BLEZ:
    case BGTZ:
        if (!RT(instr))
            // instruction is well formed
            sprintf(strbuf, "%s\t$%s, %+ld",
                    iName[OPCODE(instr)],
                    regName[RS(instr)],
                    SignExtImm(instr) << WORDSHIFT);
        else
            // istruction is ill-formed
            sprintf(strbuf, "%s", iName[RIBRANCHINAME]);
        break;

    case J:
    case JAL:
        sprintf(strbuf, "%s\t(4 bit PC)+0x%.7lX", iName[OPCODE(instr)], (instr & ~(OPCODEMASK)) << WORDSHIFT);
        break;

    default:
        sprintf(strbuf, "%s", iName[RIBRANCHINAME]);
        break;
    }
}


// This function decodes and puts into static buffer the human readable form
// for MIPS load/store type instructions
HIDDEN void strLSInstr(Word instr)
{
    switch (OPCODE(instr)) {	
    case LB:
    case LBU: 
    case LH:
    case LHU:
    case LW:
    case LWL:
    case LWR:
    case SB:
    case SH:
    case SW:
    case SWL:
    case SWR:
        sprintf(strbuf, "%s\t$%s, %+ld ($%s)",
                iName[OPCODE(instr)], regName[RT(instr)], SignExtImm(instr), regName[RS(instr)]);
        break;
		
    default:
        sprintf(strbuf, "%s", iName[RILSINAME]);
        break;
    }
}


// This function decodes and puts into static buffer the human readable form
// for MIPS coprocessor handling instructions
HIDDEN void strCopInstr(Word instr)
{
    unsigned int cp0Num;

    if (OPCODE(instr) == COP0SEL) {
        // COPOPTYPE corresponds to RS field
        switch(COPOPTYPE(instr)) {
        case CO0:
            // coprocessor 0 operations
            if (RT(instr) || RD(instr) || SHAMT(instr))
                // instruction is ill-formed
                sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
            else
                switch(FUNCT(instr))
                {
                case RFE:
                    sprintf(strbuf, "%s", cp0IName[RFEINAME]);
                    break;

                case TLBP:
                case TLBR:
                case TLBWI:
                case TLBWR:
                    sprintf(strbuf, "%s", cp0IName[FUNCT(instr)]);
                    break;

                case COFUN_WAIT:
                    std::strcpy(strbuf, "wait");
                    break;

                default:
                    // unknown coprocessor 0 operation requested
                    sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
                    break;
                }
            break;

        case BC0:
            switch(COPOPCODE(instr))
            {
            case BC0F:
                sprintf(strbuf, "%s\t%+ld", cp0IName[BC0FINAME], SignExtImm(instr) << WORDSHIFT);
                break;

            case BC0T:
                sprintf(strbuf, "%s\t%+ld", cp0IName[BC0TINAME], SignExtImm(instr) << WORDSHIFT);
                break;

            default:
                // traps BC0FL R4000 instructions
                // and the like...
                sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
                break;
            }
            break;

        case MFC0:
            // valid instruction has SHAMT and FUNCT fields
            // set to 0, and refers to a valid CP0 register
            if (ValidCP0Reg(RD(instr), &cp0Num) && !SHAMT(instr) && !FUNCT(instr))
                // istruction is valid
                sprintf(strbuf, "%s\t$%s, $%s", cp0IName[COPOPTYPE(instr)], regName[RT(instr)], cp0RegName[cp0Num]);
            else
                // invalid instruction format or CP0 reg
                sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
            break;

        case MTC0:
            // valid instruction has SHAMT and FUNCT fields
            // set to 0, and refers to a valid CP0 register
            if (ValidCP0Reg(RD(instr), &cp0Num) && !SHAMT(instr) && !FUNCT(instr))
                // valid instr.
                sprintf(strbuf, "%s\t$%s, $%s", cp0IName[COPOPTYPE(instr)], regName[RT(instr)], cp0RegName[cp0Num]);
            else
                // TLBCLR backpatch
                if (RD(instr) == CONTEXTREG)
                    sprintf(strbuf, "TLBCLR");
                else 
                    // invalid instr.
                    sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
            break;

        default:
            // unknown and CFC0, CTC0, COP0, LWC0 generic instructions
            sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
            break;
        }
    } else {
        if (((instr >> DWCOPBITPOS) & 0x1UL))
            // LDC, SDC, BEQL reserved instructions handling
            sprintf(strbuf, "%s", iName[RIR40INAME]);
        else
            // LWC, SWC instr. handling:
            // these ops are invalid for CP0 and
            // other coprocessors are unavailable
            sprintf(strbuf, "CP%.1lu %s", COPNUM(instr), cp0IName[CPUILLINAME]);
    }
}
