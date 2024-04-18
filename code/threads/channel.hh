#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "lock.hh"

class Channel {
public:

    Channel(const char *debugName);

    ~Channel();

    const char *GetName() const;

    void Send(int message);
    void Receive(int *message);

private:

    const char *name;

    int *buffer;

    Semaphore *semReceive, *semSend;

    Lock *lockReceive, *lockSend;
};

#endif
