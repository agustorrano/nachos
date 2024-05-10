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
    console = new Console(readFile, writeFile, ConsoleReadAvail, ConsoleWriteDone, 0);
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
    char ret = console->GetChar();
    readAvail->P();
    readLock->Release();
    return ret;
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