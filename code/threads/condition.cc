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

#include "condition.hh"

/// Dummy functions -- so we can compile our later assignments.
///

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    waiters = 0;
    sem = new Semaphore(debugName, 0);
    condLock = conditionLock;
    DEBUG('v', "Condition variable <%s> created.\n", debugName);
}

Condition::~Condition()
{
    delete sem;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    ASSERT(condLock->IsHeldByCurrentThread());
    condLock->Release();
    waiters++;
    DEBUG('v', "Thread waiting. Condition variable: %s.\n", this->GetName());
    sem->P();
    condLock->Acquire();
    DEBUG('v', "Thread awaken. Condition variable: %s.\n", this->GetName());
}

void
Condition::Signal()
{
    ASSERT(condLock->IsHeldByCurrentThread());
    DEBUG('v', "Thread send a signal. Condition variable: %s.\n", this->GetName());
    if (waiters > 0) {
        sem->V();
        waiters--;
    }
}

void
Condition::Broadcast()
{
    ASSERT(condLock->IsHeldByCurrentThread());
    DEBUG('v', "Thread broadcast on the condition variable %s.\n", this->GetName());
    while (waiters > 0) {
        sem->V();
        waiters--;
    }
}
