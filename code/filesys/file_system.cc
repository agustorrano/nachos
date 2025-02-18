/// Routines to manage the overall operation of the file system.  Implements
/// routines to map from textual file names to files.
///
/// Each file in the file system has:
/// * a file header, stored in a sector on disk (the size of the file header
///   data structure is arranged to be precisely the size of 1 disk sector);
/// * a number of data blocks;
/// * an entry in the file system directory.
///
/// The file system consists of several data structures:
/// * A bitmap of free disk sectors (cf. `bitmap.h`).
/// * A directory of file names and file headers.
///
/// Both the bitmap and the directory are represented as normal files.  Their
/// file headers are located in specific sectors (sector 0 and sector 1), so
/// that the file system can find them on bootup.
///
/// The file system assumes that the bitmap and directory files are kept
/// “open” continuously while Nachos is running.
///
/// For those operations (such as `Create`, `Remove`) that modify the
/// directory and/or bitmap, if the operation succeeds, the changes are
/// written immediately back to disk (the two files are kept open during all
/// this time).  If the operation fails, and we have modified part of the
/// directory and/or bitmap, we simply discard the changed version, without
/// writing it back to disk.
///
/// Our implementation at this point has the following restrictions:
///
/// * there is no synchronization for concurrent accesses;
/// * files have a fixed size, set when the file is created;
/// * files cannot be bigger than about 3KB in size;
/// * there is no hierarchical directory structure, and only a limited number
///   of files can be added to the system;
/// * there is no attempt to make the system robust to failures (if Nachos
///   exits in the middle of an operation that modifies the file system, it
///   may corrupt the disk).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "file_system.hh"
#include "directory.hh"
#include "file_header.hh"
#include "lib/bitmap.hh"
#include "threads/lock.hh"
#include "threads/system.hh"

#include <stdio.h>
#include <string.h>


/// Initialize the file system.  If `format == true`, the disk has nothing on
/// it, and we need to initialize the disk to contain an empty directory, and
/// a bitmap of free sectors (with almost but not all of the sectors marked
/// as free).
///
/// If `format == false`, we just have to open the files representing the
/// bitmap and the directory.
///
/// * `format` -- should we initialize the disk?
FileSystem::FileSystem(bool format)
{
    DEBUG('f', "Initializing the file system.\n");
    lockBitmap = new Lock("lockBitmap");
    openfiles = new OpenFileTable(TABLE_INIT_CAPACITY);
    directories = new DirectoryList();
    Lock *lockDirectory = directories->AddDirectory(DIRECTORY_SECTOR);

    if (format) {
        Bitmap     *freeMap = new Bitmap(NUM_SECTORS);
        Directory  *dir     = new Directory(NUM_DIR_ENTRIES);
        FileHeader *mapH    = new FileHeader;
        FileHeader *dirH    = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FREE_MAP_SECTOR);
        freeMap->Mark(DIRECTORY_SECTOR);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapH->Allocate(freeMap, FREE_MAP_FILE_SIZE));
        ASSERT(dirH->Allocate(freeMap, DIRECTORY_FILE_SIZE));

        // Flush the bitmap and directory `FileHeader`s back to disk.
        // We need to do this before we can `Open` the file, since open reads
        // the file header off of disk (and currently the disk has garbage on
        // it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapH->WriteBack(FREE_MAP_SECTOR);
        dirH->WriteBack(DIRECTORY_SECTOR);

        // OK to open the bitmap and directory files now.
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);
        openfiles->OpenFileAdd(FREE_MAP_SECTOR);
        openfiles->OpenFileAdd(DIRECTORY_SECTOR);

        // Once we have the files “open”, we can write the initial version of
        // each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        lockBitmap->Acquire();
        freeMap->WriteBack(freeMapFile);     // flush changes to disk
        lockBitmap->Release();
        lockDirectory->Acquire();
        dir->WriteBack(directoryFile);
        lockDirectory->Release();

        if (debug.IsEnabled('f')) {
            freeMap->Print();
            dir->Print();

            delete freeMap;
            delete dir;
            delete mapH;
            delete dirH;
        }
    } else {
        // If we are not formatting the disk, just open the files
        // representing the bitmap and directory; these are left open while
        // Nachos is running.
        freeMapFile   = new OpenFile(FREE_MAP_SECTOR);
        directoryFile = new OpenFile(DIRECTORY_SECTOR);
        openfiles->OpenFileAdd(FREE_MAP_SECTOR);
        openfiles->OpenFileAdd(DIRECTORY_SECTOR);
    }
}

FileSystem::~FileSystem()
{
    delete freeMapFile;
    delete directoryFile;
    delete lockBitmap;
    delete directories;
}

void
ParseDir(char* buffer, char outName[10], char* outDir[10]) 
{
    const char *delim = "/";
    int ntok = 0;
    char *saveptr;
    char *first = strtok_r(buffer, delim, &saveptr);
    outDir[ntok] = first;
    for (char *token = strtok_r(nullptr, delim, &saveptr); token != nullptr; token = strtok_r(nullptr, delim, &saveptr))
    {
      ntok++;
      outDir[ntok] = token;
    }    
    strcpy(outName, outDir[ntok]);
    outDir[ntok] = nullptr;
    return;
}

OpenFile *
ChangeDirectory(Directory *dir, unsigned length, char **outDir)
{
    int sector;
    OpenFile *dirFile;
    int numDirEntries;
    for (unsigned i = 0; i < length; i++) {
        sector = dir->Find(outDir[i]);

        if (sector == -1) {
            DEBUG('f', "Directory %s was not found.\n", outDir[i]);
            return nullptr;
        }

        if (dir->IsDirectory(sector)) {
            dirFile = new OpenFile(sector);
            numDirEntries = dirFile->Length() / sizeof (DirectoryEntry);
            //DEBUG('p', "numDirEntries: %d\n", numDirEntries);
            delete dir;
            dir = new Directory(numDirEntries);
            dir->FetchFrom(dirFile);
        } else {
            DEBUG('f', "%s is not a directory.\n", outDir[i]);
            return nullptr;
        }
    }
    return dirFile;
}

/// Create a file in the Nachos file system (similar to UNIX `create`).
/// Since we cannot increase the size of files dynamically, we have to give
/// `Create` the initial size of the file.
///
/// The steps to create a file are:
/// 1. Make sure the file does not already exist.
/// 2. Allocate a sector for the file header.
/// 3. Allocate space on disk for the data blocks for the file.
/// 4. Add the name to the directory.
/// 5. Store the new file header on disk.
/// 6. Flush the changes to the bitmap and the directory back to disk.
///
/// Return true if everything goes ok, otherwise, return false.
///
/// Create fails if:
/// * file is already in directory;
/// * no free space for file header;
/// * no free entry for file in directory;
/// * no free space for data blocks for the file.
///
/// Note that this implementation assumes there is no concurrent access to
/// the file system!
///
/// * `name` is the name of file to be created.
/// * `initialSize` is the size of file to be created.
bool
FileSystem::Create(const char *name, unsigned initialSize, bool isDir)
{
    ASSERT(name != nullptr);
    ASSERT(initialSize < MAX_FILE_SIZE);
    
    Directory* newDir;
    if (isDir) {
        DEBUG('f', "Creating directory %s, size %u.\n", name, initialSize);
        newDir = new Directory(NUM_DIR_ENTRIES);
    }
    else
        DEBUG('f', "Creating file %s, size %u.\n", name, initialSize);

    int sector;
    #ifdef FILESYS
    sector = currentThread->directories[currentThread->numDirectories];
    #endif

    OpenFile* actualdirFile = directoryFile;
    if (sector != 1) actualdirFile = new OpenFile(sector);
    int numDirEntries = actualdirFile->Length() / sizeof (DirectoryEntry);
    Directory *dir = new Directory(numDirEntries);
    
    dir->FetchFrom(actualdirFile);

    // length es la cantidad de directorios que hay ("userland/shell.cc == 1")
    unsigned length = 0;
    
    for (int i = 0; *(name+i) != 0; i++) {
        length += *(name+i) == '/';
    }

    char fileName[FILE_NAME_MAX_LEN + 1];
    OpenFile *dirFile;

    if (length != 0) {
        char *outDir[10] = {nullptr};
        char* buffer = new char [strlen(name)];
        strcpy(buffer, name);
        ParseDir(buffer, fileName, outDir);

        // change directory
        dirFile = ChangeDirectory(dir, length, outDir);
        if (dirFile == nullptr) {
            // lockDirectory->Release();
            delete dir;
            delete buffer;
            delete dirFile;
            return false;
        }
        delete buffer;
        sector = dirFile->GetSector();
    } else {
        strcpy(fileName, name);
    }

    Lock *lockDirectory = directories->AddDirectory(sector);
    lockDirectory->Acquire();

    bool success;

    // buscamos el archivo en el directorio
    if (dir->Find(fileName) != -1) {
        DEBUG('f', "File %s is already in directory.\n", fileName);
        success = false; 
    } else {
        Bitmap *freeMap = new Bitmap(NUM_SECTORS);
        lockBitmap->Acquire();
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();

        // Find a sector to hold the file header.
        if (sector == -1) {
            DEBUG('f', "No free block for file header.\n");
            success = false;  // No free block for file header.
        } else if (!dir->Add(fileName, sector, isDir)) {
            DEBUG('f', "No space in directory for file %s.\n", name);
            success = false;  // No space in directory.
        } else {
            FileHeader *h = new FileHeader;
            success = h->Allocate(freeMap, initialSize);
            // Fails if no space on disk for data.
            if (success) {
                // Everything worked, flush all changes back to disk.
                DEBUG('f', "Successful creation of file %s.\n", name);
                h->WriteBack(sector);
                if (isDir) {
                    directories->AddDirectory(sector);
                    OpenFile *newFile = new OpenFile(sector);
                    newDir->WriteBack(newFile);
                    //delete newFile;
                    //delete newDir;
                }
                if (length == 0)
                    dir->WriteBack(actualdirFile);
                    //dir->WriteBack(directoryFile);
                else {
                    dir->WriteBack(dirFile);
                    delete dirFile;
                }
                freeMap->WriteBack(freeMapFile);
            }
            delete h;
        }
        lockBitmap->Release();
        delete freeMap;
    }
    lockDirectory->Release();
    delete dir;
    return success;
}

/// Open a file for reading and writing.
///
/// To open a file:
/// 1. Find the location of the file's header, using the directory.
/// 2. Bring the header into memory.
///
/// * `name` is the text name of the file to be opened.
OpenFile *
FileSystem::Open(const char *name)
{
    ASSERT(name != nullptr);
    DEBUG('f', "Opening file %s.\n", name);

    OpenFile  *openFile = nullptr;
    
    int sector;
    #ifdef FILESYS
    sector = currentThread->directories[currentThread->numDirectories];
    #endif
    OpenFile* actualdirFile = directoryFile;
    if (sector != 1) actualdirFile = new OpenFile(sector);
    
    int numDirEntries = actualdirFile->Length() / sizeof (DirectoryEntry);
    Directory *dir = new Directory(numDirEntries);
    dir->FetchFrom(actualdirFile);


    unsigned length = 0;
    
    for (int i = 0; *(name+i) != 0; i++) {
        length += *(name+i) == '/';
    }

    char fileName[FILE_NAME_MAX_LEN + 1];
    OpenFile *dirFile;

    if (length != 0) {
        char *outDir[10] = {NULL};
        char* buffer = new char [strlen(name)];
        strcpy(buffer, name);
        ParseDir(buffer, fileName, outDir);

        // change directory
        dirFile = ChangeDirectory(dir, length, outDir);
        if (dirFile == nullptr) {
            // lockDirectory->Release();
            delete dir;
            delete buffer;
            delete dirFile;
            return nullptr;
        }
        delete buffer;
        sector = dirFile->GetSector();
    } else {
        strcpy(fileName, name);
    }

    Lock *lockDirectory = directories->AddDirectory(sector);
    lockDirectory->Acquire();
    sector = dir->Find(fileName);

    if (sector == -1) {
        DEBUG('f', "File %s does not exist.\n", fileName);
        lockDirectory->Release();
        return nullptr;
    }

    if (dir->IsDirectory(sector))
        DEBUG('f', "Trying to open directory %s.\n", fileName);

    else if (sector >= 0 && openfiles->OpenFileAdd(sector)) {
        DEBUG('f', "We add the file to the table for the first time.\n");
        openFile = new OpenFile(sector);  // `name` was found in directory.
    }
    lockDirectory->Release();
    delete dir;
    if (openFile != nullptr)
        DEBUG('f', "Successful opening file %s.\n", name);
    return openFile;  // Return null if not found.
}

/// Delete a file from the file system.
///
/// This requires:
/// 1. Remove it from the directory.
/// 2. Delete the space for its header.
/// 3. Delete the space for its data blocks.
/// 4. Write changes to directory, bitmap back to disk.
///
/// Return true if the file was deleted, false if the file was not in the
/// file system.
///
/// * `name` is the text name of the file to be removed.
bool
FileSystem::Remove(const char *name)
{
    ASSERT(name != nullptr);
    DEBUG('f', "Removing file %s.\n", name);
    
    int sector;
    #ifdef FILESYS
    sector = currentThread->directories[currentThread->numDirectories];
    #endif
    OpenFile* actualdirFile = directoryFile;
    if (sector != 1) actualdirFile = new OpenFile(sector);
    
    int numDirEntries = actualdirFile->Length() / sizeof (DirectoryEntry);
    Directory *dir = new Directory(numDirEntries);
    dir->FetchFrom(actualdirFile);

    unsigned length = 0;
    
    for (int i = 0; *(name+i) != 0; i++) {
        length += *(name+i) == '/';
    }

    char fileName[FILE_NAME_MAX_LEN + 1];
    OpenFile *dirFile = actualdirFile;

    if (length != 0) {
        char *outDir[10] = {NULL};
        char* buffer = new char [strlen(name)];
        strcpy(buffer, name);
        ParseDir(buffer, fileName, outDir);

        // change directory
        dirFile = ChangeDirectory(dir, length, outDir);
        if (dirFile == nullptr) {
            // lockDirectory->Release();
            delete dir;
            delete buffer;
            delete dirFile;
            return false;
        }
        delete buffer;
        sector = dirFile->GetSector();
    } else {
        strcpy(fileName, name);
    }

    Lock *lockDirectory = directories->AddDirectory(sector);
    lockDirectory->Acquire();

    sector = dir->Find(fileName);
    if (sector == -1) {
        DEBUG('f', "Unable to remove because file %s was not found.\n", name);
        lockDirectory->Release();
        delete dir;
        return false;  // file not found
    }

    int isDir = dir->IsDirectory(sector);

    if (isDir) {
        OpenFile *dirFileToDelete = new OpenFile(sector);
        numDirEntries = dirFileToDelete->Length() / sizeof (DirectoryEntry);   
        Directory *dirToDelete = new Directory(numDirEntries);
        dirToDelete->FetchFrom(dirFileToDelete);
        
        const RawDirectory *raw = dirToDelete->GetRaw();
        for (unsigned i = 0; i < raw->tableSize; i++) {
            if (raw->table[i].inUse) {
                DEBUG('f', "Removing file %s from directory %s.\n", raw->table[i].name, fileName);
                if (!Remove(raw->table[i].name)) {
                    lockDirectory->Release();
                    delete dirToDelete;
                    delete dirFile;
                    return false;
                }
            }
        }
        // lockDirectory->Release();
        delete dirToDelete;
        delete dirFileToDelete;
        // Release(sector);
    }
    
    // siempre se borra del directorio
    dir->Remove(fileName);
    if (length == 0)
        dir->WriteBack(actualdirFile);
        //dir->WriteBack(directoryFile);    // Flush to disk.
    else {
        dir->WriteBack(dirFile);
        delete dirFile;
    }

    if (openfiles->IsOpen(sector))
        openfiles->MarkToDelete(sector); // marcamos que deberá borrarse del disco
        // se elimina del directorio pero no del disco
    else 
        Release(sector);
    
    lockDirectory->Release();
    if (isDir)
        directories->CloseDirectory(sector);
    
    delete dir;
    return true;
}

void 
FileSystem::Release(int sector) {
    FileHeader *fileH = new FileHeader;
    fileH->FetchFrom(sector);
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    lockBitmap->Acquire();
    freeMap->FetchFrom(freeMapFile);
    fileH->Deallocate(freeMap);  // Remove data blocks.
    freeMap->Clear(sector);      // Remove header block.
    freeMap->WriteBack(freeMapFile);  // Flush to disk.
    lockBitmap->Release();
    delete fileH;
    delete freeMap;
}

/// List all the files in the file system directory.
void
FileSystem::List()
{
    int numDirEntries = directoryFile->Length() / sizeof (DirectoryEntry);   
    Directory *dir = new Directory(numDirEntries);
    Lock *lockDirectory = directories->AddDirectory(DIRECTORY_SECTOR);
    lockDirectory->Acquire();
    dir->FetchFrom(directoryFile);
    dir->List();
    lockDirectory->Release();
    delete dir;
}

static bool
AddToShadowBitmap(unsigned sector, Bitmap *map)
{
    ASSERT(map != nullptr);

    if (map->Test(sector)) {
        DEBUG('f', "Sector %u was already marked.\n", sector);
        return false;
    }
    map->Mark(sector);
    DEBUG('f', "Marked sector %u.\n", sector);
    return true;
}

static bool
CheckForError(bool value, const char *message)
{
    if (!value) {
        DEBUG('f', "Error: %s\n", message);
    }
    return !value;
}

static bool
CheckSector(unsigned sector, Bitmap *shadowMap)
{
    if (CheckForError(sector < NUM_SECTORS,
                      "sector number too big.  Skipping bitmap check.")) {
        return true;
    }
    return CheckForError(AddToShadowBitmap(sector, shadowMap),
                         "sector number already used.");
}

static bool
CheckFileHeader(const RawFileHeader *rh, unsigned num, Bitmap *shadowMap)
{
    ASSERT(rh != nullptr);

    bool error = false;

    DEBUG('f', "Checking file header %u.  File size: %u bytes, number of sectors: %u.\n",
          num, rh->numBytes, rh->numSectors);
    error |= CheckForError(rh->numSectors >= DivRoundUp(rh->numBytes,
                                                        SECTOR_SIZE),
                           "sector count not compatible with file size.");
    error |= CheckForError(rh->numSectors < NUM_DIRECT,
                           "too many blocks.");
    for (unsigned i = 0; i < rh->numSectors; i++) {
        unsigned s = rh->dataSectors[i];
        error |= CheckSector(s, shadowMap);
    }
    return error;
}

static bool
CheckBitmaps(const Bitmap *freeMap, const Bitmap *shadowMap)
{
    bool error = false;
    for (unsigned i = 0; i < NUM_SECTORS; i++) {
        DEBUG('f', "Checking sector %u. Original: %u, shadow: %u.\n",
              i, freeMap->Test(i), shadowMap->Test(i));
        error |= CheckForError(freeMap->Test(i) == shadowMap->Test(i),
                               "inconsistent bitmap.");
    }
    return error;
}

static bool
CheckDirectory(const RawDirectory *rd, Bitmap *shadowMap)
{
    ASSERT(rd != nullptr);
    ASSERT(shadowMap != nullptr);

    bool error = false;
    unsigned nameCount = 0;
    const char *knownNames[NUM_DIR_ENTRIES];

    for (unsigned i = 0; i < NUM_DIR_ENTRIES; i++) {
        DEBUG('f', "Checking direntry: %u.\n", i);
        const DirectoryEntry *e = &rd->table[i];

        if (e->inUse) {
            if (strlen(e->name) > FILE_NAME_MAX_LEN) {
                DEBUG('f', "Filename too long.\n");
                error = true;
            }

            // Check for repeated filenames.
            DEBUG('f', "Checking for repeated names.  Name count: %u.\n",
                  nameCount);
            bool repeated = false;
            for (unsigned j = 0; j < nameCount; j++) {
                DEBUG('f', "Comparing \"%s\" and \"%s\".\n",
                      knownNames[j], e->name);
                if (strcmp(knownNames[j], e->name) == 0) {
                    DEBUG('f', "Repeated filename.\n");
                    repeated = true;
                    error = true;
                }
            }
            if (!repeated) {
                knownNames[nameCount] = e->name;
                DEBUG('f', "Added \"%s\" at %u.\n", e->name, nameCount);
                nameCount++;
            }

            // Check sector.
            error |= CheckSector(e->sector, shadowMap);

            // Check file header.
            FileHeader *h = new FileHeader;
            const RawFileHeader *rh = h->GetRaw();
            h->FetchFrom(e->sector);
            error |= CheckFileHeader(rh, e->sector, shadowMap);
            delete h;
        }
    }
    return error;
}

bool
FileSystem::Check()
{
    DEBUG('f', "Performing filesystem check\n");
    bool error = false;

    Bitmap *shadowMap = new Bitmap(NUM_SECTORS);
    shadowMap->Mark(FREE_MAP_SECTOR);
    shadowMap->Mark(DIRECTORY_SECTOR);

    DEBUG('f', "Checking bitmap's file header.\n");

    FileHeader *bitH = new FileHeader;
    const RawFileHeader *bitRH = bitH->GetRaw();
    bitH->FetchFrom(FREE_MAP_SECTOR);
    DEBUG('f', "  File size: %u bytes, expected %u bytes.\n"
               "  Number of sectors: %u, expected %u.\n",
          bitRH->numBytes, FREE_MAP_FILE_SIZE,
          bitRH->numSectors, FREE_MAP_FILE_SIZE / SECTOR_SIZE);
    error |= CheckForError(bitRH->numBytes == FREE_MAP_FILE_SIZE,
                           "bad bitmap header: wrong file size.");
    error |= CheckForError(bitRH->numSectors == FREE_MAP_FILE_SIZE / SECTOR_SIZE,
                           "bad bitmap header: wrong number of sectors.");
    error |= CheckFileHeader(bitRH, FREE_MAP_SECTOR, shadowMap);
    delete bitH;

    DEBUG('f', "Checking directory.\n");

    FileHeader *dirH = new FileHeader;
    const RawFileHeader *dirRH = dirH->GetRaw();
    dirH->FetchFrom(DIRECTORY_SECTOR);
    error |= CheckFileHeader(dirRH, DIRECTORY_SECTOR, shadowMap);
    delete dirH;

    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    freeMap->FetchFrom(freeMapFile);
    
    int numDirEntries = directoryFile->Length() / sizeof (DirectoryEntry);   
    Directory *dir = new Directory(numDirEntries);
    
    const RawDirectory *rdir = dir->GetRaw();
    dir->FetchFrom(directoryFile);
    error |= CheckDirectory(rdir, shadowMap);
    delete dir;

    // The two bitmaps should match.
    DEBUG('f', "Checking bitmap consistency.\n");
    error |= CheckBitmaps(freeMap, shadowMap);
    delete shadowMap;
    delete freeMap;

    DEBUG('f', error ? "Filesystem check failed.\n"
                     : "Filesystem check succeeded.\n");
    return !error;
}

/// Print everything about the file system:
/// * the contents of the bitmap;
/// * the contents of the directory;
/// * for each file in the directory:
///   * the contents of the file header;
///   * the data in the file.
void
FileSystem::Print()
{
    FileHeader *bitH    = new FileHeader;
    FileHeader *dirH    = new FileHeader;
    Bitmap     *freeMap = new Bitmap(NUM_SECTORS);
    
    int numDirEntries = directoryFile->Length() / sizeof (DirectoryEntry);   
    Directory  *dir     = new Directory(numDirEntries);

    printf("--------------------------------\n");
    bitH->FetchFrom(FREE_MAP_SECTOR);
    bitH->Print("Bitmap");

    printf("--------------------------------\n");
    dirH->FetchFrom(DIRECTORY_SECTOR);
    dirH->Print("Directory");

    printf("--------------------------------\n");
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    printf("--------------------------------\n");
    dir->FetchFrom(directoryFile);
    printf("Directory root contents:\n");
    dir->Print();
    printf("--------------------------------\n");

    delete bitH;
    delete dirH;
    delete freeMap;
    delete dir;

}

bool
FileSystem::CloseOpenFile(int sector) 
{
    return openfiles->CloseOpenFile(sector);
}

Bitmap *
FileSystem::AquireFreeMap()
{
    Bitmap *freeMap = new Bitmap(NUM_SECTORS);
    // lockBitmap->Acquire();
    freeMap->FetchFrom(freeMapFile);
    return freeMap;
}

void 
FileSystem::ReleaseFreeMap(Bitmap *freeMap)
{
    freeMap->WriteBack(freeMapFile);
    // lockBitmap->Release();
    delete freeMap;
}

void 
FileSystem::AcquireRead(int sector)
{
    openfiles->AcquireRead(sector);
}

void 
FileSystem::ReleaseRead(int sector)
{
    openfiles->ReleaseRead(sector);
}

void 
FileSystem::AcquireWrite(int sector)
{
    openfiles->AcquireWrite(sector);
}

void 
FileSystem::ReleaseWrite(int sector)
{
    openfiles->ReleaseWrite(sector);
}