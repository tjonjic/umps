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

#ifndef UMPS_TIME_STAMP_H
#define UMPS_TIME_STAMP_H

#include "umps/types.h"

// This class implements the TimeStamps used for the system clock and
// in the Event objects to schedule device operations and interrupts.
// Every object is a 64 bit counter, split into two 32 bit parts (high
// & low). It is used to schedule I/O operations inside the simulator
// and to handle the system clock.  TimeStamp class is very easy to
// understand: it has been done so to provide some example to be given
// to C++ beginners.

class TimeStamp {
public:
    TimeStamp(Word hi = 0, Word lo = 0);

    // This method creates a new TS and sets it to the value of another,
    // plus an (optional) increment
    TimeStamp(const TimeStamp* ts, Word inc = 0);

    // Increase the timestamp by 1
    void Increase();

    // This method returns the current value of hiTS part
    Word getHiTS() const { return hiTS; }

    // This method returns the current value of loTS part
    Word getLoTS() const { return loTS; }

    // This method sets hiTS value
    void setHiTS(Word hi) { hiTS = hi; }

    // This method sets loTS value
    void setLoTS(Word lo) { loTS = lo; }

    // This method compares 2 TSs and returns TRUE if the first is
    // less than or equal to the second, FALSE otherwise
    bool LessEq(const TimeStamp* ts2) const;

private:
    // high part of the TimeStamp
    Word hiTS;		
    // low part
    Word loTS;
};

#endif // UMPS_TIME_STAMP_H
