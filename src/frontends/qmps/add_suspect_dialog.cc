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

#include "qmps/add_suspect_dialog.h"

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

AddSuspectDialog::AddSuspectDialog(QWidget* parent)
    : QDialog(parent),
      stab(Appl()->getDebugSession()->getSymbolTable())
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setColumnStretch(4, 1);
    layout->setColumnStretch(7, 1);

    layout->addWidget(new QLabel("ASID:"), 0, 0);
    asidEditor = new AsidLineEdit;
    layout->addWidget(asidEditor, 0, 1);
    asidEditor->setMinimumWidth(asidEditor->fontMetrics().width("0x00__"));
    asidEditor->setMaximumWidth(asidEditor->fontMetrics().width("0x00__"));

    layout->setColumnMinimumWidth(2, 12);

    layout->addWidget(new QLabel("Start:"), 0, 3);
    startAddressEdit = new AddressLineEdit;
    startAddressEdit->setMinimumWidth(startAddressEdit->fontMetrics().width("0xdead.beef__"));
    layout->addWidget(startAddressEdit, 0, 4);
    connect(startAddressEdit, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));

    layout->setColumnMinimumWidth(5, 12);

    layout->addWidget(new QLabel("End:"), 0, 6);
    endAddressEdit = new AddressLineEdit;
    endAddressEdit->setMinimumWidth(endAddressEdit->fontMetrics().width("0xdead.beef__"));
    layout->addWidget(endAddressEdit, 0, 7);
    connect(endAddressEdit, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));

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

    layout->addWidget(symbolTableView, 1, 0, 1, 8);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    okButton = buttonBox->button(QDialogButtonBox::Ok);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox, 2, 0, 1, 8);

    setWindowTitle("Add Suspect");
    resize(kInitialWidth, kInitialHeight);
}

Word AddSuspectDialog::getStartAddress() const
{
    return startAddressEdit->getAddress();
}

Word AddSuspectDialog::getEndAddress() const
{
    return endAddressEdit->getAddress();
}

Word AddSuspectDialog::getASID() const
{
    return asidEditor->getAsid();
}

void AddSuspectDialog::validate()
{
    okButton->setEnabled(startAddressEdit->getAddress() <= endAddressEdit->getAddress());
}

void AddSuspectDialog::onSelectionChanged(const QItemSelection& selected)
{
    QModelIndexList indexes = selected.indexes();
    if (!indexes.isEmpty()) {
        int row = proxyModel->mapToSource(indexes[0]).row();
        const Symbol* symbol = stab->Get(row);
        startAddressEdit->setAddress(symbol->getStart());
        endAddressEdit->setAddress(symbol->getEnd());
        asidEditor->setAsid(Appl()->getConfig()->getSymbolTableASID());
    }
}
