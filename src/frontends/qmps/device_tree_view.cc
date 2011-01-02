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

#include "qmps/device_tree_view.h"

#include <QHeaderView>

#include "qmps/application.h"
#include "qmps/boolean_item_delegate.h"
#include "qmps/device_tree_model.h"

DeviceTreeView::DeviceTreeView(QWidget* parent)
    : QTreeView(parent)
{
    BooleanItemDelegate* delegate = new BooleanItemDelegate;
    setItemDelegateForColumn(DeviceTreeModel::COLUMN_DEVICE_CONDITION, delegate);
    setAlternatingRowColors(true);

    connect(header(), SIGNAL(sectionResized(int, int, int)),
            this, SLOT(sectionResized(int, int, int)));
}

void DeviceTreeView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);

    if (model == NULL)
        return;

    QModelIndex root = rootIndex();
    int rows = model->rowCount(root);
    for (int i = 0; i < rows; i++)
        setFirstColumnSpanned(i, root, true);

    bool resizeCols = true;
    for (unsigned int i = 0; i < DeviceTreeModel::N_COLUMNS; ++i) {
        QVariant v = Appl()->settings.value(QString("DeviceTreeView/Section%1Size").arg(i));
        if (v.canConvert<int>() && v.toInt()) {
            header()->resizeSection(i, v.toInt());
            resizeCols = false;
        }
    }

    if (resizeCols) {
        resizeColumnToContents(DeviceTreeModel::COLUMN_DEVICE_NUMBER);
        resizeColumnToContents(DeviceTreeModel::COLUMN_DEVICE_CONDITION);
        resizeColumnToContents(DeviceTreeModel::COLUMN_COMPLETION_TOD);
    }

    // Why oh why is this not the default for tree views?
    header()->setMovable(false);
}

void DeviceTreeView::sectionResized(int logicalIndex, int oldSize, int newSize)
{
    UNUSED_ARG(oldSize);
    Appl()->settings.setValue(QString("DeviceTreeView/Section%1Size").arg(logicalIndex),
                              newSize);
}
