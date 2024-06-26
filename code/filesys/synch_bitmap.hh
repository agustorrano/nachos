#ifndef NACHOS_FILESYS_SYNCHBITMAP__HH
#define NACHOS_FILESYS_SYNCHBITMAP__HH


#include "lib/bitmap.hh"
#include "threads/lock.hh"


class SynchBitmap {
public:

    /// Initialize a synch bitmap with `nitems` bits; all bits are cleared.
    SynchBitmap(unsigned nitems);

    /// Uninitialize a synch bitmap.
    ~SynchBitmap();

    /// Fetch contents from disk.
    ///
    /// Note: this is not needed until the *FILESYS* assignment, when we will
    /// need to read and write the bitmap to a file.
    void FetchFrom(OpenFile *file);

    /// Write contents to disk.
    ///
    /// Note: this is not needed until the *FILESYS* assignment, when we will
    /// need to read and write the bitmap to a file.
    void WriteBack(OpenFile *file) const;

    void FetchNoRelease(OpenFile *file);

    void WriteNoAquire(OpenFile *file) const;

private:

    Bitmap *bitmap;

    Lock *lockBitmap;

};

#endif