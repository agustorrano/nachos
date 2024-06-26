#include "synch_directory.hh"

SynchDirectory::SynchDirectory(unsigned size)
{
    directory = new Directory(size);
    lockDirectory = new Lock("lockDirectory");
}

SynchDirectory::~SynchDirectory()
{
    delete directory;
    delete lockDirectory;
}

void 
SynchDirectory::FetchFrom(OpenFile *file)
{
    lockDirectory->Acquire();
    directory->FetchFrom(file);
    lockDirectory->Release();
}

void 
SynchDirectory::WriteBack(OpenFile *file)
{
    lockDirectory->Acquire();
    directory->WriteBack(file);
    lockDirectory->Release();
}

// por lo mismo que explique en synch bitmap
void 
SynchDirectory::FetchNoRelease(OpenFile *file)
{
    lockDirectory->Acquire();
    directory->FetchFrom(file);
}

void
SynchDirectory::WriteNoAquire(OpenFile *file)
{
    directory->WriteBack(file);
    lockDirectory->Release();
}
