/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"
//#include "lib/debug.hh"

int
main(void)
{
    Create("test.txt");
    OpenFileId o = Open("test.txt");
    Remove("test.txt");
    Create("test2.txt");
    OpenFileId o2 = Open("test2.txt");
    return 0;
}
