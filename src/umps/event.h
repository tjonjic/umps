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

#include <boost/function.hpp>

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
    typedef boost::function<void ()> Callback;

    Event(TimeStamp* ts, Word inc, Callback callback);

    ~Event();

    // This method returns the TimeStamp access pointer
    TimeStamp *getTS();

    Callback getCallback() const { return callback; }

    // This method links an Event to its successor in a structure
    void AddBefore(Event * ev);

    // This method inserts an Event after another, linking the former to the
    // successor of the latter
    void InsAfter(Event * ev);

    // This method returns the pointer to the successor of an Event
    Event *Next();

private:
    // Event verification time
    TimeStamp *time;

    // Event handler
    Callback callback;

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
    bool IsEmpty() const { return head == NULL; }

    // This method returns the TimeStamp in the EventQueue
    // head, if queue is not empty, and NULL otherwise
    TimeStamp *getHTS();

    Event::Callback getHCallback() const;

    // This method creates a new Event object and inserts it in the
    // EventQueue; EventQueue is sorted on ascending time order
    TimeStamp* InsertQ(TimeStamp* ts, Word delay, Event::Callback callback);

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
