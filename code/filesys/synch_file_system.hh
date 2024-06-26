#ifndef NACHOS_FILESYS_SYNCHFILESYSTEM__HH
#define NACHOS_FILESYS_SYNCHFILESYSTEM__HH

#include "file_system.hh"
#include "threads/lock.hh"

class SynchFileSystem {
public:

    /// Initialize the synch file system.
    SynchFileSystem(bool format);

    ~SynchFileSystem();

    /// Create a file (UNIX `creat`).
    bool Create(const char *name, unsigned initialSize);

    /// Open a file (UNIX `open`).
    OpenFile *Open(const char *name);

    /// Delete a file (UNIX `unlink`).
    bool Remove(const char *name);

    /// List all the files in the file system.
    void List();

    /// Check the filesystem.
    bool Check();

    /// List all the files and their contents.
    void Print();

    void CloseOpenFile(int sector);

private:

    FileSystem *fs;

    Lock *lockFS;
};

#endif