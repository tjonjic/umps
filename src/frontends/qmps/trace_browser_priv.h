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

#ifndef QMPS_TRACE_BROWSER_PRIV_H
#define QMPS_TRACE_BROWSER_PRIV_H

#include <vector>
#include <sigc++/sigc++.h>

#include <QPlainTextEdit>

#include "base/trackable_mixin.h"
#include "umps/types.h"
#include "qmps/stoppoint_list_model.h"
#include "qmps/memory_view_delegate.h"

class Stoppoint;
class StoppointSet;
class Processor;

class TracepointListModel : public BaseStoppointListModel,
                            public TrackableMixin
{
    Q_OBJECT

public:
    TracepointListModel(StoppointSet* tracepoints, QObject* parent = 0);

    int columnCount(const QModelIndex&) const { return 1; }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex& index, int role) const;

    bool IsDirty(int row) const { return dirtySet[row]; }
    void ClearDirty(int row);

protected:
    virtual void StoppointAdded();
    virtual void StoppointRemoved(int index);

private:
    void onHit(size_t spIndex,
               const Stoppoint* stoppoint,
               Word addr,
               const Processor* cpu);

    // For the uninitiated: beware of the vector<bool> specialization!
    // It might be efficient but has weird semantics. Just ask the
    // mighty Internet.
    typedef std::vector<bool> BitSet;
    BitSet dirtySet;

    // We keep this here because of pure desperation: in a perfect
    // world disconnections should work automagically via
    // sigc::trackable. It refuses to work, however, for some reason
    // or other. Sigh.
#if 0
    sigc::connection onHitConnection;
#endif
};

class AsciiView : public QPlainTextEdit,
                  public MemoryViewDelegate
{
    Q_OBJECT

public:
    static AsciiView* Create(Word start, Word end);
    virtual void Refresh();

private:
    static const unsigned int kUnicodeReplacementChar = 0xFFFD;

    AsciiView(Word start, Word end);

    const Word start;
    const Word end;
};

#endif // QMPS_TRACE_BROWSER_PRIV_H
