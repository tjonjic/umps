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

#ifndef QMPS_REGISTER_SET_SNAPSHOT_H
#define QMPS_REGISTER_SET_SNAPSHOT_H

#include <vector>
#include <boost/function.hpp>

#include <QAbstractItemModel>
#include <QFont>

#include "umps/types.h"
#include "umps/processor.h"

class RegisterSetSnapshot : public QAbstractItemModel {
    Q_OBJECT

public:
    enum {
        COL_REGISTER_MNEMONIC = 0,
        COL_REGISTER_VALUE,
        N_COLUMNS
    };

    RegisterSetSnapshot(Word cpuId, QObject* parent = 0);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex& index) const;

private:
    static const int kNumRegisterTypes = 3;

    enum RegisterType {
        RT_GENERAL = 1,
        RT_CP0     = 2,
        RT_OTHER   = 3
    };

    struct SpecialRegisterInfo {
        SpecialRegisterInfo(const char* str, boost::function<Word ()> get)
            : name(str), getter(get), value(0)
        {}
        SpecialRegisterInfo(const char* str,
                            boost::function<Word ()> get,
                            boost::function<void (Word)> set)
            : name(str), getter(get), setter(set), value(0)
        {}
        const char* name;
        boost::function<Word ()> getter;
        boost::function<void (Word)> setter;
        Word value;
    };

    static const char* const headers[N_COLUMNS];

    static const char* const registerTypeNames[kNumRegisterTypes];

    const Word cpuId;
    Processor* cpu;

    Word gprCache[Processor::kNumCPURegisters];
    Word cp0Cache[Processor::kNumCP0Registers];
    std::vector<SpecialRegisterInfo> sprCache;

    QFont topLevelFont;

private Q_SLOTS:
    void reset();
    void updateCache();
};

#endif // QMPS_REGISTER_SET_SNAPSHOT_H
