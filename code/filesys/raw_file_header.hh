/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_FILESYS_RAWFILEHEADER__HH
#define NACHOS_FILESYS_RAWFILEHEADER__HH


#include "machine/disk.hh"

static const unsigned NUM_DIRECT
   = (SECTOR_SIZE - 4 * sizeof (int) )/ sizeof (int); // 28 
static const unsigned NUM_INDIRECT
   = SECTOR_SIZE / sizeof (int); // 32
const unsigned MAX_FILE_SIZE = (NUM_INDIRECT * NUM_INDIRECT + NUM_INDIRECT + NUM_DIRECT) * SECTOR_SIZE;

struct RawFileHeader {
    unsigned numBytes;  ///< Number of bytes in the file.
    unsigned numSectors;  ///< Number of data sectors in the file.
    unsigned dataSectors[NUM_DIRECT];  ///< Disk sector numbers for each data
                                       ///< block in the file.
    unsigned simpleIndirectT;
    unsigned doubleIndirectT;
};  

struct IndirectT
{
  unsigned dataSectors[NUM_INDIRECT];
};

#endif
