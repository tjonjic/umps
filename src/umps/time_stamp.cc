/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2004 Mauro Morsiani
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

#include "umps/time_stamp.h"

#include "umps/utility.h"

TimeStamp::TimeStamp(Word hi, Word lo)
    : hiTS(hi), loTS(lo)
{}

// This method creates a new TS and sets it to the value of another,
// plus an (optional) increment
TimeStamp::TimeStamp(const TimeStamp* ts, Word inc)
    : hiTS(ts->getHiTS()),
      loTS(ts->getLoTS())
{
    if (UnsAdd(&loTS, loTS, inc))
        hiTS++;
}

// Increase the timestamp by 1
void TimeStamp::Increase(Word amount)
{
    if (UnsAdd(&loTS, loTS, amount))
        // unsigned overflow occurred: need to increase HiTS
        hiTS++;
}

// This method compares 2 TSs and returns TRUE if the first is 
// less than or equal to the second, FALSE otherwise
bool TimeStamp::LessEq(const TimeStamp* ts2) const
{
    return hiTS < ts2->hiTS || (hiTS == ts2->hiTS && loTS <= ts2->loTS);
}
