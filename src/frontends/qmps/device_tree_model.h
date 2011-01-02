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

#ifndef QMPS_DEVICE_TREE_MODEL_H
#define QMPS_DEVICE_TREE_MODEL_H

#include <sigc++/sigc++.h>
#include <QAbstractItemModel>
#include <QIcon>

#include "umps/arch.h"

class Device;
class Machine;

class DeviceTreeModel : public QAbstractItemModel,
                        public sigc::trackable
{
    Q_OBJECT

public:
    enum {
        COLUMN_DEVICE_NUMBER = 0,
        COLUMN_DEVICE_CONDITION,
        COLUMN_DEVICE_STATUS,
        COLUMN_COMPLETION_TOD,
        N_COLUMNS
    };

    DeviceTreeModel(Machine* machine);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex& index) const;

private:
    void onDeviceStatusChanged(const char* status, Device* device);
    void onDeviceConditionChanged(bool operational, Device* device);

    Machine* const machine;

    static const char* const headerNames[N_COLUMNS];
    QIcon deviceTypeIcons[N_EXT_IL];
    static const char* const iconMap[N_EXT_IL];
};

#endif // QMPS_DEVICE_TREE_MODEL_H
