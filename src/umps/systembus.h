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

#ifndef UMPS_SYSTEMBUS_H
#define UMPS_SYSTEMBUS_H

#include <sigc++/sigc++.h>

#include "base/lang.h"
#include "umps/event.h"
#include "umps/const.h"

class Machine;
class MachineConfig;
class Device;
class Processor;
class TimeStamp;
class RamSpace;
class BiosSpace;
class Block;
class MPController;
class InterruptController;

class SystemBus {
public:
    SystemBus(const MachineConfig* config, Machine* machine);
    ~SystemBus();

    // This method increments system clock and decrements interval
    // timer; on timer underflow (0 -> FFFFFFFF transition) a interrupt
    // is generated.  Event queue is checked against the current clock
    // value and device operations are completed if needed; all memory
    // changes are notified to Watch control object
    void ClockTick();

    uint32_t IdleCycles() const;

    void Skip(uint32_t cycles);

    // This method reads a data word from memory at physical address
    // addr, returning it thru datap pointer. It also returns TRUE if
    // the address was invalid and an exception was caused, FALSE
    // otherwise, and signals memory access to Watch control object
    bool DataRead(Word addr, Word* datap, Processor* cpu);

    // This method writes the data word at physical addr in RAM memory or device
    // register area.  Writes to BIOS or BOOT areas cause a DBEXCEPTION (no
    // writes allowed). It returns TRUE if an exception was caused, FALSE
    // otherwise, and notifies access to Watch control object
    bool DataWrite(Word addr, Word data, Processor* proc);

    bool CompareAndSet(Word addr, Word oldval, Word newval, bool* result, Processor* cpu);

    // This method reads a istruction from memory at physical address addr,
    // returning it thru istrp pointer. It also returns TRUE if the
    // address was invalid and an exception was caused, FALSE otherwise,
    // and notifies Watch
    bool InstrRead(Word addr, Word* instrp, Processor* proc);

    // This method transfers a block from or to memory, starting with
    // address startAddr; it returns TRUE is transfer was not successful
    // (non-existent memory, read-only memory, unaligned addresses),
    // FALSE otherwise.  It notifies too the memory accesses to Watch
    // control object
    bool DMATransfer(Block * blk, Word startAddr, bool toMemory);

    // This method transfers a partial block from or to memory, starting with
    // address startAddr; it returns TRUE is transfer was not successful
    // (non-existent memory, read-only memory, unaligned addresses),
    // FALSE otherwise.  It notifies too the memory accesses to Watch
    // control object
    bool DMAVarTransfer(Block * blk, Word startAddr, Word byteLength, bool toMemory);

    TimeStamp* ScheduleEvent(Word delay, Event::Callback callback);

    // This method sets the appropriate bits into intCauseDev[] and
    // IntPendMask to signal device interrupt pending; it notifies
    // memory changes to Watch too
    void IntReq(unsigned int intNum, unsigned int devNum);

    // This method resets the appropriate bits into intCauseDev[] and
    // IntPendMask to signal device interrupt acknowlege; it notifies
    // memory changes to Watch too
    void IntAck(unsigned int intNum, unsigned int devNum);

    // This method returns the current interrupt line status
    Word getPendingInt(const Processor* cpu);

    void AssertIRQ(unsigned int il, unsigned int target);
    void DeassertIRQ(unsigned int il, unsigned int target);

    Machine* getMachine() { return machine; }

    // This method returns the Device object with given "coordinates"
    Device * getDev(unsigned int intL, unsigned int dNum);

    // These methods allow to inspect or modify  TimeofDay Clock and
    // Interval Timer (typically for simulation reasons)

    Word getToDHI();
    Word getToDLO();
    Word getTimer();
    void setToDHI(Word hi);
    void setToDLO(Word lo);
    void setTimer(Word time);

    // These methods allow Watch to inspect or modify single memory
    // locations; they return TRUE if address is invalid or cannot be
    // changed, and FALSE otherwise

    bool WatchRead(Word addr, Word * datap);
    bool WatchWrite(Word addr, Word data);

private:
    const MachineConfig* const config;

    Machine* const machine;

    scoped_ptr<InterruptController> pic;

    scoped_ptr<MPController> mpController;

    // system clock & interval timer
    TimeStamp* timeOfDay;
    Word timer;

    // device events queue
    EventQueue * eventQ;

    // physical memory spaces
    RamSpace * ram;
    BiosSpace * bios;
    BiosSpace * boot;

    // device handling & interrupt generation tables
    Device* devTable[DEVINTUSED][DEVPERINT];
    Word instDevTable[DEVINTUSED];

    // pending interrupts on lines: this word is packed into MIPS Cause
    // Register IP field format for easy masking
    Word intPendMask;

    // This method read the data at physical address addr, and
    // passes it back thru the datap pointer. It also return FALSE if
    // the addr is valid, and TRUE otherwise
    bool busRead(Word addr, Word* datap, Processor* cpu = 0);

    // This method returns the value for the device field addressed in
    // the "bus register area"
    Word busRegRead(Word addr, Processor* cpu);

    // This method writes the data at physical address addr, and
    // passes it back thru the datap pointer. It also return FALSE if
    // the addr is valid and writable, and TRUE otherwise
    bool busWrite(Word addr, Word data, Processor* cpu = 0);

    // This method accesses the system configuration and constructs
    // the devices needed, linking them to SystemBus object
    Device * makeDev(unsigned int intl, unsigned int dnum);
};

#endif // UMPS_SYSTEMBUS_H
