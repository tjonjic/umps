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

#include <QMessageBox>

#include "qmps/application.h"

/*
 * FIXME: These are here only because of lazyness. Error handling
 * (startap, general, ...) needs a lot of improvement. For now, we do
 * what the old UI did, more or less.
 */

void Panic(const char* message)
{
    QMessageBox::critical(0, "PANIC", QString("PANIC: %1").arg(message));
    Appl()->quit();
}

void ShowAlert(const char* s1, const char* s2, const char* s3)
{
    QMessageBox::critical(0, "Fatal Error",
                          QString("Fatal Error: %1 %2 %3").arg(s1).arg(s2).arg(s3));
    Appl()->quit();
}

void ShowAlertQuit(const char* s1, const char* s2, const char* s3)
{
    QMessageBox::critical(0, "Fatal Error",
                          QString("Fatal Error: %1 %2 %3").arg(s1).arg(s2).arg(s3));
    Appl()->quit();
}
