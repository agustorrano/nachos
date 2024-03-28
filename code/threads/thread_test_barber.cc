#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_test_barber.hh"

#define CHAIRS 3
#define CLI 5

/* variable que guardará el id de cliente que está siendo atendido */
int client = -1;
/* representa la cola de clientes esperando en la barbería */
SynchList<int> *queue;

/* los clientes que están en la cola, esperarán con esta variable
de condición hasta que sea su turno. */
// pthread_cond_t esperando = PTHREAD_COND_INITIALIZER;
Condition* waiting;


/* lock utilizado junto con la condición esperando */
// pthread_mutex_t esperandoLk = PTHREAD_MUTEX_INITIALIZER;
Lock* waitingLk;

/* semáforos utilizados para manejar la concurrencia entre el barbero y 
el cliente que está siendo atendido */
Semaphore *waitCli, *waitBarber;

/* semáforo utilizado para avisar al barbero que hay clientes esperando 
ser atendidos */
Semaphore *newCli;

void me_cortan(int arg)
{
	printf("\nCliente %d: me estan cortando el pelo\n", arg);
}

void cortando(int arg)
{
	printf("\nBarbero: cortando el pelo al cliente %d\n", arg);
}

void pagando(int arg)
{
	printf("\nCliente %d: estoy pagando\n", arg);
}

void me_pagan(int arg)
{
	printf("\nBarbero: me esta pagando el cliente %d\n", arg);
}

/*
 * devuelve 0 en caso de que el cliente que llama la función
 * sea el próximo en la cola, 1 en caso contrario.
 */
int no_soy_siguiente(int x)
{
	int flag;
	flag = (client != x);
	return flag;
}

/* 
 * Función que representa el trabajo realizado por el barbero. 
 * El semáforo hayClientes fue inicializado en 0. Por ello, el barbero estará dormido 
 * hasta que un cliente entre y llame a sem_post. Cada vez que el barbero termine
 * de atender a un cliente, realizará un sem_wait. Si era el último en la barbería,
 * volverá a dormirse hasta que entre alguien, si no simplemente atiende al 
 * siguiente de la cola de espera.
 * 
 * En el momento en que decide el cliente que será próximo, lo guarda en una variable
 * y lo elimina de la cola, para que otro pueda entrar a la barbería y esperar su turno.
 * Despertará a todos los clientes que estén esperando con un broadcast.
 */
static void trabajando(void* arg)
{
	while (1)
	{
		//sem_wait(&hayClientes);
    newCli->P();
		//cliente = cola_concurrente_pop(concCola);
    client = queue->Pop();
		sleep(1); /* */
		//pthread_cond_broadcast(&esperando);
		currentThread->Yield();
    waiting->Broadcast();
		cortando(client);
		//sem_post(&esperarBarbero);
    waitBarber->V();
		//sem_wait(&esperarCliente);
    waitCli->P();
		me_pagan(client);
		//sem_post(&esperarBarbero);
    waitBarber->V();
		//sem_wait(&esperarCliente);
		currentThread->Yield();
    waitCli->P();
	}
}

/*
 * Función que representa los clientes de la barberia. 
 * Si un cliente entra y no hay sillas libres, se retira. Si no, se agrega a la cola
 * de espera. Luego, avisa al barbero que llegó un nuevo cliente y verifica si es el
 * próximo. Si no lo es, espera hasta que sea su turno con la variable de condición.
 * Cada vez que el barbero termine con un cliente, despertará a todos aquellos que estén
 * esperando, para que verifiquen quien es el siguiente en la cola.
 */
static void 
barberia(void *n_)
{
  int *id = (int *) n_;
	while (1)
	{
		//cola_concurrente_push(concCola, id);
    queue->Append(*id);
		//sem_post(&hayClientes);
    newCli->V();
		//pthread_mutex_lock(&esperandoLk);
    waitingLk->Acquire();

		while (no_soy_siguiente(*id))
		{
			//pthread_cond_wait(&esperando, &esperandoLk);
      waiting->Wait();
		}
		me_cortan(*id);
		//sem_post(&esperarCliente);
    waitCli->V();
		//sem_wait(&esperarBarbero);
		currentThread->Yield();
    waitBarber->P();
		pagando(*id);
		//sem_post(&esperarCliente);
    waitCli->V();
		//sem_wait(&esperarBarbero);
		currentThread->Yield();
    waitBarber->P();
		//pthread_mutex_unlock(&esperandoLk);
    waitingLk->Release();
	}
}

void ThreadTestBarber()
{
	queue = new SynchList<int>;
	
  waitCli = new Semaphore("wait clients", 0);
  waitBarber = new Semaphore("wait barber", 0);
  newCli = new Semaphore("there're clients", 0);

	waitingLk = new Lock("lockCond");
	waiting = new Condition("waiting", waitingLk);

  char **names = new char*[CLI];
  unsigned *values = new unsigned[CLI];

  for (unsigned i = 0; i < CLI; i++) {
      printf("Launching client %u.\n", i);
      names[i] = new char[16];
      sprintf(names[i], "Producer %u", i);
      printf("Name: %s\n", names[i]);
      values[i] = i;
      Thread *t = new Thread(names[i]);
      t->Fork(barberia, (void *) &(values[i]));
  }

  printf("Launching barber. \n");
  Thread *t = new Thread("barber");
  t->Fork(trabajando, NULL);

  while (1) {
      currentThread->Yield();
  }
	return;
}