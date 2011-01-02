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

#include "qmps/boolean_item_delegate.h"

#include <QPainter>
#include <QMouseEvent>

#include "qmps/application.h"

BooleanItemDelegate::BooleanItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

void BooleanItemDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    QVariant variant = index.data();

    if (variant.canConvert<bool>()) {
        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        if (option.state & QStyle::State_HasFocus) {
            QStyleOptionFocusRect focusOptions;
            focusOptions.rect = option.rect;
            QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOptions, painter);
        }

        QStyleOptionButton buttonStyleOptions;
        buttonStyleOptions.rect = buttonGeometry(option.rect);

        if (index.flags() & Qt::ItemIsEnabled)
            buttonStyleOptions.state |= QStyle::State_Enabled;
        else
            buttonStyleOptions.state &= ~QStyle::State_Enabled;

        if (variant.toBool())
            buttonStyleOptions.state |= QStyle::State_On;
        else
            buttonStyleOptions.state |= QStyle::State_Off;

        QApplication::style()->drawControl(QStyle::CE_CheckBox, &buttonStyleOptions, painter);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

bool BooleanItemDelegate::editorEvent(QEvent* event,
                                      QAbstractItemModel* model,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex &index)
{
    if ((event->type() == QEvent::MouseButtonRelease) ||
        (event->type() == QEvent::MouseButtonDblClick))
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton ||
            !buttonGeometry(option.rect).contains(mouseEvent->pos()))
        {
            return false;
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space &&
            static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
        {
            return false;
        }
    } else {
        return false;
    }

    return model->setData(index, !index.data().toBool(), Qt::EditRole);
}

QRect BooleanItemDelegate::buttonGeometry(const QRect& viewItemRect)
{
    QStyleOptionButton dummy;
    QRect er = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &dummy);

    return QRect(viewItemRect.x() + viewItemRect.width() / 2 - er.width() / 2,
                 viewItemRect.y() + viewItemRect.height() / 2 - er.height() / 2,
                 er.width(),
                 er.height());
}
