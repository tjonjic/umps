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

#include "qmps/processor_window.h"

#include <QMenu>
#include <QMenuBar>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QTreeView>
#include <QCheckBox>
#include <QSplitter>
#include <QTextEdit>
#include <QGroupBox>
#include <QDockWidget>
#include <QStatusBar>

#include "umps/processor.h"
#include "umps/disassemble.h"
#include "qmps/application.h"
#include "qmps/code_view.h"
#include "qmps/ui_utils.h"
#include "qmps/register_set_widget.h"

static const int kDefaultWidth = 400;
static const int kDefaultHeight = 540;

ProcessorWindow::ProcessorWindow(Processor* p,
                                 StoppointListModel* bplModel,
                                 QWidget* parent)
    : QMainWindow(parent),
      dbgSession(Appl()->getDebugSession()),
      cpu(p)
{
    setWindowTitle(QString("uMPS: Processor %1").arg(cpu->getId()));

    createDockableWidgets();

    createMenu();
    createToolBar();

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 7, 0, 0);

    statusLabel = new QLabel;
    statusLabel->setIndent(3);
    statusLabel->setFont(Appl()->getMonospaceFont());
    centralLayout->addWidget(statusLabel);

    centralLayout->addWidget(new CodeView(cpu, bplModel));
    centralLayout->addLayout(createInstrPanel());

    secStatusLabel = new QLabel;
    QWidget* secStatusWidget = new QWidget;
    QHBoxLayout* secStatusLayout = new QHBoxLayout(secStatusWidget);
    secStatusLayout->setSpacing(12);
    secStatusLayout->setContentsMargins(0, 0, 0, 0);
    secStatusLayout->addWidget(vmIndicator = new QLabel("VM"));
    secStatusLayout->addWidget(bdIndicator = new QLabel("BD"));
    secStatusLayout->addWidget(ldIndicator = new QLabel("LD"));

    statusBar()->addPermanentWidget(secStatusWidget);

    updateStatusInfo();

    QString key = QString("CpuWindow%1/geometry").arg((quint32) cpu->getId());
    QVariant savedGeometry = Appl()->settings.value(key);
    if (savedGeometry.isValid())
        restoreGeometry(savedGeometry.toByteArray());
    else
        resize(kDefaultWidth, kDefaultHeight);

    connect(dbgSession->getCpuStatusMap(), SIGNAL(Changed()),
            this, SLOT(updateStatusInfo()));
}

void ProcessorWindow::closeEvent(QCloseEvent* event)
{
    QString key = QString("CpuWindow%1/geometry").arg((quint32) cpu->getId());
    Appl()->settings.setValue(key, saveGeometry());
    event->accept();
}

void ProcessorWindow::createMenu()
{
    QMenu* debugMenu = menuBar()->addMenu("&Debug");
    debugMenu->addAction(dbgSession->debugContinueAction);
    debugMenu->addAction(dbgSession->debugStepAction);
    debugMenu->addAction(dbgSession->debugStopAction);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(regView->toggleViewAction());

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    (void) helpMenu;
}

void ProcessorWindow::createToolBar()
{
    QToolBar* toolBar = addToolBar("Toolbar");
    toolBar->addAction(dbgSession->debugContinueAction);
    toolBar->addAction(dbgSession->debugStepAction);
    toolBar->addAction(dbgSession->debugStopAction);
}

QLayout* ProcessorWindow::createInstrPanel()
{
    static const int MIN_COL_SPACING = 12;
    static const int HEADER_COL = 0;
    static const int PC_COL = 2;

    QGridLayout* grid = new QGridLayout;

    grid->setColumnMinimumWidth(1, MIN_COL_SPACING);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setColumnStretch(PC_COL, 2);

    int rows = 0;

    prevPCLabel = new QLabel;
    prevPCLabel->setMinimumWidth(30);
    prevPCLabel->setFont(Appl()->getMonospaceFont());
    prevPCLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    grid->addWidget(new QLabel("Prev. PC:"), rows, HEADER_COL, Qt::AlignLeft);
    grid->addWidget(prevPCLabel, rows++, PC_COL);

    pcLabel = new QLabel;
    pcLabel->setMinimumWidth(30);
    pcLabel->setFont(Appl()->getMonospaceFont());
    pcLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    grid->addWidget(new QLabel("PC:"), rows, HEADER_COL, Qt::AlignLeft);
    grid->addWidget(pcLabel, rows++, PC_COL);

    return grid;
}

void ProcessorWindow::createDockableWidgets()
{
    regView = new RegisterSetWidget(cpu);
    addDockWidget(Qt::BottomDockWidgetArea, regView);
}

void ProcessorWindow::updateStatusInfo()
{
    QString str;

    Word prevPC, prevInstr;
    cpu->getPrevStatus(&prevPC, &prevInstr);
    prevPCLabel->setText(str.sprintf("0x%.8X: %s", prevPC, StrInstr(prevInstr)));

    Word asid, pc, instr;
    bool isLD, isBD, isVM;
    cpu->getCurrStatus(&asid, &pc, &instr, &isLD, &isBD, &isVM);
    pcLabel->setText(str.sprintf("0x%.8X: %s", pc, StrInstr(instr)));

    vmIndicator->setEnabled(isVM);
    bdIndicator->setEnabled(isBD);
    ldIndicator->setEnabled(isLD);

    QString newStatus = dbgSession->getCpuStatusMap()->getLocation(cpu->getId());
    if (dbgSession->IsStopped())
        newStatus.append(QString(" [%1]").arg(dbgSession->getCpuStatusMap()->getStatus(cpu->getId())));
    statusLabel->setText(newStatus);
}
