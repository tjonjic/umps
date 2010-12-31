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
#include <string.h>

#include <umps/const.h>
#include "umps/blockdev_params.h"

#include "umps/error.h"


HIDDEN Word * readBIOSFile(const char * fName, Word * sizep);
HIDDEN Word * emptyFrameSignal(const char * fName, Word * sizep);


// This method creates a RamSpace object of a given size (in words) and
// fills it with core file contents if needed
RamSpace::RamSpace(Word siz, const char * fName)
{
    FILE * cFile = NULL;
    Word tag;
    unsigned int i;

    size = siz;
    memPtr = new Word [siz];

    if (fName != NULL && !SAMESTRING(fName, EMPTYSTR))
    {
        // tries to load core file from disk
        if ((cFile = fopen(fName, "r")) == NULL ||
            fread((void *) &tag, WORDLEN, 1, cFile) != 1 ||
            tag != COREFILEID)
        {
            ShowAlertQuit("Unable to load core file", fName, "cannot continue: exiting now...");
            for (i = 0; i < size; i++)
                memPtr[i] = NOP;
        } else {	
            fread((void *) memPtr, WORDLEN, size, cFile);
            if (!feof(cFile)) {
                // core too large: it did not fit in memory
                ShowAlert("Core file did not fit in memory: too large",
                          "Increase setup RAM size and reset the system", EMPTYSTR);
            }
        }
        if (cFile != NULL)
            fclose(cFile);
    }
}

// This method deletes the contents of a RamSpace object
RamSpace::~RamSpace()
{
    delete memPtr;
}


// This method returns the value of Word at ofs address
// (SystemBus must assure that ofs is in range)
Word RamSpace::MemRead(Word ofs)
{
	if (ofs < size)
		return(memPtr[ofs]);
	else
	{
		Panic("Illegal memory access in RamSpace::MemRead()");
		return(MAXWORDVAL);
	}
}


// This method allows to write data to a specified address (as word offset)
// (SystemBus must check address validity and make byte-to-word
// address conversion)
void RamSpace::MemWrite(Word ofs, Word data)
{
	if(ofs < size)
		memPtr[ofs] = data;
	else
		Panic("Illegal memory access in RamSpace::MemWrite()");
}


// This method returns RamSpace size in bytes
Word RamSpace::Size()
{
	return(size * WORDLEN);
}

/****************************************************************************/

// This method creates a BiosSpace object, filling with .rom file contents
BiosSpace::BiosSpace(const char* name)
{
    memPtr = readBIOSFile(name, &size);
}


// This method deletes the contents of a BiosSpace object
BiosSpace::~BiosSpace()
{
	delete memPtr;
}


// This method returns the value of Word at ofs address
// (SystemBus must assure that ofs is in range)
Word BiosSpace::MemRead(Word ofs)
{
	if (ofs < size)
		return(memPtr[ofs]);
	else
	{
		Panic("Illegal memory access in BiosSpace::MemRead()");
		return(MAXWORDVAL);
	}
}

// This method returns BiosSpace size in bytes
Word BiosSpace::Size()
{
	return size * WORDLEN;
}

// This function reads the ROM contents from disk and returns a pointer to the
// memory structure created and its length (via siz pointer); it recognizes
// the file using the magic file number for .rom files.
// If no correct load is possible, ROM itself is filled with NOPs

HIDDEN Word* readBIOSFile(const char * fName, Word * sizep)
{
    FILE * bFile = NULL;
    Word tag;
    Word * biosBuf;

    if (fName == NULL || SAMESTRING(fName, EMPTYSTR) || (bFile = fopen(fName, "r")) == NULL ||
        fread((void *) &tag, WORDLEN, 1, bFile) != 1 ||
        tag != BIOSFILEID || fread((void *) sizep, WORDLEN, 1, bFile) != 1)
    {
        // missing type or wrong type
        biosBuf = emptyFrameSignal(fName, sizep);
    }
    else
    {
        biosBuf = new Word[*sizep];
        if (fread((void *) biosBuf, WORDLEN, *sizep, bFile) != *sizep)
        {
            // wrong file format
            delete biosBuf;
            biosBuf = emptyFrameSignal(fName, sizep);
        }
    }
    if (bFile != NULL)
        fclose(bFile);

    return(biosBuf);
}


// This function creates a "dummy" ROM memory space filled with NOPs and
// warns user
HIDDEN Word * emptyFrameSignal(const char * fName, Word * sizep)
{
	Word * bBuf;
	unsigned int i;
	
	ShowAlertQuit("Unable to load critical ROM file:", fName, "cannot continue: exiting now...");
	bBuf = new Word[FRAMESIZE];
	for (i = 0; i < FRAMESIZE; i++)
		bBuf[i] = NOP;
		
	*sizep = FRAMESIZE;
	return(bBuf);
}
