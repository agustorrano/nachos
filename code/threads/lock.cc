/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "lock.hh"
#include "system.hh"
#include <string.h>


/// Dummy functions -- so we can compile our later assignments.

Lock::Lock(const char *debugName)
{
    name = debugName;
    sem = new Semaphore(debugName, 1);
    threadName = "none";
    thread = nullptr;
}

Lock::~Lock()
{
    delete sem;
}

const char *
Lock::GetName() const
{
    return name;
}

void
Lock::Acquire()
{
    ASSERT(!IsHeldByCurrentThread());
    // threadName = currentThread->GetName();
    sem->P();
    thread = currentThread;
}

void
Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());
    // threadName = "none";
    thread = nullptr;
    sem->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    // const char* ctname = currentThread->GetName();
    return thread == currentThread;
}
