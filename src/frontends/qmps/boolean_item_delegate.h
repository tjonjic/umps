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

#ifndef QMPS_BOOLEAN_ITEM_DELEGATE_H
#define QMPS_BOOLEAN_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class BooleanItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    BooleanItemDelegate(QObject* parent = 0);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const;

    // We reimplement `editorEvent' as the implementation in
    // QStyledItemDelegate isn't of any use for us (i.e. we don't want
    // a default editor!).
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index);

private:
    static QRect buttonGeometry(const QRect& viewItemRect);
};

#endif // QMPS_BOOLEAN_ITEM_DELEGATE_H
