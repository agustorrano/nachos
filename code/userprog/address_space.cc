/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"

#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{

    ASSERT(executable_file != nullptr);

    Executable exe (executable_file);
    ASSERT(exe.CheckMagic());

    // How big is address space?

    unsigned size = exe.GetSize() + USER_STACK_SIZE;

    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;
  
    ASSERT(numPages <= machine->freeMap->CountClear());
    //ASSERT(numPages <= machine->GetNumPhysicalPages());
      // Check we are not trying to run anything too big -- at least until we
      // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        int x = machine->freeMap->Find();
        if (x == -1)
          DEBUG('e', "Error"); // deberia ser un assert ?
        else
            pageTable[i].physicalPage = x;
        // For now, virtual page number = physical page number.
        //pageTable[i].physicalPage = i;
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }
    char *mainMemory = machine->mainMemory;

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.
    //
    for (unsigned i = 0; i < numPages; i++) 
        memset(&mainMemory[pageTable[i].physicalPage], 0, 1);
    //puede mejorarse, en realidad solo es necesario de stack y uninitdata
    // memset(mainMemory, 0, size);
    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe.GetCodeSize();
    uint32_t initDataSize = exe.GetInitDataSize();
    if (codeSize > 0) {
        uint32_t virtualAddr = exe.GetCodeAddr();
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codeSize);

        for (uint32_t i = 0; i < codeSize; i++) {
            int virtPage = DivRoundDown(virtualAddr + i, PAGE_SIZE);
            int physPage = pageTable[virtPage].physicalPage;
            int physAddr = physPage*PAGE_SIZE + (virtualAddr + i)%PAGE_SIZE;
            DEBUG('q', "CODE. virtPage: %d, physPage: %d, physAddr: %X.\n", virtPage, physPage, physAddr);
            exe.ReadCodeBlock(&mainMemory[physAddr], 1, i);
        }
    }
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe.GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        for (uint32_t i = 0; i < initDataSize; i++) {
            int virtPage = DivRoundDown(virtualAddr + i, PAGE_SIZE);
            int physPage = pageTable[virtPage].physicalPage;
            int physAddr = physPage*PAGE_SIZE + (virtualAddr + i)%PAGE_SIZE;
            DEBUG('q', "DATA. virtPage: %d, physPage: %d, physAddr: %X.\n", virtPage, physPage, physAddr);
            exe.ReadDataBlock(&mainMemory[physAddr], 1, i);
        }
    }

}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
}
