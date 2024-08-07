#ifndef NACHOS_FILESYS_DIRECTORYLIST__HH
#define NACHOS_FILESYS_DIRECTORYLIST__HH

//#include "threads/lock.hh"
class Lock;

class DirEntry {
public:

    unsigned numThreads;

    int sector;

    Lock *lockDir;

    DirEntry *next;
};

class DirectoryList {
public:
    
    DirectoryList();

    ~DirectoryList();

    bool IsEmpty();

    Lock *AddDirectory(int sector);

    bool CloseDirectory(int sector);

    bool NotInList(int sector);

    DirEntry *first;  ///< Head of the list, null if list is empty.
    DirEntry *last;   ///< Last element of list.
    
    int size;
};

#endif