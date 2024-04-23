
#include "thread_test_channels.hh"

static bool done[2];
int NUM_MSGS = 10;
int NUM_RECEPTOR = 5;
int NUM_EMISOR = 5;

Channel* channel;

static void emisor(void *n_) {
  unsigned *n = (unsigned *) n_;
  DEBUG('c', "Thread [%d]. Starting emisor.\n", *n);
  for(int i = 0; i < NUM_MSGS; i++) {
    channel->Send(i);
  }
  done[*n] = true;
}

static void receptor(void *n_) {
  unsigned *n = (unsigned *) n_; 
  DEBUG('c', "Thread [%d]. Starting receptor.\n", *n);
  int* buf = new int;
  for(int i = 0; i < NUM_MSGS; i++) {
    channel->Receive(buf);
  }
  done[*n + NUM_EMISOR] = true;
}

void
ThreadTestChannels()
{
  channel = new Channel("canal de mensajes");
  char **namesE = new char*[NUM_EMISOR];
  unsigned *valuesE = new unsigned[NUM_EMISOR];

  for (int i = 0; i < NUM_EMISOR; i++) {
        printf("Launching emisor %u.\n", i);
        namesE[i] = new char[16];
        sprintf(namesE[i], "Emisor %u", i);
        printf("Name: %s\n", namesE[i]);
        valuesE[i] = i;
        Thread *t = new Thread(namesE[i]);
        t->Fork(emisor, (void *) &(valuesE[i]));
    }

    char **namesR = new char*[NUM_RECEPTOR];
    unsigned *valuesR = new unsigned[NUM_RECEPTOR];

    for (int i = 0; i < NUM_RECEPTOR; i++) {
        printf("Launching receptor %u.\n", i);
        namesR[i] = new char[16];
        sprintf(namesR[i], "Receptor %u", i);
        printf("Name: %s\n", namesR[i]);
        valuesR[i] = i;
        Thread *t = new Thread(namesR[i]);
        t->Fork(receptor, (void *) &(valuesR[i]));
    }  
    
  // Wait until all turnstile threads finish their work.  `Thread::Join` is
  // not implemented at the beginning, therefore an ad-hoc workaround is
  // applied here.
  for (int i = 0; i < NUM_EMISOR + NUM_RECEPTOR; i++) {
      while (!done[i]) {
          currentThread->Yield();
      }
  }
  DEBUG('c', "All Threads Finished!\n");
  //for (unsigned i = 0; i < NUM_EMISOR; i++) {
	//  delete[] namesE[i];
  //}
  //for (unsigned i = 0; i < NUM_RECEPTOR; i++) {
	//  delete[] namesR[i];
  //}
  //delete []valuesE;
  //delete []valuesR;
  //delete []namesE;
  //delete []namesR;
}