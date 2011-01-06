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

#ifndef QMPS_CPU_STATUS_MAP_H
#define QMPS_CPU_STATUS_MAP_H

#include <vector>

#include <QString>
#include <QObject>

class DebugSession;
class Machine;
class Processor;

class CpuStatusMap : public QObject {
    Q_OBJECT

public:
    CpuStatusMap(DebugSession* dbgSession);

    const QString& getStatus(unsigned int cpuId) const;
    const QString& getLocation(unsigned int cpuId) const;

Q_SIGNALS:
    void Changed();

private:
    struct StatusInfo {
        QString status;
        QString location;
    };

    void formatActiveCpuStatus(Processor* cpu);
    void formatActiveCpuLocation(Processor* cpu);

    DebugSession* const dbgSession;
    Machine* const machine;

    static const char* const statusTemplates[];

    std::vector<StatusInfo> statusMap;

private Q_SLOTS:
    void update();
};

#endif // QMPS_CPU_STATUS_MAP_H
