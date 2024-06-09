#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH


#include "syscall.h"
#include "bitmap.hh"
#include "utility.hh"
#include "userprog/address_space.hh"

class Coremap {
public:
    //// Initalize a coremap with `nitems` bits.
    Coremap(unsigned nitems);

    /// Uninitalize a coremap.
    ~Coremap();

    /// Set de "nth" bit.
    void Mark(unsigned which, AddressSpace *addrSpace, unsigned vpn);

    /// Clear the "nth" bit.
    void Clear(unsigned which);

    /// Is the "nth" bit set?
    bool Test(unsigned which) const;

    /// It finds a physical frame.
    int Find(AddressSpace *addrSpace, unsigned vpn);

    /// Return the number of clear bits.
    unsigned CountClear() const;

    /// Print contents of coremap entry.
    void Print();

private:

    Bitmap *frames;

    // An array of addr spaces. The nth-physpage in the bitmap belongs to the nth-addressspace
    AddressSpace **spaces;   
    
    // An array of virtual page numbers. The nth-physpage in the bitmap matches the nth-virtualpage
    unsigned *vpns;

    unsigned coremapSize;
};


#endif
