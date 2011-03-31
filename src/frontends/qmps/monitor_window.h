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

#ifndef QMPS_MONITOR_WINDOW_H
#define QMPS_MONITOR_WINDOW_H

#include <map>
#include <QMainWindow>
#include <QPointer>

#include "umps/machine_config.h"
#include "umps/machine.h"
#include "umps/arch.h"

#include "qmps/application.h"
#include "qmps/debug_session.h"
#include "qmps/processor_list_model.h"
#include "qmps/stoppoint_list_model.h"
#include "qmps/device_tree_model.h"

class QAction;
class QActionGroup;
class QTreeView;
class QLabel;
class QPushButton;
class QToolBar;
class QSlider;
class MachineConfigView;
class ProcessorWindow;
class TraceBrowser;
class TerminalWindow;
class QModelIndex;

class MonitorWindow : public QMainWindow {
    Q_OBJECT

public:
    MonitorWindow();

protected:
    virtual void closeEvent(QCloseEvent* event);

private:
    static const int TAB_INDEX_CONFIG_VIEW = 0;
    static const int TAB_INDEX_CPU = 1;
    static const int TAB_INDEX_MEMORY = 2;
    static const int TAB_INDEX_DEVICES = 3;

    void createActions();
    void addStopMaskAction(const char* text, StopCause sc);
    void createMenu();
    void initializeToolBar();
    void createTabs();

    QPushButton* linkButtonFromAction(const QAction* action, const QString& text = QString());

    QWidget* createWelcomeTab();
    QWidget* createConfigTab();
    QWidget* createCpuTab();
    QWidget* createMemoryTab();
    QWidget* createDeviceTab();

    void updateRecentConfigList();

    bool discardMachineConfirmed();

    DebugSession* const dbgSession;
    Machine* machine;

    scoped_ptr<ProcessorListModel> cpuListModel;

    scoped_ptr<StoppointListModel> suspectListModel;

    scoped_ptr<DeviceTreeModel> deviceTreeModel;

    QAction* newConfigAction;
    QAction* loadConfigAction;
    QAction* loadRecentConfigActions[Application::kMaxRecentConfigs];
    QAction* recentConfigSeparatorAction;

    QAction* quitAction;

    QAction* viewToolbarAction;
    QAction* viewStopMaskAction;

    QAction* editConfigAction;

    QAction* addBreakpointAction;
    QAction* removeBreakpointAction;
    QAction* addSuspectAction;
    QAction* removeSuspectAction;
    QAction* addTraceAction;
    QAction* removeTraceAction;

    QActionGroup* speedActionGroup;
    QAction* simSpeedActions[DebugSession::kNumSpeedLevels];
    static const char* const simSpeedMnemonics[DebugSession::kNumSpeedLevels];
    QAction* increaseSpeedAction;
    QAction* decreaseSpeedAction;

    QAction* showCpuWindowActions[MachineConfig::MAX_CPUS];
    QAction* showTerminalActions[N_DEV_PER_IL];

    QAction* aboutAction;

    typedef std::map<StopCause, QAction*> StopMaskActionMap;
    StopMaskActionMap stopMaskActions;

    QToolBar* toolBar;
    QSlider* speedSlider;

    QTabWidget* tabWidget;

    MachineConfigView* configView;

    QWidget* cpuListPane;
    QTreeView* cpuListView;
    QTreeView* breakpointListView;
    QTreeView* suspectListView;
    TraceBrowser* traceBrowser;
    QTreeView* deviceTreeView;

    QPointer<ProcessorWindow> cpuWindows[MachineConfig::MAX_CPUS];
    QPointer<TerminalWindow> terminalWindows[N_DEV_PER_IL];

private Q_SLOTS:
    void onCreateConfig();
    void onLoadConfig();
    void onLoadRecentConfig();

    void editConfig();
    void onStopMaskChanged();

    void onSpeedActionChecked();
    void onSpeedChanged(int speed);
    void increaseSpeed();
    void decreaseSpeed();

    void showCpuWindow(int cpuId);
    void onCpuItemActivated(const QModelIndex& index);
    void showTerminal();

    void onMachineConfigChanged();

    void onMachineStarted();
    void onMachineHalted();

    void updateStoppointActionsSensitivity();

    void onAddBreakpoint();
    void onRemoveBreakpoint();
    void onAddSuspect();
    void onRemoveSuspect();
    void onAddTracepoint();
};

#endif // QMPS_MONITOR_WINDOW_H
