#include "synch_file_system.hh"

SynchFileSystem::SynchFileSystem(bool format)
{
    fs = new FileSystem(format);
    lockFreeMap = new Lock("freeMap");
    lockDirectory = new Lock("directory");
}

SynchFileSystem::~SynchFileSystem()
{
    delete fs;
    delete lockFreeMap;
    delete lockDirectory;
}

bool
SynchFileSystem::Create(const char *name, unsigned initialSize)
{
    lockFreeMap->Acquire();
    lockDirectory->Acquire();
    bool ret = fs->Create(name, initialSize);
    lockDirectory->Release();
    lockFreeMap->Release();
    return ret;
}

OpenFile*
SynchFileSystem::Open(const char *name)
{
    lockFreeMap->Acquire();
    lockDirectory->Acquire();
    OpenFile *ret = fs->Open(name);
    lockDirectory->Release();
    lockFreeMap->Release();
    return ret;
}

bool
SynchFileSystem::Remove(const char *name)
{
    lockFreeMap->Acquire();
    lockDirectory->Acquire();
    bool ret = fs->Remove(name);
    lockDirectory->Release();
    lockFreeMap->Release();
    return ret;
}

void
SynchFileSystem::List()
{
    lockFreeMap->Acquire();
    lockDirectory->Acquire();
    fs->List();
    lockDirectory->Release();
    lockFreeMap->Release();
    return;
}

bool
SynchFileSystem::Check()
{
    lockFreeMap->Acquire();
    lockDirectory->Acquire();
    bool ret = fs->Check();
    lockDirectory->Release();
    lockFreeMap->Release();
    return ret;
}

void
SynchFileSystem::Print()
{
    lockFreeMap->Acquire();
    lockDirectory->Acquire();
    fs->Print();
    lockDirectory->Release();
    lockFreeMap->Release();
    return;
}

void
SynchFileSystem::CloseOpenFile(int sector)
{
    int x = -1;
    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (fs->openFileTable[i].sector == sector) {
            fs->openFileTable[i].numThreads--;
            if (fs->openFileTable[i].numThreads == 0 && fs->openFileTable[i].toDelete) {
                fs->openFileTable[i].sector = -1;
                fs->openFileTable[i].toDelete = 0;
                fs->Remove(fs->openFileTable[i].name);
            }
        }
    }
    return;
}
