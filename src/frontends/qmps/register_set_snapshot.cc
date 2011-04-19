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

#include "qmps/register_set_snapshot.h"

#include <boost/bind.hpp>

#include "base/debug.h"
#include "base/lang.h"

#include "umps/disassemble.h"
#include "umps/systembus.h"
#include "qmps/application.h"

const char* const RegisterSetSnapshot::headers[RegisterSetSnapshot::N_COLUMNS] = {
    "Register",
    "Value"
};

const char* const RegisterSetSnapshot::registerTypeNames[RegisterSetSnapshot::kNumRegisterTypes] = {
    "CPU Registers",
    "CP0 Registers",
    "Other Registers"
};

RegisterSetSnapshot::RegisterSetSnapshot(Word cpuId, QObject* parent)
    : QAbstractItemModel(parent),
      cpuId(cpuId)
{
    connect(debugSession, SIGNAL(MachineStopped()), this, SLOT(updateCache()));
    connect(debugSession, SIGNAL(MachineRan()), this, SLOT(updateCache()));
    connect(debugSession, SIGNAL(DebugIterationCompleted()), this, SLOT(updateCache()));

    connect(debugSession, SIGNAL(MachineReset()), this, SLOT(reset()));

    topLevelFont.setBold(true);

    foreach (Word& v, gprCache)
        v = 0;
    foreach (Word& v, cp0Cache)
        v = 0;

    reset();
}

QModelIndex RegisterSetSnapshot::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent) || column >= N_COLUMNS)
        return QModelIndex();

    if (parent.isValid()) {
        switch (parent.row() + 1) {
        case RT_GENERAL:
            if ((unsigned int) row < Processor::kNumCPURegisters)
                return createIndex(row, column, (quint32) RT_GENERAL);
            break;
        case RT_CP0:
            if ((unsigned int) row < Processor::kNumCP0Registers)
                return createIndex(row, column, (quint32) RT_CP0);
            break;
        case RT_OTHER:
            if ((unsigned int) row < sprCache.size())
                return createIndex(row, column, (quint32) RT_OTHER);
            break;
        }
    } else if (row < kNumRegisterTypes) {
        return createIndex(row, column, (quint32) 0);
    }

    // Fallback case - clearly something bogus
    return QModelIndex();
}

QModelIndex RegisterSetSnapshot::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    if (index.internalId() == 0)
        return QModelIndex();
    else
        return createIndex(index.internalId() - 1, 0, (quint32) 0);
}

int RegisterSetSnapshot::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid()) {
        // It's the root index, so return the number of toplevel
        // items.
        return kNumRegisterTypes;
    } else if (parent.internalId() == 0) {
        // 2nd level items
        switch (parent.row() + 1) {
        case RT_GENERAL:
            return Processor::kNumCPURegisters;
        case RT_CP0:
            return Processor::kNumCP0Registers;
        case RT_OTHER:
            return sprCache.size();
        default:
            AssertNotReached();
        }
    } else {
        // Leaf items have no children.
        return 0;
    }
}

int RegisterSetSnapshot::columnCount(const QModelIndex& parent) const
{
    UNUSED_ARG(parent);
    return N_COLUMNS;
}

QVariant RegisterSetSnapshot::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return headers[section];
    else
        return QVariant();
}

QVariant RegisterSetSnapshot::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.internalId() == 0) {
        if (index.column() != 0)
            return QVariant();
        if (role == Qt::DisplayRole)
            return registerTypeNames[index.row()];
        if (role == Qt::FontRole)
            return topLevelFont;
    } else {
        if (role == Qt::DisplayRole) {
            switch (index.column()) {
            case COL_REGISTER_MNEMONIC:
                switch (index.internalId()) {
                case RT_GENERAL:
                    return RegName(index.row());
                case RT_CP0:
                    return CP0RegName(index.row());
                case RT_OTHER:
                    return sprCache[index.row()].name;
                default:
                    return QVariant();
                }

            case COL_REGISTER_VALUE:
                switch (index.internalId()) {
                case RT_GENERAL:
                    return gprCache[index.row()];
                case RT_CP0:
                    return cp0Cache[index.row()];
                    break;
                case RT_OTHER:
                    return sprCache[index.row()].value;
                    break;
                default:
                    return QVariant();
                }

            default:
                AssertNotReached();
            }
        } else if (role == Qt::FontRole) {
            return Appl()->getMonospaceFont();
        }
    }

    return QVariant();
}

bool RegisterSetSnapshot::setData(const QModelIndex& index, const QVariant& variant, int role)
{
    if (!(index.isValid() &&
          role == Qt::EditRole &&
          variant.canConvert<Word>() &&
          index.internalId() &&
          index.column() == COL_REGISTER_VALUE))
    {
        return false;
    }

    int r = index.row();

    switch (index.internalId()) {
    case RT_GENERAL:
        cpu->setGPR(r, variant.value<Word>());
        if (gprCache[r] != (Word) cpu->getGPR(r)) {
            gprCache[r] = cpu->getGPR(r);
            Q_EMIT dataChanged(index, index);
        }
        break;

    case RT_CP0:
        cpu->setCP0Reg(r, variant.value<Word>());
        if (cp0Cache[r] != cpu->getCP0Reg(r)) {
            cp0Cache[r] = cpu->getCP0Reg(r);
            Q_EMIT dataChanged(index, index);
        }
        break;

    case RT_OTHER:
        if (sprCache[r].setter) {
            sprCache[r].setter(variant.value<Word>());
            if (sprCache[r].value != sprCache[r].getter()) {
                sprCache[r].value = sprCache[r].getter();
                Q_EMIT dataChanged(index, index);
            }
        }
        break;

    default:
        AssertNotReached();
    }

    return true;
}

Qt::ItemFlags RegisterSetSnapshot::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    if (index.internalId() == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    switch (index.column()) {
    case COL_REGISTER_MNEMONIC:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    case COL_REGISTER_VALUE:
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    default:
        AssertNotReached();
    }

    return 0;
}

void RegisterSetSnapshot::reset()
{
    cpu = debugSession->getMachine()->getProcessor(cpuId);

    sprCache.clear();
    sprCache.push_back(SpecialRegisterInfo("nextPC",
                                           boost::bind(&Processor::getNextPC, cpu),
                                           boost::bind(&Processor::setNextPC, cpu, _1)));
    sprCache.push_back(SpecialRegisterInfo("succPC",
                                           boost::bind(&Processor::getSuccPC, cpu),
                                           boost::bind(&Processor::setSuccPC, cpu, _1)));
    sprCache.push_back(SpecialRegisterInfo("prevPhysPC",
                                           boost::bind(&Processor::getPrevPPC, cpu)));
    sprCache.push_back(SpecialRegisterInfo("currPhysPC",
                                           boost::bind(&Processor::getCurrPPC, cpu)));

    SystemBus* bus = debugSession->getMachine()->getBus();
    sprCache.push_back(SpecialRegisterInfo("Timer",
                                           boost::bind(&SystemBus::getTimer, bus),
                                           boost::bind(&SystemBus::setTimer, bus, _1)));

    updateCache();
}

void RegisterSetSnapshot::updateCache()
{
    for (unsigned int i = 0; i < Processor::kNumCPURegisters; i++) {
        Word value = cpu->getGPR(i);
        if (gprCache[i] != value) {
            gprCache[i] = value;
            QModelIndex index = createIndex(i, COL_REGISTER_VALUE, RT_GENERAL);
            Q_EMIT dataChanged(index, index);
        }
    }

    for (unsigned int i = 0; i < Processor::kNumCP0Registers; i++) {
        Word value = cpu->getCP0Reg(i);
        if (cp0Cache[i] != value) {
            cp0Cache[i] = value;
            QModelIndex index = createIndex(i, COL_REGISTER_VALUE, RT_CP0);
            Q_EMIT dataChanged(index, index);
        }
    }

    int row = 0;
    foreach (SpecialRegisterInfo& sr, sprCache) {
        Word value = sr.getter();
        if (value != sr.value) {
            sr.value = value;
            QModelIndex index = createIndex(row, COL_REGISTER_VALUE, RT_OTHER);
            Q_EMIT dataChanged(index, index);
        }
        row++;
    }
}
