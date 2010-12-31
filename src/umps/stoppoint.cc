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

#include "umps/stoppoint.h"

#include <algorithm>
#include <boost/format.hpp>

std::string Stoppoint::ToString() const
{
    static const char* fmtStr = "<Stoppoint id=%u enabled=%d, access_mode=%u, asid=0x%02x, range=[0x%08x,0x%08x]>";
    return boost::str(boost::format(fmtStr)
                      %id %enabled
                      %accessMode
                      %range.getASID() %range.getStart() %range.getEnd());
}

StoppointSet::~StoppointSet()
{
}

Stoppoint* StoppointSet::Find(Word asid, Word addr)
{
    AddressRange r(asid, addr, addr);
    StoppointMap::iterator it = addressMap.find(r);
    return (it != addressMap.end()) ? it->second : NULL;
}

bool StoppointSet::Add(const AddressRange& range, AccessMode mode)
{
    // Check for collisions
    foreach (Stoppoint::Ptr p, points) {
        if (p->getRange().Overlaps(range))
            return false;
    }

    // No overlap: safe to add
    Stoppoint* p = new Stoppoint(nextId++, range, mode);
    points.push_back(Stoppoint::Ptr(p));
    addressMap[p->getRange()] = p;

    SignalStoppointInserted();
    return true;
}

void StoppointSet::Remove(size_t index)
{
    assert(index < Size());

    Stoppoint::Ptr p = points[index];

    StoppointMap::iterator it = addressMap.find(p->getRange());
    assert(it != addressMap.end());
    addressMap.erase(it);

    points.erase(points.begin() + index);

    SignalStoppointRemoved(index);
}

void StoppointSet::SetEnabled(size_t index, bool setting)
{
    assert(index <= Size());
    if (points[index]->IsEnabled() != setting) {
        points[index]->SetEnabled(setting);
        SignalEnabledChanged(index);
    }
}

Stoppoint* StoppointSet::Probe(Word asid, Word addr, AccessMode mode, const Processor* cpu) const
{
    if (IsEmpty())
        return NULL;

    AddressRange range(asid, addr, addr);
    StoppointMap::const_iterator it = addressMap.lower_bound(range);
    if (it == addressMap.end() ||
        (range < it->first && it != addressMap.begin()))
    {
        --it;
    }
    Stoppoint* p = it->second;

    if (p->Matches(asid, addr, mode)) {
        size_t index = 0;
        for (index = 0; index < points.size(); ++index)
            if (points[index].get() == p)
                break;
        assert(index < points.size());
        SignalHit.emit(index, p, addr, cpu);
        return p;
    } else {
        return NULL;
    }
}

std::string StoppointSet::ToString(bool sorted) const
{
    std::string result = "[";

    bool first = true;
    if (sorted) {
        StoppointMap::const_iterator it;
        for (it = addressMap.begin(); it != addressMap.end(); ++it) {
            if (!first)
                result.append(",\n ");
            first = false;
            result.append(it->second->ToString());
        }
    } else {
        foreach (const Stoppoint::Ptr p, points) {
            if (!first)
                result.append(",\n ");
            first = false;
            result.append(p->ToString());
        }
    }

    return result.append("]");
}
