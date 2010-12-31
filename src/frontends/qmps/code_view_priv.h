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

#ifndef QMPS_CODE_VIEW_PRIV_H
#define QMPS_CODE_VIEW_PRIV_H

#include <QWidget>

#include "umps/types.h"

class CodeView;

class CodeViewMargin : public QWidget {
    Q_OBJECT

public:
    static const int kMarkerSize = 16;

    CodeViewMargin(CodeView* codeView);

    virtual QSize sizeHint() const;

protected:
    bool event(QEvent* event);
    void paintEvent(QPaintEvent* event);
    void mousePressEvent(QMouseEvent* event);

    // For some reason we have to proxy this to QAbstractScollArea
    // _explicitely_! Cannot figure out why, since by default the
    // parent should handle it automatically if ignored.
    void wheelEvent(QWheelEvent* event);

private:
    int indexAt(const QPoint& pos) const;

    CodeView* const codeView;
};

#endif // QMPS_CODE_VIEW_PRIV_H
