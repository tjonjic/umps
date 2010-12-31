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

#ifndef QMPS_PROCESSOR_LIST_MODEL_H
#define QMPS_PROCESSOR_LIST_MODEL_H

#include <QModelIndex>
#include <QAbstractTableModel>

class MachineConfig;
class DebugSession;
class CpuStatusMap;

class ProcessorListModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum {
        COLUMN_CPU_ID,
        COLUMN_CPU_STATUS,
        COLUMN_CPU_ADDRESS,
        N_COLUMNS
    };

    ProcessorListModel(QObject* parent = 0);

    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;

private:
    static const char* headers[N_COLUMNS];

    const MachineConfig* const config;
    const DebugSession* const dbgSession;
    const CpuStatusMap* const cpuStatusMap;

private Q_SLOTS:
    void notifyStatusChanged();
};

#endif // QMPS_PROCESSOR_LIST_MODEL_H
