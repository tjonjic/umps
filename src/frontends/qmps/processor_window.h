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

#ifndef QMPS_PROCESSOR_WINDOW_H
#define QMPS_PROCESSOR_WINDOW_H

#include <QMainWindow>

#include "umps/types.h"

class Processor;
class StoppointListModel;
class DebugSession;
class QVBoxLayout;
class QLabel;
class QCheckBox;
class QTextEdit;
class CodeView;
class QLayout;
class RegisterSetWidget;

class ProcessorWindow : public QMainWindow {
    Q_OBJECT

public:
    ProcessorWindow(Word cpuId, QWidget* parent = 0);

protected:
    virtual void closeEvent(QCloseEvent* event);

private:
    void createMenu();
    void createToolBar();
    QLayout* createInstrPanel();
    void createDockableWidgets();

    DebugSession* const dbgSession;
    const Word cpuId;
    Processor* cpu;

    QVBoxLayout* centralLayout;

    QLabel* statusLabel;
    CodeView* codeView;

    QLabel* prevPCLabel;
    QLabel* prevInstructionLabel;
    QLabel* pcLabel;
    QLabel* instructionLabel;

    QLabel* vmIndicator;
    QLabel* bdIndicator;
    QLabel* ldIndicator;
    QLabel* secStatusLabel;

    RegisterSetWidget* regView;

private Q_SLOTS:
    void onMachineReset();
    void updateStatusInfo();
};

#endif // QMPS_PROCESSOR_WINDOW_H
