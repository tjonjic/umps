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

#include <QtDebug>

#include "qmps/register_set_widget.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QTreeView>
#include <QStyledItemDelegate>

#include "qmps/register_set_snapshot.h"
#include "qmps/ui_utils.h"

class RegisterItemDelegate : public QStyledItemDelegate {
public:
    RegisterItemDelegate(QObject* parent = 0)
        : QStyledItemDelegate(parent) {}

    virtual QString displayText(const QVariant& value, const QLocale& locale) const;

protected:
    virtual QString Text(Word value) const = 0;
};

QString RegisterItemDelegate::displayText(const QVariant& variant, const QLocale& locale) const
{
    if (variant.canConvert<Word>())
        return Text(variant.value<Word>());
    else
        return QStyledItemDelegate::displayText(variant, locale);
}

class RIDelegateHex : public RegisterItemDelegate {
public:
    RIDelegateHex(QObject* parent = 0)
        : RegisterItemDelegate(parent) {}

    virtual QString Text(Word value) const
    {
        return QString("0x%1").arg(value, 8, 16, QLatin1Char('0'));
    }
};

class RIDelegateSignedDecimal : public RegisterItemDelegate {
public:
    RIDelegateSignedDecimal(QObject* parent = 0)
        : RegisterItemDelegate(parent) {}

    virtual QString Text(Word value) const
    {
        return QString::number((SWord) value, 10);
    }
};

class RIDelegateUnsignedDecimal : public RegisterItemDelegate {
public:
    RIDelegateUnsignedDecimal(QObject* parent = 0)
        : RegisterItemDelegate(parent) {}

    virtual QString Text(Word value) const
    {
        return QString::number(value, 10);
    }
};

class RIDelegateBinary : public RegisterItemDelegate {
public:
    RIDelegateBinary(QObject* parent = 0)
        : RegisterItemDelegate(parent) {}

    virtual QString Text(Word value) const
    {
        return (QString("%1|%2|%3|%4")
                .arg((value >> 24) & 0xFFU, 8, 2, QLatin1Char('0'))
                .arg((value >> 16) & 0xFFU, 8, 2, QLatin1Char('0'))
                .arg((value >>  8) & 0xFFU, 8, 2, QLatin1Char('0'))
                .arg((value >>  0) & 0xFFU, 8, 2, QLatin1Char('0')));
    }
};

static void addDisplayAction(const QString& text,
                             QStyledItemDelegate* delegate,
                             QActionGroup* group,
                             QToolBar* toolBar,
                             bool checked = false)
{
    QAction* action = new QAction(text, group);
    action->setCheckable(true);
    action->setData(QVariant::fromValue((void*) delegate));
    action->setChecked(checked);
    toolBar->addAction(action);
}

RegisterSetWidget::RegisterSetWidget(Processor* cpu, QWidget* parent)
    : QDockWidget("Registers", parent),
      model(new RegisterSetSnapshot(cpu, this))
{
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    setWidget(widget);

    QActionGroup* displayGroup = new QActionGroup(this);
    QToolBar* toolBar = new QToolBar;
    QStyledItemDelegate* hexDelegate = new RIDelegateHex(this);
    addDisplayAction("Hex", hexDelegate,
                     displayGroup, toolBar, true);
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

    treeView = new QTreeView;
    treeView->setItemDelegateForColumn(RegisterSetSnapshot::COL_REGISTER_VALUE, hexDelegate);
    treeView->setAlternatingRowColors(true);
    treeView->setModel(model);
    SetFirstColumnSpanned(treeView, true);
    layout->addWidget(treeView);

    setAllowedAreas(Qt::AllDockWidgetAreas);
}

void RegisterSetWidget::setDisplayType(QAction* action)
{
    QStyledItemDelegate* delegate = (QStyledItemDelegate*) action->data().value<void*>();
    treeView->setItemDelegateForColumn(RegisterSetSnapshot::COL_REGISTER_VALUE, delegate);
}
