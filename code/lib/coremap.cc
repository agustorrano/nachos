
#include <stdio.h>
#include "coremap.hh"

CoremapEntry::CoremapEntry()
{
  sid = -1;
  virtualPageNumber = -1;
}

/*
CoremapEntry::~CoremapEntry()
{
} */

void 
CoremapEntry::Print() 
{
  printf("Address Space: %d. Virtual Page Number: %d.\n", sid, virtualPageNumber);
}
