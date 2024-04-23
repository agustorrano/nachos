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
#include <stdio.h>


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

    // A continuación se resuelve el problema de inversión de prioridades mediante
    // la herencia de prioridades.
    // En el caso de los semáforos no se puede hacer porque no tenemos información
    // sobre el hilo que está utilizando los recursos. Es decir, si un hilo de alta
    // prioridad queda bloqueado mediante sem->P(), no sabemos cual será el hilo que
    // llame a sem->V() para desbloqueralo.
    if (thread != nullptr)
        if (thread->GetPriority() < currentThread->GetPriority()) {
            DEBUG('t', "Thread \"%s\" inherits priority from thread \"%s\"\n", thread->GetName(), currentThread->GetName());
            thread->InheritPriority(currentThread->GetPriority());
            scheduler->ChangePriority(thread);      
        }
    sem->P();
    thread = currentThread;
}

void
Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());
    currentThread->RestorePriority();
    thread = nullptr;
    sem->V();
}

bool
Lock::IsHeldByCurrentThread() const
{
    return thread == currentThread;
}
