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

#include "qmps/add_breakpoint_dialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTreeView>
#include <QFontMetrics>
#include <QItemSelectionModel>

#include "umps/symbol_table.h"
#include "qmps/application.h"
#include "qmps/address_line_edit.h"
#include "qmps/symbol_table_model.h"

AddBreakpointDialog::AddBreakpointDialog(QWidget* parent)
    : QDialog(parent),
      stab(Appl()->getDebugSession()->getSymbolTable())
{
    QGridLayout* layout = new QGridLayout(this);

    layout->addWidget(new QLabel("ASID:"), 0, 0);
    asidEditor = new AsidLineEdit;
    layout->addWidget(asidEditor, 0, 1);
    asidEditor->setMinimumWidth(asidEditor->fontMetrics().width(QLatin1Char('0')) * 5);
    asidEditor->setMaximumWidth(asidEditor->fontMetrics().width(QLatin1Char('0')) * 6);

    layout->setColumnMinimumWidth(2, 12);

    layout->addWidget(new QLabel("Address:"), 0, 3);
    addressEditor = new AddressLineEdit;
    layout->addWidget(addressEditor, 0, 4);

    QAbstractTableModel* stabModel = new SymbolTableModel(this);
    proxyModel = new SortFilterSymbolTableModel(Symbol::TYPE_FUNCTION, this);
    proxyModel->setSourceModel(stabModel);

    QTreeView* symbolTableView = new QTreeView;
    symbolTableView->setSortingEnabled(true);
    symbolTableView->sortByColumn(0, Qt::AscendingOrder);
    symbolTableView->setAlternatingRowColors(true);
    symbolTableView->setModel(proxyModel);
    symbolTableView->resizeColumnToContents(0);
    symbolTableView->hideColumn(SymbolTableModel::COLUMN_END_ADDRESS);

    connect(symbolTableView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this,
            SLOT(onSelectionChanged(const QItemSelection&)));

    layout->addWidget(symbolTableView, 1, 0, 1, 5);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox, 2, 0, 1, 5);

    setWindowTitle("Add Breakpoint");
    resize(kInitialWidth, kInitialHeight);

    //connect(addressEditor, SIGNAL(textChanged(const QString&)), this, SLOT(debugg()));
}

Word AddBreakpointDialog::getStartAddress() const
{
    return addressEditor->getAddress();
}

Word AddBreakpointDialog::getASID() const
{
    return asidEditor->getAsid();
}

void AddBreakpointDialog::onSelectionChanged(const QItemSelection& selected)
{
    QModelIndexList indexes = selected.indexes();
    if (!indexes.isEmpty()) {
        int row = proxyModel->mapToSource(indexes[0]).row();
        const Symbol* symbol = stab->Get(row);
        addressEditor->setAddress(symbol->getStart());
        asidEditor->setAsid(Appl()->getConfig()->getSymbolTableASID());
    }
}
