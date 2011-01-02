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

#include "qmps/monitor_window.h"

#include <map>
#include <sigc++/sigc++.h>

#include <QtDebug>

#include <QWidget>
#include <QMenu>
#include <QTabWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QSlider>
#include <QTreeView>
#include <QTableView>
#include <QPixmap>
#include <QToolButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>

#include "umps/machine.h"
#include "umps/device.h"
#include "umps/systembus.h"
#include "umps/error.h"

#include "qmps/application.h"
#include "qmps/debug_session.h"
#include "qmps/stop_mask_view.h"
#include "qmps/machine_config_dialog.h"
#include "qmps/machine_config_view.h"
#include "qmps/processor_list_model.h"
#include "qmps/stoppoint_list_model.h"
#include "qmps/suspect_type_delegate.h"
#include "qmps/device_tree_view.h"
#include "qmps/add_breakpoint_dialog.h"
#include "qmps/add_suspect_dialog.h"
#include "qmps/add_tracepoint_dialog.h"
#include "qmps/flat_push_button.h"
#include "qmps/create_machine_dialog.h"
#include "qmps/trace_browser.h"
#include "qmps/processor_window.h"
#include "qmps/terminal_window.h"
#include "qmps/monitor_window_priv.h"

static const int kDefaultWidth = 640;
static const int kDefaultHeight = 480;

const char* const MonitorWindow::simSpeedMnemonics[DebugSession::kNumSpeedLevels] = {
    "Slowest",
    "Slow",
    "Medium",
    "Fast",
    "Fastest"
};

MonitorWindow::MonitorWindow()
    : QMainWindow(),
      dbgSession(Appl()->getDebugSession())
{
    setWindowTitle("uMPS");
    resize(kDefaultWidth, kDefaultHeight);

    connect(Appl(), SIGNAL(MachineConfigChanged()), this, SLOT(onMachineConfigChanged()));

    connect(dbgSession, SIGNAL(MachineStarted()),
            this, SLOT(onMachineStarted()));
    connect(dbgSession, SIGNAL(MachineAboutToBeHalted()),
            this, SLOT(onMachineAboutToBeHalted()));
    connect(dbgSession, SIGNAL(StatusChanged()),
            this, SLOT(updateStoppointActionsSensitivity()));

    createActions();
    updateRecentConfigList();

    toolBar = addToolBar("Toolbar");
    createMenu();
    initializeToolBar();
    createTabs();
    statusBar()->addPermanentWidget(new StatusDisplay);

    QWidget* centralWidget = new QWidget;

    QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);

    centralLayout->addWidget(tabWidget);

    StopMaskView* stopMaskView = new StopMaskView(stopMaskActions);
    centralLayout->addWidget(stopMaskView);
    stopMaskView->setVisible(viewStopMaskAction->isChecked());
    connect(viewStopMaskAction, SIGNAL(toggled(bool)), stopMaskView, SLOT(setVisible(bool)));

    setCentralWidget(centralWidget);

    updateUISensitivity();

    configView = NULL;
}

void MonitorWindow::closeEvent(QCloseEvent* event)
{
    Appl()->settings.setValue("MonitorWindow/ShowStopMask", viewStopMaskAction->isChecked());
    event->accept();
}

void MonitorWindow::createActions()
{
    newConfigAction = new QAction("New Configuration", this);
    newConfigAction->setIcon(QIcon(":/icons/config_create-22.png"));
    newConfigAction->setShortcut(QKeySequence("Ctrl+N"));
    newConfigAction->setStatusTip("Create a new machine configuration");
    connect(newConfigAction, SIGNAL(triggered()), this, SLOT(onCreateConfig()));

    loadConfigAction = new QAction("Open Configuration...", this);
    loadConfigAction->setShortcut(QKeySequence("Ctrl+O"));
    loadConfigAction->setIcon(QIcon(":/icons/config_open-22.png"));
    loadConfigAction->setStatusTip("Open a machine configuration");
    connect(loadConfigAction, SIGNAL(triggered()), this, SLOT(onLoadConfig()));

    for (unsigned int i = 0; i < Application::kMaxRecentConfigs; i++) {
        loadRecentConfigActions[i] = new QAction(this);
        loadRecentConfigActions[i]->setVisible(false);
        connect(loadRecentConfigActions[i], SIGNAL(triggered()), this, SLOT(onLoadRecentConfig()));
    }

    quitAction = new QAction("&Quit", this);
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    quitAction->setStatusTip("Quit uMPS");
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    viewStopMaskAction = new QAction("Stop Mask", this);
    viewStopMaskAction->setCheckable(true);
    viewStopMaskAction->setChecked(Appl()->settings.value("MonitorWindow/ShowStopMask", true).toBool());

    editConfigAction = new QAction("Edit Configuration...", this);
    editConfigAction->setIcon(QIcon(":/icons/machine_settings-22.png"));
    connect(editConfigAction, SIGNAL(triggered()), this, SLOT(editConfig()));
    editConfigAction->setEnabled(Appl()->getConfig() != NULL);

    addBreakpointAction = new QAction("Add Breakpoint...", this);
    connect(addBreakpointAction, SIGNAL(triggered()), this, SLOT(onAddBreakpoint()));
    addBreakpointAction->setShortcut(QKeySequence("Ctrl+B"));
    addBreakpointAction->setEnabled(false);
    removeBreakpointAction = new QAction("Remove Breakpoint", this);
    connect(removeBreakpointAction, SIGNAL(triggered()), this, SLOT(onRemoveBreakpoint()));
    removeBreakpointAction->setEnabled(false);

    addSuspectAction = new QAction("Add Suspect...", this);
    connect(addSuspectAction, SIGNAL(triggered()), this, SLOT(onAddSuspect()));
    addSuspectAction->setEnabled(false);
    removeSuspectAction = new QAction("Remove Suspect", this);
    connect(removeSuspectAction, SIGNAL(triggered()), this, SLOT(onRemoveSuspect()));
    removeSuspectAction->setEnabled(false);

    addTraceAction = new QAction("Add Traced Region...", this);
    connect(addTraceAction, SIGNAL(triggered()), this, SLOT(onAddTracepoint()));
    addTraceAction->setEnabled(false);
    removeTraceAction = new QAction("Remove Traced Region", this);
    removeTraceAction->setEnabled(false);

    speedActionGroup = new QActionGroup(this);
    for (int i = 0; i < DebugSession::kNumSpeedLevels; i++) {
        simSpeedActions[i] = new QAction(simSpeedMnemonics[i], speedActionGroup);
        simSpeedActions[i]->setCheckable(true);
        simSpeedActions[i]->setData(QVariant(i));
        connect(simSpeedActions[i], SIGNAL(triggered()), this, SLOT(onSpeedActionChecked()));
    }
    simSpeedActions[dbgSession->getSpeed()]->setChecked(true);
    connect(dbgSession, SIGNAL(SpeedChanged(int)), this, SLOT(onSpeedChanged(int)));

    increaseSpeedAction = new QAction("Increase Speed", this);
    connect(increaseSpeedAction, SIGNAL(triggered()), this, SLOT(increaseSpeed()));
    increaseSpeedAction->setShortcut(QKeySequence("Ctrl++"));
    increaseSpeedAction->setEnabled(dbgSession->getSpeed() != DebugSession::kMaxSpeed);

    decreaseSpeedAction = new QAction("Decrease Speed", this);
    connect(decreaseSpeedAction, SIGNAL(triggered()), this, SLOT(decreaseSpeed()));
    decreaseSpeedAction->setShortcut(QKeySequence("Ctrl+-"));
    decreaseSpeedAction->setEnabled(dbgSession->getSpeed() != 0);

    for (unsigned int i = 0; i < MachineConfig::MAX_CPUS; ++i) {
        showCpuWindowActions[i] = new QAction(QString("Processor %1").arg(i), this);
        showCpuWindowActions[i]->setShortcut(QKeySequence(QString("Alt+Shift+%1").arg(i)));
        showCpuWindowActions[i]->setData(QVariant(i));
        connect(showCpuWindowActions[i], SIGNAL(triggered()), this, SLOT(showCpuWindow()));
        showCpuWindowActions[i]->setEnabled(false);
    }

    for (unsigned int i = 0; i < N_DEV_PER_IL; ++i) {
        showTerminalActions[i] = new QAction(QString("Terminal %1").arg(i), this);
        showTerminalActions[i]->setShortcut(QKeySequence(QString("Alt+%1").arg(i)));
        showTerminalActions[i]->setData(QVariant(i));
        connect(showTerminalActions[i], SIGNAL(triggered()), this, SLOT(showTerminal()));
        showTerminalActions[i]->setEnabled(false);
    }

    aboutAction = new QAction("&About", this);

    addStopMaskAction("Breakpoints", SC_BREAKPOINT);
    addStopMaskAction("Suspects", SC_SUSPECT);
    addStopMaskAction("Exceptions", SC_EXCEPTION);
    addStopMaskAction("Kernel UTLB", SC_UTLB_KERNEL);
    addStopMaskAction("User UTLB", SC_UTLB_USER);
}

void MonitorWindow::addStopMaskAction(const char* text, StopCause sc)
{
    QAction* a = new QAction(text, this);
    a->setCheckable(true);
    a->setData(QVariant(sc));
    a->setChecked(dbgSession->getStopMask() & sc);
    connect(a, SIGNAL(toggled(bool)), this, SLOT(onStopMaskChanged()));
    stopMaskActions[sc] = a;
}

void MonitorWindow::createMenu()
{
    QMenu* simulatorMenu = menuBar()->addMenu("&Simulator");
    simulatorMenu->addAction(newConfigAction);
    simulatorMenu->addAction(loadConfigAction);
    recentConfigSeparatorAction = simulatorMenu->addSeparator();
    for (unsigned int i = 0; i < Application::kMaxRecentConfigs; i++)
        simulatorMenu->addAction(loadRecentConfigActions[i]);
    simulatorMenu->addSeparator();
    simulatorMenu->addAction(quitAction);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(toolBar->toggleViewAction());
    viewMenu->addAction(viewStopMaskAction);

    QMenu* machineMenu = menuBar()->addMenu("&Machine");
    machineMenu->addAction(dbgSession->startMachineAction);
    machineMenu->addAction(dbgSession->haltMachineAction);
    machineMenu->addSeparator();
    machineMenu->addAction(editConfigAction);

    QMenu* debugMenu = menuBar()->addMenu("&Debug");
    debugMenu->addAction(dbgSession->debugContinueAction);
    debugMenu->addAction(dbgSession->debugStepAction);
    debugMenu->addAction(dbgSession->debugStopAction);
    debugMenu->addSeparator();
    debugMenu->addAction(addBreakpointAction);
    debugMenu->addAction(removeBreakpointAction);
    debugMenu->addSeparator();
    debugMenu->addAction(addSuspectAction);
    debugMenu->addAction(removeSuspectAction);
    debugMenu->addSeparator();
    debugMenu->addAction(addTraceAction);
    debugMenu->addAction(removeTraceAction);

    debugMenu->addSeparator();
    QMenu* stopMaskSubMenu = debugMenu->addMenu("Stop On");
    for (StopMaskActionMap::const_iterator it = stopMaskActions.begin();
         it != stopMaskActions.end();
         ++it)
    {
        stopMaskSubMenu->addAction(it->second);
    }

    QMenu* settingsMenu = menuBar()->addMenu("Se&ttings");
    QMenu* speedLevelsSubMenu = settingsMenu->addMenu("Simulation Speed");
    for (int i = 0; i < DebugSession::kNumSpeedLevels; i++)
        speedLevelsSubMenu->addAction(simSpeedActions[i]);
    settingsMenu->addAction(increaseSpeedAction);
    settingsMenu->addAction(decreaseSpeedAction);

    QMenu* windowMenu = menuBar()->addMenu("&Windows");

    for (unsigned int i = 0; i < N_DEV_PER_IL; ++i)
        windowMenu->addAction(showTerminalActions[i]);
    windowMenu->addSeparator();
    for (unsigned int i = 0; i < MachineConfig::MAX_CPUS; ++i)
        windowMenu->addAction(showCpuWindowActions[i]);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(aboutAction);
}

void MonitorWindow::initializeToolBar()
{
    toolBar->addAction(newConfigAction);
    toolBar->addAction(loadConfigAction);
    toolBar->addSeparator();
    toolBar->addAction(dbgSession->startMachineAction);
    toolBar->addAction(dbgSession->haltMachineAction);
    toolBar->addAction(editConfigAction);
    toolBar->addSeparator();
    toolBar->addAction(dbgSession->debugContinueAction);
    toolBar->addAction(dbgSession->debugStepAction);
    toolBar->addAction(dbgSession->debugStopAction);

    toolBar->addSeparator();

    QWidget* speedWidget = new QWidget;
    speedWidget->setToolTip("Adjust simulation speed");
    QHBoxLayout* speedLayout = new QHBoxLayout(speedWidget);
    speedLayout->setSpacing(3);
    toolBar->addWidget(speedWidget);

    QLabel* slowestLabel = new QLabel("Slowest");
    QFont speedTipFont = slowestLabel->font();
    speedTipFont.setItalic(true);
    speedTipFont.setPointSizeF(speedTipFont.pointSizeF() * .8);
    slowestLabel->setFont(speedTipFont);
    speedLayout->addWidget(slowestLabel);

    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, DebugSession::kMaxSpeed);
    slider->setSingleStep(1);
    slider->setPageStep(1);
    slider->setValue(dbgSession->getSpeed());
    connect(slider, SIGNAL(valueChanged(int)), dbgSession, SLOT(setSpeed(int)));
    connect(dbgSession, SIGNAL(SpeedChanged(int)), slider, SLOT(setValue(int)));
    speedLayout->addWidget(slider);

    QLabel* fastestLabel = new QLabel("Fastest");
    fastestLabel->setFont(speedTipFont);
    speedLayout->addWidget(fastestLabel);
    speedLayout->addStretch(1);
}

void MonitorWindow::createTabs()
{
    tabWidget = new QTabWidget;
    connect(tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(updateStoppointActionsSensitivity()));

    tabWidget->addTab(createWelcomeTab(),
                      QIcon(":/icons/machine-16.png"), "&Overview");
    tabWidget->addTab(createCpuTab(),
                      QIcon(":/icons/processor-16.png"), "&Processors");
    tabWidget->addTab(createMemoryTab(),
                      QIcon(":/icons/memory-16.png"), "&Memory");
    tabWidget->addTab(createDeviceTab(),
                      QIcon(":/icons/device-16.png"), "&Device Status");

    tabWidget->setTabEnabled(TAB_INDEX_CPU, false);
    tabWidget->setTabEnabled(TAB_INDEX_MEMORY, false);
    tabWidget->setTabEnabled(TAB_INDEX_DEVICES, false);
}

QPushButton* MonitorWindow::linkButtonFromAction(const QAction* action, const QString& text)
{
    QPushButton* b = new FlatPushButton(action->icon(), text);
    b->setStyleSheet("text-decoration: underline;");
    connect(b, SIGNAL(clicked()), action, SLOT(trigger()));
    return b;
}

QWidget* MonitorWindow::createWelcomeTab()
{
    QScrollArea* scroller = new QScrollArea;
    QWidget* tab = new QWidget;
    tab->setObjectName("WelcomeTab");
    scroller->setWidget(tab);
    scroller->setWidgetResizable(true);
    QGridLayout* layout = new QGridLayout(tab);

    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setStyleSheet("QAbstractScrollArea, #WelcomeTab {background: transparent}");

    layout->setColumnMinimumWidth(0, 16);
    layout->setColumnStretch(2, 1);
    layout->setContentsMargins(11, 22, 11, 11);

    QLabel* heading = new QLabel(QString("Welcome to %1MPS").arg(QChar(0x00b5)));
    QFont font = heading->font();
    font.setPointSizeF(font.pointSizeF() * 1.3);
    font.setBold(true);
    heading->setFont(font);
    layout->addWidget(heading, 0, 0, 1, 3);

    QLabel* hint = new QLabel("To start using the simulator, you can:");
    hint->setWordWrap(true);
    layout->addWidget(hint, 1, 0, 1, 3);

    layout->addWidget(linkButtonFromAction(newConfigAction,
                                           "Create a new machine configuration"),
                      2, 1, Qt::AlignLeft);
    layout->addWidget(linkButtonFromAction(loadConfigAction,
                                           "Open an existing machine configuration"),
                      3, 1, Qt::AlignLeft);

    layout->addWidget(new QLabel("<b>Recent machines</b>"), 4, 0, 1, 3);
    int recentRow = 5;
    for (unsigned int i = 0; i < Application::kMaxRecentConfigs; i++) {
        if (loadRecentConfigActions[i]->isEnabled())
            layout->addWidget(linkButtonFromAction(loadRecentConfigActions[i],
                                                   loadRecentConfigActions[i]->text()),
                              recentRow++, 1, Qt::AlignLeft);
    }

    layout->setRowStretch(recentRow, 1);

    return scroller;
}

QWidget* MonitorWindow::createConfigTab()
{
    configView = new MachineConfigView;

    QScrollArea* scroller = new QScrollArea;
    scroller->setWidget(configView);
    scroller->setFrameShape(QFrame::NoFrame);
    scroller->setStyleSheet("QAbstractScrollArea, MachineConfigView {background: transparent}");

    return scroller;
}

QWidget* MonitorWindow::createCpuTab()
{
    cpuListView = new QTreeView;
    cpuListView->setRootIsDecorated(false);

    breakpointListView = new QTreeView;
    breakpointListView->setRootIsDecorated(false);
    breakpointListView->setAlternatingRowColors(true);
    breakpointListView->setContextMenuPolicy(Qt::ActionsContextMenu);
    breakpointListView->addAction(addBreakpointAction);
    breakpointListView->addAction(removeBreakpointAction);

    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(cpuListView);
    splitter->addWidget(breakpointListView);

    return splitter;
}

QWidget* MonitorWindow::createMemoryTab()
{
    suspectListView = new QTreeView;
    suspectListView->setContextMenuPolicy(Qt::ActionsContextMenu);
    suspectListView->addAction(addSuspectAction);
    suspectListView->addAction(removeSuspectAction);

    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(suspectListView);
    traceBrowser = new TraceBrowser(addTraceAction, removeTraceAction);
    splitter->addWidget(traceBrowser);

    return splitter;
}

QWidget* MonitorWindow::createDeviceTab()
{
    deviceTreeView = new DeviceTreeView(this);
    return deviceTreeView;
}

void MonitorWindow::updateRecentConfigList()
{
    QStringList files = Appl()->settings.value("RecentFiles").toStringList();

    for (unsigned int i = 0; i < (unsigned int) files.size() && i < Application::kMaxRecentConfigs; i++) {
        QString baseName = QFileInfo(files[i]).fileName();
        loadRecentConfigActions[i]->setText(QString("%1. %2").arg(i + 1).arg(baseName));
        loadRecentConfigActions[i]->setVisible(true);
        loadRecentConfigActions[i]->setData(files[i]);
    }
    for (unsigned int i = files.size(); i < Application::kMaxRecentConfigs; i++) {
        loadRecentConfigActions[i]->setVisible(false);
        loadRecentConfigActions[i]->setData(QVariant());
    }
}

void MonitorWindow::updateUISensitivity()
{
#if 0
    bool started = dbgSession->getStatus() != MS_HALTED;

    tabWidget->setTabEnabled(TAB_INDEX_CPU, started);
    tabWidget->setTabEnabled(TAB_INDEX_MEMORY, started);
    tabWidget->setTabEnabled(TAB_INDEX_DEVICES, started);
#endif
}

void MonitorWindow::updateActionSensitivity()
{
}

bool MonitorWindow::discardMachineConfirmed()
{
    if (!dbgSession->IsStarted())
        return true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,
                                  "Warning",
                                  "A machine is currently active. "
                                  "Do you really want to do switch to a different machine?",
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No);
    return reply == QMessageBox::Yes;
}

void MonitorWindow::onCreateConfig()
{
    if (!discardMachineConfirmed())
        return;

    CreateMachineDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        Appl()->CreateConfig_(dialog.getFileName());
}

void MonitorWindow::onLoadConfig()
{
    if (!discardMachineConfirmed())
        return;

    QString fileName = QFileDialog::getOpenFileName(this, "Open Machine Configuration");
    qDebug() << fileName;
    if (!fileName.isEmpty())
        Appl()->LoadConfig_(fileName);
}

void MonitorWindow::onLoadRecentConfig()
{
    QAction* action = static_cast<QAction*>(sender());
    Appl()->LoadConfig_(action->data().toString());
}

void MonitorWindow::editConfig()
{
    assert(Appl()->getConfig());

    MachineConfigDialog dialog(Appl()->getConfig(), this);
    if (dialog.exec() == QDialog::Accepted) {
        try {
            Appl()->getConfig()->Save();
        } catch (FileError& e) {
            QMessageBox::critical(this, QString("%1: Error").arg(Appl()->applicationName()), e.what());
            return;
        }
        configView->Update();
    }
}

void MonitorWindow::onStopMaskChanged()
{
    unsigned int acc = 0;

    for (StopMaskActionMap::const_iterator it = stopMaskActions.begin();
         it != stopMaskActions.end();
         ++it)
    {
        if (it->second->isChecked())
            acc |= it->first;
    }

    dbgSession->setStopMask(acc);
}

void MonitorWindow::onMachineConfigChanged()
{
    if (configView == NULL) {
        delete tabWidget->widget(TAB_INDEX_CONFIG_VIEW);
        tabWidget->insertTab(0, createConfigTab(),
                             QIcon(":/icons/machine-16.png"), "&Overview");
        tabWidget->setCurrentIndex(TAB_INDEX_CONFIG_VIEW);

        editConfigAction->setEnabled(true);
    } else {
        configView->Update();
    }

    setWindowTitle(QString("%1 (%2) - uMPS").arg(Appl()->document, Appl()->getCurrentDir()));

    updateRecentConfigList();
}

void MonitorWindow::onSpeedActionChecked()
{
    dbgSession->setSpeed(speedActionGroup->checkedAction()->data().toInt());
}

void MonitorWindow::onSpeedChanged(int speed)
{
    simSpeedActions[speed]->setChecked(true);
    increaseSpeedAction->setEnabled(speed != DebugSession::kMaxSpeed);
    decreaseSpeedAction->setEnabled(speed != 0);
}

void MonitorWindow::increaseSpeed()
{
    dbgSession->setSpeed(dbgSession->getSpeed() + 1);
}

void MonitorWindow::decreaseSpeed()
{
    dbgSession->setSpeed(dbgSession->getSpeed() - 1);
}

void MonitorWindow::showCpuWindow()
{
    QAction* action = static_cast<QAction*>(sender());
    unsigned int cpuId = action->data().toUInt();
    if (cpuWindows[cpuId].isNull()) {
        cpuWindows[cpuId] = new ProcessorWindow(dbgSession->getMachine()->getProcessor(0),
                                                breakpointListModel.get());
        cpuWindows[cpuId]->setAttribute(Qt::WA_QuitOnClose, false);
        cpuWindows[cpuId]->setAttribute(Qt::WA_DeleteOnClose);
        cpuWindows[cpuId]->show();
    } else {
        cpuWindows[cpuId]->activateWindow();
        cpuWindows[cpuId]->raise();
    }
}

void MonitorWindow::showTerminal()
{
    QAction* action = static_cast<QAction*>(sender());
    unsigned int devNo = action->data().toUInt();
    if (terminalWindows[devNo].isNull()) {
        terminalWindows[devNo] = new TerminalWindow(devNo);
        terminalWindows[devNo]->setAttribute(Qt::WA_QuitOnClose, false);
        terminalWindows[devNo]->setAttribute(Qt::WA_DeleteOnClose);
        terminalWindows[devNo]->show();
    } else {
        terminalWindows[devNo]->activateWindow();
        terminalWindows[devNo]->raise();
    }
}

void MonitorWindow::onMachineStarted()
{
    cpuListModel.reset(new ProcessorListModel);
    cpuListView->setModel(cpuListModel.get());
    cpuListView->resizeColumnToContents(ProcessorListModel::COLUMN_CPU_ID);
    cpuListView->resizeColumnToContents(ProcessorListModel::COLUMN_CPU_STATUS);
    cpuListView->resizeColumnToContents(ProcessorListModel::COLUMN_CPU_ADDRESS);

    breakpointListModel.reset(new StoppointListModel(dbgSession->getBreakpoints(),
                                                     "Breakpoint", 'B'));
    breakpointListView->setModel(breakpointListModel.get());
    breakpointListView->hideColumn(StoppointListModel::COLUMN_ACCESS_TYPE);
    breakpointListView->resizeColumnToContents(StoppointListModel::COLUMN_STOPPOINT_ID);
    breakpointListView->resizeColumnToContents(StoppointListModel::COLUMN_ACCESS_TYPE);
    breakpointListView->resizeColumnToContents(StoppointListModel::COLUMN_ASID);

    suspectListModel.reset(new StoppointListModel(dbgSession->getSuspects(),
                                                  "Suspect", 'S'));
    suspectListView->setModel(suspectListModel.get());
    suspectListView->resizeColumnToContents(StoppointListModel::COLUMN_STOPPOINT_ID);
    suspectListView->resizeColumnToContents(StoppointListModel::COLUMN_ASID);
    suspectListView->setItemDelegateForColumn(StoppointListModel::COLUMN_ACCESS_TYPE,
                                              new SuspectTypeDelegate);

    deviceTreeModel.reset(new DeviceTreeModel(dbgSession->getMachine()));
    deviceTreeView->setModel(deviceTreeModel.get());

    Machine* machine = dbgSession->getMachine();
    for (unsigned int i = 0; i < N_DEV_PER_IL; ++i) {
        const Device* d = machine->getDevice(EXT_IL_INDEX(IL_TERMINAL), i);
        showTerminalActions[i]->setEnabled(d->Type() == TERMDEV);
    }

    const MachineConfig* config = Appl()->getConfig();

    for (unsigned int i = 0; i < config->getNumProcessors(); i++)
        showCpuWindowActions[i]->setEnabled(true);

    tabWidget->setTabEnabled(TAB_INDEX_CPU, true);
    tabWidget->setTabEnabled(TAB_INDEX_MEMORY, true);
    tabWidget->setTabEnabled(TAB_INDEX_DEVICES, true);

    editConfigAction->setEnabled(false);
}

void MonitorWindow::onMachineAboutToBeHalted()
{
    for (unsigned int i = 0; i < MachineConfig::MAX_CPUS; i++)
        if (cpuWindows[i])
            delete cpuWindows[i].data();

    for (unsigned int i = 0; i < N_DEV_PER_IL; i++)
        if (terminalWindows[i])
            delete terminalWindows[i].data();

    tabWidget->setTabEnabled(TAB_INDEX_CPU, false);
    tabWidget->setTabEnabled(TAB_INDEX_MEMORY, false);
    tabWidget->setTabEnabled(TAB_INDEX_DEVICES, false);

    for (unsigned int i = 0; i < N_DEV_PER_IL; ++i)
        showTerminalActions[i]->setEnabled(false);

    for (unsigned int i = 0; i < MachineConfig::MAX_CPUS; ++i)
        showCpuWindowActions[i]->setEnabled(false);

    editConfigAction->setEnabled(true);
}

void MonitorWindow::updateStoppointActionsSensitivity()
{
    bool stopped = dbgSession->IsStopped();

    addBreakpointAction->setEnabled(stopped);
    addSuspectAction->setEnabled(stopped);
    addTraceAction->setEnabled(stopped);

    removeBreakpointAction->setEnabled(stopped && tabWidget->currentIndex() == TAB_INDEX_CPU);
    removeSuspectAction->setEnabled(stopped && tabWidget->currentIndex() == TAB_INDEX_MEMORY);
    removeTraceAction->setEnabled(stopped && tabWidget->currentIndex() == TAB_INDEX_MEMORY);
}

void MonitorWindow::onAddBreakpoint()
{
    AddBreakpointDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        AddressRange r(dialog.getASID(), dialog.getStartAddress(), dialog.getStartAddress());
        breakpointListModel->Add(r, AM_EXEC);
    }
}

void MonitorWindow::onRemoveBreakpoint()
{
    QModelIndexList idx = breakpointListView->selectionModel()->selectedRows();
    if (!idx.isEmpty())
        breakpointListModel->Remove(idx.first().row());
}

void MonitorWindow::onAddSuspect()
{
    AddSuspectDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        AddressRange r(dialog.getASID(), dialog.getStartAddress(), dialog.getEndAddress());
        if (!suspectListModel->Add(r, AM_WRITE)) {
            QMessageBox::warning(this, "Warning",
                                 "<b>Could not insert suspect range:</b> "
                                 "it overlaps with an already inserted range");
        }
    }
}

void MonitorWindow::onRemoveSuspect()
{
    QModelIndexList idx = suspectListView->selectionModel()->selectedRows();
    if (!idx.isEmpty())
        suspectListModel->Remove(idx.first().row());
}

void MonitorWindow::onAddTracepoint()
{
    AddTracepointDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (!traceBrowser->AddTracepoint(dialog.getStartAddress(), dialog.getEndAddress()))
        QMessageBox::warning(this, "Warning",
                             "<b>Could not insert traced range:</b> "
                             "it overlaps with an already inserted range");
}


StatusDisplay::StatusDisplay(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QLabel("Status:"));
    statusLabel = new QLabel;
    statusLabel->setFixedWidth(statusLabel->fontMetrics().width("Powered off_"));
    layout->addWidget(statusLabel);

    layout->addSpacing(kFieldSpacing);

    layout->addWidget(new QLabel("ToD:"));
    todLabel = new QLabel;
    todLabel->setFont(Appl()->getMonospaceFont());
    todLabel->setFixedWidth(todLabel->fontMetrics().width("00000000:00000000"));
    layout->addWidget(todLabel);

    statusLabel->setText("-");

    connect(Appl(), SIGNAL(MachineConfigChanged()), this, SLOT(refreshAll()));
    connect(debugSession, SIGNAL(StatusChanged()), this, SLOT(refreshAll()));
    connect(debugSession, SIGNAL(DebugIterationCompleted()), this, SLOT(refreshTod()));

    refreshAll();
}

void StatusDisplay::refreshAll()
{
    setEnabled(Appl()->getConfig() != NULL);

    if (Appl()->getConfig()) {
        todLabel->setEnabled(debugSession->getStatus() != MS_HALTED);
        switch (debugSession->getStatus()) {
        case MS_HALTED:
            statusLabel->setText("Powered off");
            todLabel->setText("-");
            break;

        case MS_RUNNING:
            statusLabel->setText("Running");
            refreshTod();
            break;

        case MS_STOPPED:
            statusLabel->setText("Stopped");
            refreshTod();
            break;
        }
    } else {
        statusLabel->setText("-");
        todLabel->setText("-");
    }
}

void StatusDisplay::refreshTod()
{
    SystemBus* bus = debugSession->getMachine()->getBus();
    todLabel->setText(QString("%1:%2")
                      .arg(bus->getToDHI(), 8, 16, QLatin1Char('0'))
                      .arg(bus->getToDLO(), 8, 16, QLatin1Char('0')));
}
