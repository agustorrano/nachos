/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "threads/system.hh"
#include "swap.hh"

#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    executableFile = executable_file;

    ASSERT(executableFile != nullptr);

    Executable exe (executableFile);
    ASSERT(exe.CheckMagic());


    // How big is address space?
    unsigned size = exe.GetSize() + USER_STACK_SIZE;

    // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    #ifdef USE_SWAP
    swapName = new char [FILE_NAME_MAX_LEN + 1];
    swapMap = new Bitmap(numPages);
    #else
        ASSERT(numPages <= memBitMap->CountClear());
    #endif

    // Check we are not trying to run anything too big -- at least until we
    // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    // First, set up the translation.

    pageTable = new TranslationEntry[numPages];
    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;

        #ifndef USE_DEMANDLOADING
        #ifndef USE_SWAP
        int x = memBitMap->Find();
        if (x == -1) {
            DEBUG('a', "Error: there are no free physical pages.\n");
            break;
        }

        #else 
        int x = memCoreMap->Find(currentThread->space, i);
        if (x == -1) { 
            x = DoSwapOut();
            DEBUG('w', "Number of frame %d.\n", x);
        }
        #endif

        pageTable[i].physicalPage = x;
        pageTable[i].valid        = true;

        #else 
        pageTable[i].valid        = false;
        #endif

        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }

    char *mainMemory = machine->mainMemory;
    for (unsigned i = 0; i < numPages; i++) 
        memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE);
    
    codeSize = exe.GetCodeSize();
    initDataSize = exe.GetInitDataSize();
    codeVAddr = exe.GetCodeAddr();
    initDataVAddr = exe.GetInitDataAddr();
    DEBUG('a', "codeSize: %d, initDataSize: %d.\n", codeSize, initDataSize);
    DEBUG('a', "codeAddr: %d, initDataAddr: %d.\n", codeVAddr, initDataVAddr);

    #ifndef USE_DEMANDLOADING

    // Zero out the entire address space, to zero the unitialized data
    // segment and the stack segment.
    //
    // Then, copy in the code and data segments into memory.

    if (codeSize > 0) {
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              codeVAddr, codeSize);

        for (uint32_t i = 0; i < codeSize; i++) {
            int virtPage = DivRoundDown(codeVAddr + i, PAGE_SIZE);
            int physPage = pageTable[virtPage].physicalPage;
            int physAddr = physPage*PAGE_SIZE + (codeVAddr + i)%PAGE_SIZE;
            exe.ReadCodeBlock(&mainMemory[physAddr], 1, i);
        }
    }

    if (initDataSize > 0) {
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              initDataVAddr, initDataSize);
        
        for (uint32_t i = 0; i < initDataSize; i++) {
            int virtPage = DivRoundDown(initDataVAddr + i, PAGE_SIZE);
            int physPage = pageTable[virtPage].physicalPage;
            int physAddr = physPage*PAGE_SIZE + (initDataVAddr + i)%PAGE_SIZE;
            exe.ReadDataBlock(&mainMemory[physAddr], 1, i);
        }
    }
    #endif
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    #ifdef USE_SWAP
    for (unsigned i = 0; i < numPages; i++) {
        memCoreMap->Clear(pageTable[i].physicalPage);
    }
    fileSystem->Remove(swapName);
    delete swapFile;
    delete swapMap;

    #else
    for (unsigned i = 0; i < numPages; i++) {
        memBitMap->Clear(pageTable[i].physicalPage);
    }
    #endif

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
{   
    #ifdef USE_TLB
    //machine->GetMMU()->PrintTLB();
    TranslationEntry page;
    for (unsigned i = 0; i < TLB_SIZE; i++) {
        page = machine->GetMMU()->tlb[i];
        if (page.valid) {
            pageTable[page.virtualPage].use = page.use;
            pageTable[page.virtualPage].dirty = page.dirty;
        }
    }
    #endif
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.

void
AddressSpace::RestoreState()
{   
    #ifdef USE_TLB
    for (unsigned i = 0; i < TLB_SIZE; i++)
        machine->GetMMU()->tlb[i].valid = false;

    #else
    machine->GetMMU()->pageTable     = pageTable;
    machine->GetMMU()->pageTableSize = numPages;
    #endif
}


TranslationEntry* 
AddressSpace::GetPageTable() 
{
   return pageTable;
}

unsigned 
AddressSpace::GetNumPages()
{
    return numPages;
}

 
TranslationEntry
AddressSpace::CheckPageinMemory(uint32_t vpn)
{
    int flag = 0;
    if (!pageTable[vpn].valid) { // page's not in memory
        #ifdef USE_SWAP 
            flag = !swapMap->Test(vpn);
        #else
            flag = 1; // si no hay swap y no esta en memoria si o si hay que cargarla
        #endif
        if (flag) { // page's not in memory nor swap
            DEBUG('a',"Loading VPN %d into memory.\n", vpn);

            #ifdef USE_SWAP
            int physPage = memCoreMap->Find(currentThread->space, vpn);
            if (physPage == -1) {
                physPage = DoSwapOut();
                DEBUG('a', "SWAP: number of frame %d.\n", physPage);
            }

            #else 
            int physPage = memBitMap->Find();
            if (physPage == -1) {
                DEBUG('a', "Error: there are no free physical pages.\n");
                ASSERT(0);
            }
            #endif

            pageTable[vpn].physicalPage = physPage;   
            pageTable[vpn].valid = true;

            uint32_t virtAddr = vpn * PAGE_SIZE; // direcc donde comienza la pag a cargar en memoria
            char *mainMemory = machine->mainMemory;

            /* de donde saco el ejecutable? */
            ASSERT(executableFile != nullptr);
            Executable exe (executableFile);
            ASSERT(exe.CheckMagic());

            uint32_t codeVAddrFinal = codeVAddr + codeSize;
            uint32_t initDataVAddrFinal = initDataVAddr + initDataSize;

            uint32_t j;
            uint32_t readCode = 0;
            uint32_t readData = 0;

            if (virtAddr >= codeVAddr && virtAddr < codeVAddrFinal)
            {
                DEBUG('a', "Writing code from VPN [%d] into PPN [%d].\n", vpn, physPage);
                // chequeo si toda la pagina es de codigo
                if (virtAddr + PAGE_SIZE < codeVAddrFinal) j = PAGE_SIZE; // leo toda la pagina
                else j = codeVAddrFinal - virtAddr; // leo hasta el final del segmento de codigo
                for (; readCode < j; readCode++) {
                    int offset = (virtAddr + readCode)%PAGE_SIZE;
                    int physAddr = physPage*PAGE_SIZE + offset;
                    exe.ReadCodeBlock(&mainMemory[physAddr], 1, (virtAddr + readCode - codeVAddr));
                }
                virtAddr = virtAddr + readCode; // actualizo a donde quiero seguir leyendo
            }

            if (virtAddr >= initDataVAddr  && virtAddr < initDataVAddrFinal)
            {
                DEBUG('a', "Writing data from VPN [%d] into PPN [%d].\n", vpn, physPage);
                // calculo la parte de la pagina que es de datos:
                if (virtAddr + (PAGE_SIZE - readCode) < initDataVAddrFinal) j = (PAGE_SIZE - readCode); 
                else j = initDataVAddrFinal - virtAddr;
                for (; readData < j; readData++) {
                    int offset = (virtAddr + readData)%PAGE_SIZE;
                    int physAddr = physPage*PAGE_SIZE + offset;
                    exe.ReadDataBlock(&mainMemory[physAddr], 1, (virtAddr + readData - initDataVAddr));
                }
                virtAddr = virtAddr + readData; // actualizo a donde quiero seguir leyendo
            }

            if (virtAddr > (codeSize + initDataSize)) // pertenece a la stack
            {
                DEBUG('a', "In stack segment. VPN: [%d], PPN: [%d].\n", vpn, physPage);
                // hay que rellenar con 0's la pagina que pedimos antes
                for (uint32_t i = 0; i < PAGE_SIZE; i++) {
                    int physAddr = physPage*PAGE_SIZE + (virtAddr + i)%PAGE_SIZE;
                    memset(&mainMemory[physAddr], 0, PAGE_SIZE);
                }
            }
        }
        else { // page is in swap 
            #ifdef USE_SWAP
            DEBUG('w', "PAGE IN SWAP.\n");
            DoSwapIn(this, vpn);
            pageTable[vpn].valid = true;
            #endif
        }
    }
    return pageTable[vpn];
}