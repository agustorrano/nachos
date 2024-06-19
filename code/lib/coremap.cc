#include <stdio.h>
#include "coremap.hh"

Coremap::Coremap(unsigned nitems)
{
    ASSERT(nitems > 0);
    
    frames = new Bitmap(nitems);
    coremapSize = nitems;
    
    vpns = new unsigned[nitems];
    spaces = new AddressSpace*[nitems];
    
    #ifdef PRPOLICY_FIFO
    fifoFrames = new List <int>;
    #endif

    #ifdef PRPOLICY_CLOCK
    clockFrames = new List <int>;
    #endif
}

Coremap::~Coremap()
{
    delete frames;
    delete [] spaces;
    delete [] vpns;

    #ifdef PRPOLICY_FIFO
    delete fifoFrames;
    #endif

    #ifdef PRPOLICY_CLOCK
    delete clockFrames;
    #endif
}

void
Coremap::Mark(unsigned which, AddressSpace *addrSpace, unsigned vpn)
{
    frames->Mark(which);
    spaces[which] = addrSpace;
    vpns[which] = vpn;
    #ifdef PRPOLICY_FIFO
    fifoFrames->Update(which);
    #endif

    #ifdef PRPOLICY_CLOCK
    clockFrames->Update(which);
    #endif
}

void
Coremap::Clear(unsigned which)
{
    frames->Clear(which);
    spaces[which] = nullptr;
}

bool
Coremap::Test(unsigned which) const
{
    return frames->Test(which);
}

int 
Coremap::Find(AddressSpace *addrSpace, unsigned vpn)
{
    int which = frames->Find();

    if (which != -1) {
        #ifdef PRPOLICY_FIFO
        fifoFrames->Update(which);
        #endif

        #ifdef PRPOLICY_CLOCK
        clockFrames->Update(which);
        #endif
        spaces[which] = addrSpace;
        vpns[which] = vpn;
    }
    return which;
}

unsigned 
Coremap::CountClear() const
{
    return frames->CountClear();
}

void 
Coremap::Print() 
{
    for (unsigned i = 0; i < coremapSize; i++) {
        if (Test(i)) {
            printf("[%d]: Address Space: %p. Virtual Page Number: %d.\n", i, spaces[i], vpns[i]);
        }
        else {
            printf("[%d]: Empty.\n", i);
        }
    }
}

void 
Coremap::CheckFrame(unsigned which, AddressSpace **addrSpace, unsigned *vpn)
{
    ASSERT(Test(which));
    *addrSpace = spaces[which];
    *vpn = vpns[which];
}

#ifdef PRPOLICY_FIFO
void
Coremap::PrintList()
{
    printf ("List: ");
    fifoFrames->Apply(printInt);
    printf("\n \n");
}

void 
Coremap::printInt(int x) {
    printf("%d ", x);
}

#endif