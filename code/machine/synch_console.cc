#include "synch_console.hh"

static void
ConsoleReadAvail(void *arg)
{
    ASSERT(arg != nullptr);
    SynchConsole *console = (SynchConsole *) arg;
    console->ReadAvail(arg);
}

static void
ConsoleWriteDone(void *arg)
{
    ASSERT(arg != nullptr);
    SynchConsole *console = (SynchConsole *) arg;
    console->WriteDone(arg);
}

SynchConsole::SynchConsole(const char *readFile, const char *writeFile)
{
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    readLock = new Lock("synch console read lock");
    writeLock = new Lock("synch console write lock");
    console = new Console(readFile, writeFile, ConsoleReadAvail, ConsoleWriteDone, this);
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete readLock;
    delete writeLock;
    delete readAvail;
    delete writeDone;
}

void
SynchConsole::ReadAvail(void *arg)
{
    readAvail->V();
}

void
SynchConsole::WriteDone(void *arg)
{
    writeDone->V();
}

void
SynchConsole::WriteChar(char ch)
{
    writeLock->Acquire();
    console->PutChar(ch);
    writeDone->P();
    writeLock->Release();
}

char 
SynchConsole::ReadChar()
{
    readLock->Acquire();
    readAvail->P();
    char ret = console->GetChar();
    readLock->Release();
    return ret;
}

void
SynchConsole::WriteBuffer(char *buffer, unsigned size)
{
    writeLock->Acquire();
    for (unsigned i = 0; i < size; i++) {
        console->PutChar(buffer[i]);
        writeDone->P();
    }
    writeLock->Release();
}

void 
SynchConsole::ReadBuffer(char *buffer, unsigned size) {
    readLock->Acquire();
    for (unsigned i = 0; i < size; i++) {
        readAvail->P();
        buffer[i] = console->GetChar();
    }
    readLock->Release();
}
