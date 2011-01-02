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

#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QDialogButtonBox>
#include <QTreeView>
#include <QFontMetrics>
#include <QItemSelectionModel>
#include <QPushButton>

#include "umps/machine_config.h"
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

    int gridRow = 0;

    layout->addWidget(new QLabel("Start:"), gridRow, 0);
    startAddressEdit = new AddressLineEdit;
    startAddressEdit->setMinimumWidth(startAddressEdit->fontMetrics().width("0xdead.beef__"));
    layout->addWidget(startAddressEdit, gridRow, 1);
    connect(startAddressEdit, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));

    layout->setColumnMinimumWidth(2, 12);

    layout->addWidget(new QLabel("End:"), gridRow, 3);
    endAddressEdit = new AddressLineEdit;
    endAddressEdit->setMinimumWidth(endAddressEdit->fontMetrics().width("0xdead.beef__"));
    layout->addWidget(endAddressEdit, gridRow, 4);
    connect(endAddressEdit, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));

    gridRow++;

    if (Appl()->getConfig()->getSymbolTableASID() == MachineConfig::MAX_ASID) {
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

        layout->addWidget(symbolTableView, gridRow++, 0, 1, 5);

        // Some sensible initial size since we're showing a symbol table
        resize(kInitialWidth, kInitialHeight);
    }

    layout->addWidget(inputErrorLabel = new QLabel, gridRow++, 0, 1, 5);
    QPalette pError = inputErrorLabel->palette();
    pError.setColor(inputErrorLabel->foregroundRole(), Qt::red);
    inputErrorLabel->setPalette(pError);

    QFrame* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator, gridRow++, 0, 1, 5);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    okButton = buttonBox->button(QDialogButtonBox::Ok);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttonBox, gridRow, 0, 1, 5);

    setWindowTitle("Add Traced Region");
}

Word AddTracepointDialog::getStartAddress() const
{
    return startAddressEdit->getAddress();
}

Word AddTracepointDialog::getEndAddress() const
{
    return endAddressEdit->getAddress();
}

void AddTracepointDialog::validate()
{
    Word s = startAddressEdit->getAddress();
    Word e = endAddressEdit->getAddress();

    if (s > e) {
        okButton->setEnabled(false);
        inputErrorLabel->setText("Start address must not be higher than end address");
        return;
    }
    if (((e - s) >> 2) + 1 > kMaxTracedRangeSize) {
        okButton->setEnabled(false);
        inputErrorLabel->setText(QString("Range size must not exceed %1 bytes")
                                 .arg(kMaxTracedRangeSize * WS));
        return;
    }

    okButton->setEnabled(true);
    inputErrorLabel->setText("");
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
