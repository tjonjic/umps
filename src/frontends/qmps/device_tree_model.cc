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

#include "qmps/device_tree_model.h"

#include <QFont>

#include "umps/machine.h"
#include "umps/device.h"

const char* const DeviceTreeModel::headerNames[N_COLUMNS] = {
    "Device",
    "HW Failure",
    "Status",
    "Completion Time"
};

const char* const DeviceTreeModel::iconMap[N_EXT_IL] = {
    ":/icons/disk-16.png",
    ":/icons/tape-16.png",
    ":/icons/network-16.png",
    ":/icons/printer-16.png",
    ":/icons/terminal-16.png"
};

static const char* const devTypeStr[N_EXT_IL + 1] = {
    "NULLDEV ",
    "DISK    ",
    "TAPE    ",
    "NETWORK ",
    "PRINTER ",
    "TERMINAL"
};

DeviceTreeModel::DeviceTreeModel(Machine* m)
    : QAbstractItemModel(),
      machine(m)
{
    for (unsigned int il = 0; il < N_EXT_IL; ++il) {
        for (unsigned int devNo = 0; devNo < N_DEV_PER_IL; ++devNo) {
            Device* dev = machine->getDevice(il, devNo);
            dev->SignalStatusChanged.connect(
                sigc::bind<Device*>(sigc::mem_fun(this, &DeviceTreeModel::onDeviceStatusChanged), dev)
            );
            dev->SignalConditionChanged.connect(
                sigc::bind<Device*>(sigc::mem_fun(this, &DeviceTreeModel::onDeviceConditionChanged), dev)
            );
        }
    }

    for (unsigned int i = 0; i < N_EXT_IL; ++i)
        deviceTypeIcons[i] = QIcon(iconMap[i]);
}

QModelIndex DeviceTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    // QAbstractItemModel::hasIndex() kindly checks for us if a valid
    // index with the requested row/col and parent could actually
    // exist: in other words, if parent.row() is in the range [0,
    // #rows(parent.parent()) - 1].
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (parent.isValid()) {
        unsigned int devNo = (unsigned int) row;
        if (devNo < N_DEV_PER_IL)
            return createIndex(row, column, machine->getDevice(parent.row(), devNo));
        else
            return QModelIndex();
    } else {
        if (row <= N_EXT_IL)
            return createIndex(row, column, (void*) NULL);
        else
            return QModelIndex();
    }
}

QModelIndex DeviceTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    // Presumably the index is valid. In our case that means it either
    // points to a top-level item (device class node) or a
    // second-level (device) node. The QAbstractItemModel API contract
    // specifies we have to return an invalid index for any top-level
    // items.  Also, by convention only the first column "nodes" have
    // children, so we specify 0 for the "column" of the parent. Yuck.
    Device* device = static_cast<Device*>(index.internalPointer());
    if (device != NULL)
        return createIndex(device->getInterruptLine(), 0, (void*) NULL);
    else
        return QModelIndex();
}

int DeviceTreeModel::rowCount(const QModelIndex& parent) const
{
    // Remember the convention (or is it?) that only indexes with
    // column equal 0 identify nodes with children!
    if (parent.column() > 0)
        return 0;

    // Well then, column 0 it is. We're in one of 3 cases: 1) parent
    // is the phantom root node; 2) parent is a top level node; 3)
    // parent is a leaf (device) node. In code:
    if (!parent.isValid())
        return N_EXT_IL;
    else if (parent.internalPointer() == NULL)
        return N_DEV_PER_IL;
    else
        return 0;
}

int DeviceTreeModel::columnCount(const QModelIndex& parent) const
{
    UNUSED_ARG(parent);
    return N_COLUMNS;
}

QVariant DeviceTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return headerNames[section];
    else
        return QVariant();
}

QVariant DeviceTreeModel::data(const QModelIndex& index, int role) const
{
    // Unlike in the cases above, here an invalid index does _not_
    // consitute valid input, and we wouldn't be expecting one at
    // all. Sanity check then.
    if (!index.isValid())
        return QVariant();

    // So it's either a device-class row or a device row (leaf). Which
    // one is it?
    Device* device = static_cast<Device*>(index.internalPointer());
    if (device == NULL) {
        if (index.column() == 0) {
            // Device class (interrupt line)
            if (role == Qt::DisplayRole) {
                static const char* const dtName[N_EXT_IL] = {
                    "Int. Line 3 (Disks)",
                    "Int. Line 4 (Tapes)",
                    "Int. Line 5 (Ethernet)",
                    "Int. Line 6 (Printers)",
                    "Int. Line 7 (Terminals)"
                };
                return dtName[index.row()];
            } else if (role == Qt::DecorationRole) {
                return deviceTypeIcons[index.row()];
            } else if (role == Qt::FontRole) {
                QFont f = QFont();
                f.setBold(true);
                return f;
            } else {
                return QVariant();
            }
        } else {
            return QVariant();
        }
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case COLUMN_DEVICE_NUMBER:
            return device->getNumber();

        case COLUMN_DEVICE_CONDITION:
            return device->getDevNotWorking();

        case COLUMN_DEVICE_STATUS:
            return device->getDevSStr();

        case COLUMN_COMPLETION_TOD:
            return device->getDevCTStr();
        }
    }

    // Catch-all case: something we were not interested in handling.
    return QVariant();
}

Qt::ItemFlags DeviceTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    // FIXME: Should the items here really be disabled?
#if 0
    Device* device = static_cast<Device*>(index.internalPointer());
    if (device && index.column() == COLUMN_DEVICE_CONDITION && device->Type() == NULLDEV)
        return Qt::ItemIsSelectable;
    else
#endif
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool DeviceTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    Device* device = static_cast<Device*>(index.internalPointer());
    if (device &&
        index.column() == COLUMN_DEVICE_CONDITION &&
        role == Qt::EditRole &&
        value.canConvert<bool>())
    {
        device->setCondition(!value.toBool());
        return true;
    }

    return false;
}

void DeviceTreeModel::onDeviceStatusChanged(const char* status, Device* device)
{
    UNUSED_ARG(status);

    QModelIndex idx1 = createIndex(device->getNumber(),
                                   COLUMN_DEVICE_STATUS,
                                   device);
    QModelIndex idx2 = createIndex(device->getNumber(),
                                   COLUMN_COMPLETION_TOD,
                                   device);
    Q_EMIT dataChanged(idx1, idx2);
}

void DeviceTreeModel::onDeviceConditionChanged(bool operational, Device* device)
{
    UNUSED_ARG(operational);

    QModelIndex idx = createIndex(device->getNumber(),
                                  COLUMN_DEVICE_CONDITION,
                                  device);
    Q_EMIT dataChanged(idx, idx);
}
