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

#ifndef QMPS_DEBUG_SESSION_H
#define QMPS_DEBUG_SESSION_H

#include <QObject>

#include "base/lang.h"
#include "umps/machine.h"
#include "umps/symbol_table.h"
#include "umps/stoppoint.h"
#include "qmps/cpu_status_map.h"
#include "qmps/stoppoint_list_model.h"

enum MachineStatus {
    MS_HALTED,
    MS_RUNNING,
    MS_STOPPED
};

class QAction;
class Machine;
class QTimer;

class DebugSession : public QObject {
    Q_OBJECT

    Q_PROPERTY(int speed READ getSpeed WRITE setSpeed NOTIFY SpeedChanged)

public:
    static const int kNumSpeedLevels = 5;
    static const int kMaxSpeed = kNumSpeedLevels - 1;

    static const int kDefaultStopMask = (SC_BREAKPOINT |
                                         SC_SUSPECT |
                                         SC_EXCEPTION);

    DebugSession();

    MachineStatus getStatus() const { return status; }

    bool IsStopped() const { return status == MS_STOPPED; }
    bool IsStoppedByUser() const { return stoppedByUser; }
    bool IsRunning() const { return status == MS_RUNNING; }
    bool IsStarted() const { return status != MS_HALTED; }

    void Halt();

    unsigned int getStopMask() const { return stopMask; }
    int getSpeed() const { return speed; }

    Machine* getMachine() const { return machine.get(); }
    SymbolTable* getSymbolTable() { return symbolTable.get(); }

    StoppointSet* getBreakpoints() { return &breakpoints; }
    StoppointListModel* getBreakpointListModel() { return bplModel.get(); }

    StoppointSet* getSuspects() { return &suspects; }
    StoppointSet* getTracepoints() { return &tracepoints; }

    const CpuStatusMap* getCpuStatusMap() const { return cpuStatusMap.get(); }

    // Global actions
    QAction* startMachineAction;
    QAction* haltMachineAction;
    QAction* resetMachineAction;

    QAction* debugContinueAction;
    QAction* debugStepAction;
    QAction* debugMultiStepAction;
    QAction* debugStopAction;

public Q_SLOTS:
    void setStopMask(unsigned int value);
    void setSpeed(int value);
    void Stop();

Q_SIGNALS:
    void StatusChanged();
    void MachineStarted();
    void MachineStopped();
    void MachineRan();
    void MachineHalted();
    void MachineReset();
    void DebugIterationCompleted();

    void SpeedChanged(int);

private:
    void createActions();
    void setStatus(MachineStatus newStatus);

    void initializeMachine();

    void step(unsigned int steps);
    void runStepIteration();
    void runContIteration();

    void relocateStoppoints(const SymbolTable* newTable, StoppointSet& set);

    MachineStatus status;
    scoped_ptr<Machine> machine;

    scoped_ptr<SymbolTable> symbolTable;

    // We need a "proxy" stop mask here since it has to live through
    // machine reconfigurations, resets, etc.
    unsigned int stopMask;

    unsigned int speed;
    static unsigned int cyclesPerIteration[kNumSpeedLevels];
    static unsigned int iterationTimeoutInterval[kNumSpeedLevels];

    StoppointSet breakpoints;
    scoped_ptr<StoppointListModel> bplModel;
    StoppointSet suspects;
    StoppointSet tracepoints;

    scoped_ptr<CpuStatusMap> cpuStatusMap;

    bool stoppedByUser;

    bool stepping;
    unsigned int stepsLeft;

    QTimer* timer;

    bool skipIdle;
    uint32_t idleSteps;

private Q_SLOTS:
    void onMachineConfigChanged();

    void startMachine();
    void onHaltMachine();
    void onResetMachine();
    void onContinue();
    void onStep();
    void onMultiStep();

    void updateActionSensitivity();

    void runIteration();
};

#endif // QMPS_DEBUG_SESSION_H
