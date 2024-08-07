#ifdef USE_SWAP
#include "swap.hh"
#include "address_space.hh"
#include "threads/system.hh"
#include <stdio.h>

int PickVictim(AddressSpace** spaceDir, unsigned* vpnDir) 
{
    int victim;
    #ifdef PRPOLICY_FIFO
    //DEBUG('w', "Pick Victim FIFO.\n");
    ASSERT(!memCoreMap->fifoFrames->IsEmpty());
    victim = memCoreMap->fifoFrames->Pop();
    memCoreMap->fifoFrames->Append(victim);
    
    #elif PRPOLICY_CLOCK
    //DEBUG('w', "Pick Victim CLOCK.\n");
    unsigned numPhysPages = machine->GetNumPhysicalPages();
    int frame;
    for (int clock = 0; clock < 4; clock++) {
        for (unsigned i = 0; i < numPhysPages; i++) {
            frame = memCoreMap->clockFrames->Head();
            memCoreMap->CheckFrame(frame, spaceDir, vpnDir);
            AddressSpace *space = *spaceDir;
            unsigned vpn = *vpnDir;
            TranslationEntry* pageTable = space->GetPageTable();
            if (clock == 0 || clock == 2) {
                if (pageTable[vpn].use == 0 && pageTable[vpn].dirty == 0) {
                    victim = frame;
                    return victim;
                }
            }
            else if (clock == 1) {
                if (pageTable[vpn].use == 0 && pageTable[vpn].dirty == 1) {
                    victim = frame;
                    return victim;
                }
                else {
                    pageTable[vpn].use = 0;
                    memCoreMap->clockFrames->Pop();
                    memCoreMap->clockFrames->Append(frame);
                }
            }
            else { // clock == 3
                victim = frame;
                return victim;
            }
        }
    }

    #else
    // DEBUG('w', "Pick Victim RANDOM.\n");
    unsigned numPhysPages = machine->GetNumPhysicalPages();
    victim = random() % numPhysPages;
    #endif
    memCoreMap->CheckFrame(victim, spaceDir, vpnDir);
    DEBUG('w', "Victim picked: Frame: %d, Vpn: %d.\n", victim, *vpnDir);
    return victim;
}


int DoSwapOut()
{
    stats->numSwapOut++;
    AddressSpace *space;
    unsigned vpn;
    int frame = PickVictim(&space, &vpn);
    DEBUG('w', "Swap Out. Save VPN: %d, from PPN: %d.\n", vpn, frame);
    char *mainMemory = machine->mainMemory;
    TranslationEntry *pageTable = space->GetPageTable();

    // escribir la pagina en swap
    if (pageTable[vpn].dirty || !space->swapMap->Test(vpn)) {
      // DEBUG('w', "Really writing to swap.\n");  
      space->swapFile->WriteAt(&mainMemory[frame * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
      space->swapMap->Mark(vpn);
    }

    // actualizar la tabla del proceso al que pertenece
    pageTable[vpn].valid = false;

    // actualizar la tlb
    for (unsigned i = 0; i < TLB_SIZE; i++) {
        if (machine->GetMMU()->tlb[i].virtualPage == vpn) {
            machine->GetMMU()->tlb[i].valid = false;
        }
    }
    return frame;
}

int DoSwapIn(unsigned vpn)
{
  stats->numSwapIn++;
  int physPage = memCoreMap->Find(currentThread->space, vpn);
  if (physPage == -1) {
    physPage = DoSwapOut();
    memCoreMap->Mark(physPage, currentThread->space, vpn);
  }
  DEBUG('w', "Swap In: Bring VPN: %d, to PPN: %d.\n", vpn, physPage);
  char *mainMemory = machine->mainMemory;
  currentThread->space->swapFile->ReadAt(&mainMemory[physPage * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
  return physPage;
}


/* void PrintPageTable(AddressSpace* space) {
    TranslationEntry* pageTable = space->GetPageTable();
    int size = space->GetNumPages();
    for (int i = 0; i < size; i++) {
        TranslationEntry p = pageTable[i];
        printf("[%d]: PhysPage Number: %d, valid: %d, use: %d, dirty: %d.\n", i, p.physicalPage, p.valid, p.use, p.dirty);
    }
} */

#endif