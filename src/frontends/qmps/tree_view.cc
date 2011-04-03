/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2011 Tomislav Jonjic
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

#include "qmps/tree_view.h"

#include <QHeaderView>

#include "base/lang.h"
#include "qmps/application.h"
#include "qmps/ui_utils.h"

TreeView::TreeView(const QString& name,
                   const std::list<int>& resizedToContents,
                   bool persistItemState,
                   QWidget* parent)
    : QTreeView(parent),
      resizedToContents(resizedToContents),
      persistItemState(persistItemState)
{
    setObjectName(name);

    connect(header(), SIGNAL(sectionResized(int, int, int)),
            this, SLOT(sectionResized(int, int, int)));

    if (persistItemState) {
        itemStateKey = QString("%1/ExpandedItems").arg(objectName());
        connect(this, SIGNAL(expanded(const QModelIndex&)),
                this, SLOT(saveItemState()));
        connect(this, SIGNAL(collapsed(const QModelIndex&)),
                this, SLOT(saveItemState()));
    }
}

void TreeView::setModel(QAbstractItemModel* model)
{
    QTreeView::setModel(model);

    if (model == NULL)
        return;

    // Not really sure if this should be here, but otoh cannot think
    // of a _single case_ where it would be undesired.
    header()->setMovable(false);

    bool resizeCols = true;
    for (int i = 0; i < model->columnCount(); ++i) {
        QVariant v = Appl()->settings.value(QString("%1/Section%2Size").arg(objectName()).arg(i));
        if (v.canConvert<int>() && v.toInt()) {
            header()->resizeSection(i, v.toInt());
            resizeCols = false;
        }
    }

    if (resizeCols) {
        foreach (int col, resizedToContents)
            resizeColumnToContents(col);
    }

    if (persistItemState) {
        QVariant var = Appl()->settings.value(itemStateKey);
        foreach (const QString& s, var.toStringList()) {
            bool ok;
            int row = s.toInt(&ok);
            if (!ok)
                continue;
            QModelIndex idx = model->index(row, 0);
            if (idx.isValid())
                setExpanded(idx, true);
        }
    }
}

void TreeView::sectionResized(int logicalIndex, int oldSize, int newSize)
{
    UNUSED_ARG(oldSize);
    Appl()->settings.setValue(QString("%1/Section%2Size").arg(objectName()).arg(logicalIndex),
                              newSize);
}

void TreeView::saveItemState()
{
    QStringList list;

    int nr = model()->rowCount();
    for (int i = 0; i < nr; i++)
        if (isExpanded(model()->index(i, 0)))
            list << QString::number(i);

    Appl()->settings.setValue(itemStateKey, list);
}
