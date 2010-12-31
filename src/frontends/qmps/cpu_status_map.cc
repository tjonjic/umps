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

#include "qmps/cpu_status_map.h"

#include "umps/symbol_table.h"
#include "umps/machine.h"
#include "umps/processor.h"
#include "qmps/application.h"
#include "qmps/debug_session.h"
#include "qmps/ui_utils.h"

CpuStatusMap::CpuStatusMap(DebugSession* dbgSession)
    : QObject(),
      dbgSession(dbgSession),
      machine(dbgSession->getMachine()),
      statusMap(Appl()->getConfig()->getNumProcessors()),
      statusMap_(Appl()->getConfig()->getNumProcessors())
{
    connect(dbgSession, SIGNAL(MachineStopped()), this, SLOT(update()));
    connect(dbgSession, SIGNAL(MachineRan()), this, SLOT(update()));
    connect(dbgSession, SIGNAL(DebugIterationCompleted()), this, SLOT(update()));
    update();
}

const QString& CpuStatusMap::operator[](unsigned int cpuId) const
{
    return statusMap[cpuId];
}

const QString& CpuStatusMap::getStatus(unsigned int cpuId) const
{
    return statusMap[cpuId];
}

const QString& CpuStatusMap::getLocation(unsigned int cpuId) const
{
    return statusMap_[cpuId].location;
}

const char* const CpuStatusMap::statusTemplates[] = {
    "Running",
    "Stopped: User",
    "Stopped: Breakpoint(B%brkpt%)",
    "Stopped: User+Breakpoint(B%brkpt%)",
    "Stopped: Suspect",
    "Stopped: User+Suspect",
    "Stopped: Breakpoint+Suspect",
    "Stopped: User+Breakpoint(B%brkpt%)+Suspect",
    "Stopped: Exception(%excpt%)",
    "Stopped: User+Exception(%excpt%)",
    "Stopped: Breakpoint+Exception(%excpt%)",
    "Stopped: User+Breakpoint(B%brkpt%)+Exception(%excpt%)",
    "Stopped: Suspect+Exception(%excpt%)",
    "Stopped: User+Suspect+Exception(%excpt%)",
    "Stopped: Breakpoint+Suspect+Exception(%excpt%)",
    "Stopped: User+Breakpoint(B%brkpt%)+Suspect+Exception(%excpt%)"
};

void CpuStatusMap::update()
{
    Machine* machine = dbgSession->getMachine();
    MachineConfig* config = Appl()->getConfig();

    for (unsigned int cpuId = 0; cpuId < config->getNumProcessors(); cpuId++) {
        Processor* cpu = machine->getProcessor(cpuId);

        switch (cpu->getStatus()) {
        case PS_HALTED:
            statusMap[cpuId] = "Halted";
            break;
        case PS_WAITING:
            statusMap[cpuId] = "Waiting";
            break;
        case PS_RUNNING:
            statusMap[cpuId] = formatActiveCpuStatus(cpu);
            formatLocation(cpu);
            break;
        default:
            statusMap[cpuId] = "Unknown";
            break;
        }
    }

    Q_EMIT Changed();
}

QString CpuStatusMap::formatActiveCpuStatus(Processor* cpu)
{
    switch (dbgSession->getStatus()) {
    case MS_STOPPED: {
        unsigned int stopCause = machine->getStopCause(cpu->getId());
        if (dbgSession->IsStoppedByUser())
            stopCause |= SC_USER;

        QString msg = statusTemplates[stopCause];
        if (stopCause & SC_BREAKPOINT)
            msg.replace("%brkpt%", QString::number(machine->getActiveBreakpoint(cpu->getId())));
        if (stopCause & SC_EXCEPTION)
            msg.replace("%excpt%", cpu->getExcCauseStr());

        return msg;
    }

    case MS_RUNNING:
        return "Running";

    default:
        // We should never get here!
        return QString();
    }
}

void CpuStatusMap::formatLocation(Processor* cpu)
{
    SymbolTable* stab = dbgSession->getSymbolTable();

    Word asid = (cpu->getVM()) ? cpu->getASID() : MachineConfig::MAX_ASID;
    SWord offset;
    const char* symbol = GetSymbolicAddress(stab, asid, cpu->getPC(), true, &offset);
    if (symbol)
        statusMap_[cpu->getId()].location = QString("%1+0x%2").arg(symbol).arg((quint32) offset, 0, 16);
    else
        statusMap_[cpu->getId()].location = FormatAddress(cpu->getPC());
}
