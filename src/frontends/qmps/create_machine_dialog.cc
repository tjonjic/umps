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

#include "qmps/create_machine_dialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>

#include "base/debug.h"

CreateMachineDialog::CreateMachineDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create a Machine Configuration");

    QGridLayout* layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setColumnStretch(1, 1);

    QLabel* message = new QLabel(
        QString("A basic machine configuration will be created which you can later<br />"
                "customize to your needs "
                "(<b>Machine %1 Edit Configuration</b>).").arg(QChar(0x2192))
    );
    layout->addWidget(message, 0, 0, 1, 3);

    layout->setRowMinimumHeight(1, 16);

    layout->addWidget(new QLabel("Create in:"), 2, 0);
    dirEdit = new QLineEdit;
    layout->addWidget(dirEdit, 2, 1);
    QPushButton* fileChooserButton = new QPushButton("&Browse...");
    connect(fileChooserButton, SIGNAL(clicked()), this, SLOT(browseDir()));
    layout->addWidget(fileChooserButton, 2, 2);

    layout->addWidget(new QLabel("Name:"), 3, 0);
    nameEdit = new QLineEdit;
    layout->addWidget(nameEdit, 3, 1, 1, 2);

    layout->setRowMinimumHeight(4, 16);

    QFrame* separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator, 5, 0, 1, 3);

    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    createButton = buttonBox->addButton("Create", QDialogButtonBox::AcceptRole);
    createButton->setEnabled(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(buttonBox, 6, 0, 1, 3);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(dirEdit, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));
    connect(nameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(validate()));
}

QString CreateMachineDialog::getFileName() const
{
    assert(hasValidInput());
    return QDir::cleanPath(QDir(dirEdit->text()).filePath(nameEdit->text()));
}

bool CreateMachineDialog::hasValidInput() const
{
    QDir dir(dirEdit->text());

    return (!dirEdit->text().isEmpty() &&
            dir.isAbsolute() &&
            dir.exists() &&
            !nameEdit->text().isEmpty());
}

void CreateMachineDialog::validate()
{
    createButton->setEnabled(hasValidInput());
}

void CreateMachineDialog::browseDir()
{
    QString fileName = QFileDialog::getExistingDirectory(this, "Choose Directory",
                                                         dirEdit->text());
    if (!fileName.isEmpty())
        dirEdit->setText(fileName);
}
