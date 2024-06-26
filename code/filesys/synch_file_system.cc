#include "synch_file_system.hh"

SynchFileSystem::SynchFileSystem(bool format)
{
    fs = new FileSystem(format);
    lockFS = new Lock("fileSystem Lock");
}

SynchFileSystem::~SynchFileSystem()
{
    delete fs;
    delete lockFS;
}

bool
SynchFileSystem::Create(const char *name, unsigned initialSize)
{
    lockFS->Acquire();
    bool ret = fs->Create(name, initialSize);
    lockFS->Release();
    return ret;
}

OpenFile*
SynchFileSystem::Open(const char *name)
{
    lockFS->Acquire();
    OpenFile *ret = fs->Open(name);
    lockFS->Release();
    return ret;
}

bool
SynchFileSystem::Remove(const char *name)
{
    lockFS->Acquire();
    bool ret = fs->Remove(name);
    lockFS->Release();
    return ret;
}

void
SynchFileSystem::List()
{
    lockFS->Acquire();
    fs->List();
    lockFS->Release();
    return;
}

bool
SynchFileSystem::Check()
{
    lockFS->Acquire();
    bool ret = fs->Check();
    lockFS->Release();
    return ret;
}

void
SynchFileSystem::Print()
{
    lockFS->Acquire();
    fs->Print();
    lockFS->Release();
    return;
}

void
SynchFileSystem::CloseOpenFile(int sector)
{
    lockFS->Acquire(); // es necesario?
    fs->CloseOpenFile(sector);
    lockFS->Release(); // es necesario?
}
