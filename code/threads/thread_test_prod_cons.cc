/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "thread.hh"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SZ 3
static const int ITEMS_PRODUCED = 1000;
static const unsigned N = 1;
static const unsigned M = 1;
static bool done[N+M];

/*
 * El buffer guarda punteros a enteros, los
 * productores los consiguen con malloc() y los
 * consumidores los liberan con free()
 */
int buffer[SZ], contador = 0;

Lock* lockCond;
Lock* lockCounter;
Condition* notFull;
Condition* notEmpty;


void 
enviar(int p, int n)
{   
    lockCounter->Acquire();
	buffer[contador] = p;
    printf("Productor %d produce: %d en %d\n", n, p, contador);
    contador++;
    lockCounter->Release();
    return;
}

int 
recibir(int n)
{
    int recibido;
    lockCounter->Acquire();
    recibido = buffer[contador-1];
    contador--;
    printf("Consumidor %d consume: %d en %d\n", n, recibido, contador);
    lockCounter->Release();
	return recibido;
}

int 
isFull() 
{ 
    return (contador == SZ); 
}

int 
isEmpty() 
{ 
    return (contador == 0); 
}

static void 
Producer(void *n_)
{
	unsigned *n = (unsigned *) n_;
    DEBUG('z', "Entering Producer %u. thread %s \n", *n, currentThread->GetName());
	for (int i = 1; i <= ITEMS_PRODUCED; i++) {
        lockCond->Acquire();
        DEBUG('z', "lockCond acquire. thread %s \n", currentThread->GetName());
        
        while (isFull()) {
            printf("Productor %d esperando (buffer lleno)\n", *n);
            notFull->Wait();
        }
        
        DEBUG('z', "After Wait. thread %s \n", currentThread->GetName());
        enviar(i, *n);
        currentThread->Yield();
        notEmpty->Signal();
        lockCond->Release();
        currentThread->Yield();   
	}
    done[N + (*n)] = true;
	return;
}

static void 
Consumer(void *n_)
{
	unsigned *n = (unsigned *) n_;
    DEBUG('z', "Entering Consumer %u. thread %s \n", *n, currentThread->GetName());
	for (unsigned i = 0; i < ITEMS_PRODUCED; i++) {
        lockCond->Acquire();
        DEBUG('z', "lockCond acquire. thread %s \n", currentThread->GetName());

		while (isEmpty()) {
            printf("Consumidor %d esperando (buffer vacio)\n", *n);
            notEmpty->Wait();
        }
        
        DEBUG('z', "After Wait. thread %s \n", currentThread->GetName());
        recibir(*n);
        currentThread->Yield();
        notFull->Signal();
        lockCond->Release();
        currentThread->Yield();
	}
    done[*n] = true;
	return;
}

void 
ThreadTestProdCons()
{
    lockCond = new Lock("lockCond");
    lockCounter = new Lock("lockCounter");
	notFull = new Condition("notFull", lockCond);
    notEmpty = new Condition("notEmpty", lockCond);

    char **namesP = new char*[M];
    unsigned *valuesP = new unsigned[M];

    for (unsigned i = 0; i < M; i++) {
        printf("Launching producer %u.\n", i);
        namesP[i] = new char[16];
        sprintf(namesP[i], "Producer %u", i);
        printf("Name: %s\n", namesP[i]);
        valuesP[i] = i;
        Thread *t = new Thread(namesP[i], 0);
        t->Fork(Producer, (void *) &(valuesP[i]));
    }

    char **namesC = new char*[N];
    unsigned *valuesC = new unsigned[N];

    for (unsigned i = 0; i < N; i++) {
        printf("Launching consumer %u.\n", i);
        namesC[i] = new char[16];
        sprintf(namesC[i], "Consumer %u", i);
        printf("Name: %s\n", namesC[i]);
        valuesC[i] = i;
        Thread *t = new Thread(namesC[i], 0);
        t->Fork(Consumer, (void *) &(valuesC[i]));
    }
	
    for (unsigned i = 0; i < N+M; i++) {
        while (!done[i]) {
            currentThread->Yield();
        }
    }
    
    printf("All threads finished. \n");

    // Free all the memory
    for (unsigned i = 0; i < M; i++) {
	    delete namesP[i];
    }
    for (unsigned i = 0; i < N; i++) {
	    delete namesC[i];
    }
    delete []valuesP;
    delete []valuesC;
    delete []namesP;
    delete []namesC;

  return;
}