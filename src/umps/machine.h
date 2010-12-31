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

#ifndef UMPS_MACHINE_H
#define UMPS_MACHINE_H

#include <vector>
#include <list>

#include <sigc++/sigc++.h>

#include "base/lang.h"
#include "umps/machine_config.h"

enum StopCause {
    SC_USER         = 1 << 0,
    SC_BREAKPOINT   = 1 << 1,
    SC_SUSPECT      = 1 << 2,
    SC_EXCEPTION    = 1 << 3,
    SC_UTLB_KERNEL  = 1 << 4,
    SC_UTLB_USER    = 1 << 5
};

class Processor;
class SystemBus;
class Device;
class StoppointSet;

class Machine {
public:
    sigc::signal<void> SignalStatusChanged;
    sigc::signal<void, Processor*> SignalCpuStatusChanged;

    Machine(const MachineConfig* config,
            StoppointSet* breakpoints,
            StoppointSet* suspects,
            StoppointSet* tracepoints);
    ~Machine();

    void Step(bool* stopped = NULL);
    void Step(unsigned int steps, unsigned int* stepped = NULL, bool* stopped = NULL);

    Processor* getProcessor(unsigned int cpuId);
    Device* getDevice(unsigned int line, unsigned int devNo);
    SystemBus* getBus();

    void setStopMask(unsigned int mask);
    unsigned int getStopMask() const;

    unsigned int getStopCause(unsigned int cpuId) const;
    unsigned int getActiveBreakpoint(unsigned int cpuId) const;

    bool ReadMemory(Word physAddr, Word* data);

    void busAccessHandler(Word pAddr, Word access, Processor* cpu);

private:
    struct ProcessorData {
        unsigned int stopCause;
        unsigned int breakpointId;
        unsigned int suspectId;
    };

    void cpuStatusChangedHandler(Processor* cpu);
    void cpuExceptionHandler(unsigned int, Processor* cpu);

    unsigned int stopMask;

    const MachineConfig* const config;

    scoped_ptr<SystemBus> bus;

    typedef std::vector<Processor*> CpuVector;
    std::vector<Processor*> cpus;

    ProcessorData pd[MachineConfig::MAX_CPUS];

    typedef std::list<unsigned int> CpuIdList;
    CpuIdList activeCpus;

    bool stopPointsReached;

    StoppointSet* breakpoints;
    StoppointSet* suspects;
    StoppointSet* tracepoints;
};

#endif // UMPS_MACHINE_H
