#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH


#include "syscall.h"

class CoremapEntry {
public:
    // The identification number for the addr space where the physical page belongs
    SpaceId sid;    
    
    /// The page number in virtual memory. en realidad creo que aloja varias, como seria?
    unsigned virtualPageNumber;

    /// Print contents of coremap entry.
    void Print();

private:

};


#endif
