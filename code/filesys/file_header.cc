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
    unsigned numDataSectors = DivRoundUp(fileSize, SECTOR_SIZE); // cantidad de sectores que guardaran datos
    unsigned numOtherSectors = 0;
    unsigned numDirSect       = min(numDataSectors, NUM_DIRECT);
    unsigned numIndSect       = numDataSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);

    // sector para la tabla de primera indireccion
    if (numSimpleIndSect > 0) {numOtherSectors++;} 
    // sectores para la tabla de segunda indireccion y las tablas dentro de ella
    if (numDoubleIndSect > 0) {numOtherSectors += numInTables + 1;}

    if (freeMap->CountClear() < numDataSectors + numOtherSectors) {
        return false;  // Not enough space.
    }
    raw.numSectors = numDataSectors;

    // alocamos los sectores directos
    for (unsigned i = 0; i < numDirSect; i++) {
        raw.dataSectors[i] = freeMap->Find();
    }

    if (numIndSect > 0) {
        IndirectHeader *simpleIH = new IndirectHeader;
        // alocamos los sectores indirectos de primer nivel
        raw.simpleIndirectH = freeMap->Find();
        for (unsigned j = 0; j < numSimpleIndSect; j++)
            simpleIH->dataSectors[j] = freeMap->Find(); 
        
        ((FileHeader*) simpleIH)->WriteBack(raw.simpleIndirectH);
        delete simpleIH;
        if (numIndSect - numSimpleIndSect > 0) {
            IndirectHeader *doubleIH = new IndirectHeader;
            IndirectHeader *arrayIH[NUM_INDIRECT];
            // alocamos los sectores indirectos de segundo nivel
            raw.doubleIndirectH = freeMap->Find();

            for (unsigned i = 0; i < numInTables - 1; i++) {
                doubleIH->dataSectors[i] = freeMap->Find();
                arrayIH[i] = new IndirectHeader;
                for (unsigned j = 0; j < NUM_INDIRECT; j++)
                    arrayIH[i]->dataSectors[j] = freeMap->Find();
            }
            unsigned j = numDoubleIndSect % NUM_INDIRECT;
            arrayIH[numInTables - 1] = new IndirectHeader;
            doubleIH->dataSectors[numInTables - 1] = freeMap->Find();
            for (unsigned i = 0; i < j; i++) 
               arrayIH[numInTables - 1]->dataSectors[i] = freeMap->Find(); 
            ((FileHeader*) doubleIH)->WriteBack(raw.doubleIndirectH);
            for (unsigned i = 0; i < numInTables; i++) {
                ((FileHeader*) arrayIH[i])->WriteBack(doubleIH->dataSectors[i]);
                delete arrayIH[i];
            }
            delete doubleIH;
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
        ASSERT(freeMap->Test(raw.doubleIndirectH));
        IndirectHeader *doubleIH = new IndirectHeader;
        ((FileHeader*)doubleIH)->FetchFrom(raw.doubleIndirectH);
        
        IndirectHeader *arrayIH[NUM_INDIRECT];
        for (unsigned i = 0; i < numInTables; i++) {
            arrayIH[i] = new IndirectHeader;
            ((FileHeader*) arrayIH[i])->FetchFrom(doubleIH->dataSectors[i]);
        }

        // desalocamos por completo
        for (unsigned i = 0; i < numInTables - 1; i++) {
            for (unsigned j = 0; j < NUM_INDIRECT; j++) {
                ASSERT(freeMap->Test(arrayIH[i]->dataSectors[j]));
                freeMap->Clear(arrayIH[i]->dataSectors[j]);
            }
            ASSERT(freeMap->Test(doubleIH->dataSectors[i]));
            freeMap->Clear(doubleIH->dataSectors[i]);
        }

        // desalocamos lo necesario de la ultima tabla
        unsigned j = numDoubleIndSect % NUM_INDIRECT;
        for (unsigned i = 0; i < j; i++) {
            ASSERT(freeMap->Test(arrayIH[numInTables - 1]->dataSectors[i]));
            freeMap->Clear(arrayIH[numInTables - 1]->dataSectors[i]);
        }
        ASSERT(freeMap->Test(doubleIH->dataSectors[numInTables - 1]));
        freeMap->Clear(doubleIH->dataSectors[numInTables - 1]);
        
        for (unsigned i = 0; i < numInTables; i++)
            delete arrayIH[i];

        delete doubleIH;
        freeMap->Clear(raw.doubleIndirectH);
    }

    if (numSimpleIndSect > 0) {
        ASSERT(freeMap->Test(raw.simpleIndirectH));
        IndirectHeader *simpleIH = new IndirectHeader;
        ((FileHeader*)simpleIH)->FetchFrom(raw.simpleIndirectH);

        for (unsigned j = 0; j < numSimpleIndSect; j++) {
            ASSERT(freeMap->Test(simpleIH->dataSectors[j]));
            freeMap->Clear(simpleIH->dataSectors[j]);
        }
        
        freeMap->Clear(raw.simpleIndirectH);
        delete simpleIH;
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
}

/// Write the modified contents of the file header back to disk.
///
/// * `sector` is the disk sector to contain the file header.
void
FileHeader::WriteBack(unsigned sector)
{
    synchDisk->WriteSector(sector, (char *) &raw);
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
    unsigned numSector = offset / SECTOR_SIZE;
    if (numSector < NUM_DIRECT) // esta en los bloques directos
        return raw.dataSectors[numSector];

    else if (numSector < NUM_DIRECT + NUM_INDIRECT) {
        IndirectHeader *simpleIH = new IndirectHeader;
        ((FileHeader*)simpleIH)->FetchFrom(raw.simpleIndirectH);
        unsigned result = simpleIH->dataSectors[numSector - NUM_DIRECT];
        delete simpleIH;
        return result; // esta en el primer nivel de indirecc
    }
    else { // está en el segundo nivel de indirecc
        int numTable = (numSector - NUM_DIRECT - NUM_INDIRECT) / NUM_INDIRECT;
        int sector =  (numSector - NUM_DIRECT - NUM_INDIRECT) % NUM_INDIRECT;
        IndirectHeader *doubleIH = new IndirectHeader;
        ((FileHeader*)doubleIH)->FetchFrom(raw.doubleIndirectH);
        IndirectHeader *internDoubleIH = new IndirectHeader;
        ((FileHeader*)internDoubleIH)->FetchFrom(doubleIH->dataSectors[numTable]);
        unsigned result = internDoubleIH->dataSectors[sector];
        delete doubleIH;
        delete internDoubleIH;
        return result;
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

    IndirectHeader *simpleIH = new IndirectHeader;
    IndirectHeader *doubleIH = new IndirectHeader;
    IndirectHeader *arrayIH[NUM_INDIRECT];

    for (unsigned i = 0; i < numDirSect; i++) {
        printf("%u ", raw.dataSectors[i]);
    }
    printf("\n");

    if (raw.numSectors > NUM_DIRECT) {
        ((FileHeader*)simpleIH)->FetchFrom(raw.simpleIndirectH);

        printf("    first level indirect block indexes: ");

        for (unsigned i = 0; i < numSimpleIndSect; i++)
            printf("%u ", simpleIH->dataSectors[i]);
        printf("\n");
    }
    if (raw.numSectors > NUM_DIRECT + NUM_INDIRECT) {
        ((FileHeader*)doubleIH)->FetchFrom(raw.doubleIndirectH);
        
        for (unsigned i = 0; i < numInTables; i++) {
            arrayIH[i] = new IndirectHeader;
            ((FileHeader*) arrayIH[i])->FetchFrom(doubleIH->dataSectors[i]);
        }

        printf("    second level indirect block indexes: ");
        for (unsigned i = 0; i < numInTables - 1; i++) {
            printf("\n     table[%u]: ", i);
            for (unsigned j = 0; j < NUM_INDIRECT; j++)
                printf("%u ", arrayIH[i]->dataSectors[j]);
        }

        unsigned j = numDoubleIndSect % NUM_INDIRECT;
        printf("\n     table[%u]: ", numInTables - 1);
        for (unsigned i = 0; i < j; i++)
            printf("%u ", arrayIH[numInTables - 1]->dataSectors[i]);
        
        printf("\n");
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
    if (raw.numSectors > NUM_DIRECT) {
        // contenido de primer nivel de indireccion
        for (unsigned i = 0; i < numSimpleIndSect; i++) {
            printf("    contents of block %u:\n", simpleIH->dataSectors[i]);
            synchDisk->ReadSector(simpleIH->dataSectors[i], data);
            for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
                if (isprint(data[j])) {
                    printf("%c", data[j]);
                } else {
                    printf("\\%X", (unsigned char) data[j]);
                }
            }
            printf("\n");
        }
        delete simpleIH;
    }
    if (raw.numSectors > NUM_DIRECT + NUM_INDIRECT) {
        // contenido de segundo nivel de indireccion
        for (unsigned x = 0; x < numInTables - 1; x++) {
            for (unsigned i = 0; i < NUM_INDIRECT; i++) {
                printf("    contents of block %u:\n", arrayIH[x]->dataSectors[i]);
                synchDisk->ReadSector(arrayIH[x]->dataSectors[i], data);
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
            printf("    contents of block %u:\n", arrayIH[numInTables - 1]->dataSectors[i]);
            synchDisk->ReadSector(arrayIH[numInTables - 1]->dataSectors[i], data);
            for (unsigned j = 0; j < SECTOR_SIZE && k < raw.numBytes; j++, k++) {
                if (isprint(data[j])) {
                    printf("%c", data[j]);
                } else {
                    printf("\\%X", (unsigned char) data[j]);
                }
            }
            printf("\n");
        }
        delete doubleIH;
        for (unsigned i = 0; i < numInTables; i++)
            delete arrayIH[i];

    }   
    delete [] data;
}

const RawFileHeader *
FileHeader::GetRaw() const
{
    return &raw;
}

bool 
FileHeader::Extend(Bitmap *freeMap, unsigned extendSize)
{
    ASSERT(freeMap != nullptr);

    DEBUG('f', "Extending %u to already %u.\n", extendSize, raw.numBytes);

    unsigned oldNumBytes         = raw.numBytes;
    unsigned oldNumDataSectors   = raw.numSectors; // aca hay solo sectores de datos
    unsigned oldNumOtherSectors  = 0; // lo vamos a calcular ahora

    unsigned oldNumDirSect       = min(oldNumDataSectors, NUM_DIRECT);
    unsigned oldNumIndSect       = oldNumDataSectors - oldNumDirSect;
    unsigned oldNumSimpleIndSect = min(oldNumIndSect, NUM_INDIRECT); 
    unsigned oldNumDoubleIndSect = oldNumIndSect - oldNumSimpleIndSect;
    unsigned oldNumInTables      = DivRoundUp(oldNumDoubleIndSect, NUM_INDIRECT);

     // sector para la tabla de primera indireccion
    if (oldNumSimpleIndSect > 0) {oldNumOtherSectors++;} 
    // sectores para la tabla de segunda indireccion y las tablas dentro de ella
    if (oldNumDoubleIndSect > 0) {oldNumOtherSectors += oldNumInTables + 1;}

    raw.numBytes += extendSize;
    unsigned numDataSectors   = DivRoundUp(raw.numBytes, SECTOR_SIZE);
    unsigned numOtherSectors  = 0; // lo calculo de cero
    unsigned numDirSect       = min(numDataSectors, NUM_DIRECT);
    unsigned numIndSect       = numDataSectors - numDirSect;
    unsigned numSimpleIndSect = min(numIndSect, NUM_INDIRECT);
    unsigned numDoubleIndSect = numIndSect - numSimpleIndSect;
    unsigned numInTables      = DivRoundUp(numDoubleIndSect, NUM_INDIRECT);

     // sector para la tabla de primera indireccion
    if (numSimpleIndSect > 0) {numOtherSectors++;} 
    // sectores para la tabla de segunda indireccion y las tablas dentro de ella
    if (numDoubleIndSect > 0) {numOtherSectors += numInTables + 1;}

    raw.numSectors = numDataSectors;

    // ya tengo la cantidad de sectores necesarios
    if (oldNumDataSectors == numDataSectors) {
        ASSERT(oldNumOtherSectors == numOtherSectors);
        DEBUG('f', "It is not necessary to add secotrs.\n");
        return true;
    }

    // debemos asegurarnos que el tamaño del archivo no sea tan
    // grande
    if (freeMap->CountClear() < ((numDataSectors + numOtherSectors) - (oldNumDataSectors + oldNumOtherSectors)) || raw.numBytes > MAX_FILE_SIZE) {
        DEBUG('f', "Not enough space to extend the file.\n");
        raw.numBytes = oldNumBytes;
        raw.numSectors = oldNumDataSectors;
        return false;
    }

    // alocamos lo que falte de los sectores directos
    for (unsigned i = oldNumDataSectors; i < numDirSect; i++) {
        // DEBUG('f', "Alocamos lo que falte de los sectores directos.\n");
        raw.dataSectors[i] = freeMap->Find();
    }

    // alocamos la tabla de primer nivel
    if (oldNumSimpleIndSect < numSimpleIndSect) {
        // DEBUG('f', "Alocamos la tabla de primer nivel.\n");
        IndirectHeader *simpleIH = new IndirectHeader;
        if (oldNumSimpleIndSect == 0) {
            // DEBUG('f', "Alocamos la tabla de primer nivel por primera vez.\n");
            raw.simpleIndirectH = freeMap->Find();
        }
        else {
            ((FileHeader*)simpleIH)->FetchFrom(raw.simpleIndirectH);
        }
        for (unsigned i = oldNumSimpleIndSect; i < numSimpleIndSect; i++)
            simpleIH->dataSectors[i] = freeMap->Find(); 

        ((FileHeader*)simpleIH)->WriteBack(raw.simpleIndirectH);
        delete simpleIH;
    }

    // alocamos la tabla de segundo nivel
    if (oldNumDoubleIndSect < numDoubleIndSect) {
        //DEBUG('y', "Alocamos la tabla de segundo nivel.\n");
        unsigned k = oldNumDoubleIndSect % NUM_INDIRECT;
        IndirectHeader *doubleIH = new IndirectHeader;

        if (oldNumDoubleIndSect == 0) {
            //DEBUG('y', "Alocamos la tabla de segundo nivel por primera vez:\n");
            //DEBUG('y', "oldNumInTables=%u, numInTables=%u, oldNumDoubleIndSect=%u, numDoubleIndSect=%u.\n", oldNumInTables, numInTables, oldNumDoubleIndSect, numDoubleIndSect);

            raw.doubleIndirectH = freeMap->Find();
        }
        else {
            ((FileHeader*)doubleIH)->FetchFrom(raw.doubleIndirectH);
        }

        // si la cantidad de tablas es la misma, como el número de sectores
        // es mayor, sólo habría que alocar más sectores de la última tabla        
        if (oldNumInTables == numInTables) {
            //DEBUG('y', "Alocamos más sectores a la última tabla, oldNumInTables=%u, numInTables=%u, oldNumDoubleIndSect=%u, numDoubleIndSect=%u.\n", oldNumInTables, numInTables, oldNumDoubleIndSect, numDoubleIndSect);
            ASSERT(k != 0);
            //DEBUG('f', "k = %u\n", k);
            IndirectHeader *internDoubleIH = new IndirectHeader;
            ((FileHeader*)internDoubleIH)->FetchFrom(doubleIH->dataSectors[numInTables - 1]);
            for (unsigned i = k; i < numDoubleIndSect - oldNumDoubleIndSect + k; i++)
                internDoubleIH->dataSectors[i] = freeMap->Find();
            ((FileHeader*)internDoubleIH)->WriteBack(doubleIH->dataSectors[numInTables - 1]);
        }
        
        if (oldNumInTables < numInTables) {
            if (k != 0) {
                //DEBUG('y', "Alocamos los sectores restantes de la última tabla.\n");
                // alocamos los sectores restantes de la última tabla
                IndirectHeader *internDoubleIH = new IndirectHeader;
                ((FileHeader*)internDoubleIH)->FetchFrom(doubleIH->dataSectors[oldNumInTables - 1]);
                for (unsigned i = k; i < NUM_INDIRECT; i++)
                    internDoubleIH->dataSectors[i] = freeMap->Find();
                    // aca necesito
                ((FileHeader*)internDoubleIH)->WriteBack(doubleIH->dataSectors[numInTables - 1]);
                delete internDoubleIH;
            }

            for (unsigned i = oldNumInTables; i < numInTables - 1; i++) {
                //DEBUG('y', "Alocamos las tablas.\n");
                IndirectHeader *internDoubleIH = new IndirectHeader;
                doubleIH->dataSectors[i] = freeMap->Find();
                for (unsigned j = 0; j < NUM_INDIRECT; j++)
                    internDoubleIH->dataSectors[j] = freeMap->Find();
                ((FileHeader*)internDoubleIH)->WriteBack(doubleIH->dataSectors[i]);
                delete internDoubleIH;
            }

            IndirectHeader *internDoubleIH = new IndirectHeader;
            doubleIH->dataSectors[numInTables - 1] = freeMap->Find();
            unsigned j = numDoubleIndSect % NUM_INDIRECT;
            for (unsigned i = 0; i < j; i++)
                internDoubleIH->dataSectors[i] = freeMap->Find(); 
            ((FileHeader*)internDoubleIH)->WriteBack(doubleIH->dataSectors[numInTables - 1]);
            delete internDoubleIH;
        }
        ((FileHeader*)doubleIH)->WriteBack(raw.doubleIndirectH);

    }

    DEBUG('f', "Success extending the file with size %u.\n", raw.numBytes);
    return true;
}
