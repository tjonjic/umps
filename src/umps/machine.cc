/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2010 Tomislav Jonjic
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

#include "umps/machine.h"

#include <iostream>

#include <algorithm>
#include <cassert>
#include <cstdlib>

#include "base/lang.h"
#include "base/debug.h"

#include "umps/types.h"
#include "umps/const.h"
#include "umps/processor.h"

#include "umps/machine_config.h"
#include "umps/stoppoint.h"
#include "umps/systembus.h"

Machine::Machine(const MachineConfig* config,
                 StoppointSet* breakpoints,
                 StoppointSet* suspects,
                 StoppointSet* tracepoints)
    : stopMask(0),
      config(config),
      breakpoints(breakpoints),
      suspects(suspects),
      tracepoints(tracepoints)
{
    assert(config->Validate(NULL));

    bus.reset(new SystemBus(config, this));
    bus->SignalAccess.connect(sigc::mem_fun(this, &Machine::busAccessHandler));

    for (unsigned int i = 0; i < config->getNumProcessors(); i++) {
        Processor* cpu = new Processor(config, i, this, bus.get());
        cpu->SignalException.connect(
            sigc::bind(sigc::mem_fun(this, &Machine::cpuExceptionHandler), cpu)
        );
        cpu->SignalStatusChanged.connect(
            sigc::bind(sigc::mem_fun(this, &Machine::cpuStatusChangedHandler), cpu)
        );
        pd[i].stopCause = 0;
        cpus.push_back(cpu);

        bus->LinkProcessor(cpu);
    }

    cpus[0]->Start();
}

Machine::~Machine()
{
    foreach (Processor* p, cpus)
        delete p;
}

void Machine::Step(unsigned int steps, unsigned int* stepped, bool* stopped)
{
    stopPointsReached = false;
    foreach (unsigned int cpuId, activeCpus)
        pd[cpuId].stopCause = 0;

    unsigned int i;
    for (i = 0; i < steps && !stopPointsReached; ++i) {
        bus->ClockTick();
        foreach (unsigned int cpuId, activeCpus)
            cpus[cpuId]->Cycle();
    }
    if (stepped)
        *stepped = i;
    if (stopped)
        *stopped = stopPointsReached;
}

void Machine::Step(bool* stopped)
{
    stopPointsReached = false;

    bus->ClockTick();

    foreach (unsigned int cpuId, activeCpus) {
        pd[cpuId].stopCause = 0;
        cpus[cpuId]->Cycle();
    }

    if (stopped != NULL)
        *stopped = stopPointsReached;
}

void Machine::cpuExceptionHandler(unsigned int excCode, Processor* cpu)
{
    if (stopMask & SC_EXCEPTION) {
        pd[cpu->getId()].stopCause |= SC_EXCEPTION;
        stopPointsReached = true;
    }
}

void Machine::cpuStatusChangedHandler(Processor* cpu)
{
    if (cpu->getStatus() == PS_RUNNING) {
        activeCpus.push_back(cpu->getId());
    }
}

void Machine::busAccessHandler(Word pAddr, Word access, Processor* cpu)
{
    // Check for breakpoints and suspects
    switch (access) {
    case READ:
    case WRITE:
        if (stopMask & SC_SUSPECT) {
            Stoppoint* suspect = suspects->Probe(MAXASID, pAddr,
                                                 (access == READ) ? AM_READ : AM_WRITE,
                                                 cpu);
            if (suspect != NULL) {
                pd[cpu->getId()].stopCause |= SC_SUSPECT;
                pd[cpu->getId()].suspectId = suspect->getId();
                stopPointsReached = true;
            }
        }
        break;

    case EXEC:
        if (stopMask & SC_BREAKPOINT) {
            Stoppoint* breakpoint = breakpoints->Probe(MAXASID, pAddr, AM_EXEC, cpu);
            if (breakpoint != NULL) {
                pd[cpu->getId()].stopCause |= SC_BREAKPOINT;
                pd[cpu->getId()].breakpointId = breakpoint->getId();
                stopPointsReached = true;
            }
        }
        break;

    default:
        AssertNotReached();
    }

    // Check for traced ranges
    if (access == WRITE) {
        Stoppoint* tracepoint = tracepoints->Probe(MAXASID, pAddr, AM_WRITE, cpu);
        (void) tracepoint;
    }
}

Processor* Machine::getProcessor(unsigned int cpuId)
{
    return cpus[cpuId];
}

Device* Machine::getDevice(unsigned int line, unsigned int devNo)
{
    return bus->getDev(line, devNo);
}

SystemBus* Machine::getBus()
{
    return bus.get();
}

void Machine::setStopMask(unsigned int mask)
{
    stopMask = mask;
}

unsigned int Machine::getStopMask() const
{
    return stopMask;
}

unsigned int Machine::getStopCause(unsigned int cpuId) const
{
    assert(cpuId < config->getNumProcessors());
    return pd[cpuId].stopCause;
}

unsigned int Machine::getActiveBreakpoint(unsigned int cpuId) const
{
    assert(cpuId < config->getNumProcessors());
    return pd[cpuId].breakpointId;
}

unsigned int Machine::getActiveSuspect(unsigned int cpuId) const
{
    assert(cpuId < config->getNumProcessors());
    return pd[cpuId].suspectId;
}

bool Machine::ReadMemory(Word physAddr, Word* data)
{
    return bus->WatchRead(physAddr, data);
}

bool Machine::WriteMemory(Word paddr, Word data)
{
    return bus->WatchWrite(paddr, data);
}
