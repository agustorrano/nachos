/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"
#include "semaphore.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SEMAPHORE_TEST
    Semaphore* semaphore = new Semaphore("semTest", 3);
#endif

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.

bool threadDone[4] = {false};
void
SimpleThread(void *name_)
{
    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        #ifdef SEMAPHORE_TEST
            semaphore->P();
        #endif
        printf("*** Thread `%s` is running: iteration %u\n", currentThread->GetName(), num);
        #ifdef SEMAPHORE_TEST
            semaphore->V();
        #endif
        currentThread->Yield();
    }
    if (strcmp(currentThread->GetName(),"main")!= 0) {
        int i = 0;
        while(threadDone[i]) {
            i++;
        }
	    threadDone[i] = true;
    }
    printf("!!! Thread `%s` has finished SimpleThread\n", currentThread->GetName());
 
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
    for(int i = 2; i <= 5; i++) {
        char str[4][2];
        sprintf(str[i-2], "%d", i);
        Thread *newThread = new Thread(str[i-2], 0);
        newThread->Fork(SimpleThread, NULL);
    }
    //the "main" thread also executes the same function
    SimpleThread(NULL);

   //Wait for the others thread to finish if needed
    while (!threadDone[3]) {
        printf("main waiting\n");
        currentThread->Yield(); 
    }
    printf("Test finished\n");
    #ifdef SEMAPHORE_TEST
        delete semaphore;
    #endif
}
