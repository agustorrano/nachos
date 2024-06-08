#include <stdio.h>
#include "coremap.hh"

Coremap::Coremap(unsigned nitems)
{
    ASSERT(nitems > 0);
    
    frames = new Bitmap(nitems);
    spaces = new AddressSpace*[nitems];
    vpns = new unsigned[nitems];
    coremapSize = nitems;
}

Coremap::~Coremap()
{
    delete frames;
    delete [] spaces;
    delete [] vpns;
}

void
Coremap::Mark(unsigned which, AddressSpace *addrSpace, unsigned vpn)
{
    frames->Mark(which);
    spaces[which] = addrSpace;
    vpns[which] = vpn;
}

void
Coremap::Clear(unsigned which)
{
    frames->Clear(which);
    spaces[which] = nullptr;

    // que hago con los vpns? 
    // los seteo en -1 o los dejo como estan?
}

bool
Coremap::Test(unsigned which) const
{
    return frames->Test(which) && spaces[which] != nullptr;
}

int 
Coremap::Find(AddressSpace *addrSpace, unsigned vpn)
{
    int which = frames->Find();
    spaces[which] = addrSpace;
    vpns[which] = vpn;
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
            printf("Address Space: %p. Virtual Page Number: %d.\n", spaces[i], vpns[i]);
        }
    }
}
