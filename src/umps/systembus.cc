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
 * This module implements the SystemBus object class.
 * 
 * A SystemBus connects a Processor object to memory and devices. 
 * It satisfies the Processor requests to read and write memory positions,
 * and simulates interrupts generation and other external exceptions.
 * SystemBus defines the physical memory map of the system, mapping RamSpace
 * and BiosSpace objects and Device registers to "physical" addresses in
 * memory.  
 * It also implements the system clock and timer, and keeps track of some
 * important system constants.  All this information is mapped in the "bus
 * device register area", at the start of the device register area.
 *
 * On creation, it initializes the device and memory subsystems
 * as set in the simulator configuration (contained in the SetupInfo 
 * object); on destruction, it removes them.  
 * 
 * SystemBus notifies all changes to memory location to Watch controlling
 * object, to track memory accesses for simulation brkpt/susp/trace handling
 * 
 ****************************************************************************/

#define BE_SLOW 0

#include <iostream>
#include <stdio.h>

#include <assert.h>

#include <umps/const.h>

#include "umps/blockdev_params.h"

#include "umps/utility.h"

#include "umps/device.h"
#include "umps/blockdev.h"
#include "umps/processor.h"
#include "umps/systembus.h"

#include "umps/types.h"
#include "umps/arch.h"
#include "umps/machine_config.h"
#include "umps/machine.h"
#include "umps/mp_controller.h"
#include "umps/time_stamp.h"
#include "umps/error.h"
#include "umps/memspace.h"
#include "umps/event.h"

// This macro converts a byte address into a word address (minus offset)
#define CONVERT(ad, bs)	((ad - bs) >> WORDSHIFT)	

class DeviceAreaAddress {
public:
    DeviceAreaAddress(Word paddr)
        : pa(paddr)
    {
        assert(DEV_BASE <= paddr && paddr < DEV_END);
    }

    DeviceAreaAddress(unsigned int line, unsigned int device, unsigned int field)
        : pa(DEV_REG_ADDR(line + DEV_IL_START, device) + field * WS)
    {
        assert(line < N_EXT_IL && device < N_DEV_PER_IL && field < DEV_REG_SIZE_W);
    }

    Word address() const { return pa; }

    unsigned int regIndex() const { return wordIndex() / DEV_REG_SIZE_W; }
    unsigned int line() const { return regIndex() / N_DEV_PER_IL; }
    unsigned int device() const { return regIndex() % N_DEV_PER_IL; }
    unsigned int field() const { return wordIndex() % DEV_REG_SIZE_W; }

private:
    unsigned int wordIndex() const { return (pa - DEV_REG_START) >> 2; }

    Word pa;
};


SystemBus::SystemBus(const MachineConfig* conf, Machine* machine)
    : config(conf),
      machine(machine)
{
    timeOfDay = new TimeStamp();
    timer = MAXWORDVAL;
    eventQ = new EventQueue();

    const char *coreFile = NULL;
    if (config->isLoadCoreEnabled())
        coreFile = config->getROM(ROM_TYPE_CORE).c_str();
    ram = new RamSpace(config->getRamSize() * FRAMESIZE, coreFile);

    bios = new BiosSpace(config->getROM(ROM_TYPE_BIOS).c_str());
    boot = new BiosSpace(config->getROM(ROM_TYPE_BOOT).c_str());

    mpc = new MPController(this);

    // Create devices and initialize registers used for interrupt
    // handling.
    intPendMask = 0UL;
    for (unsigned intl = 0; intl < N_EXT_IL; intl++) {
        instDevTable[intl] = 0UL;
        intCauseDev[intl] = 0UL;
        for (unsigned int devNo = 0; devNo < N_DEV_PER_IL; devNo++) {
            devTable[intl][devNo] = makeDev(intl, devNo);
            if (devTable[intl][devNo]->Type() != NULLDEV)
                instDevTable[intl] = SetBit(instDevTable[intl], devNo);
        }
    }
}

// This method deletes a SystemBus object and all related structures
SystemBus::~SystemBus()
{
    unsigned int intl, dnum;
	
    delete timeOfDay;
    delete eventQ;
	
    delete ram;
    delete bios;
    delete boot;
	
    for (intl = 0; intl < DEVINTUSED; intl++)
        for (dnum = 0; dnum < DEVPERINT; dnum++)
            delete devTable[intl][dnum];
}

void SystemBus::LinkProcessor(Processor* p)
{
}

// This method increments system clock and decrements interval timer;
// on timer underflow (0 -> FFFFFFFF transition) a interrupt is
// generated.  Event queue is checked against the current clock value
// and device operations are completed if needed; all memory changes
// are notified to Watch control object
void SystemBus::ClockTick()
{
    timeOfDay->Increase();

    // both registers signal "change" because they are conceptually one
#if 0
    watch->SignalBusAccess(BUS_REG_TOD_HI, WRITE);
    watch->SignalBusAccess(BUS_REG_TOD_LO, WRITE);
#endif

#if BE_SLOW
    // temp
    SignalAccess(BUS_REG_TOD_HI, WRITE, NULL);
    SignalAccess(BUS_REG_TOD_LO, WRITE, NULL);
#else
    machine->busAccessHandler(BUS_REG_TOD_HI, WRITE, NULL);
    machine->busAccessHandler(BUS_REG_TOD_LO, WRITE, NULL);
#endif

    if (UnsSub(&timer, timer, 1))
        // timer interrupt is now pending
        intPendMask = SetBit(intPendMask, TIMERINT + IPMASKBASE);

#if 0
    watch->SignalBusAccess(BUS_REG_TIMER, WRITE);
#endif

#if BE_SLOW
    SignalAccess(BUS_REG_TIMER, WRITE, NULL);
#else
    machine->busAccessHandler(BUS_REG_TIMER, WRITE, NULL);
#endif

    // scans the event queue
    while (!eventQ->IsEmptyQ() && (eventQ->getHTS())->LessEq(timeOfDay)) {
        // a device operation need to be completed and that event removed
        uint intl = eventQ->getHIntLine();
        uint dnum = eventQ->getHDevNum();
        uint devAddr = (devTable[intl][dnum])->CompleteDevOp();
        eventQ->RemoveHead();

        // a device op completion changes device registers: memory is altered
        DeviceAreaAddress da(intl, dnum, devAddr);
#if 0
        watch->SignalBusAccess(da.address(), WRITE);
#endif
        SignalAccess(da.address(), WRITE, NULL);
    }
}


//
// The following methods allow to inspect or modify  TimeofDay Clock and
// Interval Timer (typically for simulation reasons); they are self-explanatory
//

Word SystemBus::getToDHI()
{
    return timeOfDay->getHiTS();
}

Word SystemBus::getToDLO()
{
    return timeOfDay->getLoTS();
}

Word SystemBus::getTimer()
{
    return timer;
}

void SystemBus::setToDHI(Word hi)
{
    timeOfDay->setHiTS(hi);
}

void SystemBus::setToDLO(Word lo)
{
    timeOfDay->setLoTS(lo);
}

void SystemBus::setTimer(Word time)
{
    timer = time;
}


// This method reads a data word from memory at address addr, returning it
// thru datap pointer. It also returns TRUE if the address was invalid and
// an exception was caused, FALSE otherwise, and signals memory access to
// Watch control object
bool SystemBus::DataRead(Word addr, Word* datap, Processor* cpu)
{
#if 0
    watch->SignalBusAccess(addr, READ);
#endif

#if BE_SLOW
    SignalAccess.emit(addr, READ, cpu);
#else
    machine->busAccessHandler(addr, READ, cpu);
#endif

    if (busRead(addr, datap, cpu)) {
        // address invalid: signal exception to processor
        cpu->SignalExc(DBEXCEPTION);
        return true;
    }

    return false;
}


//
// These methods allow Watch to inspect or modify single memory locations;
// they return TRUE if address is invalid or memory cannot be altered, and
// FALSE otherwise
// 

bool SystemBus::WatchRead(Word addr, Word * datap)
{
    return busRead(addr, datap);
}

bool SystemBus::WatchWrite(Word addr, Word data)
{	
    return busWrite(addr, data);
}


// This method writes the data word at physical addr in RAM memory or device
// register area.  Writes to BIOS or BOOT areas cause a DBEXCEPTION (no
// writes allowed). It returns TRUE if an exception was caused, FALSE
// otherwise, and notifies access to Watch control object
bool SystemBus::DataWrite(Word addr, Word data, Processor* proc)
{
#if 0
    watch->SignalBusAccess(addr, WRITE);
#endif

#if BE_SLOW
    SignalAccess.emit(addr, WRITE, proc);
#else
    machine->busAccessHandler(addr, WRITE, proc);
#endif

    if (busWrite(addr, data, proc)) {
        // data write is out of valid write bounds
        proc->SignalExc(DBEXCEPTION);
        return true;
    } else
        return false;
}


// This method transfers a block from or to memory, starting with address
// startAddr; it returns TRUE is transfer was not successful (non-existent
// memory, read-only memory, unaligned addresses), FALSE otherwise.
// It notifies too the memory accesses to Watch control object
bool SystemBus::DMATransfer(Block * blk, Word startAddr, bool toMemory)
{
    if (BADADDR(startAddr))
        return true;

    bool error = false;

    if (toMemory) {
        for (Word ofs = 0; ofs < BLOCKSIZE && !error; ofs++) {
            error = busWrite(startAddr + (ofs * WORDLEN), blk->getWord(ofs));  
#if 0
            watch->SignalBusAccess(startAddr + (ofs * WORDLEN), WRITE);
#endif
        }
    } else {
        Word val;
        for (Word ofs = 0; ofs < BLOCKSIZE && !error; ofs++) {
            error = busRead(startAddr + (ofs * WORDLEN), &val);
#if 0
            watch->SignalBusAccess(startAddr + (ofs * WORDLEN), READ);
#endif
            blk->setWord(ofs, val);
        }
    }

    return error;
}


// This method transfers a partial block from or to memory, starting with address
// startAddr; it returns TRUE is transfer was not successful (non-existent
// memory, read-only memory, unaligned addresses), FALSE otherwise.
// It notifies too the memory accesses to Watch control object
bool SystemBus::DMAVarTransfer(Block* blk, Word startAddr, Word byteLength, bool toMemory)
{
    // fit bytes into words
    Word length;
    if (byteLength % WORDLEN)
        length = (byteLength / WORDLEN) + 1;
    else
        length = byteLength / WORDLEN; 
		
    if (BADADDR(startAddr) || length > BLOCKSIZE)
        return true;

    bool error = false;

    if (toMemory) {
        for (Word ofs = 0; ofs < length && !error; ofs++) {
            error = busWrite(startAddr + (ofs * WORDLEN), blk->getWord(ofs));
#if 0
            watch->SignalBusAccess(startAddr + (ofs * WORDLEN), WRITE);
#endif
        }
    } else {
        Word val;
        for (Word ofs = 0; ofs < length && !error; ofs++) {
            error = busRead(startAddr + (ofs * WORDLEN), &val);
#if 0
            watch->SignalBusAccess(startAddr + (ofs * WORDLEN), READ);
#endif
            blk->setWord(ofs, val);
        }
    }

    return error;
}

				
// This method reads a istruction from memory at address addr, returning
// it thru istrp pointer. It also returns TRUE if the address was invalid and
// an exception was caused, FALSE otherwise, and notifies Watch
bool SystemBus::InstrRead(Word addr, Word* instrp, Processor* proc)
{
#if 0
    watch->SignalBusAccess(addr, EXEC);
#endif

#if BE_SLOW
    SignalAccess.emit(addr, EXEC, proc);
#else
    machine->busAccessHandler(addr, EXEC, proc);
#endif

    if (busRead(addr, instrp)) {
        // address invalid: signal exception to processor
        proc->SignalExc(IBEXCEPTION);
        return true;
    } else {
        // address was valid
        return false;
    }
}


// This method inserts in the eventQ a event that must happen
// at (current system time) + (increment)
TimeStamp* SystemBus::EventReq(unsigned int intNum, unsigned int devNum, Word inc)
{
    return eventQ->InsertQ(timeOfDay, inc, intNum, devNum);
}


// This method sets the appropriate bits into intCauseDev[] and
// IntPendMask to signal device interrupt pending; it notifies memory
// changes to Watch too
void SystemBus::IntReq(unsigned int intl, unsigned int devNum)
{
    intCauseDev[intl] = SetBit(intCauseDev[intl], devNum);
    intPendMask = SetBit(intPendMask, intl + DEVINTBASE + IPMASKBASE);
    // memory address changes
#if 0
    watch->SignalBusAccess(CDEV_BITMAP_ADDR(intl + DEV_IL_START), WRITE);
#endif
}


// This method resets the appropriate bits into intCauseDev[] and
// IntPendMask to signal device interrupt acknowlege; it notifies memory
// changes to Watch too
void SystemBus::IntAck(unsigned int intl, unsigned int devNum)
{
    intCauseDev[intl] = ResetBit(intCauseDev[intl], devNum);
    // memory address changes
#if 0
    watch->SignalBusAccess(CDEV_BITMAP_ADDR(intl + DEV_IL_START), WRITE);
#endif
    if (intCauseDev[intl] == 0UL)
        // there are no more interrupts pending for that line
        intPendMask = ResetBit(intPendMask, intl + DEVINTBASE + IPMASKBASE);
}


// This method returns the current interrupt line status
Word SystemBus::getPendingInt(void)
{
    return(intPendMask);
}


// This method returns the Device object with given "coordinates"
Device * SystemBus::getDev(unsigned int intL, unsigned int dNum)
{
    if (intL < DEVINTUSED  && dNum < DEVPERINT)
        return(devTable[intL][dNum]);
    else {
        Panic("Unknown device specified in SystemBus::getDev()");
        // never returns
        return NULL;
    }
}


/****************************************************************************/
/* Definitions strictly local to the module.                                */
/****************************************************************************/

// This method reads the data at the address addr, and passes it back thru
// the datap pointer. It also return FALSE if the addr is valid, and TRUE
// otherwise
bool SystemBus::busRead(Word addr, Word* datap, Processor* cpu)
{
    if (INBOUNDS(addr, RAMBASE, RAMBASE + ram->Size()))
        *datap = ram->MemRead(CONVERT(addr, RAMBASE));
    else if (INBOUNDS(addr, BIOSBASE, BIOSBASE + bios->Size()))
        *datap = bios->MemRead(CONVERT(addr,BIOSBASE));
    else if (INBOUNDS(addr, BOOTBASE, BOOTBASE + boot->Size()))
        *datap = boot->MemRead(CONVERT(addr, BOOTBASE));
    else if (INBOUNDS(addr, DEV_BASE, DEV_END))
        *datap = busRegRead(addr, cpu);
    else {
        // address invalid: data read is out of bounds
        *datap = MAXWORDVAL;
        return true;
    }
    // address was valid 
    return false;
}


// This method returns the value for the device field addressed in the "bus
// register area"
Word SystemBus::busRegRead(Word addr, Processor* cpu)
{
    Word data;

    if (DEV_REG_START <= addr && addr < DEV_REG_END) {
        // We're in the device register space
        DeviceAreaAddress da(addr);
        Device* device = devTable[da.line()][da.device()];
        data = device->ReadDevReg(da.field());
    } else if (INBOUNDS(addr, IDEV_BITMAP_BASE, IDEV_BITMAP_END)) {
        // We're in the "installed-devices bitmap" structure space
        unsigned int wordIndex = CONVERT(addr, IDEV_BITMAP_BASE);
        data = instDevTable[wordIndex];
    } else if (INBOUNDS(addr, CDEV_BITMAP_BASE, CDEV_BITMAP_END)) {
        // We're in the "interrupting-devices bitmap" structure space
        unsigned int wordIndex = CONVERT(addr, CDEV_BITMAP_BASE);
        data = intCauseDev[wordIndex];
    } else if (MPC_BASE <= addr && addr < MPC_END) {
        data = mpc->Read(addr, cpu);
    } else {
        // We're in the low "bus register area" space
        switch (addr) {
        case BUS_REG_TIME_SCALE:
            // number of ticks for a microsecond
            data = config->getClockRate();
            break;
        case BUS_REG_TOD_HI:
            data = timeOfDay->getHiTS();
            break;
        case BUS_REG_TOD_LO:
            data = timeOfDay->getLoTS();
            break;
        case BUS_REG_TIMER:
            data = timer;
            break;
        case BUS_REG_RAM_BASE:
            data = RAMBASE;
            break;
        case BUS_REG_RAM_SIZE:
            data = ram->Size();
            break;
        case BUS_REG_BIOS_BASE:
            data = BIOSBASE;
            break;
        case BUS_REG_BIOS_SIZE:
            data = bios->Size();
            break;
        case BUS_REG_BOOT_BASE:
            data = BOOTBASE;
            break;
        case BUS_REG_BOOT_SIZE:
            data = boot->Size();
            break;
        default:
            // unmapped bus device register area:
            // read give 0, write has no effects
            data = 0UL;
            break;
        }
    }
    return data;
}

// This method accesses the system configuration and constructs
// the devices needed, linking them to SystemBus object
Device* SystemBus::makeDev(unsigned int intl, unsigned int dnum)
{
    unsigned int devt;
    Device * dev;

    devt = config->getDeviceType(intl, dnum);

#define CREATE_DEV(type) (setup ? new type(this, setup, intl, dnum) : new type(this, config, intl, dnum))

    switch(devt)
    {
    case PRNTDEV:
        dev = new PrinterDevice(this, config, intl, dnum);
        break;

    case TERMDEV:
        dev = new TerminalDevice(this, config, intl, dnum);
        break;
			
    case ETHDEV:
        dev = new EthDevice(this, config, intl, dnum);
        break;
			
    case DISKDEV:
        dev = new DiskDevice(this, config, intl, dnum);
        break;
		
    case TAPEDEV:
        dev = new TapeDevice(this, config, intl, dnum);
        break;
			
    default:
        dev = new Device(this, intl, dnum);
        break;
    }

    return dev;
}

// This method writes the data at the physical address addr, and passes it
// back thru the datap pointer. It also return FALSE if the addr is valid
// and writable, and TRUE otherwise
bool SystemBus::busWrite(Word addr, Word data, Processor* cpu)
{
    if (INBOUNDS(addr, RAMBASE, RAMBASE + ram->Size())) {
        ram->MemWrite(CONVERT(addr, RAMBASE), data);
    } else if (INBOUNDS(addr, DEV_BASE, DEV_END)) {
        if (DEV_REG_START <= addr && addr < DEV_REG_END) {
            DeviceAreaAddress dva(addr);
            Device* device = devTable[dva.line()][dva.device()];
            device->WriteDevReg(dva.field(), data);
        } else if (MPC_BASE <= addr && addr < MPC_END) {
            mpc->Write(addr, data, NULL);
        } else {
            // data write is in bus registers area
            if (addr == BUS_REG_TIMER) {
                // update the interval timer and reset its interrupt line
                timer = data;
                intPendMask = ResetBit(intPendMask, TIMERINT + IPMASKBASE);
            }	
            // else data write is on a read only bus register, and
            // has no harmful effects
        }
    } else {
        // Address out of valid write bounds
        return(true);
    }

    return(false);
}
