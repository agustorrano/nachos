#ifndef NACHOS_SWAP__HH
#define NACHOS_SWAP__HH

#include "address_space.hh"

int PickVictim(AddressSpace* space, unsigned vpn);
void DoSwapIn(AddressSpace* space, unsigned vpn);
int DoSwapOut();

#endif