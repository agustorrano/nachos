#ifndef NACHOS_FILESYS_OPENFILETABLE__HH
#define NACHOS_FILESYS_OPENFILETABLE__HH

#include <string.h>
#include <unordered_map>
#include "directory_entry.hh"

class OpenFileEntry {
public:

    bool toDelete;

    unsigned numThreads;

    char name[FILE_NAME_MAX_LEN + 1];
    
    // Constructor por defecto
    OpenFileEntry() : toDelete(false), numThreads(0) {
        name[0] = '\0'; 
    }
    
    OpenFileEntry(char* name);

    ~OpenFileEntry();
};

class OpenFileTable {
public:
    bool OpenFileAdd(int sector, const char* name);

    bool IsOpen(int sector);

    void MarkToDelete(int sector);

    void CloseOpenFile(int sector);

private:
    std::unordered_map<int, OpenFileEntry> openFilesTable;
};

#endif