#ifdef USE_SWAP
#include "swap.hh"
#include "address_space.hh"
#include "threads/system.hh"

int PickVictim(AddressSpace** spaceDir, unsigned* vpnDir) 
{
    int victim;
    unsigned numPhysPages = machine->GetNumPhysicalPages();
    #ifdef PRPOLICY_FIFO
    DEBUG('w', "Pick Victim FIFO.\n");
    ASSERT(!memCoreMap->fifoFrames->IsEmpty())
    victim = memCoreMap->fifoFrames->Pop();
    memCoreMap->fifoFrames->Append(victim);
    
    #elif PRPOLICY_CLOCK
    DEBUG('w', "Pick Victim CLOCK.\n");
    int frame;
    for (int clock = 0; clock < 4; clock++) {
        int size = memCoreMap->clockFrames->GetSizeList()
        for (int i = 0; i < size; i++) {
            frame = memCoreMap->clockFrames->Head();
            memCoreMap->CheckFrame(frame, spaceDir, vpnDir);
            AddressSpace *space = *spaceDir;
            unsigned vpn = *vpnDir;
            if (clock == 0 || clock == 2) {
                if (space->pageTable[vpn].use == 0 && space->pageTable[vpn].dirty == 0) {
                    victim = frame;
                    return victim;
                }
            }
            else if (clock == 1) {
                if (space->pageTable[vpn].use == 0 && space->pageTable[vpn].dirty == 1) {
                    victim = frame;
                    return victim;
                }
                else {
                    space->pageTable[vpn].use = 0;
                    memCoreMap->fifoFrames->Pop();
                    memCoreMap->fifoFrames->Append(frame);
                }
            }
            else { // clock == 3
                victim = frame;
                return victim;
            }
        }
    }

    #else
    DEBUG('w', "Pick Victim RANDOM.\n");
    // sleep(1); por las dudas
    victim = random() % numPhysPages;
    #endif
    memCoreMap->CheckFrame(victim, spaceDir, vpnDir);
    DEBUG('w', "Victim picked: Frame: %d, Vpn: %d.\n", victim, *vpnDir);
    return victim;
}


int DoSwapOut()
{
    DEBUG('w', "Swap Out.\n");
    stats->numSwapOut++;
    AddressSpace *space;
    unsigned vpn;
    int frame = PickVictim(&space, &vpn);
    char *mainMemory = machine->mainMemory;
    TranslationEntry *pageTable = space->GetPageTable();
    if (pageTable[vpn].dirty || !space->swapMap->Test(vpn)) {
      DEBUG('w', "Really writing to swap.\n");  
      space->swapFile->WriteAt(&mainMemory[frame * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
      DEBUG('w', "after write At.\n");
      space->swapMap->Mark(vpn);
    }
    return frame;
}

void DoSwapIn(AddressSpace* space, unsigned vpn)
{
  DEBUG('w', "Swap In.\n");
  stats->numSwapIn++;
  int physPage = memCoreMap->Find(currentThread->space, vpn);
  if (physPage == -1) physPage = DoSwapOut();
  char *mainMemory = machine->mainMemory;
  space->swapFile->ReadAt(&mainMemory[physPage * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
}

#endif