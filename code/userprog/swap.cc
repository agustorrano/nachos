#ifdef USE_SWAP
#include "swap.hh"
#include "address_space.hh"
#include "threads/system.hh"

int PickVictim(AddressSpace** spaceDir, unsigned* vpnDir) 
{
    int victim;
    unsigned numPhysPages = machine->GetNumPhysicalPages();
    #ifdef PRPOLICY_FIFO
    ASSERT(!memCoreMap->fifoFrames->IsEmpty())
    victim = memCoreMap->fifoFrames->Pop();
    memCoreMap->fifoFrames->Append(victim);
    // quiero hacer un pop? porque lo estoy sacando de la fifo. no deberia pasarlo al final?
    #elif PRPOLICY_CLOCK
    int frame, i;
    for (int clock = 0; clock < 4; clock++) {
        // esto esta mal. estamos recorriendo la memoria 
        // secuencialmente en vez de usar clockFrames
        for (frame = memCoreMap->clockFrames->Head(), i = 0; i < memCoreMap->clockFrames->GetSizeList(); frame = memCoreMap->clockFrames->Head(), i++) {
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
    // sleep(1); por las dudas
    victim = random() % numPhysPages;
    #endif
    memCoreMap->CheckFrame(victim, spaceDir, vpnDir);
    return victim;
}


int DoSwapOut()
{
    stats->numSwapOut++;
    AddressSpace *space;
    unsigned vpn;
    int frame = PickVictim(&space, &vpn);
    char *mainMemory = machine->mainMemory;
    TranslationEntry *pageTable = space->GetPageTable();
    if (pageTable[vpn].dirty || !space->swapMap->Test(vpn)) {
      space->swapFile->WriteAt(&mainMemory[frame * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
      space->swapMap->Mark(vpn);
    }
    return frame;
}

void DoSwapIn(AddressSpace* space, unsigned vpn)
{
  stats->numSwapIn++;
  int physPage = memCoreMap->Find(currentThread->space, vpn);
  if (physPage == -1) physPage = DoSwapOut();
  char *mainMemory = machine->mainMemory;
  space->swapFile->ReadAt(&mainMemory[physPage * PAGE_SIZE], PAGE_SIZE, vpn * PAGE_SIZE);
}

#endif