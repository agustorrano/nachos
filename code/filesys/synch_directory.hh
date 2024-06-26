#ifndef NACHOS_FILESYS_SYNCHDIRECTORY__HH
#define NACHOS_FILESYS_SYNCHDIRECTORY__HH


#include "directory.hh"
#include "threads/lock.hh"


class SynchDirectory {
public:

    /// Initialize an empty directory with space for `size` files.
    SynchDirectory(unsigned size);

    /// De-allocate the directory.
    ~SynchDirectory();

    /// Initialize directory contents from disk.
    void FetchFrom(OpenFile *file);

    /// Write modifications to directory contents back to disk.
    void WriteBack(OpenFile *file);

    void FetchNoRelease(OpenFile *file);

    void WriteNoAquire(OpenFile *file);

private:
    
    Directory *directory;

    Lock *lockDirectory;
};


#endif