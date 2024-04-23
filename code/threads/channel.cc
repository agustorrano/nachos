#include "channel.hh"

Channel::Channel(const char *debugName)
{
    name = debugName;
    buffer = new int;
    semReceive = new Semaphore("semReceive", 0);
    semSend = new Semaphore("semSend", 0);
    lockReceive = new Lock("lockReceive");
    lockSend = new Lock("lockSend");
}

Channel::~Channel()
{
    delete semReceive;
    delete semSend;
    delete lockReceive;
    delete lockSend;
}

const char *
Channel::GetName() const
{
    return name;
}

void
Channel::Send(int message)
{
    lockSend->Acquire();
    *buffer = message;
    DEBUG('c', "Emisor send %d.\n", message);
    semSend->V();
    semReceive->P();
    lockSend->Release();
}

void
Channel::Receive(int *message)
{
    lockReceive->Acquire();
    semSend->P();
    *message = *buffer;
    DEBUG('c', "Receptor received %d.\n", *message);
    semReceive->V();
    lockReceive->Release();
}