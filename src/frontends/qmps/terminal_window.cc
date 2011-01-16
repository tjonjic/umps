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

#include "qmps/terminal_window.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

#include "base/debug.h"
#include "umps/types.h"
#include "umps/device.h"
#include "qmps/application.h"
#include "qmps/terminal_view.h"
#include "qmps/terminal_window_priv.h"
#include "qmps/flat_push_button.h"

TerminalWindow::TerminalWindow(unsigned int devNo, QWidget* parent)
    : QMainWindow(parent),
      devNo(devNo)
{
    setWindowTitle(QString("uMPS Terminal %1").arg(devNo));

    TerminalDevice* terminal = getTerminal(devNo);

    QWidget* centralWidget = new QWidget;

    layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);
    terminalView = new TerminalView(terminal);
    layout->addWidget(terminalView);
    statusWidget = new TerminalStatusWidget(terminal);
    layout->addWidget(statusWidget);

    QString key = QString("TerminalWindow%1/geometry").arg(devNo);
    QVariant savedGeometry = Appl()->settings.value(key);
    if (savedGeometry.isValid()) {
        restoreGeometry(savedGeometry.toByteArray());
    } else {
        QFontMetrics fm = terminalView->fontMetrics();
        resize(fm.width(QLatin1Char('x')) * kDefaultCols, fm.lineSpacing() * kDefaultRows);
    }

    connect(debugSession, SIGNAL(MachineReset()), this, SLOT(onMachineReset()));
}

void TerminalWindow::closeEvent(QCloseEvent* event)
{
    QString key = QString("TerminalWindow%1/geometry").arg(devNo);
    Appl()->settings.setValue(key, saveGeometry());
    event->accept();
}

void TerminalWindow::onMachineReset()
{
    delete terminalView;
    delete statusWidget;

    TerminalDevice* terminal = getTerminal(devNo);

    terminalView = new TerminalView(terminal);
    layout->addWidget(terminalView);

    statusWidget = new TerminalStatusWidget(terminal);
    layout->addWidget(statusWidget);
}

TerminalDevice* TerminalWindow::getTerminal(unsigned int devNo)
{
    Device* device = debugSession->getMachine()->getDevice(4, devNo);
    assert(device->Type() == TERMDEV);
    return static_cast<TerminalDevice*>(device);
}


TerminalStatusWidget::TerminalStatusWidget(TerminalDevice* t, QWidget* parent)
    : QWidget(parent),
      terminal(t),
      expanded(false),
      expandedIcon(":/icons/expander_down-16.png"),
      collapsedIcon(":/icons/expander_up-16.png")
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(5, 0, 5, 0);
    layout->setColumnStretch(0, 1);

    hwFailureCheckBox = new QCheckBox("Hardware Failure");
    hwFailureCheckBox->setChecked(terminal->getDevNotWorking());
    connect(hwFailureCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(onHardwareFailureButtonClicked(bool)));
    layout->addWidget(hwFailureCheckBox, 0, 0);

    expanderButton = new FlatPushButton(collapsedIcon, "Show Status");
    connect(expanderButton, SIGNAL(clicked()), this, SLOT(onExpanderButtonClicked()));
    expanderButton->setIconSize(QSize(16, 16));
    layout->addWidget(expanderButton, 0, 1);

    statusAreaWidget = new QWidget;
    QGridLayout* statusAreaLayout = new QGridLayout(statusAreaWidget);
    statusAreaLayout->setContentsMargins(0, 0, 0, 5);
    statusAreaLayout->setVerticalSpacing(5);
    statusAreaLayout->setHorizontalSpacing(15);
    statusAreaLayout->setColumnStretch(1, 1);
    statusAreaLayout->setColumnStretch(3, 1);

    statusAreaLayout->addWidget(new QLabel("RX:"), 0, 0);
    statusAreaLayout->addWidget(new QLabel("TX:"), 1, 0);

    rxStatusLabel = new QLabel;
    rxStatusLabel->setMinimumWidth(kStatusLabelsMinimumWidth);
    rxStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusAreaLayout->addWidget(rxStatusLabel, 0, 1);

    txStatusLabel = new QLabel;
    txStatusLabel->setMinimumWidth(kStatusLabelsMinimumWidth);
    txStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusAreaLayout->addWidget(txStatusLabel, 1, 1);

    statusAreaLayout->addWidget(new QLabel("At:"), 0, 2);
    statusAreaLayout->addWidget(new QLabel("At:"), 1, 2);

    rxCompletionTime = new QLabel;
    rxCompletionTime->setMinimumWidth(kStatusLabelsMinimumWidth);
    rxCompletionTime->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusAreaLayout->addWidget(rxCompletionTime, 0, 3);

    txCompletionTime = new QLabel;
    txCompletionTime->setMinimumWidth(kStatusLabelsMinimumWidth);
    txCompletionTime->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusAreaLayout->addWidget(txCompletionTime, 1, 3);

    layout->addWidget(statusAreaWidget, 1, 0, 1, 2);
    statusAreaWidget->hide();

    terminal->SignalStatusChanged.connect(
        sigc::hide(sigc::mem_fun(*this, &TerminalStatusWidget::updateStatus))
    );
    terminal->SignalConditionChanged.connect(
        sigc::mem_fun(*this, &TerminalStatusWidget::onConditionChanged)
    );

    updateStatus();
}

void TerminalStatusWidget::updateStatus()
{
    rxStatusLabel->setText(terminal->getRXStatus());
    txStatusLabel->setText(terminal->getTXStatus());

    rxCompletionTime->setText(terminal->getRXCompletionTime());
    txCompletionTime->setText(terminal->getTXCompletionTime());
}

void TerminalStatusWidget::onConditionChanged(bool isWorking)
{
    hwFailureCheckBox->setChecked(!isWorking);
}

void TerminalStatusWidget::onHardwareFailureButtonClicked(bool checked)
{
    terminal->setCondition(!checked);
}

void TerminalStatusWidget::onExpanderButtonClicked()
{
    if (expanded) {
        expanded = false;
        expanderButton->setIcon(collapsedIcon);
        expanderButton->setText("Show Status");
        statusAreaWidget->hide();
    } else {
        expanded = true;
        expanderButton->setIcon(expandedIcon);
        expanderButton->setText("Hide Status");
        statusAreaWidget->show();
    }
}
