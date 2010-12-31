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

#include "qmps/suspect_type_delegate.h"

#include <QComboBox>

#include "base/lang.h"
#include "umps/stoppoint.h"

SuspectTypeDelegate::ItemInfo SuspectTypeDelegate::valueMap[] = {
    { AM_WRITE, "Write" },
    { AM_READ, "Read" },
    { AM_READ_WRITE, "Read/Write" }
};

SuspectTypeDelegate::SuspectTypeDelegate(QWidget* parent)
    : QStyledItemDelegate(parent)
{}

QWidget* SuspectTypeDelegate::createEditor(QWidget* parent,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const
{
    UNUSED_ARG(option);
    UNUSED_ARG(index);

    QComboBox* editor = new QComboBox(parent);
    for (unsigned int i = 0; i < kValidTypes; i++)
        editor->addItem(valueMap[i].label);
    return editor;
}

void SuspectTypeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* comboBox = static_cast<QComboBox*>(editor);

    unsigned int value = index.data(Qt::EditRole).toUInt();
    unsigned int i;
    for (i = 0; i < kValidTypes; i++)
        if (value == valueMap[i].value)
            break;
    comboBox->setCurrentIndex(i);
}

void SuspectTypeDelegate::setModelData(QWidget* editor,
                                       QAbstractItemModel* model,
                                       const QModelIndex& index) const
{
    QComboBox* comboBox = static_cast<QComboBox*>(editor);
    model->setData(index, valueMap[comboBox->currentIndex()].value, Qt::EditRole);
}

void SuspectTypeDelegate::updateEditorGeometry(QWidget* editor,
                                               const QStyleOptionViewItem& option,
                                               const QModelIndex& index) const
{
    editor->setGeometry(option.rect);
}
