/// Routines for managing the disk file header (in UNIX, this would be called
/// the i-node).
///
/// The file header is used to locate where on disk the file's data is
/// stored.  We implement this as a fixed size table of pointers -- each
/// entry in the table points to the disk sector containing that portion of
/// the file data (in other words, there are no indirect or doubly indirect
/// blocks). The table size is chosen so that the file header will be just
/// big enough to fit in one disk sector,
///
/// Unlike in a real system, we do not keep track of file permissions,
/// ownership, last modification date, etc., in the file header.
///
/// A file header can be initialized in two ways:
///
/// * for a new file, by modifying the in-memory data structure to point to
///   the newly allocated data blocks;
/// * for a file already on disk, by reading the file header from disk.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_header.hh"
#include "threads/system.hh"
#include "lib/utility.hh"

#include <ctype.h>
#include <stdio.h>


/// Initialize a fresh file header for a newly created file.  Allocate data
/// blocks for the file out of the map of free disk blocks.  Return false if
/// there are not enough free blocks to accomodate the new file.
///
/// * `freeMap` is the bit map of free disk sectors.
/// * `fileSize` is the bit map of free disk sectors.
#define min(a, b)  (((a) < (b)) ? (a) : (b))

bool
FileHeader::Allocate(Bitmap *freeMap, unsigned fileSize)
{
    ASSERT(freeMap != nullptr);

    if (fileSize > MAX_FILE_SIZE) {
        return false;
    }
    raw.numBytes = fileSize;
    raw.numSectors = DivRoundUp(fileSize, SECTOR_SIZE);

    unsigned numDirSect       = min(raw.numSectors, NUM_DIRECT);
    unsigned numIndSect       = raw.numSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);
    
    // alocamos los sectores directos
    for (unsigned i = 0; i < numDirSect; i++) {
        raw.dataSectors[i] = freeMap->Find();
    }

    if (numIndSect > 0) {
        // alocamos los sectores indirectos de primer nivel
        raw.simpleIndirectT = freeMap->Find();
        for (unsigned j = 0; j < numSimpleIndSect; j++)
            simpleT.dataSectors[j] = freeMap->Find(); 
        
        if (numIndSect - numSimpleIndSect > 0) {
            // alocamos los sectores indirectos de segundo nivel
            raw.doubleIndirectT = freeMap->Find();

            for (unsigned i = 0; i < numInTables - 1; i++) {
                doubleT.dataSectors[i] = freeMap->Find();
                for (unsigned j = 0; j < NUM_INDIRECT; j++)
                    simpleDoublesT[i].dataSectors[j] = freeMap->Find(); 
            }
            doubleT.dataSectors[numInTables - 1] = freeMap->Find();
            unsigned j = numDoubleIndSect % NUM_INDIRECT;
            for (unsigned i = 0; i < j; i++)
                simpleDoublesT[numInTables - 1].dataSectors[i] = freeMap->Find(); 
        }
    }
    return true;
}

/// De-allocate all the space allocated for data blocks for this file.
///
/// * `freeMap` is the bit map of free disk sectors.
void
FileHeader::Deallocate(Bitmap *freeMap)
{
    ASSERT(freeMap != nullptr);

    unsigned numDirSect       = min(raw.numSectors, NUM_DIRECT);
    unsigned numIndSect       = raw.numSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);
    
    if (numDoubleIndSect > 0) {

        // desalocamos por completo
        for (unsigned i = 0; i < numInTables - 1; i++) {
            for (unsigned j = 0; j < NUM_INDIRECT; j++) {
                ASSERT(freeMap->Test(simpleDoublesT[i].dataSectors[j]));
                freeMap->Clear(simpleDoublesT[i].dataSectors[j]);
            }
            ASSERT(freeMap->Test(doubleT.dataSectors[i]));
            freeMap->Clear(doubleT.dataSectors[i]);
        }

        // desalocamos lo necesario de la ultima tabla
        unsigned j = numDoubleIndSect % NUM_INDIRECT;
        for (unsigned i = 0; i < j; i++) {
            ASSERT(freeMap->Test(simpleDoublesT[numInTables - 1].dataSectors[i]));
            freeMap->Clear(simpleDoublesT[numInTables - 1].dataSectors[i]);
        }
        ASSERT(freeMap->Test(doubleT.dataSectors[numInTables - 1]));
        freeMap->Clear(doubleT.dataSectors[numInTables - 1]);

    }

    if (numSimpleIndSect > 0) {
        for (unsigned j = 0; j < numSimpleIndSect; j++) {
            ASSERT(freeMap->Test(simpleT.dataSectors[j]));
            freeMap->Clear(simpleT.dataSectors[j]);
        }
        
        ASSERT(freeMap->Test(raw.simpleIndirectT));
        freeMap->Clear(raw.simpleIndirectT);
    }

    // desalocamos los sectores directos
    for (unsigned i = 0; i < numDirSect; i++) {
        ASSERT(freeMap->Test(raw.dataSectors[i]));
        freeMap->Clear(raw.dataSectors[i]);
    }
}

/// Fetch contents of file header from disk.
///
/// * `sector` is the disk sector containing the file header.
void
FileHeader::FetchFrom(unsigned sector)
{
    synchDisk->ReadSector(sector, (char *) &raw);
    synchDisk->ReadSector(raw.simpleIndirectT, (char*) &simpleT);
    synchDisk->ReadSector(raw.doubleIndirectT, (char*) &doubleT);
    
    unsigned numDirSect       = min(raw.numSectors, NUM_DIRECT);
    unsigned numIndSect       = raw.numSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);

    for (int i = 0; i < numInTables; i++) {
        unsigned sec = doubleT.dataSectors[i];
        synchDisk->ReadSector(sec, (char*) &simpleDoublesT[i]);
    }
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    synchDisk->WriteSector(sector, (char *) &raw);
    synchDisk->WriteSector(raw.simpleIndirectT, (char*) &simpleT);
    synchDisk->WriteSector(raw.doubleIndirectT, (char*) &doubleT);
    
    unsigned numDirSect       = min(raw.numSectors, NUM_DIRECT);
    unsigned numIndSect       = raw.numSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);

    for (int i = 0; i < numInTables; i++) {
        unsigned sec = doubleT.dataSectors[i];
        synchDisk->WriteSector(sec, (char*) &simpleDoublesT[i]);
    }
}

/// Return which disk sector is storing a particular byte within the file.
/// This is essentially a translation from a virtual address (the offset in
/// the file) to a physical address (the sector where the data at the offset
/// is stored).
///
/// * `offset` is the location within the file of the byte in question.
unsigned
FileHeader::ByteToSector(unsigned offset)
{
    int numSector = offset / SECTOR_SIZE;
    if (numSector < NUM_DIRECT) // esta en los bloques directos
        return raw.dataSectors[numSector];

    else if (numSector < NUM_DIRECT + NUM_INDIRECT) 
        return simpleT.dataSectors[numSector - NUM_DIRECT]; // esta en el primer nivel de indirecc
    else { // está en el segundo nivel de indirecc
        int numTable = (numSector - NUM_DIRECT - NUM_INDIRECT) / NUM_INDIRECT;
        int sector =  (numSector - NUM_DIRECT - NUM_INDIRECT) % NUM_INDIRECT;
        return simpleDoublesT[numTable].dataSectors[sector];
    }
}

/// Return the number of bytes in the file.
unsigned
FileHeader::FileLength() const
{
    return raw.numBytes;
}

/// Print the contents of the file header, and the contents of all the data
/// blocks pointed to by the file header.
void
FileHeader::Print(const char *title)
{
    char *data = new char [SECTOR_SIZE];

    if (title == nullptr) {
        printf("File header:\n");
    } else {
        printf("%s file header:\n", title);
    }

    printf("    size: %u bytes\n"
           "    direct block indexes: ",
           raw.numBytes);

    unsigned numDirSect = min(raw.numSectors, NUM_DIRECT);
    unsigned numIndSect       = raw.numSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);

    for (unsigned i = 0; i < numDirSect; i++) {
        printf("%u ", raw.dataSectors[i]);
    }
    printf("\n");

    if (raw.numSectors >= NUM_DIRECT) {
        printf("    first level indirect block indexes: ");

        for (unsigned i = 0; i < numSimpleIndSect; i++)
            printf("%u ", simpleT.dataSectors[i]);
        printf("\n");

        if (raw.numSectors >= NUM_DIRECT + NUM_INDIRECT) {
            printf("    second level indirect block indexes: ");

            for (unsigned i = 0; i < numInTables - 1; i++) {
                printf("\n     table[%u]: ", i);
                for (unsigned j = 0; j < NUM_INDIRECT; j++)
                    printf("%u ", simpleDoublesT[i].dataSectors[j]);
            }
            unsigned j = numDoubleIndSect % NUM_INDIRECT;
            printf("\n     table[%u]: ", numInTables - 1);
            for (unsigned i = 0; i < j; i++)
                printf("%u ", simpleDoublesT[numInTables - 1].dataSectors[i]);

            printf("\n");
        }
    }

    // contenido de bloques directos
    unsigned k = 0;
    for (unsigned i = 0; i < numDirSect; i++) {
        printf("    contents of block %u:\n", raw.dataSectors[i]);
        synchDisk->ReadSector(raw.dataSectors[i], data);
        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if (isprint(data[j])) {
                printf("%c", data[j]);
            } else {
                printf("\\%X", (unsigned char) data[j]);
            }
        }
        printf("\n");
    }
    // contenido de primer nivel de indireccion
    for (unsigned i = 0; i < numSimpleIndSect; i++) {
        printf("    contents of block %u:\n", simpleT.dataSectors[i]);
        synchDisk->ReadSector(simpleT.dataSectors[i], data);
        for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
            if (isprint(data[j])) {
                printf("%c", data[j]);
            } else {
                printf("\\%X", (unsigned char) data[j]);
            }
        }
        printf("\n");
    }

    // contenido de segundo nivel de indireccion
    for (unsigned x = 0; x < numInTables - 1; x++) {
        for (unsigned i = 0; i < NUM_INDIRECT; i++) {
            printf("    contents of block %u:\n", simpleDoublesT[x].dataSectors[i]);
            synchDisk->ReadSector(simpleDoublesT[x].dataSectors[i], data);
            for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
                if (isprint(data[j])) {
                    printf("%c", data[j]);
                } else {
                    printf("\\%X", (unsigned char) data[j]);
                }
            }
            printf("\n");
        }
    }
    unsigned ult = numDoubleIndSect % NUM_INDIRECT;
    for (unsigned i = 0; i < ult; i++) {
            printf("    contents of block %u:\n", simpleDoublesT[numInTables - 1].dataSectors[i]);
            synchDisk->ReadSector(simpleDoublesT[numInTables - 1].dataSectors[i], data);
            for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
                if (isprint(data[j])) {
                    printf("%c", data[j]);
                } else {
                    printf("\\%X", (unsigned char) data[j]);
                }
            }
            printf("\n");
        }

    delete [] data;
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}
