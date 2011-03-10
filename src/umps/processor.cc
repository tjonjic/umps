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

/*
 * This module implements the Processor class. A Processor object emulates
 * MIPS R2/3000 processor features almost perfectly. It is linked to a
 * SystemBus object for physical memory accesses.
 *
 * MIPS processor features are too complex to be fully descripted
 * here: refer to external documentation.
 *
 * This module also contains TLBEntry class definition: it is used to
 * streamline TLB build and handling in Processor.
 */

#include "umps/processor.h"

#include <stdio.h>
#include <cassert>
#include <iostream>

#include <umps/const.h>
#include "umps/processor_defs.h"

#include "umps/systembus.h"
#include "umps/utility.h"

#include "umps/machine_config.h"
#include "umps/error.h"
#include "umps/disassemble.h"


// Names of exceptions
HIDDEN const char* const excName[] = {
    "NO EXCEPTION",
    "INT",
    "MOD",
    "TLBL Refill",
    "TLBL",
    "TLBS Refill",
    "TLBS",
    "ADEL",
    "ADES",
    "DBE",
    "IBE",
    "SYS",
    "BP",
    "RI",
    "CPU",
    "OV"
};

// exception code table (each corresponding to an exception cause);
// each exception cause is mapped to one exception type expressed in 
// CAUSE register field format 
HIDDEN const Word excCode[] = {
    0UL,
    0UL,
    1UL,
    2UL,
    2UL,
    3UL,
    3UL,
    4UL,
    5UL,
    6UL,
    7UL,
    8UL,
    9UL,
    10UL,
    11UL,
    12UL
};


// Each TLBEntry object represents a single entry in the TLB contained in
// the CP0 coprocessor part of a real MIPS processor.
// Each one is a 64-bit field split in two parts (HI and LO), with special
// fields and control bits (see external documentation for more details)
// This class is local to this module, but must be defined before Processor, 
// for compiling reasons

class TLBEntry {
public:
    // This method builds an entry and sets its initial contents	
    TLBEntry(Word entHI = 0, Word entLO = 0);

    // This method returns the HI 32-bit part of the entry
    Word getHI() const { return tlbHI; }

    // This method returns the LO 32-bit part of the entry
    Word getLO() const { return tlbLO; }

    // This method sets the entry HI part (leaving the zero-filled
    // field untouched)
    void setHI(Word entHI);

    // This method sets the entry LO part (leaving the zero-filled field
    // untouched)
    void setLO(Word entLO);

    // This method compares the entry contents with the VPN part of a
    // virtual address and returns TRUE if a match is found, FALSE
    // otherwise
    bool VPNMatch(Word vaddr);

    // This method compares the entry contents with the ASID field in a
    // CP0 special register and returns TRUE if a match is found, FALSE
    // otherwise
    bool ASIDMatch(Word cpreg);

    // the following methods return the bit value for the corresponding 
    // access control bit
    bool IsG();
    bool IsV();
    bool IsD();
				
private:
    // contains the VPN + ASID fields, and a zero-filled field
    Word tlbHI;

    // contains the PFN field, some access control bits and a
    // zero-filled field
    Word tlbLO;
};

// This method builds an entry and sets its initial contents
TLBEntry::TLBEntry(Word entHI, Word entLO)
{
    tlbHI = entHI;
    tlbLO = entLO;
}

// This method sets the entry HI part (leaving the zero-filled field untouched)
void TLBEntry::setHI(Word entHI)
{
    tlbHI = (entHI & (VPNMASK | ASIDMASK));
}

// This method sets the entry LO part (leaving the zero-filled field untouched)
void TLBEntry::setLO(Word entLO)
{
    tlbLO = (entLO & ENTRYLOMASK);
}

// This method compares the entry contents with the VPN part of a virtual
// address and returns TRUE if a match is found, FALSE otherwise
bool TLBEntry::VPNMatch(Word vaddr)
{
    return VPN(tlbHI) == VPN(vaddr);
}

// This method compares the entry contents with the ASID field in a CP0
// special register and returns TRUE if a match is found, FALSE otherwise
bool TLBEntry::ASIDMatch(Word cpreg)
{
    return ASID(tlbHI) == ASID(cpreg);
}

// This method returns the value of G (Global) access control bit in an
// entry LO part
bool TLBEntry::IsG()
{
    return BitVal(tlbLO, GBITPOS);
}

// This method returns the value of V (Valid) access control bit in an
// entry LO part
bool TLBEntry::IsV()
{
    return BitVal(tlbLO, VBITPOS);
}

// This method returns the value of D (Dirty) access control bit in an
// entry LO part
bool TLBEntry::IsD()
{
    return BitVal(tlbLO, DBITPOS);
}


Processor::Processor(const MachineConfig* config, Word cpuId, Machine* machine, SystemBus* bus)
    : id(cpuId),
      machine(machine),
      bus(bus),
      status(PS_HALTED),
      tlbSize(config->getTLBSize()),
      tlb(new TLBEntry[tlbSize])
{}

Processor::~Processor() {}

ProcessorStatus Processor::getStatus()
{
    return status;
}

void Processor::Start()
{
    assert(status == PS_HALTED);
    reset();
    status = PS_RUNNING;
    SignalStatusChanged.emit();
}

// This method makes Processor execute a single instruction.
// For simulation purposes, it differs from traditional processor cycle:
// the first instruction after a reset is pre-loaded, and cycle is
// execute - fetch instead of fetch - execute.
// This way, it is possible to know what instruction will be executed 
// before its execution happens
//
// Method works as follows: 
// the instruction currently loaded is executed; if no exception occurs,
// interrupt status is checked to see if an INT exception has to be
// processed; if no interrupts are pending, next instruction is located and
// fetched, so Processor is ready for another Cycle(). If an exception
// occurs, first instruction at vector address is loaded, so a new Cycle()
// may start.
// Other MIPS-specific minor tasks are performed at proper points.
void Processor::Cycle()
{
    assert(status == PS_RUNNING);

    // decode & exec instruction
    if (execInstr(currInstr))
        // exception caused from instruction execution
        handleExc();

    // PC saving for book-keeping purposes	
    prevPC = currPC;
    prevPhysPC = currPhysPC;
    prevInstr = currInstr;

    // RANDOM register is decremented
    randomRegTick();

    // currPC is loaded so a new cycle fetch may start: this "PC stack" is
    // used to emulate delayed branch slots
    currPC = nextPC;
    nextPC = succPC;
    succPC += WORDLEN;

    // check for interrupt exception
    if (checkForInt())
        handleExc();

    // processor cycle fetch part
    if (mapVirtual(currPC, &currPhysPC, EXEC)) {
        // TLB or Address exception caused: current instruction is nullified
        currInstr = NOP;		
        handleExc();
    } else if (bus->InstrRead(currPhysPC, &currInstr, this)) {
        // IBE exception caused: current instruction is nullified
        currInstr = NOP;
        handleExc();
    }
}

// This method allows SystemBus and Processor itself to signal Processor
// when an exception happens. SystemBus signal IBE/DBE exceptions; Processor
// itself signal all other kinds of exception. 
// 
// Method works as follows:
// the exception internal code is put into proper Processor private
// variable(s), and is signaled to Watch (so Watch may stop simulation if
// needed). 
// Exception processing is done by Processor private method handleExc()
void Processor::SignalExc(unsigned int exc, Word cpuNum)
{
    excCause = exc;
    SignalException.emit(excCause);
    // used only for CPUEXCEPTION handling
    copENum = cpuNum;
}

// This method allows to get critical information on Processor current
// internal status. Parameters are self-explanatory: they are extracted from
// proper places inside Processor itself
void Processor::getCurrStatus(Word * asid, Word * pc, Word * instr, bool * isLD, bool * isBD, bool * isVM)
{
    *asid = (ASID(cpreg[ENTRYHI])) >> ASIDOFFS;
    *pc = currPC;
    *instr = currInstr;
    *isLD = (loadPending != LOAD_TARGET_NONE);
    *isBD = isBranchD; 
    *isVM = BitVal(cpreg[STATUS], VMCBITPOS);
}

Word Processor::getASID() const
{
    return ASID(cpreg[ENTRYHI]) >> ASIDOFFS;
}

bool Processor::getVM() const
{
    return BitVal(cpreg[STATUS], VMCBITPOS);
}

bool Processor::InUserMode() const
{
    return BitVal(cpreg[STATUS], KUCBITPOS);
}

bool Processor::InKernelMode() const
{
    return (!BitVal(cpreg[STATUS], KUCBITPOS));
}

// This method allows to get Processor previously executed instruction and 
// location
void Processor::getPrevStatus(Word * pc, Word * instr)
{
    *pc = prevPC;
    *instr = prevInstr; 
}

// This method allows to get a human-readable mnemonic expression for the last
// exception happened (thanks to excName[] array)
const char* Processor::getExcCauseStr()
{
    // 0 means no exception
    if (excCause)
        return excName[excCause];
    else
        return (EMPTYSTR);
}

// This method allows to get the physical location of instruction executed
// in the previous Processor Cycle()
Word Processor::getPrevPPC()
{
    return(prevPhysPC);
}

// This method allows to get the physical location of instruction executed
// in the current Processor Cycle()
Word Processor::getCurrPPC()
{
    return(currPhysPC);
}


// This method allows to get the virtual location of instruction that will
// be executed in the next Processor Cycle()
Word Processor::getNextPC()
{
    return(nextPC);
}

// This method allows to get the virtual location of instruction that will
// be executed in the Processor Cycle() _after_ the next
Word Processor::getSuccPC()
{
    return(succPC);
}

// This method allows to get the value of the general purpose register
// indexed by num (HI and LO are the last ones in the array)
SWord Processor::getGPR(unsigned int num)
{
    return(gpr[num]);
}

// This method allows to get the value of the CP0 special register indexed
// by num. num coding itself is internal (see h/processor.h for mapping)
Word Processor::getCP0Reg(unsigned int num)
{
    return(cpreg[num]);
}

void Processor::getTLB(unsigned int index, Word* hi, Word* lo) const
{
    *hi = tlb[index].getHI();
    *lo = tlb[index].getLO();
}

Word Processor::getTLBHi(unsigned int index) const
{
    return tlb[index].getHI();
}

Word Processor::getTLBLo(unsigned int index) const
{
    return tlb[index].getLO();
}

// This method allows to modify the current value of a general purpose
// register (HI and LO are the last ones in the array)
void Processor::setGPR(unsigned int num, SWord val)
{
    if (num > 0)
        // register $0 is read-only
        gpr[num] = val;
}

// This method allows to modify the current value of a CP0 special 
// register. num coding itself is internal (see h/processor.h for mapping)
void Processor::setCP0Reg(unsigned int num, Word val)
{
    if (num < CP0REGNUM)
        cpreg[num] = val;			
}

// This method allows to modify the current value of nextPC to force sudden
// branch: it violates delayed branch slot conventions
void Processor::setNextPC(Word npc)
{
    nextPC = npc;
}


// This method allows to modify the current value of succPC to force sudden
// branch: it does not violate branch slot conventions, if used to change
// the target of a branch already taken
void Processor::setSuccPC(Word spc)
{
    succPC = spc;
}

// This method allows to modify the current value of a specified TLB entry
void Processor::setTLB(unsigned int index, Word hi, Word lo)
{
    if (index < tlbSize) {
        tlb[index].setHI(hi);
        tlb[index].setLO(lo);
        SignalTLBChanged(index);
    } else {
        Panic("Unknown TLB entry in Processor::setTLB()");
    }
}

void Processor::setTLBHi(unsigned int index, Word value)
{
    assert(index < tlbSize);
    tlb[index].setHI(value);
    SignalTLBChanged(index);
}

void Processor::setTLBLo(unsigned int index, Word value)
{
    assert(index < tlbSize);
    tlb[index].setLO(value);
    SignalTLBChanged(index);
}


//
// Processor private methods start here
//

// This method puts Processor in startup state. This is done following the
// MIPS conventions on register fields to be set; it also pre-loads the
// first instruction since Cycle() goes on with execute-load loop
void Processor::reset()
{
    unsigned int i;

    // first instruction is not in a branch delay slot
    isBranchD = false;

    // no exception pending at start	
    excCause = NOEXCEPTION;
    copENum = 0;

    // no loads pending at start
    loadPending = LOAD_TARGET_NONE;
    loadReg = 0;
    loadVal = MAXWORDVAL;

    // clear general purpose registers
    for (i = 0; i < CPUREGNUM; i++)
        gpr[i] = 0;

    // no previous instruction is available
    prevPC = MAXWORDVAL;
    prevPhysPC = MAXWORDVAL;
    prevInstr = NOP;

    // clears CP0 registers and then sets them
    for (i = 0; i < CP0REGNUM; i++)
        cpreg[i] = 0UL;

    // first instruction is already loaded
    cpreg[RANDOM] =  ((tlbSize - 1UL) << RNDIDXOFFS) - RANDOMSTEP;
    cpreg[STATUS] = STATUSRESET;
    cpreg[PRID] = PRIDREGVAL;

#if 0
    // execution starts from specific reset vector in kseg0 segment	
    currPC = BOOTBASE;
#endif
    if (bus->DataRead(MPC_CPU_BOOT_PC, &currPC, this))
        Panic("FIXME");

    // maps PC to physical address space and fetches first instruction
    // mapVirtual and SystemBus cannot signal TRUE on this call
    if (mapVirtual(currPC, &currPhysPC, EXEC) || bus->InstrRead(currPhysPC, &currInstr, this))
        Panic("Illegal memory access in Processor::Reset");

    // sets values for following PCs
    nextPC = currPC + WORDLEN;
    succPC = nextPC + WORDLEN;
}


// This method advances CP0 RANDOM register, following MIPS conventions; it
// cycles from RANDOMTOP to RANDOMBASE, one STEP less for each clock tick
void Processor::randomRegTick()
{
    cpreg[RANDOM] = (cpreg[RANDOM] - RANDOMSTEP) & (((tlbSize - 1UL) << RNDIDXOFFS));
    if (cpreg[RANDOM] < RANDOMBASE)
        cpreg[RANDOM] =  ((tlbSize - 1UL) << RNDIDXOFFS);
}


// This method pushes the KU/IE and VM bit stacks in CP0 STATUS register to start
// exception handling
void Processor::pushKUIEVMStack()
{
    unsigned int bitp;
	
    // push the KUIE stack
    for (bitp = KUOBITPOS; bitp > KUCBITPOS; bitp--)
        if (BitVal(cpreg[STATUS], bitp - 2))
            cpreg[STATUS] = SetBit(cpreg[STATUS], bitp);
        else
            cpreg[STATUS] = ResetBit(cpreg[STATUS], bitp);

    // push the VM stack
    for (bitp = VMOBITPOS; bitp > VMCBITPOS; bitp--)
        if (BitVal(cpreg[STATUS], bitp - 1))
            cpreg[STATUS] = SetBit(cpreg[STATUS], bitp);
        else
            cpreg[STATUS] = ResetBit(cpreg[STATUS], bitp);
			
    // sets to 0 current KU IE VM bits
    cpreg[STATUS] = ResetBit(cpreg[STATUS], KUCBITPOS);
    cpreg[STATUS] = ResetBit(cpreg[STATUS], IECBITPOS);
    cpreg[STATUS] = ResetBit(cpreg[STATUS], VMCBITPOS);
}


// This method pops the KU/IE and VM bit stacks in CP0 STATUS register to end
// exception handling. It is invoked on RFE instruction execution
void Processor::popKUIEVMStack()
{
    unsigned int bitp;
	
    for (bitp = IECBITPOS; bitp < IEOBITPOS; bitp++) {
        if (BitVal(cpreg[STATUS], bitp + 2))
            cpreg[STATUS] = SetBit(cpreg[STATUS], bitp);
        else
            cpreg[STATUS] = ResetBit(cpreg[STATUS], bitp);
    }

    for (bitp = VMCBITPOS; bitp < VMOBITPOS; bitp++) {
        if (BitVal(cpreg[STATUS], bitp + 1))
            cpreg[STATUS] = SetBit(cpreg[STATUS], bitp);
        else
            cpreg[STATUS] = ResetBit(cpreg[STATUS], bitp);
    }
}


// This method test for pending interrupts, checking global abilitation (IEc
// bit in STATUS register) and comparing interrupt mask with interrupts
// pending on bus; returns TRUE if an INT exception must be raised, FALSE
// otherwise, and sets CP0 registers if needed
bool Processor::checkForInt()
{
    // computes current IP status bit mask (software and hardware interrupts)
    Word currIPMask = bus->getPendingInt(this) | (cpreg[CAUSE] & CAUSEWMASK);
	
    if (BitVal(cpreg[STATUS], IECBITPOS) && (currIPMask & IM(cpreg[STATUS])) != 0UL)
    {
        // interrupts are enabled and at least one pending request is unmasked:
        // set the IP field of CAUSE reg, clear other fields
        // leaving the software interrupt bits unchanged, and signal for
        // exception 
        SignalExc(INTEXCEPTION);
        cpreg[CAUSE] = currIPMask;
        return true;
    }
    else
    {
        // set current interrupt pending bits into CAUSE register but do not
        // raise any exception nor clear other fields
        cpreg[CAUSE] = (cpreg[CAUSE] & ~(INTMASK)) | currIPMask;
        return false;
    }
}


// This method sets the appropriate registers in Processor CP0 at exception
// raising, following the MIPS conventions; it also set the PC value to
// point the appropriate exception handler vector
void Processor::handleExc()
{
    // If there is a load pending, it is completed while the processor
    // prepares for exception handling (a small bubble...).
    completeLoad();

    // set the excCode into CAUSE reg
    cpreg[CAUSE] = IM(cpreg[CAUSE]) | (excCode[excCause] << EXCCODEOFFS);

    if (isBranchD) {
        // previous instr. is branch/jump: must restart from it
        cpreg[CAUSE] = SetBit(cpreg[CAUSE], BDBITPOS);
        cpreg[EPC] = prevPC;
        // first instruction in exception handler itself is not in a BD slot
        isBranchD = false;
    } else {
        // BD is already set to 0 by CAUSE masking with IM; sets only EPC
        cpreg[EPC] = currPC;
    }

    // Set coprocessor unusable number in CAUSE register for
    // `Coprocessor Unusable' exceptions
    if (excCause == CPUEXCEPTION)
        cpreg[CAUSE] = cpreg[CAUSE] | (copENum << COPEOFFSET);

    // Set PC to the right handler
    Word excVector;
    if (BitVal(cpreg[STATUS], BEVBITPOS)) {
        // Machine bootstrap exception handler
        excVector = BOOTEXCBASE;
    } else {
        excVector = KSEG0BASE;
    }

    pushKUIEVMStack();

    if (excCause == UTLBLEXCEPTION || excCause == UTLBSEXCEPTION)
        excVector += TLBREFOFFS;
    else
        excVector += OTHEREXCOFFS;

    if (excCause == INTEXCEPTION) {
        // interrupt: test is before istruction fetch, so handling
        // could start immediately
        currPC = excVector;
        nextPC = currPC + WORDLEN;
        succPC = nextPC + WORDLEN;
    } else {
        // other exception, at instruction fetch or later: handling
        // could be done only in next processor cycle (current
        // instruction has been nullified)
        nextPC = excVector;
        succPC = nextPC + WORDLEN;
    }
}

// This method zeroes out the TLB
void Processor::zapTLB()
{
    // Leave out the first entry ([0])
    for (size_t i = 1; i < tlbSize; ++i) {
        tlb[i].setHI(0);
        tlb[i].setLO(0);
        SignalTLBChanged(i);
    }
}

// This method allows to handle the delayed load slot: it provides to load 
// the target register with the needed value during the execution of other 
// instructions, when invoked at the appropriate point in the "pipeline" 
void Processor::completeLoad()
{
    // loadPending tells whether a general purpose or a CP0 special register
    // is the target
    switch(loadPending)
    {
    case LOAD_TARGET_GPREG:
        setGPR(loadReg, loadVal);
        loadPending = LOAD_TARGET_NONE;
        break;

    case LOAD_TARGET_CPREG:
        // CP0 registers have some zero-filled fields or are read-only
        // so individual handling is needed
        switch(loadReg) {
        case INDEX:
            // loadable part is index field only, and P bit
            // remain unchanged
            cpreg[INDEX] = (cpreg[INDEX] & SIGNMASK) | (((Word) loadVal) & ((tlbSize - 1UL) << RNDIDXOFFS));
            break;

        case ENTRYLO:	
            // loadable part is PFN and status bits only
            cpreg[ENTRYLO] = ((Word) loadVal) & ENTRYLOMASK;
            break;

        case ENTRYHI:
            // loadable parts are VPN and ASID fields
            cpreg[ENTRYHI] = ((Word) loadVal) & (VPNMASK | ASIDMASK);
            break;

        case STATUS:
            // loadable parts are CU0 bit, BEV bit in DS, IM mask and
            // KUIE bit stack
            cpreg[STATUS] = ((Word) loadVal) & STATUSMASK;
            break;

        case CAUSE:
            // loadable parts are interrupt pending bits 1 and 0 only
            // ExcCode, hardware IP, BD and CE fields remain unchanged
            cpreg[CAUSE] = (cpreg[CAUSE] & ~(CAUSEWMASK)) | (((Word) loadVal) & CAUSEWMASK);
            break;
					
        case EPC:
        case PRID:
        case RANDOM:
        case BADVADDR:
        default:
            // read-only regs: writes have no effects
            break;
        }
        loadPending = LOAD_TARGET_NONE;
        break;

    case LOAD_TARGET_NONE:
    default:
        break;
    }
}

// This method maps the virtual addresses to physical ones following the
// complex mapping algorithm and TLB used by MIPS (see external doc). 
// It returns TRUE if conversion was not possible (this implies an exception
// have been raised) and FALSE if conversion has taken place: physical value
// for address conversion is returned thru paddr pointer.
// AccType details memory access type (READ/WRITE/EXECUTE)
bool Processor::mapVirtual(Word vaddr, Word * paddr, Word accType)
{
    //unsigned int i;

    if (BitVal(cpreg[STATUS], VMCBITPOS)) {
        // VM is on
	
        // SignalProcVAccess() is always done so it is possible 
        // to track accesses which produce exceptions
        SignalVAMappingRequested.emit(ASID(cpreg[ENTRYHI]) >> ASIDOFFS, vaddr, accType);

        // address validity and bounds check
        if (BADADDR(vaddr) || (InUserMode() && (vaddr < KUSEG2BASE))) {
            // bad offset or kernel segment access from user mode
            *paddr = MAXWORDVAL;
		
            // the bad virtual address is put into BADVADDR reg
            cpreg[BADVADDR] = vaddr;
		
            if (accType == WRITE)
                SignalExc(ADESEXCEPTION);
            else
                SignalExc(ADELEXCEPTION);

            return true;
        } else if (vaddr >= KSEG0BASE && vaddr < KSEG0TOP) {
            // no bad offset; if vaddr < KUSEG2BASE the processor is surely 
            // in kernelMode
            // valid access to KSEG0 area
            *paddr = vaddr;
            return false;
        }

        // The access is in user mode to user space, or in kernel mode
        // to KSEGOS or KSEG2/3 spaces.
	
        unsigned int index;
        if (probeTLB(&index, cpreg[ENTRYHI], vaddr)) {
            if (tlb[index].IsV()) {
                if (accType != WRITE || tlb[index].IsD()) {
                    // All OK
                    *paddr = PHADDR(vaddr, tlb[index].getLO());
                    return false;
                } else {
                    // write operation on frame with D bit set to 0
                    *paddr = MAXWORDVAL;
                    setTLBRegs(vaddr);
                    SignalExc(MODEXCEPTION);
                    return true;
                }
            } else  {
                // invalid access to frame with V bit set to 0
                *paddr = MAXWORDVAL;
                setTLBRegs(vaddr);
                if (accType == WRITE)
                    SignalExc(TLBSEXCEPTION);
                else 
                    SignalExc(TLBLEXCEPTION);

                return true;
            }		
        } else {
            // bad or missing VPN match: Refill event required
            *paddr = MAXWORDVAL;
            setTLBRegs(vaddr);
            if (accType == WRITE)
                SignalExc(UTLBSEXCEPTION);
            else 
                SignalExc(UTLBLEXCEPTION);

            return true;
        }	
    } else {
        // VM is off	
		
        // SignalProcVAccess() is always done so it is possible 
        // to track accesses which produce exceptions
        SignalVAMappingRequested.emit(MAXASID, vaddr, accType);

        // address validity and bounds check
        if (BADADDR(vaddr) || (InUserMode() && (vaddr < KSEG0TOP))) {
            // bad offset or kernel segment access from user mode
            *paddr = MAXWORDVAL;

            // the bad address is put into BADVADDR reg
            cpreg[BADVADDR] = vaddr;

            if (accType == WRITE)
                SignalExc(ADESEXCEPTION);
            else
                SignalExc(ADELEXCEPTION);

            return true;
        } else {
            *paddr = vaddr;
            return false;
        }
    }
}

// This method sets the CP0 special registers on exceptions forced by TLB
// handling (see mapVirtual() for invocation/specific cases).
void Processor::setTLBRegs(Word vaddr)
{
    // Note that ENTRYLO is left undefined!
    cpreg[BADVADDR] = vaddr;
    cpreg[ENTRYHI] = VPN(vaddr) | ASID(cpreg[ENTRYHI]);
}

// This method make Processor execute a single MIPS instruction, emulating
// pipeline constraints and load delay slots (see external doc).
bool Processor::execInstr(Word instr)
{
    Word temp;
    unsigned int i, cp0Num;
    bool error = false;
    bool isValidBranch = false;
	
    switch(OpType(instr)) {
    case REGTYPE:
        // MIPS register-type instruction execution
        error = execRegInstr(&temp, instr, &isValidBranch);

        // delayed load is completed _after_ istruction execution, but
        // _before_ instruction result is moved to target register
        completeLoad();

        if (!error && RD(instr))
            // no errors & target register != r0: put instruction result 
            // into target register
            gpr[RD(instr)] = (SWord) temp;
        break;

    case IMMTYPE:
        // MIPS immediate-type instruction
        error = execImmInstr(&temp, instr);

        // delayed load is completed _after_ istruction execution, but
        // _before_ instruction result is moved to target register
        completeLoad();

        if (!error && RT(instr))
            // no errors & target register != r0: put instruction result
            // into target register
            gpr[RT(instr)] = (SWord) temp;
        break;

    case BRANCHTYPE:
        // MIPS branch-type instruction
        error = execBranchInstr(instr, &isValidBranch);

        // delayed load is completed just after instruction execution
        completeLoad();
        break;					

    case COPTYPE:
        // MIPS coprocessor-type instruction

        // Some simulation issues:
        // CP0 is built-in and its Cp0Cond condition line is always
        // FALSE; other coprocessors are hard-wired to
        // non-availability 

        // detects coprocessor referred
        if (OPCODE(instr) == COP0SEL && cp0Usable()) {
            // COPOPTYPE corresponds to RS field
            switch(COPOPTYPE(instr)) {
            case CO0:
                // coprocessor 0 operations
                if (RT(instr) || RD(instr) || SHAMT(instr)) {
                    // instruction is ill-formed: exception signaled
                    // is CPU to help detection cause
                    SignalExc(CPUEXCEPTION, 0);
                    error = true;
                } else {
                    // all instructions follow MIPS guidelines
                    switch(FUNCT(instr)) {
                    case RFE:
                        popKUIEVMStack();
                        break;

                    case TLBP: 
                        // solution "by the book"
                        cpreg[INDEX] = SIGNMASK;
                        if (probeTLB(&i, cpreg[ENTRYHI], cpreg[ENTRYHI]))
                            cpreg[INDEX] = (i << RNDIDXOFFS);
                        break;

                    case TLBR:
                        cpreg[ENTRYHI] = tlb[RNDIDX(cpreg[INDEX])].getHI();
                        cpreg[ENTRYLO] = tlb[RNDIDX(cpreg[INDEX])].getLO();
                        break;	

                    case TLBWI:
                        tlb[RNDIDX(cpreg[INDEX])].setHI(cpreg[ENTRYHI]);
                        tlb[RNDIDX(cpreg[INDEX])].setLO(cpreg[ENTRYLO]);
                        SignalTLBChanged(RNDIDX(cpreg[INDEX]));
                        break;

                    case TLBWR:
                        tlb[RNDIDX(cpreg[RANDOM])].setHI(cpreg[ENTRYHI]);
                        tlb[RNDIDX(cpreg[RANDOM])].setLO(cpreg[ENTRYLO]);
                        SignalTLBChanged(RNDIDX(cpreg[INDEX]));
                        break;

                    default:
                        // unknown coprocessor 0 operation requested
                        SignalExc(CPUEXCEPTION, 0);
                        error = true;
                        break;
                    }
                }
                // delayed load is completed after instruction execution
                completeLoad();
                break;

            case BC0:
                switch(COPOPCODE(instr))
                {
                case BC0F:	
                    // condition line for CP0 is always FALSE
                    succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
                    isValidBranch = true;
                    break;
						
                case BC0T:
                    // condition line for CP0 is always FALSE
                    // so this is a nop instruction
                    isValidBranch = true;
                    break;
								
                default:
                    // traps BC0FL R4000 instructions
                    // and the like...
                    SignalExc(CPUEXCEPTION, 0);
                    error = true;
                    break;
                }	
                completeLoad();
                break;		
						
            case MFC0:
                // delayed load is completed _before_ istruction
                // execution since instruction itself produces a
                // delayed load
                completeLoad();
	
                // valid instruction has SHAMT and FUNCT fields
                // set to 0, and refers to a valid CP0 register
                if (ValidCP0Reg(RD(instr), &cp0Num) && !SHAMT(instr) && !FUNCT(instr)) {
                    setLoad(LOAD_TARGET_GPREG, RT(instr), (SWord) cpreg[cp0Num]);
                } else {
                    // invalid instruction format or CP0 reg 
                    SignalExc(CPUEXCEPTION, 0);
                    error = true;
                }
                break;
						
            case MTC0:
                // delayed load is completed _before_ istruction
                // execution since instruction itself produces a
                // delayed load
                completeLoad();
	
                // valid instruction has SHAMT and FUNCT fields
                // set to 0, and refers to a valid CP0 register
                if (ValidCP0Reg(RD(instr), &cp0Num) && !SHAMT(instr) && !FUNCT(instr))
                    setLoad(LOAD_TARGET_CPREG, cp0Num, gpr[RT(instr)]);
                else 
                {
                    // check if it is TLBCLR backpatch
                    if (RD(instr) == CONTEXTREG && !SHAMT(instr) && !FUNCT(instr))
                        zapTLB();
                    else 
                    {
                        // invalid instruction
                        SignalExc(CPUEXCEPTION, 0);
                        error = true;
                    }
                }
                break;
							 
            default:
                // unknown and CFC0, CTC0, COP0, LWC0 generic instructions
                SignalExc(CPUEXCEPTION, 0);
                error = true;
                break;
            }
        } else {
            // coprocessor 0 (or other) unusable
            SignalExc(CPUEXCEPTION, COPNUM(instr));
            error = true;
        }	
        break;
			
    case LOADTYPE:

        // MIPS load instruction
			
        // delayed load is completed _before_ istruction execution since
        // instruction itself produces a delayed load
        completeLoad();
			
        error = execLoadInstr(instr);
        break;
			
    case STORETYPE:
			
        // MIPS store instruction
			
        // delayed load is completed _before_ istruction execution
        // since it happens "logically" so in the pipeline
        completeLoad();

        error = execStoreInstr(instr);
        break;					

    case LOADCOPTYPE:
        if (BitVal(instr, DWCOPBITPOS)) {
            // LDC, SDC reserved instructions handling
            SignalExc(RIEXCEPTION);
            error = true;
        } else {
            // delayed load is completed _before_ istruction execution since
            // instruction itself produces a delayed load
            completeLoad();
            error = execLoadCopInstr(instr);
        }
        break;

    case STORECOPTYPE:
        if (BitVal(instr, DWCOPBITPOS)) {
            // LDC, SDC reserved instructions handling
            SignalExc(RIEXCEPTION);
            error = true;
        } else {
            // delayed load is completed _before_ istruction execution
            // since it happens "logically" so in the pipeline
            completeLoad();
            error = execStoreCopInstr(instr);
        }
        break;

    default:
        // unknown instruction (generic)
        SignalExc(RIEXCEPTION);
        error = true;
        break;
    }		
	
    if (!error)
    {
        // a correct instruction has been executed
        if (isValidBranch)
            // next instr. is in a branch delay slot
            isBranchD = true;
        else
            isBranchD = false;
    } 
    // else instr. execution has generated an exception: 
    // isBranchD is not modified because exception handler
    // will need it
	
    return(error);
}

// This method tests for CP0 availability (as set in STATUS register and in
// MIPS conventions)
bool Processor::cp0Usable()
{
    // CP0 is usable only when marked in user mode, and always in kernel mode
    return BitVal(cpreg[STATUS], CP0UBITPOS) || !BitVal(cpreg[STATUS], KUCBITPOS);
}

// This method scans the TLB looking for a entry that matches ASID/VPN pair;
// scan algorithm follows MIPS specifications, and returns the _highest_
// entry that matches
bool Processor::probeTLB(unsigned int* index, Word asid, Word vpn)
{
    bool found = false;

    for (unsigned int i = 0; i < tlbSize; i++) {
        if (tlb[i].VPNMatch(vpn) && (tlb[i].IsG() || tlb[i].ASIDMatch(asid))) {
            found = true;
            *index = i;
        }
    }

    return found;
}		


// This method sets delayed load handling variables when needed by
// instruction execution
void Processor::setLoad(LoadTargetType loadCode, unsigned int regNum, SWord regVal)
{
    loadPending = loadCode;
    loadReg = regNum;
    loadVal = regVal;
}


// This method returns a sign-extended byte from inside a word following
// MIPS conventions about big and little endianness; it is requested by LB
// instruction execution
SWord Processor::signExtByte(Word val, unsigned int bytep)
{
    if (BIGENDIANCPU)
        // byte to be extended is on the other "side" of the word
        bytep = (WORDLEN - 1) - bytep;
	
    // shifts byte into first position
    val = val >> (BYTELEN * bytep);
	
    if (BitVal(val, BYTELEN - 1))
        // must sign-extend with 1
        return(val | ~(BYTESIGNMASK));
    else
        return(val & BYTESIGNMASK);
}

// This method returns a zero-extended byte from inside a word following
// MIPS conventions about big and little endianness; requested by LBU
// instruction
Word Processor::zExtByte(Word val, unsigned int bytep)
{
    if (BIGENDIANCPU)
        // byte is on the other "side" of the word
        bytep = (WORDLEN - 1) - bytep;
	
    // shifts byte into first position and zero-extends it
    return((val >> (BYTELEN * bytep)) & BYTEMASK);
}	


// This method returns a word with one byte overwritten, taken from another
// word, and following MIPS conventions about big and little endianness:
// requested by SB instruction
Word Processor::mergeByte(Word dest, Word src, unsigned int bytep)
{
    if (BIGENDIANCPU)
        bytep = (WORDLEN - 1) - bytep;	

    // shifts least significant byte of src into position and clears around it
    src = (src & BYTEMASK) << (bytep * BYTELEN);
	
    //clears specified byte and overwrites it with src 
    return((dest & ~(BYTEMASK << (bytep * BYTELEN))) | src);
}


// This method returns the sign-extended halfword taken from a word, following 
// MIPS conventions about big and little endianness (LH instruction)
SWord Processor::signExtHWord(Word val, unsigned int hwp)
{
    if (BIGENDIANCPU)
        hwp = 1 - hwp;
	
    // shifts halfword into first position
    val = val >> (HWORDLEN * hwp);
	
    return(SignExtImm(val));	
}


// This method returns the zero-extended halfword taken from a word, following
// MIPS conventions about big and little endianness (LHU instruction)
Word Processor::zExtHWord(Word val, unsigned int hwp)
{
    if (BIGENDIANCPU)
        hwp = 1 - hwp;
	
    // shifts halfword into first position
    val = val >> (HWORDLEN * hwp);
	
    return(ZEXTIMM(val));	
}		


// this method returns a word partially overwritten with another, following
// MIPS conventions about big and little endianness (SH instruction) 
Word Processor::mergeHWord(Word dest, Word src, unsigned int hwp)
{
    if (BIGENDIANCPU)
        hwp = 1 - hwp;	

    // shifts least significant halfword of src into position and clears around it
    src = (src & IMMMASK) << (hwp * HWORDLEN);
	
    // clears specified halfword and overwrites it with src 
    return((dest & ~(IMMMASK << (hwp * HWORDLEN))) | src);
}	 


// This method copies into dest some bytes from src (starting from bytep
// position in src), beginning the copy from the left or right side of dest.
// It computes the partial overlaps needed the LWL/LWR and SWL/SWR.
// instructions following the MIPS conventions on big and little endianness
Word Processor::merge(Word dest, Word src, unsigned int bytep, bool loadBig, bool startLeft)
{
    if (loadBig)
        // LWL/LWR with BIGENDIANCPU == 1 or SWR/SWL with BIGENDIANCPU == 0
        bytep = (WORDLEN - 1) - bytep;
    // else bytep is already correct:
    // LWL/LWR with BIGENDIANCPU == 0 or SWR/SWL with BIGENDIANCPU = 1
	
    if (startLeft)
    {
        // starts from left end of dest:
        // shifts src part into position and clear right of it
        src = src << (((WORDLEN - 1) - bytep) * BYTELEN);
		
        // clear the left part of dest and merges it with src
        dest = (dest & ~(MAXWORDVAL << (((WORDLEN - 1) - bytep) * BYTELEN))) | src;
    }
    else
    {
        // starts from right end of dest: shifts src part into position
        // and clears left of it (because src is unsigned)
        src = src >> (bytep * BYTELEN);
		
        // clears the right part of dest and merges it with src
        dest = (dest & ~(MAXWORDVAL >> (bytep * BYTELEN))) | src;
    }
    return(dest);
}
		

// This method executes a MIPS register-type instruction, following MIPS
// guidelines; returns instruction result thru res pointer, and branch delay
// slot indication if needed (due to JR/JALR presence). It also returns TRUE
// if an exception occurred, FALSE otherwise
bool Processor::execRegInstr(Word * res, Word instr, bool * isBD)
{
    bool error = false;

    *isBD = false;

    error = InvalidRegInstr(instr);
    if (!error)
    {
        // instruction format is correct		
        switch(FUNCT(instr))
        {
        case ADD:
            if (SignAdd(res, gpr[RS(instr)], gpr[RT(instr)]))
            {
                SignalExc(OVEXCEPTION);
                error = true;
            }
            break;
				
        case ADDU:
            *res = gpr[RS(instr)] + gpr[RT(instr)];
            break;
		
        case AND:
            *res = gpr[RS(instr)] & gpr[RT(instr)];
            break;
			
        case BREAK:
            SignalExc(BPEXCEPTION);
            error = true;
            break;
			
        case DIV:
            if (gpr[RT(instr)] != 0)
            {
                gpr[LO] = gpr[RS(instr)] / gpr[RT(instr)];
                gpr[HI] = gpr[RS(instr)] % gpr[RT(instr)];
            }
            else
            {
                // divisor is zero
                gpr[LO] = MAXWORDVAL;
                gpr[HI] = 0;
            }
            break;
					
        case DIVU:
            if (gpr[RT(instr)] != 0)
            {
                gpr[LO] = ((Word) gpr[RS(instr)]) / ((Word) gpr[RT(instr)]);
                gpr[HI] = ((Word) gpr[RS(instr)]) % ((Word) gpr[RT(instr)]);
            }
            else
            {
                // divisor is zero
                gpr[LO] = MAXWORDVAL;
                gpr[HI] = 0;
            }
            break;
				
        case JALR:
            // solution "by the book"
            succPC = gpr[RS(instr)];
            *res = currPC + (2 * WORDLEN);
            *isBD = true;
            // alternative: *res = succPC; succPC = gpr[RS(instr)]
            break;

        case JR:
            succPC = gpr[RS(instr)];
            *isBD = true;
            break;
				
        case MFHI:
            *res = gpr[HI];
            break;
				
        case MFLO:
            *res = gpr[LO];
            break;
					
        case MTHI:
            gpr[HI] = gpr[RS(instr)];
            break;
				
        case MTLO:
            gpr[LO] = gpr[RS(instr)];
            break;	
				
        case MULT:
            SignMult(gpr[RS(instr)], gpr[RT(instr)], &(gpr[HI]), &(gpr[LO]));
            break;				
				
        case MULTU:
            UnsMult((Word) gpr[RS(instr)], (Word) gpr[RT(instr)], (Word *)&(gpr[HI]), (Word *)&(gpr[LO]));
            break;
				
        case NOR:
            *res = ~(gpr[RS(instr)]	| gpr[RT(instr)]);
            break;
				
        case OR:
            *res = gpr[RS(instr)] | gpr[RT(instr)];
            break;
				
        case SLL:
            *res = gpr[RT(instr)] << SHAMT(instr);
            break;
				
        case SLLV:
            *res = gpr[RT(instr)] << REGSHAMT(gpr[RS(instr)]);	
            break;
					
        case SLT:
            if (gpr[RS(instr)] < gpr[RT(instr)])
                *res = 1UL;
            else
                *res = 0UL;
            break;

        case SLTU:
            if (((Word) gpr[RS(instr)]) < ((Word) gpr[RT(instr)]))
                *res = 1UL;
            else
                *res = 0UL;
            break;
				
        case SRA:
            *res = (gpr[RT(instr)] >> SHAMT(instr));
            break;
				
        case SRAV:
            *res = (gpr[RT(instr)] >> REGSHAMT(gpr[RS(instr)]));
            break;
				
        case SRL:
            *res = (((Word) gpr[RT(instr)]) >> SHAMT(instr));	
            break;
					
        case SRLV:
            *res = (((Word) gpr[RT(instr)]) >> REGSHAMT(gpr[RS(instr)]));
            break;
					
        case SUB:	
            if (SignSub(res, gpr[RS(instr)], gpr[RT(instr)]))
            {
                SignalExc(OVEXCEPTION);
                error = true;
            }
            break;
				
        case SUBU:
            *res = gpr[RS(instr)] - gpr[RT(instr)];
            break;			
				
        case SYSCALL:
            SignalExc(SYSEXCEPTION);
            error = true;
            break;
				
        case XOR:
            *res = gpr[RS(instr)] ^ gpr[RT(instr)];
            break;
													
        default:
            // unknown instruction
            SignalExc(RIEXCEPTION);			
            error = true;
        }
    }
    else
        // istruction is ill-formed
        SignalExc(RIEXCEPTION);
		
    return(error);
}

// This method executes a MIPS immediate-type instruction, following MIPS
// guidelines; returns instruction result thru res pointer. It also returns
// TRUE if an exception occurred, FALSE otherwise
bool Processor::execImmInstr(Word * res, Word instr)
{
    bool error = false;
	
    switch(OPCODE(instr))
    {
    case ADDI:
        if (SignAdd(res, gpr[RS(instr)], SignExtImm(instr)))
        {
            SignalExc(OVEXCEPTION);
            error = true;
        }
        break;
		
    case ADDIU:
        *res = gpr[RS(instr)] + SignExtImm(instr);
        break;
		
    case ANDI:
        *res = gpr[RS(instr)] & ZEXTIMM(instr);
        break;
		
    case LUI:
        if (!RS(instr))
            *res = (ZEXTIMM(instr) << HWORDLEN);
        else
        {
            // instruction is ill-formed
            SignalExc(RIEXCEPTION);
            error = true;
        }
        break;
			
    case ORI:
        *res = gpr[RS(instr)] | ZEXTIMM(instr);
        break;
			
    case SLTI:
        if (gpr[RS(instr)] < SignExtImm(instr))
            *res = 1UL;
        else
            *res = 0UL;
        break;
		
    case SLTIU:
        if (((Word) gpr[RS(instr)]) < ((Word) SignExtImm(instr)))
            *res = 1UL;
        else
            *res = 0UL;
        break;
    case XORI:
        *res = gpr[RS(instr)] ^ ZEXTIMM(instr);
        break;
			
    default:
        SignalExc(RIEXCEPTION);
        error = true;
        break;	
    }
    return(error);
}


// This method executes a MIPS branch-type instruction, following MIPS
// guidelines; returns instruction result thru res pointer, and branch delay
// slot indication if needed. It also returns TRUE if an exception occurred,
// FALSE otherwise
bool Processor::execBranchInstr(Word instr, bool * isBD)
{
    bool error = false;
	
    switch (OPCODE(instr))
    {
    case BEQ:
        if (gpr[RS(instr)] == gpr[RT(instr)])
            succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
        break;
		
    case BGL:
        // uses RT field to choose which branch type is requested
        switch (RT(instr))
        {
        case BGEZ:
            if (!SIGNBIT(gpr[RS(instr)]))
                succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
            break;
				
        case BGEZAL:
            // solution "by the book"; alternative: gpr[..] = succPC 
            gpr[LINKREG] = currPC + (2 * WORDLEN);
            if (!SIGNBIT(gpr[RS(instr)]))
                succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
            break;						
				
        case BLTZ:
            if (SIGNBIT(gpr[RS(instr)]))
                succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
            break;		
				
        case BLTZAL:
            gpr[LINKREG] = currPC + (2 * WORDLEN);
            if (SIGNBIT(gpr[RS(instr)]))
                succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
            break;							
					
        default:
            // unknown instruction
            SignalExc(RIEXCEPTION);
            error = true;
            break;
        }
        break;
						
    case BGTZ:
        if (!RT(instr))
        {
            // instruction is well formed
            if (gpr[RS(instr)] > 0)
                succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
        }
        else
        {
            // istruction is ill-formed
            SignalExc(RIEXCEPTION);
            error = true;
        }
        break;
			
    case BLEZ:
        if (!RT(instr))
        {
            // instruction is well formed
            if (gpr[RS(instr)] <= 0)
                succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
        }
        else 
        {
            // istruction is ill-formed
            SignalExc(RIEXCEPTION);
            error = true;
        }
        break;
		
    case BNE:
        if (gpr[RS(instr)] != gpr[RT(instr)])
            succPC = nextPC + (SignExtImm(instr) << WORDSHIFT);
        break;
			
    case J:
        succPC = JUMPTO(nextPC, instr);	
        break;
			
    case JAL:
        // solution "by the book": alt. gpr[..] = succPC
        gpr[LINKREG] = currPC + (2 * WORDLEN);
        succPC = JUMPTO(nextPC, instr);
        break;
					
    default:
        SignalExc(RIEXCEPTION);
        error = true;
        break;
    }
    if (error)
        // not a valid branch instruction
        *isBD = false;
    else
        // a correct branch instr. has been executed
        *isBD = true; 

    return(error);
}


// This method executes a MIPS load-type instruction, following MIPS
// guidelines. It returns TRUE if an exception occurred, FALSE
// otherwise.
bool Processor::execLoadInstr(Word instr)
{
    Word paddr, vaddr, temp;
    bool error = false;
	
    switch(OPCODE(instr))
    {	
    case LB:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);

        // reads the full word from bus and then extracts the byte
        if (!mapVirtual(ALIGN(vaddr), &paddr, READ) && !bus->DataRead(paddr, &temp, this))
            setLoad(LOAD_TARGET_GPREG, RT(instr), signExtByte(temp, BYTEPOS(vaddr)));
        else 
            // exception signaled: rt not loadable
            error = true;
        break;

    case LBU: 
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
			
        // reads the full word from bus and then extracts the byte
        if (!mapVirtual(ALIGN(vaddr), &paddr, READ) && !bus->DataRead(paddr, &temp, this))
            setLoad(LOAD_TARGET_GPREG, RT(instr), (SWord) zExtByte(temp, BYTEPOS(vaddr)));
        else
            // exception signaled: rt not loadable
            error = true;
        break;
			
    case LH:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (BitVal(vaddr, 0))
        {
            // unaligned halfword
            SignalExc(ADELEXCEPTION);
            error = true;
        }
        else
            // reads the full word from bus and then extracts the halfword
            if (!mapVirtual(ALIGN(vaddr), &paddr, READ) && !bus->DataRead(paddr, &temp, this))
                setLoad(LOAD_TARGET_GPREG, RT(instr), signExtHWord(temp, HWORDPOS(vaddr)));
            else
                // exception signaled: rt not loadable
                error = true;
        break;
					
    case LHU:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (BitVal(vaddr, 0))
        {
            // unaligned halfword
            SignalExc(ADELEXCEPTION);
            error = true;
        }
        else
            // reads the full word from bus and then extracts the halfword
            if (!mapVirtual(ALIGN(vaddr), &paddr, READ) && !bus->DataRead(paddr, &temp, this))
                setLoad(LOAD_TARGET_GPREG, RT(instr), (SWord) zExtHWord(temp, HWORDPOS(vaddr)));
            else
                // exception signaled: rt not loadable
                error = true;
        break;
		
    case LW:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (!mapVirtual(vaddr, &paddr, READ) && !bus->DataRead(paddr, &temp, this))
            setLoad(LOAD_TARGET_GPREG, RT(instr), (SWord) temp);
        else
            // exception signaled: rt not loadable
            error = true;
        break;

    case LWL:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);

        // reads the full word from bus and then extracts the desired part
        if (!mapVirtual(ALIGN(vaddr), &paddr, READ) && !bus->DataRead(paddr, &temp, this))
        {
            temp = merge((Word) gpr[RT(instr)], temp,  BYTEPOS(vaddr), BIGENDIANCPU, true);
            setLoad(LOAD_TARGET_GPREG, RT(instr), temp);
        }
        else
            // exception signaled: rt not loadable
            error = true;
        break;
			
    case LWR:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);

        // reads the full word from bus and then extracts the desired part
        if (!mapVirtual(ALIGN(vaddr), &paddr, READ) && !bus->DataRead(paddr, &temp, this))
        {
            temp = merge((Word) gpr[RT(instr)], temp, BYTEPOS(vaddr), BIGENDIANCPU, false);
            setLoad(LOAD_TARGET_GPREG, RT(instr), temp);
        }
        else
            // exception signaled: rt not loadable
            error = true;
        break;	
	
    default:
        SignalExc(RIEXCEPTION);
        error = true;
        break;
    }
    return(error);
}

// This method executes a MIPS load-type instruction, following MIPS
// guidelines; It returns TRUE if an exception occurred, FALSE
// otherwise.
bool Processor::execStoreInstr(Word instr)
{
    Word paddr, vaddr, temp;
    bool error = false;

    switch(OPCODE(instr))
    {
    case SB:
        // here things are a little dirty: instead of writing
        // the byte directly into memory, it reads the full word,
        // modifies the byte as needed, and writes the word back.
        // This works because there could be read-only memory but 
        // not write-only...
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (!mapVirtual(ALIGN(vaddr), &paddr, WRITE) && !bus->DataRead(paddr, &temp, this))
        {
            temp = mergeByte(temp, (Word) gpr[RT(instr)], BYTEPOS(vaddr));
            if (bus->DataWrite(paddr, temp, this))
                // bus exception signaled
                error = true;
        }
        else 
            // address or bus exception signaled
            error = true;
        break;

    case SH:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (BitVal(vaddr, 0))
        {
            // unaligned halfword
            SignalExc(ADESEXCEPTION);
            error = true;
        }
        else
            // the same "dirty" thing here...
            if (!mapVirtual(ALIGN(vaddr), &paddr, WRITE) && !bus->DataRead(paddr, &temp, this))
            {
                temp = mergeHWord(temp, (Word) gpr[RT(instr)], HWORDPOS(vaddr));
                if (bus->DataWrite(paddr, temp, this))
                    // bus exception signaled
                    error = true;							
            }
            else
                // address or bus exception signaled
                error = true;
        break;
			
    case SW:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (mapVirtual(vaddr, &paddr, WRITE) || bus->DataWrite(paddr, (Word) gpr[RT(instr)], this))
            // address or bus exception signaled
            error = true;
        break;	
	
    case SWL:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (!mapVirtual(ALIGN(vaddr), &paddr, WRITE) && !bus->DataRead(paddr, &temp, this))
        {
            temp = merge(temp, (Word) gpr[RT(instr)], BYTEPOS(vaddr), !(BIGENDIANCPU), false);
			
            if(bus->DataWrite(paddr, temp, this))
                // bus exception
                error = true;	
        }
        else
            // address or bus exception signaled
            error = true;
        break;
			
    case SWR:
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (!mapVirtual(ALIGN(vaddr), &paddr, WRITE) && !bus->DataRead(paddr, &temp, this))
        {
            temp = merge(temp, (Word) gpr[RT(instr)], BYTEPOS(vaddr), !(BIGENDIANCPU), true);
            if (bus->DataWrite(paddr, temp, this))
                // bus exception signaled
                error = true;	
        }
        else
            // addresss or bus exception signaled
            error = true;
        break;	
	
    default:
        SignalExc(RIEXCEPTION);
        error = true;
        break;
    }
    return(error);
}


bool Processor::execLoadCopInstr(Word instr)
{
    Word vaddr;
    Word paddr;
    Word word;

    bool error = false;

    switch(OPCODE(instr)) {
    case LWC0:
        // Load-linked (LL)
        vaddr = gpr[RS(instr)] + SignExtImm(instr);
        if (!mapVirtual(vaddr, &paddr, READ) && !bus->DataRead(paddr, &word, this)) {
            setLoad(LOAD_TARGET_GPREG, RT(instr), (SWord) word);
            LLAddr = paddr;
            llscAtomic = true;
        } else
            error = true;
        break;

    default:
        SignalExc(CPUEXCEPTION, COPNUM(instr));
        error = true;
        break;
    }

    return error;
}


bool Processor::execStoreCopInstr(Word instr)
{
    Word vaddr;
    Word paddr;

    bool error = false;

    switch(OPCODE(instr)) {
    case SWC0:
        // Store-conditional (SC)
        if (LLAddr == MAXWORDVAL || !llscAtomic) {
            setGPR(RT(instr), 0);
        } else {
            vaddr = gpr[RS(instr)] + SignExtImm(instr);
            if (mapVirtual(vaddr, &paddr, WRITE) || bus->DataWrite(paddr, (Word) gpr[RT(instr)], 0))
                error = true;
            else
                setGPR(RT(instr), 1);
        }
        break;

    default:
        SignalExc(CPUEXCEPTION, COPNUM(instr));
        error = true;
        break;
    }

    return error;
}
