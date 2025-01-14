/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    unsigned count = 0;
    int temp;
    int maxIter = 3;
    int i;
    while (count < byteCount) {
        count++;
        //ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        for (i = 0; i < maxIter && !machine->ReadMem(userAddress, 1, &temp); i++);
        userAddress++;
        if (i == maxIter) ASSERT(0);
        *outBuffer = (unsigned char) temp;
        outBuffer++;
    }
    return;
}

bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    int maxIter = 3;
    int i;
    do {
        int temp;
        count++;
        //ASSERT(machine->ReadMem(userAddress++, 1, &temp));
        for (i = 0; i < maxIter && !machine->ReadMem(userAddress, 1, &temp); i++);
        userAddress++;
        if (i == maxIter) ASSERT(0);
        *outString = (unsigned char) temp;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    unsigned count = 0;
    int maxIter = 3;
    int i;
    while (count < byteCount) {
        int temp = (int) *buffer;
        count++;
        //ASSERT(machine->WriteMem(userAddress++, 1, temp));
        for (i = 0; i < maxIter && !machine->WriteMem(userAddress, 1, temp); i++);
        userAddress++;
        if (i == maxIter) ASSERT(0);
        buffer++;
    }
    return;
}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(string != nullptr);
    ASSERT(userAddress != 0);
    int maxIter = 3;
    int i;
    do {
        int temp = (int) *string;
        //ASSERT(machine->WriteMem(userAddress++, 1, temp));
        for (i = 0; i < maxIter && !machine->WriteMem(userAddress, 1, temp); i++);
        userAddress++;
        if (i == maxIter) ASSERT(0);
    } while (*string++ != '\0');
    return;
}
