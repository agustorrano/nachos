#ifdef USE_SWAP
#ifndef NACHOS_SWAP__HH
#define NACHOS_SWAP__HH

#include <stdlib.h>

class AddressSpace;

int PickVictim(AddressSpace** space, unsigned* vpn);
void DoSwapIn(AddressSpace* space, unsigned vpn);
int DoSwapOut();

#endif
#endif