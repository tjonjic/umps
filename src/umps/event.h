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

#ifndef UMPS_EVENT_H
#define UMPS_EVENT_H

#include "umps/types.h"

class TimeStamp;

// Event class is used to keep track of the external events of the
// system: device operations and interrupt generation.
// Every object contains a device number saying what device will complete
// its operation and generate a interrupt, a TimeStamp saying when it 
// will happen, and a pointer to the next event in a structure, if needed
// (set to NULL otherwise) 

class Event {
public:
    // This method creates a new Event object and initalizes it
    Event(TimeStamp * ts, Word inc, unsigned int il, unsigned int dev);

    ~Event();

    // This method returns the interrupt line of the device requiring
    // the Event
    unsigned int getIntLine();

    // This method returns the device number of the device requiring
    // the Event
    unsigned int getDevNum();

    // This method returns the TimeStamp access pointer
    TimeStamp *getTS();

    // This method links an Event to its successor in a structure
    void AddBefore(Event * ev);

    // This method inserts an Event after another, linking the former to the
    // successor of the latter
    void InsAfter(Event * ev);

    // This method returns the pointer to the successor of an Event
    Event *Next();

private:
    // Interrupt line and device number of associated dev.
    unsigned int intL;
    unsigned int devNum;

    // Event verification time
    TimeStamp *time;

    Event *next;
};


// This class implements a NULL-terminated sorted queue of Event objects,
// used to schedule the device events in the system.
// An object contains a pointer to the head of the queue, and a pointer to
// the last inserted Event (to speed up insertion).
// The queue is sorted on ascending timestamp order

class EventQueue {
public:
    // This method creates a new (empty) queue
    EventQueue();

    ~EventQueue();

    // This method returns TRUE if the queue is empty, FALSE otherwise
    bool IsEmptyQ();

    // This method returns the TimeStamp in the EventQueue
    // head, if queue is not empty, and NULL otherwise
    TimeStamp *getHTS();

    // This method returns the interrupt line of the EventQueue head, if
    // not empty (0 otherwise)
    unsigned int getHIntLine();

    // This method returns the device number of the EventQueue head, if
    // not empty (0 otherwise)
    unsigned int getHDevNum();

    // This method creates a new Event object and inserts it in the
    // EventQueue; EventQueue is sorted on ascending time order
    TimeStamp* InsertQ(TimeStamp* ts, Word inc, unsigned int il, unsigned int dev);

    // This method removes the head of a (not empty) queue and sets it to the
    // following Event          
    void RemoveHead();

private:
    // head of the queue
    Event* head;

    // last Event inserted
    Event* lastIns;
};

#endif // UMPS_EVENT_H
