#include "thread_test_channels.hh"
#define DOJOIN 1

int NUM_MSGS = 10;
int NUM_RECEPTOR = 5;
int NUM_EMISOR = 5;
Channel *channel;

static void 
emisor(void *n_)
{
    unsigned *n = (unsigned *)n_;
    for (int i = 0; i < NUM_MSGS; i++)
    {
        channel->Send(i);
        printf("Emisor %u sent %d\n", *n, i);
    }
}

static void 
receptor(void *n_)
{
    unsigned *n = (unsigned *)n_;
    int *buf = new int;
    for (int i = 0; i < NUM_MSGS; i++)
    {
        channel->Receive(buf);
        printf("Receptor %u received %d\n", *n, *buf);
    }
}

void 
ThreadTestChannels()
{
    channel = new Channel("canal de mensajes");
    char **namesE = new char *[NUM_EMISOR];
    unsigned *valuesE = new unsigned[NUM_EMISOR];
    List<Thread *> *threads;
    threads = new List<Thread *>;

    for (int i = 0; i < NUM_EMISOR; i++)
    {
        printf("Launching emisor %u.\n", i);
        namesE[i] = new char[16];
        sprintf(namesE[i], "Emisor %u", i);
        printf("Name: %s\n", namesE[i]);
        valuesE[i] = i;
        Thread *t = new Thread(namesE[i], DOJOIN);
        threads->Append(t);
        t->Fork(emisor, (void *)&(valuesE[i]));
    }

    char **namesR = new char *[NUM_RECEPTOR];
    unsigned *valuesR = new unsigned[NUM_RECEPTOR];

    for (int i = 0; i < NUM_RECEPTOR; i++)
    {
        printf("Launching receptor %u.\n", i);
        namesR[i] = new char[16];
        sprintf(namesR[i], "Receptor %u", i);
        printf("Name: %s\n", namesR[i]);
        valuesR[i] = i;
        Thread *t = new Thread(namesR[i], DOJOIN);
        threads->Append(t);
        t->Fork(receptor, (void *)&(valuesR[i]));
    }

    Thread *t;
    for (int i = 0; i < NUM_EMISOR + NUM_RECEPTOR; i++)
    {
        t = threads->Pop();
        t->Join();
    }

    printf("All Threads Finished!\n");

    for (int i = 0; i < NUM_EMISOR; i++)
    {
        delete[] namesE[i];
    }
    for (int i = 0; i < NUM_RECEPTOR; i++)
    {
        delete[] namesR[i];
    }
    delete[] valuesE;
    delete[] valuesR;
    delete[] namesE;
    delete[] namesR;
}