#include "../userprog/syscall.h"
#include "lib.c"

int main(int argc, char *argv[])
{
  // No recibe argumentos
  if (argc > 1) {
    putss("Error: invalid number of arguments.\n");
    return -1;
  }

  Ls();

  return 0;
}