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

/****************************************************************************
 *
 * This module implements the classes which represents memory spaces:
 * RamSpace (for RAM) and BiosSpace (for ROMs), completely under control of 
 * SystemBus.
 * 
 ****************************************************************************/

#include "umps/memspace.h"

#include <stdlib.h>
#include <stdio.h>

#include <boost/format.hpp>

#include "umps/arch.h"
#include "umps/const.h"
#include "umps/blockdev_params.h"
#include "umps/error.h"

// This method creates a RamSpace object of a given size (in words) and
// fills it with core file contents if needed
RamSpace::RamSpace(Word size_, const char* fName)
    : ram(new Word[size_]),
      size(size_)
{
    if (fName != NULL && *fName) {
        FILE* cFile;
        if ((cFile = fopen(fName, "r")) == NULL)
            throw FileError(fName);

        // Check validity
        Word tag;
        if (fread((void *) &tag, WORDLEN, 1, cFile) != 1 ||
            tag != COREFILEID)
        {
            fclose(cFile);
            throw InvalidCoreFileError(fName, "Invalid core file");
        }

        fread((void *) ram.get(), WORDLEN, size, cFile);
        if (!feof(cFile)) {
            fclose(cFile);
            throw CoreFileOverflow();
        }

        fclose(cFile);
    }
}

bool RamSpace::CompareAndSet(Word index, Word oldval, Word newval)
{
    if (ram[index] == oldval) {
        ram[index] = newval;
        return true;
    } else {
        return false;
    }
}


/****************************************************************************/


// This method creates a BiosSpace object, filling with .rom file contents
BiosSpace::BiosSpace(const char* fileName)
{
    assert(fileName != NULL && *fileName);

    FILE* file;

    if ((file = fopen(fileName, "r")) == NULL)
        throw FileError(fileName);

    Word tag;
    if ((fread((void *) &tag, WS, 1, file) != 1) ||
        (tag != BIOSFILEID) ||
        (fread((void *) &size, WS, 1, file) != 1))
    {
        fclose(file);
        throw InvalidFileFormatError(fileName, "ROM file expected");
    }

    memPtr.reset(new Word[size]);
    if (fread((void*) memPtr.get(), WS, size, file) != size) {
        fclose(file);
        throw InvalidFileFormatError(fileName, "Wrong ROM file size");
    }

    fclose(file);
}

// This method returns the value of Word at ofs address
// (SystemBus must assure that ofs is in range)
Word BiosSpace::MemRead(Word ofs)
{
    assert(ofs < size);
    return memPtr[ofs];
}

// This method returns BiosSpace size in bytes
Word BiosSpace::Size()
{
    return size * WORDLEN;
}
