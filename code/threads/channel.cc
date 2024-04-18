#include "channel.hh"

Channel::Channel(const char *debugName)
{
    name = debugName;
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
    semSend->P();
    *buffer = message;
    semReceive->V();
    lockSend->Release();
}

void
Channel::Receive(int *message)
{
    lockReceive->Acquire();
    semSend->V();
    message = buffer;
    semReceive->P();
    lockReceive->Release();
}