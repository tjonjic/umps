/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2010, 2011 Tomislav Jonjic
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

#include "qmps/register_set_widget.h"

#include <boost/assign.hpp>

#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>

#include "qmps/register_set_snapshot.h"
#include "qmps/ui_utils.h"
#include "qmps/register_item_delegate.h"
#include "qmps/tree_view.h"
#include "qmps/application.h"

RegisterSetWidget::RegisterSetWidget(Word cpuId, QWidget* parent)
    : QDockWidget("Registers", parent),
      model(new RegisterSetSnapshot(cpuId, this)),
      delegateKey(QString("RegisterSetWidget%1/delegate").arg(cpuId))
{
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setWidget(widget);

    QActionGroup* displayGroup = new QActionGroup(this);
    QToolBar* toolBar = new QToolBar;

    addDisplayAction("Hex", new RIDelegateHex(this),
                     displayGroup, toolBar);
    addDisplayAction("Signed Decimal", new RIDelegateSignedDecimal(this),
                     displayGroup, toolBar);
    addDisplayAction("Unsigned Decimal", new RIDelegateUnsignedDecimal(this),
                     displayGroup, toolBar);
    addDisplayAction("Binary", new RIDelegateBinary(this),
                     displayGroup, toolBar);

    connect(displayGroup, SIGNAL(triggered(QAction*)), this, SLOT(setDisplayType(QAction*)));

    layout->addWidget(toolBar, 0, Qt::AlignRight);
    QFont toolBarFont = toolBar->font();
    toolBarFont.setPointSizeF(toolBarFont.pointSizeF() * .75);
    toolBar->setFont(toolBarFont);
    toolBar->setStyleSheet("QToolButton { padding: 0; }");

    treeView = new TreeView(QString("RegisterSetView%1").arg(cpuId),
                            boost::assign::list_of<int>(RegisterSetSnapshot::COL_REGISTER_MNEMONIC),
                            true);
    treeView->setItemDelegateForColumn(RegisterSetSnapshot::COL_REGISTER_VALUE,
                                       delegates[currentDelegate()]);
    treeView->setAlternatingRowColors(true);
    treeView->setModel(model);
    SetFirstColumnSpanned(treeView, true);
    layout->addWidget(treeView);

    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable);
}

void RegisterSetWidget::setDisplayType(QAction* action)
{
    int i = action->data().toInt();
    treeView->setItemDelegateForColumn(RegisterSetSnapshot::COL_REGISTER_VALUE, delegates[i]);
    Appl()->settings.setValue(delegateKey, i);
}

void RegisterSetWidget::addDisplayAction(const QString& text,
                                         QStyledItemDelegate* delegate,
                                         QActionGroup* group,
                                         QToolBar* toolBar)
{
    QAction* action = new QAction(text, group);
    action->setCheckable(true);
    int index = delegates.size();
    action->setData(QVariant::fromValue(index));
    action->setChecked(currentDelegate() == index);
    delegates.push_back(delegate);
    toolBar->addAction(action);
}

int RegisterSetWidget::currentDelegate() const
{
    return Appl()->settings.value(delegateKey).toInt();
}
