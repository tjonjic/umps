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

#include "qmps/debug_session.h"

#include <cstdlib>
#include <algorithm>
#include <list>

#include <QAction>
#include <QTimer>
#include <QMessageBox>

#include <QtDebug>

#include "umps/error.h"
#include "qmps/application.h"

unsigned int DebugSession::cyclesPerIteration[kNumSpeedLevels] = {
    5,
    25,
    250,
    2500,
    20000
};

unsigned int DebugSession::iterationTimeoutInterval[kNumSpeedLevels] = {
    50,
    25,
    5,
    1,
    0
};

DebugSession::DebugSession()
    : status(MS_HALTED)
{
    createActions();
    updateActionSensitivity();
    connect(this, SIGNAL(StatusChanged()), this, SLOT(updateActionSensitivity()));

    connect(Appl(), SIGNAL(MachineConfigChanged()), this, SLOT(onMachineConfigChanged()));

#if 0
    breakpoints.Add(AddressRange(64, 0x20003F20, 0x20003F20), AM_EXEC);
    breakpoints.Add(AddressRange(64, 0x1fc00004, 0x1fc00004), AM_EXEC);
    suspects.Add(AddressRange(64, 0x20003F20, 0x20003F20), AM_READ);
    suspects.Add(AddressRange(64, 0x20003F24, 0x20003F24), AM_READ_WRITE);
    suspects.Add(AddressRange(64, 0x2000DA30, 0x2000DA40), AM_WRITE);
    suspects.Add(AddressRange(64, 0x200A0000, 0x200ABBB0), AM_READ);
    tracepoints.Add(AddressRange(64, 0x2000D010, 0x2000D034), AM_WRITE);
    tracepoints.Add(AddressRange(64, 0x20001010, 0x20001010), AM_WRITE);
    tracepoints.Add(AddressRange(64, 0x20002200, 0x20002900), AM_WRITE);
    tracepoints.Add(AddressRange(64, 0x20000000, 0x20000500), AM_WRITE);
    tracepoints.Add(AddressRange(64, 0x20010000, 0x200100A0), AM_WRITE);
#endif

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(runIteration()));

    setSpeed(Appl()->settings.value("SimulationSpeed", kMaxSpeed).toInt());

    stopMask = Appl()->settings.value("StopMask", kDefaultStopMask).toUInt();
}

void DebugSession::Halt()
{
    if (!IsStarted())
        return;

    if (IsRunning())
        timer->stop();

    Q_EMIT MachineAboutToBeHalted();
    machine.reset();
    symbolTable.reset();

    setStatus(MS_HALTED);
}

void DebugSession::setSpeed(int value)
{
    value = qBound(0, value, kMaxSpeed);
    if (speed != value) {
        speed = value;
        Appl()->settings.setValue("SimulationSpeed", speed);
        timer->setInterval(iterationTimeoutInterval[speed]);
        Q_EMIT SpeedChanged(speed);
    }
}

void DebugSession::setStopMask(unsigned int value)
{
    stopMask = value;
    Appl()->settings.setValue("StopMask", stopMask);

    if (machine.get())
        machine->setStopMask(stopMask);
}

void DebugSession::createActions()
{
    startMachineAction = new QAction("Power On", this);
    startMachineAction->setShortcut(QKeySequence("F5"));
    startMachineAction->setIcon(QIcon(":/icons/machine_start-22.png"));
    connect(startMachineAction, SIGNAL(triggered()), this, SLOT(onStartMachine()));
    startMachineAction->setEnabled(false);

    haltMachineAction = new QAction("Shut Down", this);
    haltMachineAction->setShortcut(QKeySequence("Shift+F5"));
    haltMachineAction->setIcon(QIcon(":/icons/machine_halt-22.png"));
    connect(haltMachineAction, SIGNAL(triggered()), this, SLOT(onHaltMachine()));
    haltMachineAction->setEnabled(false);

    debugContinueAction = new QAction("&Continue", this);
    debugContinueAction->setShortcut(QKeySequence("F9"));
    debugContinueAction->setIcon(QIcon(":/icons/debug_continue-22.png"));
    connect(debugContinueAction, SIGNAL(triggered()), this, SLOT(onContinue()));

    debugStepAction = new QAction("Step", this);
    debugStepAction->setShortcut(QKeySequence("F10"));
    debugStepAction->setIcon(QIcon(":/icons/debug_step-22.png"));
    connect(debugStepAction, SIGNAL(triggered()), this, SLOT(onStep()));

    debugStopAction = new QAction("&Stop", this);
    debugStopAction->setShortcut(QKeySequence("F12"));
    debugStopAction->setIcon(QIcon(":/icons/debug_stop-22.png"));
    connect(debugStopAction, SIGNAL(triggered()), this, SLOT(onStop()));
}

void DebugSession::updateActionSensitivity()
{
    bool started = (status != MS_HALTED);
    bool stopped = (status == MS_STOPPED);
    bool running = (status == MS_RUNNING);

    startMachineAction->setEnabled(Appl()->getConfig() != NULL && !started);
    haltMachineAction->setEnabled(started);

    debugContinueAction->setEnabled(stopped);
    debugStepAction->setEnabled(stopped);

    debugStopAction->setEnabled(running);
}

void DebugSession::setStatus(MachineStatus newStatus)
{
    if (newStatus != status) {
        status = newStatus;
        Q_EMIT StatusChanged();
    }
}

void DebugSession::onMachineConfigChanged()
{
    if (Appl()->getConfig() != NULL)
        startMachineAction->setEnabled(true);

    breakpoints.Clear();
    suspects.Clear();
    tracepoints.Clear();
}

void DebugSession::onStartMachine()
{
    assert(status == MS_HALTED);

    MachineConfig* config = Appl()->getConfig();
    assert(config != NULL);

    std::list<std::string> errors;
    if (!config->Validate(&errors)) {
        QString el;
        el += "<ul>";
        foreach (const std::string& s, errors)
            el += QString("<li>%1</li>").arg(s.c_str());
        el += "</ul>";
        QMessageBox::critical(
            0, QString("%1: Error").arg(Appl()->applicationName()),
            "Invalid and/or incomplete machine configuration: " + el);
        return;
    }

    try {
        machine.reset(new Machine(config, &breakpoints, &suspects, &tracepoints));
    } catch (const FileError& e) {
        QMessageBox::critical(
            Appl()->getApplWindow(),
            QString("%1: Error").arg(Appl()->applicationName()),
            QString("<b>Could not initialize machine:</b> "
                    "the file `%1' is nonexistent or inaccessible").arg(e.fileName.c_str()));
        return;
    } catch (const InvalidCoreFileError& e) {
        QMessageBox::critical(
            Appl()->getApplWindow(),
            QString("%1: Error").arg(Appl()->applicationName()),
            QString("<b>Could not initialize machine:</b> "
                    "the file `%1' does not appear to be a valid <i>Core</i> file; "
                    "make sure you are creating the file with the <code>umps2-elf2umps</code> utility")
            .arg(e.fileName.c_str()));
        return;
    } catch (const CoreFileOverflow& e) {
        QMessageBox::critical(
            Appl()->getApplWindow(),
            QString("%1: Error").arg(Appl()->applicationName()),
            "<b>Could not initialize machine:</b> "
            "the core file does not fit in memory; "
            "please increase available RAM and try again");
        return;
    } catch (const InvalidFileFormatError& e) {
        QMessageBox::critical(
            Appl()->getApplWindow(),
            QString("%1: Error").arg(Appl()->applicationName()),
            QString("<b>Could not initialize machine:</b> "
                    "the file `%1' has wrong format").arg(e.fileName.c_str()));
        return;
    }
    machine->setStopMask(stopMask);

    try {
        symbolTable.reset(new SymbolTable(config->getSymbolTableASID(),
                                          config->getROM(ROM_TYPE_STAB).c_str()));
    } catch (const Error& e) {
        QMessageBox::critical(
            Appl()->getApplWindow(),
            QString("%1: Error").arg(Appl()->applicationName()),
            "<b>Could not initialize machine:</b> "
            "invalid or missing symbol table file");
        machine.reset();
        return;
    }

    stoppedByUser = true;
    setStatus(MS_STOPPED);

    cpuStatusMap.reset(new CpuStatusMap(this));

    Q_EMIT MachineStarted();
}

void DebugSession::onHaltMachine()
{
    assert(status != MS_HALTED);
    Halt();
}

void DebugSession::onContinue()
{
    assert(status == MS_STOPPED);

    stepping = false;
    stoppedByUser = false;
    Q_EMIT MachineRan();
    setStatus(MS_RUNNING);

    timer->start();
}

void DebugSession::onStep()
{
    step(1);
}

void DebugSession::onMultiStep()
{
    step(1);
}

void DebugSession::onStop()
{
    if (status == MS_RUNNING) {
        stoppedByUser = true;
        timer->stop();
        setStatus(MS_STOPPED);
        Q_EMIT MachineStopped();
    }
}

void DebugSession::step(unsigned int steps)
{
    assert(status == MS_STOPPED);

    stepping = true;
    stepsLeft = steps;
    stoppedByUser = false;
    Q_EMIT MachineRan();
    setStatus(MS_RUNNING);

    // Always step through at least one cycle (might be a bit too
    // pedantic but oh well...)
    bool stopped;
    machine->Step(&stopped);
    --stepsLeft;

    if (!stepsLeft || stopped) {
        stoppedByUser = !stepsLeft;
        setStatus(MS_STOPPED);
        Q_EMIT MachineStopped();
    } else {
        timer->start();
    }
}

void DebugSession::runIteration()
{
    if (stepping)
        runStepIteration();
    else
        runContIteration();
}

void DebugSession::runStepIteration()
{
    unsigned int steps = std::min(stepsLeft, cyclesPerIteration[speed]);

    bool stopped = false;
    unsigned int stepped;
    machine->Step(steps, &stepped, &stopped);
    stepsLeft -= stepped;

    if (stopped || stepsLeft == 0) {
        stoppedByUser = (stepsLeft == 0);
        timer->stop();
        setStatus(MS_STOPPED);
        Q_EMIT MachineStopped();
    } else {
        Q_EMIT DebugIterationCompleted();
    }
}

void DebugSession::runContIteration()
{
    bool stopped;
    unsigned int stepped;
    machine->Step(cyclesPerIteration[speed], &stepped, &stopped);

    if (stopped) {
        setStatus(MS_STOPPED);
        Q_EMIT MachineStopped();
        timer->stop();
    } else {
        Q_EMIT DebugIterationCompleted();
    }
}
