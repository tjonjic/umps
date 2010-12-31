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
 * This is a stand-alone program which converts an ELF object or executable
 * file into one of MPS project file formats: .core, .rom or .aout.
 * See external documentation for format details.
 *
 ****************************************************************************/

#include <fcntl.h>
#include <stdio.h>

#include <libelf.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <umps/const.h>
#include "umps/types.h"
#include "umps/blockdev_params.h"
#include "umps/aout.h"

// some Linux platforms need it
#ifndef EM_MIPS
#define EM_MIPS         8
#endif

// ELF recognition tag for MIPS .reginfo section
#define SHT_REGINFO     0x70000006

// start symbol for linker
#define SYMSTART        "__start"

// .aout header field names
HIDDEN const char *const aoutName[] = {
    "",
    "Program (virtual) starting address",
    ".text (virtual) start address",
    ".text memory size",
    ".text file start offset",
    ".text file size",
    ".data (virtual) start address",
    ".data memory size",
    ".data file start offset",
    ".data file size"
};

// ELF symbol types: memory objects or functions
HIDDEN const char *const symType[] = {
    "",
    "OBJ",
    "FUN"
};

// ELF symbol binding (local/global/weak)
HIDDEN const char *const symBind[] = {
    "LOC",
    "GLB",
    "WK "
};

//
// Program functions
//

HIDDEN void showHelp(const char *prgName);
HIDDEN int elfConvert(const char *prgName, const char *fileName,
                      const char *ext, Word tag, bool verb, bool map);
HIDDEN int mkBIOS(const char *prg, Elf * elf, const char *out, bool verb);
HIDDEN int mkaOut(const char *prg, Elf * elf, Word prgStart, Word phEnt,
                  Word ndx, const char *out, bool verb, Word * coreHdr,
                  Word blkSz);
HIDDEN Word *mkCore(const char *prg, Elf * elf, Word phEnt, bool verb,
                    Word * blkSz);
HIDDEN int mkSTab(const char *prg, Elf * elf, const char *out, bool verb);
HIDDEN const char *locateStrTab(const char *prg, Elf * elf,
                                unsigned int ndx);
HIDDEN void elfFail(const char *prg);


/****************************************************************************/
/* Definitions to be exported.                                              */
/****************************************************************************/

// This function scans the line arguments; if no error is found, the
// required conversion is performed, or a warning/help message is printed.
// See showHelp() for argument format; "verbose" action prints more info on
// conversion operations, while map file requirement produces a text file
// containing a human-readable version of the symbol table contained in the
// ELF file.
// Returns an EXIT_SUCCESS/FAILURE code according to conversion success

int main(int argc, char *argv[])
{
    bool verbose = false;
    bool mapfile = false;
    Word tag = 0UL;
    int i;
    int ret = EXIT_SUCCESS;
    const char *inpFile = NULL;
    const char *extFile = NULL;

    if (argc == 1)
        // no arguments found
        showHelp(argv[0]);
    else {
        i = 1;
        // scan arguments and set appropriate variables
        while (i < argc && !tag) {
            if (SAMESTRING("-v", argv[i])) {
                verbose = true;
                i++;
            } else if (SAMESTRING("-m", argv[i])) {
                mapfile = true;
                i++;
            } else if (SAMESTRING("-k", argv[i])) {
                i++;
                tag = COREFILEID;
                if (i < argc) {
                    inpFile = argv[i];
                    extFile = COREFILETYPE;
                    mapfile = true;
                }
            } else if (SAMESTRING("-b", argv[i])) {
                i++;
                tag = BIOSFILEID;
                extFile = BIOSFILETYPE;
                if (i < argc)
                    inpFile = argv[i];
            } else if (SAMESTRING("-a", argv[i])) {
                i++;
                tag = AOUTFILEID;
                extFile = AOUTFILETYPE;
                if (i < argc)
                    inpFile = argv[i];
            } else
                // unrecognized option
                tag = MAXWORDVAL;
        }
        if (inpFile != NULL)
            // conversion is attempted, depending on user request: magic file 
            // number identifies type
            ret =
                elfConvert(argv[0], inpFile, extFile, tag, verbose,
                           mapfile);
        else {
            fprintf(stderr, "%s : Wrong/unknown argument(s)\n", argv[0]);
            showHelp(argv[0]);
            ret = EXIT_FAILURE;
        }
    }
    return (ret);
}


/****************************************************************************/
/* Definitions strictly local to the module.                                */
/****************************************************************************/

// This function prints a warning/help message on standard error
HIDDEN void showHelp(const char *prgName)
{
    fprintf(stderr, "%s syntax : %s [-v] [-m] {-k | -b | -a} <file>\n\n",
            prgName, prgName);
    fprintf(stderr,
            "where:\n\n-v\tverbose operation\n-m\tmake map file <file>%s\n\n",
            STABFILETYPE);
    fprintf(stderr, "-k\tmake kernel core file <file>%s + map file\n",
            COREFILETYPE);
    fprintf(stderr,
            "-b\tmake BIOS file <file>%s\n-a\tmake a.out file <file>%s\n\n",
            BIOSFILETYPE, AOUTFILETYPE);
}


// This function detects which kind of conversion is required on fileName (to
// ..core, rom or .aout file format) by magic file number (tag) and tries to
// perform it, putting the output into fileName + ext.  
// It builds symbol table file with proper name+extension as map value
// suggests, and produces verbose output if needed.
// It verifies ELF library capabilities and ELF source file validity before
// attempting conversion, and returns an EXIT_SUCCESS/FAILURE code according
// to conversion success.
HIDDEN int elfConvert(const char *prgName, const char *fileName,
                      const char *ext, Word tag, bool verb, bool map)
{
    Elf32_Ehdr *elfFileHdr;
    Elf *elfFile;
    Elf32_Half expType;

    int ret = EXIT_SUCCESS;
    int fd;
    Word *corep = NULL;
    Word blockSize = 0;

    // builds target file name with extension       
    char *outFileName = new char[strlen(fileName) + strlen(ext) + 1];
    sprintf(outFileName, "%s%s", fileName, ext);

    // set ELF expected file type vs. requested conversion: .rom files
    // are built from ELF object files, other formats need ELF executables
    if (tag == BIOSFILEID)
        expType = ET_REL;
    else
        expType = ET_EXEC;

    // open input file
    if ((fd = open(fileName, O_RDONLY)) == EOF) {
        fprintf(stderr, "%s : Error opening file %s : %s\n", prgName,
                fileName, strerror(errno));
        ret = EXIT_FAILURE;
    } else {
        // ELF library access
        if (elf_version(EV_CURRENT) == EV_NONE) {
            fprintf(stderr, "%s : ELF library out of date\n", prgName);
            ret = EXIT_FAILURE;
        } else {
            // ELF file manipulation
            if ((elfFile = elf_begin(fd, ELF_C_READ, NULL)) == NULL)
                elfFail(prgName);

            // check for file validity              
            if ((elfFileHdr = elf32_getehdr(elfFile)) == NULL)
                elfFail(prgName);

            // Checking for ELF correct machine type: it must be MIPS one               
            if (elfFileHdr->e_type != expType
                || elfFileHdr->e_machine != EM_MIPS
                || elfFileHdr->e_version != EV_CURRENT) {
                fprintf(stderr,
                        "%s : Error opening file %s : invalid ELF file type\n",
                        prgName, fileName);
                ret = EXIT_FAILURE;
            } else {
                // conversion may happen, depending on request
                switch (tag) {
                case BIOSFILEID:
                    ret = mkBIOS(prgName, elfFile, outFileName, verb);
                    break;

                case AOUTFILEID:
                    ret =
                        mkaOut(prgName, elfFile, elfFileHdr->e_entry,
                               elfFileHdr->e_phnum, elfFileHdr->e_shstrndx,
                               outFileName, verb, NULL,
                               BLOCKSIZE * WORDLEN);
                    break;

                case COREFILEID:
                    if ((corep =
                         mkCore(prgName, elfFile, elfFileHdr->e_phnum,
                                verb, &blockSize)) == NULL) {
                        fprintf(stderr,
                                "%s : Cannot create kernel TLB mapping page : kernel too large\n",
                                prgName);
                        ret = EXIT_FAILURE;
                    } else
                        ret =
                            mkaOut(prgName, elfFile, elfFileHdr->e_entry,
                                   elfFileHdr->e_phnum,
                                   elfFileHdr->e_shstrndx, outFileName,
                                   verb, corep, blockSize);
                    break;

                default:
                    fprintf(stderr, "%s : Unknown conversion\n", prgName);
                    break;
                }
                if (map && ret != EXIT_FAILURE) {
                    // print symbol table map file
                    delete outFileName;
                    outFileName =
                        new char[strlen(fileName) + strlen(STABFILETYPE) +
                                 1];
                    sprintf(outFileName, "%s%s", fileName, STABFILETYPE);
                    ret = mkSTab(prgName, elfFile, outFileName, verb);
                }
            }
            elf_end(elfFile);
        }
        close(fd);
    }
    delete outFileName;
    return (ret);
}

// This function tries to build a .rom file called "out" from the ELF
// structure passed as argument. The ELF file should be of object type, and
// does not contain relocations to be successfully converted; .text section
// is extracted and dumped into target file, with a small header containing
// .rom file magic number and size in words.
// Function returns indication of conversion success or failure
HIDDEN int mkBIOS(const char *prg, Elf * elf, const char *out, bool verb)
{
    Elf_Scn *scn;
    Elf32_Shdr *shdr;
    Elf_Data *data;
    FILE *outFile = NULL;
    bool found = false;
    int ret = EXIT_SUCCESS;
    unsigned int relocs = 0;
    Word tag = BIOSFILEID;
    Word size = 0;

    if (verb)
        printf("Scanning ELF file sections...\n");
    scn = NULL;
    while (!found && (scn = elf_nextscn(elf, scn)) != NULL) {
        if ((shdr = elf32_getshdr(scn)) == NULL)
            elfFail(prg);
        // else all is ok
        if (shdr->sh_type == SHT_PROGBITS
            && ((shdr->sh_flags) & (SHF_ALLOC + SHF_EXECINSTR))) {
            // found wanted section
            found = true;
            if ((outFile = fopen(out, "w")) == NULL ||
                fwrite((void *) &tag, WORDLEN, 1, outFile) != 1 ||
                fwrite((void *) &size, WORDLEN, 1, outFile) != 1) {
                fprintf(stderr, "%s : Cannot open output file %s : %s\n",
                        prg, out, strerror(errno));
                ret = EXIT_FAILURE;
            } else {
                // scanning .text section 
                data = NULL;
                while ((data = elf_getdata(scn, data)) != NULL
                       && ret != EXIT_FAILURE) {
                    if ((data->d_type) == ELF_T_BYTE && (data->d_size) > 0) {
                        size += (data->d_size) / WORDLEN;
                        if (verb)
                            printf("Writing %u words of BIOS code to %s\n",
                                   (data->d_size) / WORDLEN, out);
                        if (fwrite
                            ((void *) (data->d_buf), WORDLEN,
                             data->d_size / WORDLEN,
                             outFile) != (data->d_size / WORDLEN)) {
                            fprintf(stderr, "%s : Error writing %s : %s\n",
                                    prg, out, strerror(errno));
                            ret = EXIT_FAILURE;
                        }
                    }
                }
                if (ret != EXIT_FAILURE)
                    // no errors writing BIOS; rewrite size in file header
                    if (fseek(outFile, WORDLEN, SEEK_SET) == EOF ||
                        fwrite((void *) &size, WORDLEN, 1, outFile) != 1)
                        ret = EXIT_FAILURE;

                fclose(outFile);

                // searches for REL/RELA section and tests for relocations 
                scn = NULL;
                while ((scn = elf_nextscn(elf, scn)) != NULL) {
                    if ((shdr = elf32_getshdr(scn)) == NULL)
                        elfFail(prg);
                    // else all is ok

                    if (shdr->sh_type == SHT_REL
                        || shdr->sh_type == SHT_RELA) {
                        // counts relocations
                        data = NULL;
                        while ((data = elf_getdata(scn, data)) != NULL)
                            if (data->d_type == ELF_T_REL)
                                relocs +=
                                    (data->d_size / sizeof(Elf32_Rel));
                            else if (data->d_type == ELF_T_RELA)
                                relocs +=
                                    (data->d_size / sizeof(Elf32_Rela));
                    }
                }
                if (relocs > 0)
                    fprintf(stderr,
                            "%s : Warning : BIOS code may contain %u unresolved relocations\n",
                            prg, relocs);
            }
        }
    }
    if (!found) {
        fprintf(stderr, "%s : BIOS code search failed\n", prg);
        ret = EXIT_FAILURE;
    }
    return (ret);
}


// This function tries to build a .aout or .core file called "out" from the
// ELF structure passed as argument.  
// The ELF file should be of executable type; it will contain phEnt program 
// sections, and ndx will tell which section contains the string table (ELF
// conventions). 
// The ELF file sections will be scanned for .text and .data sections; they
// will be dumped to the resulting file, each padded to a blkSz bytes
// multiple as blocking factor: it will be 512 bytes for regular .aout
// files, and 4KB (frame size) for .core files, to make its fitting in
// memory easier. The .core file will contain a bigger header, with BIOS
// reserved space and .bss section too.
// .aout header fits in the first few words of .text area, and contains
// .aout magic number; .core file will contain an .aout header, but also
// will have a .core magic number for simulator recognition.
// To build file image, temporary buffers for sections are allocated in memory.
// Function returns indication of conversion success or failure
HIDDEN int mkaOut(const char *prg, Elf * elf, Word prgStart, Word phEnt,
                  Word ndx, const char *out, bool verb, Word * coreHdr,
                  Word blkSz)
{
    Elf32_Phdr *phdr;
    Elf32_Shdr *shdr;
    Elf_Scn *scn;
    Elf_Data *data;
    Word aoutTab[AOUTENTNUM];
    unsigned char *textA = NULL, *dataA = NULL, *buf;
    Word *wBuf;
    Word dataSz, pos, bufmax, endpos;

    bool foundData = false;
    bool foundText = false;
    int ret = EXIT_SUCCESS;
    Word tag = AOUTFILEID;
    unsigned int i, phNum;
    FILE *outFile = NULL;

    aoutTab[AOUTTAG] = tag;
    aoutTab[PROGSTART] = prgStart;

    // scan ELF program sections
    for (phNum = 0; phNum < phEnt; phNum++) {
        phdr = elf32_getphdr(elf) + phNum;

        if (phdr == NULL)
            elfFail(prg);
        // else all is ok

        // scan for .text & .data sizes (ignores .reginfo for now)
        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_flags == (PF_R | PF_W) && !foundData) {
                // this entry is for .data area
                foundData = true;
                aoutTab[DATAVSIZE] = phdr->p_memsz;
                aoutTab[DATAVSTART] = phdr->p_vaddr;
                if (coreHdr == NULL)
                    // regular .aout file: will not include bss
                    dataSz = phdr->p_filesz;
                else
                    // core file: .bss should be included into file 
                    dataSz = phdr->p_memsz;
                aoutTab[DATAFSIZE] = (dataSz / blkSz) * blkSz;

                if (aoutTab[DATAFSIZE] < dataSz)
                    aoutTab[DATAFSIZE] += blkSz;
                dataA = new unsigned char[aoutTab[DATAFSIZE]];
                // clear area
                for (i = 0; i < aoutTab[DATAFSIZE]; i++)
                    dataA[i] = 0;
            } else if (phdr->p_flags == (PF_R | PF_X) && !foundText) {
                // this entry is for .text area
                foundText = true;
                // here memsz and filesz should be the same
                aoutTab[TEXTVSIZE] = phdr->p_memsz;
                aoutTab[TEXTVSTART] = phdr->p_vaddr;
                dataSz = phdr->p_memsz;
                aoutTab[TEXTFSIZE] = (dataSz / blkSz) * blkSz;
                if (aoutTab[TEXTFSIZE] < dataSz)
                    aoutTab[TEXTFSIZE] += blkSz;
                textA = new unsigned char[aoutTab[TEXTFSIZE]];
                for (i = 0; i < aoutTab[TEXTFSIZE]; i++)
                    textA[i] = 0;
            } else {
                fprintf(stderr,
                        "%s : Error in ELF file : multiple/unknown program header entries\n",
                        prg);
                ret = EXIT_FAILURE;
            }
        }
    }
    if (ret != EXIT_FAILURE && foundText && foundData) {
        // program headers scanned successfully and memory areas prepared
        aoutTab[TEXTFOFFS] = 0UL;
        aoutTab[DATAFOFFS] = aoutTab[TEXTFSIZE];

        // fill areas with sections
        scn = NULL;
        while ((scn = elf_nextscn(elf, scn)) != NULL
               && ret != EXIT_FAILURE) {
            if ((shdr = elf32_getshdr(scn)) == NULL)
                elfFail(prg);
            // else all is ok
            if ((shdr->sh_type == SHT_PROGBITS
                 || shdr->sh_type == SHT_REGINFO
                 || shdr->sh_type == SHT_NOBITS) && (shdr->sh_addr > 0)) {
                // found useful section 
                if (verb)
                    if (shdr->sh_type != SHT_NOBITS || coreHdr != NULL)
                        // .bss is added only if .core file is built 
                        printf
                            ("Adding section %-10.10s : start 0x%.8lX : size 0x%.8lX  to a.out image\n",
                             elf_strptr(elf, ndx, shdr->sh_name),
                             shdr->sh_addr, shdr->sh_size);
                    else
                        printf
                            ("Found  section %-10.10s : start 0x%.8lX : size 0x%.8lX\n",
                             elf_strptr(elf, ndx, shdr->sh_name),
                             shdr->sh_addr, shdr->sh_size);

                if (shdr->sh_type != SHT_NOBITS) {
                    // not a .bss section: .data sub-section proper
                    if (shdr->sh_addr >= aoutTab[DATAVSTART]) {
                        buf = dataA;
                        bufmax = aoutTab[DATAFSIZE];
                        pos = shdr->sh_addr - aoutTab[DATAVSTART];
                        endpos = pos + shdr->sh_size;
                    } else {
                        // it's .text area
                        buf = textA;
                        bufmax = aoutTab[TEXTFSIZE];
                        pos = shdr->sh_addr - aoutTab[TEXTVSTART];
                        endpos = pos + shdr->sh_size;
                    }

                    // copy section parts to memory area
                    data = NULL;
                    while ((data = elf_getdata(scn, data)) != NULL) {
                        for (i = 0;
                             i < data->d_size && ret != EXIT_FAILURE; i++)
                            if ((i + pos) < bufmax)
                                buf[i + pos] =
                                    *((unsigned char *) (data->d_buf) + i);
                            else {
                                fprintf(stderr,
                                        "%s : Error in ELF file : two or more sections overlap\n",
                                        prg);
                                ret = EXIT_FAILURE;
                            }
                        pos += data->d_size;
                    }

                    if (pos != endpos) {
                        fprintf(stderr,
                                "%s : Error in ELF file : declared section size differs from real size : %.8lX: %.8lX \n",
                                prg, pos, endpos);
                        ret = EXIT_FAILURE;
                    }
                }
            }
        }

        if (ret != EXIT_FAILURE) {
            // no errors: may fold a.out header into text area
            wBuf = (Word *) textA;
            for (i = 0; i < AOUTENTNUM && ret != EXIT_FAILURE; i++)
                if (wBuf[i] == 0UL)
                    wBuf[i] = aoutTab[i];
                else {
                    fprintf(stderr,
                            "%s : Error in ELF file : no space for a.out header\n",
                            prg);
                    ret = EXIT_FAILURE;
                }
            if (ret != EXIT_FAILURE) {
                // may write a.out output file
                if ((outFile = fopen(out, "w")) == NULL ||
                    (coreHdr != NULL
                     && fwrite((void *) coreHdr, WORDLEN, COREHDRSIZE,
                               outFile) != COREHDRSIZE)
                    || fwrite((void *) textA, sizeof(unsigned char),
                              aoutTab[TEXTFSIZE],
                              outFile) != aoutTab[TEXTFSIZE]
                    || fwrite((void *) dataA, sizeof(unsigned char),
                              aoutTab[DATAFSIZE],
                              outFile) != aoutTab[DATAFSIZE]
                    || fclose(outFile) == EOF) {
                    fprintf(stderr, "%s : Error writing a.out file %s\n",
                            prg, out);
                    ret = EXIT_FAILURE;
                } else
                    // all OK : print verbose mode info
                    if (verb) {
                        printf("\na.out file %s created: \n\n", out);
                        for (i = 1; i < AOUTENTNUM; i++)
                            printf("%-35.35s: 0x%.8lX\n", aoutName[i],
                                   aoutTab[i]);
                    }
            }
        }
    } else if (ret != EXIT_FAILURE) {
        // a section is absent
        fprintf(stderr,
                "%s : Error in ELF file :  program header text/data section missing\n",
                prg);
        ret = EXIT_FAILURE;
    }
    return (ret);
}



// This function builds a .core file header: it contains an empty frame for
// BIOS use. The program header for the ELF structure must be scanned to 
// retrieve program sections sizes; .bss should be added too. 
// The function returns a pointer to the core header buffer (to be written
// on file by mkaOut()) and the block size for the resulting
// .aout file (thru blkSz pointer)
HIDDEN Word *mkCore(const char *prg, Elf * elf, Word phEnt, bool verb,
                    Word * blkSz)
{
    Elf32_Phdr *phdr;
    Word *wBuf;

    bool foundData = false;
    bool foundText = false;
    Word tag = COREFILEID;
    int ret = EXIT_SUCCESS;
    SWord dataSize = 0, textSize = 0;
    unsigned int phNum;

    *blkSz = FRAMESIZE * WORDLEN;
    wBuf = new Word[COREHDRSIZE];

    // scan for program header sections     
    for (phNum = 0; phNum < phEnt; phNum++) {
        phdr = elf32_getphdr(elf) + phNum;

        if (phdr == NULL)
            elfFail(prg);
        // else all is ok

        // scan for .text & .data sizes
        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_flags == (PF_R | PF_W) && !foundData) {
                // this entry is for .data area
                foundData = true;
                dataSize = phdr->p_memsz;
            } else if (phdr->p_flags == (PF_R | PF_X) && !foundText) {
                // this entry is for .text area
                foundText = true;
                textSize = phdr->p_memsz;
            } else {
                fprintf(stderr,
                        "%s : Error in ELF file : multiple/unknown program header entries\n",
                        prg);
                ret = EXIT_FAILURE;
            }
        }
    }
    if (ret != EXIT_FAILURE && foundText && foundData) {
        wBuf[0] = tag;
        if (verb)
            printf("Core header size: %u frames (reserved for BIOS use)\n",
                   BIOSPAGES);
    }
    if (ret == EXIT_SUCCESS)
        return (wBuf);
    else
        return (NULL);
}


// This function builds the symbol table file named "out" matching the ELF
// structure passed as argument: each symbol table entry contains name, type
// of the object (FUN/OBJ), binding, virtual starting address and size in
// bytes
HIDDEN int mkSTab(const char *prg, Elf * elf, const char *out, bool verb)
{
    Elf_Scn *scn;
    Elf32_Shdr *shdr;
    Elf_Data *data;
    Elf32_Sym *symb;
    FILE *outFile = NULL;

    int ret = EXIT_SUCCESS;
    Word tag = STABFILEID;
    unsigned int fNum = 0, oNum = 0;

    unsigned char stype, sbind;
    unsigned int pos;


    const char *strTabp;

    // creates output file
    if ((outFile = fopen(out, "w")) == NULL ||
        fwrite((void *) &tag, WORDLEN, 1, outFile) != 1) {
        fprintf(stderr, "%s : Cannot open output file %s : %s\n", prg, out,
                strerror(errno));
        ret = EXIT_FAILURE;
    } else {
        // add function and object number fields (and a newline) to header
        fprintf(outFile, "%.8X %.8X\n", fNum, oNum);

        if (verb)
            printf("\nSymbol table file %s created:\n\n", out);

        // scan for symbol table section
        scn = NULL;
        while ((scn = elf_nextscn(elf, scn)) != NULL && ret != EXIT_FAILURE) {
            if ((shdr = elf32_getshdr(scn)) == NULL)
                elfFail(prg);
            // else all is ok
            if (shdr->sh_type == SHT_SYMTAB || shdr->sh_type == SHT_DYNSYM) {
                if ((strTabp = locateStrTab(prg, elf, shdr->sh_link)) == NULL) {
                    fprintf(stderr,
                            "%s : Cannot locate ELF string table section\n",
                            prg);
                    ret = EXIT_FAILURE;
                } else {
                    // string table located
                    data = NULL;
                    while ((data = elf_getdata(scn, data)) != NULL)
                        // scans it and locates symbols
                        if (data->d_type == ELF_T_SYM && data->d_size > 0) {
                            for (pos = 0; pos < data->d_size; pos += sizeof(Elf32_Sym)) {
                                // each symbol is decoded 
                                symb = (Elf32_Sym *) ((char *) (data->d_buf) + pos);
                                stype = ELF32_ST_TYPE(symb->st_info);
                                sbind = ELF32_ST_BIND(symb->st_info);
                                // checks if symbol is valid
                                if (symb->st_name > 0 &&
                                    (stype == STT_FUNC || stype == STT_OBJECT) &&
                                    (*(strTabp + symb->st_name) != '_' || SAMESTRING((strTabp + symb->st_name), SYMSTART)))
                                {
                                    // symbol is valid: it is written in the file
                                    fprintf(outFile,
                                            "%-16.16s :%s:0x%.8lX:0x%.8lX:%s\n",
                                            strTabp + symb->st_name,
                                            symType[stype], symb->st_value,
                                            symb->st_size, symBind[sbind]);
                                    if (verb)
                                        printf
                                            ("%-10.10s : %s : 0x%.8lX : 0x%.8lX : %s\n",
                                             strTabp + symb->st_name,
                                             symType[stype],
                                             symb->st_value, symb->st_size,
                                             symBind[sbind]);

                                    // counts functions and objects
                                    if (stype == STT_FUNC)
                                        fNum++;
                                    else
                                        oNum++;
                                }
                            }
                        }
                }
            }
        }
        if (ret != EXIT_FAILURE) {
            // backpatches function and object number into file
            fseek(outFile, WORDLEN, SEEK_SET);
            fprintf(outFile, "%.8X %.8X", fNum, oNum);
        }
        fclose(outFile);
    }
    return (ret);
}

// This function locates the string table inside an ELF structure and returns 
// its starting point
const char *locateStrTab(const char *prg, Elf * elf, unsigned int ndx)
{
    Elf_Scn *scn;
    Elf32_Shdr *shdr;
    Elf_Data *data;
    const char *strp = NULL;

    if ((scn = elf_getscn(elf, ndx)) != NULL) {
        if ((shdr = elf32_getshdr(scn)) == NULL)
            elfFail(prg);
        // else all is ok

        if (shdr->sh_type == SHT_STRTAB) {
            data = NULL;
            if ((data = elf_getdata(scn, data)) != NULL
                && data->d_type == ELF_T_BYTE && data->d_size > 0)
                // string table found   
                strp = (char *) data->d_buf;

        }
    }
    return (strp);
}


// This function forces termination upon error accessing ELF structures thru
// manipulation library 
void elfFail(const char *prg)
{
    fprintf(stderr, "%s : %s\n", prg, elf_errmsg(elf_errno()));
    exit(EXIT_FAILURE);
}
