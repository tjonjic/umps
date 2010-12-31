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

#include "stoppoint_list_model.h"

#include <QtDebug>

#include "base/debug.h"
#include "umps/stoppoint.h"
#include "umps/symbol_table.h"
#include "umps/types.h"
#include "umps/processor.h"
#include "qmps/application.h"
#include "qmps/debug_session.h"
#include "qmps/ui_utils.h"

BaseStoppointListModel::BaseStoppointListModel(StoppointSet* set, QObject* parent)
    : QAbstractTableModel(parent),
      stoppoints(set),
      symbolTable(Appl()->getDebugSession()->getSymbolTable())
{
    // Just a dumb sanity check: out lifetime is from powerup to
    // shutdown of a single machine, which implies that a symbol table
    // exists and is valid.
    assert(symbolTable != NULL);

    foreach (Stoppoint::Ptr sp, *stoppoints) {
        formattedRangeCache.push_back(formatAddressRange(sp->getRange()));
    }
}

int BaseStoppointListModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return stoppoints->Size();
    else 
        return 0;
}

void BaseStoppointListModel::Add(const AddressRange& range, AccessMode mode)
{
    beginInsertRows(QModelIndex(), stoppoints->Size(), stoppoints->Size());
    stoppoints->Add(range, mode);
    Stoppoint* sp = stoppoints->Get(stoppoints->Size() - 1);
    formattedRangeCache.push_back(formatAddressRange(sp->getRange()));
    StoppointAdded();
    endInsertRows();
}

void BaseStoppointListModel::Remove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    stoppoints->Remove(index);
    formattedRangeCache.erase(formattedRangeCache.begin() + index);
    StoppointRemoved(index);
    endRemoveRows();
}

void BaseStoppointListModel::Remove(Stoppoint* sp)
{
    for (size_t i = 0; i < stoppoints->Size(); i++) {
        if (stoppoints->Get(i) == sp) {
            Remove(i);
            break;
        }
    }
}

QString BaseStoppointListModel::getAddressRange(int i) const
{
    return formattedRangeCache[i];
}

QString BaseStoppointListModel::formatAddressRange(const AddressRange& range)
{
    Word start = range.getStart(), end = range.getEnd();

    const char *saStart, *saEnd;
    SWord ofsStart, ofsEnd;
    saStart = GetSymbolicAddress(symbolTable, range.getASID(), start, false, &ofsStart);
    if (start != end)
        saEnd = GetSymbolicAddress(symbolTable, range.getASID(), end, false, &ofsEnd);
    else
        saEnd = NULL;

    if (saStart == NULL || (start != end && saEnd == NULL)) {
        if (start != end)
            return (QString("%1 %2 %3")
                    .arg(FormatAddress(start))
                    .arg(QChar(0x2192))
                    .arg(FormatAddress(end)));
        else
            return QString(FormatAddress(start));
    } else {
        if (start != end) {
            return (QString("%1+0x%2 %3 %4+0x%5")
                    .arg(saStart)
                    .arg((quint32) ofsStart, 0, 16)
                    .arg(QChar(0x2192))
                    .arg(saEnd)
                    .arg((quint32) ofsEnd, 0, 16));
        } else {
            return QString("%1+0x%2").arg(saStart).arg((quint32) ofsStart, 0, 16);
        }
    }
}


const char* const StoppointListModel::headers[StoppointListModel::N_COLUMNS] = {
    "Id",
    "Type",
    "ASID",
    "Location",
    "Victims"
};

StoppointListModel::StoppointListModel(StoppointSet* spSet, 
                                       const char* collectionName,
                                       char idPrefix,
                                       QObject* parent)
    : BaseStoppointListModel(spSet, parent),
      collectionName(collectionName),
      idPrefix(idPrefix),
      victims(spSet->Size(), 0)
{
    RegisterSigc(stoppoints->SignalHit.connect(
                     sigc::mem_fun(this, &StoppointListModel::onHit)));
    RegisterSigc(stoppoints->SignalEnabledChanged.connect(
                     sigc::mem_fun(this, &StoppointListModel::onEnabledChanged)));

    connect(Appl()->getDebugSession(), SIGNAL(MachineRan()),
            this, SLOT(onMachineRan()));
}

int StoppointListModel::columnCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return N_COLUMNS;
    else
        return 0;
}

QVariant StoppointListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < N_COLUMNS) {
        if (section == COLUMN_STOPPOINT_ID)
            return collectionName;
        else
            return headers[section];
    }

    return QVariant();
}

QVariant StoppointListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Stoppoint* sp = stoppoints->Get(index.row());

    switch (index.column()) {
    case COLUMN_STOPPOINT_ID:
        if (role == Qt::DisplayRole)
            return QString("%1%2").arg(idPrefix).arg(sp->getId());
        if (role == Qt::CheckStateRole)
            return sp->IsEnabled() ? Qt::Checked : Qt::Unchecked;
        if (role == Qt::FontRole) {
            QFont font;
            font.setBold(victims[index.row()]);
            return font;
        }
        break;

    case COLUMN_ACCESS_TYPE:
        if (role == Qt::DisplayRole) {
            switch (sp->getAccessMode()) {
            case AM_WRITE:
                return "Write";
            case AM_READ:
                return "Read";
            case AM_READ_WRITE:
                return "Read/Write";
            default:
                return "Invalid";
            }
        }
        if (role == Qt::EditRole)
            return sp->getAccessMode();
        break;

    case COLUMN_ASID:
        if (role == Qt::DisplayRole)
            return QString("0x%1").arg((unsigned int) sp->getRange().getASID(), 0, 16);
        if (role == Qt::FontRole)
            return Appl()->getMonospaceFont();
        break;

    case COLUMN_ADDRESS_RANGE:
        if (role == Qt::DisplayRole)
            return getAddressRange(index.row());
        if (role == Qt::FontRole)
            return Appl()->getMonospaceFont();
        break;

    case COLUMN_VICTIMS:
        if (role == Qt::DisplayRole) {
            QString cpus;
            uint32_t temp = victims[index.row()];
            for (uint32_t i = 0; temp && i < 32; ++i) {
                if (temp & (1U << i)) {
                    if (!cpus.isEmpty())
                        cpus += ", ";
                    cpus += QString("CPU%1").arg(i);
                }
                temp &= ~(1U << i);
            }
            return cpus;
        }
        if (role == Qt::FontRole) {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;

    default:
        AssertNotReached();
    }

    return QVariant();
}

Qt::ItemFlags StoppointListModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    switch (index.column()) {
    case COLUMN_STOPPOINT_ID:
        return QAbstractTableModel::flags(index) | Qt::ItemIsUserCheckable;

    case COLUMN_ACCESS_TYPE:
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    default:
        return QAbstractTableModel::flags(index);
    }
}

bool StoppointListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    if (index.column() == COLUMN_STOPPOINT_ID && role == Qt::CheckStateRole) {
        Stoppoint* sp = stoppoints->Get(index.row());
        stoppoints->SetEnabled(index.row(), !sp->IsEnabled());
    }

    if (index.column() == COLUMN_ACCESS_TYPE &&
        role == Qt::EditRole &&
        value.canConvert<unsigned int>())
    {
        // This is as unkosher as it gets...
        AccessMode newMode = (AccessMode) value.toUInt();
        stoppoints->Get(index.row())->setAccessMode(newMode);
        Q_EMIT dataChanged(index, index);
        return true;
    }

    return false;
}

void StoppointListModel::onHit(size_t spIndex,
                               const Stoppoint* stoppoint,
                               Word addr,
                               const Processor* cpu)
{
    victims[spIndex] |= 1 << cpu->getId();

    QModelIndex idIndex = index(spIndex, COLUMN_STOPPOINT_ID);
    Q_EMIT dataChanged(idIndex, idIndex);

    QModelIndex victimIndex = index(spIndex, COLUMN_VICTIMS);
    Q_EMIT dataChanged(victimIndex, victimIndex);
}

void StoppointListModel::onEnabledChanged(size_t spIndex)
{
    QModelIndex idx = index(spIndex, COLUMN_STOPPOINT_ID);
    Q_EMIT dataChanged(idx, idx);
}

void StoppointListModel::onMachineRan()
{
    for (unsigned int i = 0; i < stoppoints->Size(); i++) {
        if (victims[i]) {
            victims[i] = 0;
            QModelIndex idIndex = index(i, COLUMN_STOPPOINT_ID);
            Q_EMIT dataChanged(idIndex, idIndex);
            QModelIndex victimIndex = index(i, COLUMN_VICTIMS);
            Q_EMIT dataChanged(victimIndex, victimIndex);
        }
    }
}

void StoppointListModel::StoppointAdded()
{
    victims.push_back(0);
}

void StoppointListModel::StoppointRemoved(int index)
{
    victims.erase(victims.begin() + index);
}
