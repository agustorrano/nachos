/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include <stdlib.h>
#include <stdio.h>
#include "thread.hh"
#include <unistd.h>

#define SZ 8
static const unsigned N = 2;
static const unsigned M = 2;
static bool done[N+M];

/*
 * El buffer guarda punteros a enteros, los
 * productores los consiguen con malloc() y los
 * consumidores los liberan con free()
 */
int *buffer[SZ], contador = 0;

Lock* lockCond;
Lock* lockCounter;
Condition* notFull;
Condition* notEmpty;


void enviar(int *p)
{   
	// pthread_mutex_lock(&mutexContador);
    lockCounter->Acquire();
	buffer[contador] = p;
    contador++;
    lockCounter->Release();
	// pthread_mutex_unlock(&mutexContador);
  return;
}

int * recibir()
{
    int *recibido;
	// pthread_mutex_lock(&mutexContador);
    lockCounter->Acquire();
    recibido = buffer[contador-1];
    contador--;
    lockCounter->Release();
	// pthread_mutex_unlock(&mutexContador);
	return recibido;
}

int isFull() { return (contador == SZ); }
int isEmpty() { return (contador == 0); }

static void Producer(void *n_)
{
	unsigned *n = (unsigned *) n_;
    DEBUG('z', "Entering Producer %u. thread %s \n", *n, currentThread->GetName());
	while (1) {
        sleep(0.1);
		// pthread_mutex_lock(&mutexCond);
        lockCond->Acquire();
        DEBUG('z', "lockCond acquire. thread %s \n", currentThread->GetName());
        
        while (isFull()) notFull->Wait();
        // pthread_cond_wait(&notFull, &mutexCond);
        
        DEBUG('z', "After Wait. thread %s \n", currentThread->GetName());
		int *p = new int (sizeof *p);
		*p = random() % 100;
		printf("Productor %u: produje %p->%d\n", *n, p, *p);
		enviar(p);
		// pthread_cond_signal(&notEmpty);
        notEmpty->Signal();
		//pthread_mutex_unlock(&mutexCond);
        lockCond->Release();    
	}
	return;
}

static void Consumer(void *n_)
{
	unsigned *n = (unsigned *) n_;
    DEBUG('z', "Entering Consumer %u. thread %s \n", *n, currentThread->GetName());
	while (1) {
		sleep(0.1);
		// pthread_mutex_lock(&mutexCond);
        lockCond->Acquire();
        DEBUG('z', "lockCond acquire. thread %s \n", currentThread->GetName());

		while (isEmpty()) notEmpty->Wait();
			// pthread_cond_wait(&notEmpty, &mutexCond);
        
        DEBUG('z', "After Wait. thread %s \n", currentThread->GetName());
		int *p = recibir();
		printf("Consumidor %d: obtuve %p->%d\n", *n, p, *p);
		free(p);
		// pthread_cond_signal(&notFull);
        notFull->Signal();
		// pthread_mutex_unlock(&mutexCond);
        lockCond->Release();
	}
	return;
}

void ThreadTestProdCons()
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
        Thread *t = new Thread(namesP[i]);
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
        Thread *t = new Thread(namesC[i]);
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
	delete[] namesP[i];
    }
    for (unsigned i = 0; i < N; i++) {
	delete[] namesC[i];
    }
    delete []valuesP;
    delete []valuesC;
    delete []namesP;
    delete []namesC;

  return;
}