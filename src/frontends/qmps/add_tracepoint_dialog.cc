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

#include "qmps/add_tracepoint_dialog.h"

#include <QtDebug>

#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTreeView>
#include <QFontMetrics>
#include <QItemSelectionModel>
#include <QPushButton>

#include "umps/symbol_table.h"
#include "qmps/application.h"
#include "qmps/address_line_edit.h"
#include "qmps/symbol_table_model.h"

AddTracepointDialog::AddTracepointDialog(QWidget* parent)
    : QDialog(parent),
      stab(Appl()->getDebugSession()->getSymbolTable())
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(4, 1);

    layout->addWidget(new QLabel("Start:"), 0, 0);
    startAddressEdit = new AddressLineEdit;
    startAddressEdit->setMinimumWidth(startAddressEdit->fontMetrics().width("0xdead.beef__"));
    layout->addWidget(startAddressEdit, 0, 1);

    layout->setColumnMinimumWidth(2, 12);

    layout->addWidget(new QLabel("End:"), 0, 3);
    endAddressEdit = new AddressLineEdit;
    endAddressEdit->setMinimumWidth(endAddressEdit->fontMetrics().width("0xdead.beef__"));
    layout->addWidget(endAddressEdit, 0, 4);

    QAbstractTableModel* stabModel = new SymbolTableModel(this);
    proxyModel = new SortFilterSymbolTableModel(Symbol::TYPE_OBJECT, this);
    proxyModel->setSourceModel(stabModel);

    QTreeView* symbolTableView = new QTreeView;
    symbolTableView->setSortingEnabled(true);
    symbolTableView->sortByColumn(SymbolTableModel::COLUMN_SYMBOL, Qt::AscendingOrder);
    symbolTableView->setAlternatingRowColors(true);
    symbolTableView->setModel(proxyModel);
    symbolTableView->resizeColumnToContents(SymbolTableModel::COLUMN_SYMBOL);

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

    setWindowTitle("Add Traced Region");
    resize(kInitialWidth, kInitialHeight);
}

Word AddTracepointDialog::getStartAddress() const
{
    return startAddressEdit->getAddress();
}

Word AddTracepointDialog::getEndAddress() const
{
    return endAddressEdit->getAddress();
}

void AddTracepointDialog::onSelectionChanged(const QItemSelection& selected)
{
    QModelIndexList indexes = selected.indexes();
    if (!indexes.isEmpty()) {
        int row = proxyModel->mapToSource(indexes[0]).row();
        const Symbol* symbol = stab->Get(row);
        startAddressEdit->setAddress(symbol->getStart());
        endAddressEdit->setAddress(symbol->getEnd());
    }
}
