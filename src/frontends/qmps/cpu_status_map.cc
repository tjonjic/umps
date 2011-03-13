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
      statusMap(Appl()->getConfig()->getNumProcessors())
{
    connect(dbgSession, SIGNAL(MachineStopped()), this, SLOT(update()));
    connect(dbgSession, SIGNAL(MachineRan()), this, SLOT(update()));
    connect(dbgSession, SIGNAL(DebugIterationCompleted()), this, SLOT(update()));
    update();
}

const QString& CpuStatusMap::getStatus(unsigned int cpuId) const
{
    return statusMap[cpuId].status;
}

const QString& CpuStatusMap::getLocation(unsigned int cpuId) const
{
    return statusMap[cpuId].location;
}

const char* const CpuStatusMap::statusTemplates[] = {
    "Running",
    "Stopped: User",
    "Stopped: Breakpoint(B%brkpt%)",
    "Stopped: User+Breakpoint(B%brkpt%)",
    "Stopped: Suspect(S%susp%)",
    "Stopped: User+Suspect(S%susp%)",
    "Stopped: Breakpoint(B%brkpt%)+Suspect(S%susp%)",
    "Stopped: User+Breakpoint(B%brkpt%)+Suspect(S%susp%)",
    "Stopped: Exception(%excpt%)",
    "Stopped: User+Exception(%excpt%)",
    "Stopped: Breakpoint(B%brkpt%)+Exception(%excpt%)",
    "Stopped: User+Breakpoint(B%brkpt%)+Exception(%excpt%)",
    "Stopped: Suspect(S%susp%)+Exception(%excpt%)",
    "Stopped: User+Suspect(S%susp%)+Exception(%excpt%)",
    "Stopped: Breakpoint(B%brkpt%)+Suspect(S%susp%)+Exception(%excpt%)",
    "Stopped: User+Breakpoint(B%brkpt%)+Suspect(S%susp%)+Exception(%excpt%)"
};

void CpuStatusMap::update()
{
    MachineConfig* config = Appl()->getConfig();

    for (unsigned int cpuId = 0; cpuId < config->getNumProcessors(); cpuId++) {
        Processor* cpu = machine->getProcessor(cpuId);

        switch (cpu->getStatus()) {
        case PS_OFFLINE:
            statusMap[cpuId].status = "Offline";
            break;
        case PS_IDLE:
            statusMap[cpuId].status = "Idle";
            break;
        case PS_ONLINE:
            formatActiveCpuStatus(cpu);
            formatActiveCpuLocation(cpu);
            break;
        default:
            statusMap[cpuId].status = "Unknown";
            break;
        }
    }

    Q_EMIT Changed();
}

void CpuStatusMap::formatActiveCpuStatus(Processor* cpu)
{
    unsigned int stopCause;
    QString str;

    switch (dbgSession->getStatus()) {
    case MS_STOPPED:
        stopCause = machine->getStopCause(cpu->getId());
        if (dbgSession->IsStoppedByUser())
            stopCause |= SC_USER;

        str = statusTemplates[stopCause];
        if (stopCause & SC_BREAKPOINT)
            str.replace("%brkpt%", QString::number(machine->getActiveBreakpoint(cpu->getId())));
        if (stopCause & SC_SUSPECT)
            str.replace("%susp%", QString::number(machine->getActiveSuspect(cpu->getId())));
        if (stopCause & SC_EXCEPTION)
            str.replace("%excpt%", cpu->getExcCauseStr());

        statusMap[cpu->getId()].status = str;
        break;

    case MS_RUNNING:
        statusMap[cpu->getId()].status = "Running";
        break;

    default:
        // We should never get here!
        statusMap[cpu->getId()].status = QString();
        break;
    }
}

void CpuStatusMap::formatActiveCpuLocation(Processor* cpu)
{
    SymbolTable* stab = dbgSession->getSymbolTable();

    Word asid = (cpu->getVM()) ? cpu->getASID() : MachineConfig::MAX_ASID;
    SWord offset;
    const char* symbol = GetSymbolicAddress(stab, asid, cpu->getPC(), true, &offset);
    if (symbol)
        statusMap[cpu->getId()].location = QString("%1+0x%2").arg(symbol).arg(offset, 0, 16);
    else
        statusMap[cpu->getId()].location = FormatAddress(cpu->getPC());
}
