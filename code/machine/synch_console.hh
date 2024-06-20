#ifndef NACHOS_MACHINE_SYNCHCONSOLE__HH
#define NACHOS_MACHINE_SYNCHCONSOLE__HH

#include "console.hh"
#include "threads/lock.hh"

class SynchConsole {
public:

  SynchConsole(const char *readFile, const char *writeFile);

  ~SynchConsole();

  void ReadAvail(void *arg);

  void WriteDone(void *arg);

  void WriteChar(char ch);

  char ReadChar();

  void WriteBuffer(char *buffer, unsigned size);

  void ReadBuffer(char *buffer, unsigned size);

private:

  Console *console;

  Semaphore *readAvail;

  Semaphore *writeDone;

  Lock *readLock;

  Lock *writeLock; 

};

#endif