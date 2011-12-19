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
#include <QDockWidget>
#include <QStatusBar>
#include <QTableView>
#include <QHeaderView>

#include "umps/processor.h"
#include "umps/disassemble.h"
#include "qmps/application.h"
#include "qmps/code_view.h"
#include "qmps/ui_utils.h"
#include "qmps/register_set_widget.h"
#include "qmps/tlb_model.h"
#include "qmps/register_item_delegate.h"

static const int kDefaultWidth = 420;
static const int kDefaultHeight = 580;

ProcessorWindow::ProcessorWindow(Word cpuId, QWidget* parent)
    : QMainWindow(parent),
      dbgSession(Appl()->getDebugSession()),
      cpuId(cpuId)
{
    cpu = dbgSession->getMachine()->getProcessor(cpuId);

    setWindowTitle(QString("uMPS Processor %1").arg(cpuId));
    setDockOptions(AnimatedDocks | AllowTabbedDocks);

    createToolBar();
    createDockableWidgets();
    createMenu();

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 7, 0, 0);

    statusLabel = new QLabel;
    statusLabel->setIndent(3);
    statusLabel->setFont(Appl()->getMonospaceFont());
    centralLayout->addWidget(statusLabel);

    centralLayout->addWidget(new CodeView(cpuId));
    centralLayout->addLayout(createInstrPanel());

    QWidget* secStatusWidget = new QWidget;
    QHBoxLayout* secStatusLayout = new QHBoxLayout(secStatusWidget);
    secStatusLayout->setSpacing(12);
    secStatusLayout->setContentsMargins(0, 0, 0, 0);
    secStatusLayout->addWidget(vmIndicator = new QLabel("VM"));
    secStatusLayout->addWidget(bdIndicator = new QLabel("BD"));
    secStatusLayout->addWidget(ldIndicator = new QLabel("LD"));

    statusBar()->addPermanentWidget(secStatusWidget);

    updateStatusInfo();

    QString key;

    key = QString("CpuWindow%1/geometry").arg((quint32) cpu->getId());
    QVariant savedGeometry = Appl()->settings.value(key);
    if (savedGeometry.isValid())
        restoreGeometry(savedGeometry.toByteArray());
    else
        resize(kDefaultWidth, kDefaultHeight);

    key = QString("CpuWindow%1/state").arg((quint32) cpu->getId());
    QVariant savedState = Appl()->settings.value(key);
    if (savedState.isValid())
        restoreState(savedState.toByteArray());
    else {
        tlbWidget->hide();
    }

    connect(dbgSession->getCpuStatusMap(), SIGNAL(Changed()),
            this, SLOT(updateStatusInfo()));
    connect(dbgSession, SIGNAL(MachineReset()),
            this, SLOT(onMachineReset()));
}

void ProcessorWindow::closeEvent(QCloseEvent* event)
{
    Appl()->settings.setValue(QString("CpuWindow%1/geometry").arg((quint32) cpu->getId()),
                              saveGeometry());
    Appl()->settings.setValue(QString("CpuWindow%1/state").arg((quint32) cpu->getId()),
                              saveState());
    event->accept();
}

void ProcessorWindow::createMenu()
{
    QMenu* debugMenu = menuBar()->addMenu("&Debug");
    debugMenu->addAction(dbgSession->debugContinueAction);
    debugMenu->addAction(dbgSession->debugStepAction);
    debugMenu->addAction(dbgSession->debugStopAction);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(toolBar->toggleViewAction());
    viewMenu->addSeparator();
    viewMenu->addAction(regView->toggleViewAction());
    viewMenu->addAction(tlbWidget->toggleViewAction());

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    (void) helpMenu;
}

void ProcessorWindow::createToolBar()
{
    toolBar = addToolBar("ToolBar");
    toolBar->setObjectName("ToolBar");
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
    regView = new RegisterSetWidget(cpuId);
    regView->setObjectName("RegisterSetWidget");
    addDockWidget(Qt::BottomDockWidgetArea, regView);

    QTableView* tlbView = new QTableView;
    tlbView->setModel(new TLBModel(cpuId, this));
    tlbView->setSelectionMode(QAbstractItemView::SingleSelection);
    tlbView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tlbView->horizontalHeader()->setStretchLastSection(true);
    tlbView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    tlbView->horizontalHeader()->setHighlightSections(false);
    tlbView->verticalHeader()->setHighlightSections(false);
    tlbView->verticalHeader()->setResizeMode(QHeaderView::Fixed);
    tlbView->setAlternatingRowColors(true);
    tlbView->setItemDelegate(new RIDelegateHex(this));
    tlbView->resizeRowsToContents();

    tlbWidget = new QDockWidget("TLB");
    tlbWidget->setWidget(tlbView);
    tlbWidget->setObjectName("TLBWidget");
    tlbWidget->setFeatures(QDockWidget::DockWidgetClosable |
                           QDockWidget::DockWidgetMovable |
                           QDockWidget::DockWidgetFloatable);
    connect(tlbWidget, SIGNAL(topLevelChanged(bool)), this, SLOT(updateTLBViewTitle(bool)));

    addDockWidget(Qt::BottomDockWidgetArea, tlbWidget);
    tabifyDockWidget(regView, tlbWidget);
}

void ProcessorWindow::updateStatusInfo()
{
    QString str;

    if (!cpu->isHalted()) {
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
    } else {
        prevPCLabel->clear();
        pcLabel->clear();

        vmIndicator->setEnabled(false);
        bdIndicator->setEnabled(false);
        ldIndicator->setEnabled(false);
    }

    if (!cpu->isHalted()) {
        QString newStatus = dbgSession->getCpuStatusMap()->getLocation(cpuId);
        if (dbgSession->isStopped() || cpu->isIdle())
            newStatus.append(QString(" [%1]").arg(dbgSession->getCpuStatusMap()->getStatus(cpu->getId())));
        statusLabel->setText(newStatus);
    } else {
        statusLabel->setText("[Halted]");
    }
}

void ProcessorWindow::onMachineReset()
{
    cpu = dbgSession->getMachine()->getProcessor(cpuId);

    connect(dbgSession->getCpuStatusMap(), SIGNAL(Changed()),
            this, SLOT(updateStatusInfo()));

    updateStatusInfo();
}

void ProcessorWindow::updateTLBViewTitle(bool topLevel)
{
    if (topLevel)
        tlbWidget->setWindowTitle(QString("Processor %1 TLB").arg(cpuId));
    else
        tlbWidget->setWindowTitle("TLB");
}
