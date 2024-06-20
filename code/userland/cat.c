#include "../userprog/syscall.h"
#include "lib.c"

int main(int argc, char *argv[])
{
  // Tiene que haber dos argumuentos
  // argv[0] es el programa y argv[1] es el archivo que le pasamos a cat
  if (argc < 2) {
    putss("Error: invalid number of arguments.\n");
    return -1;
  }

  OpenFileId o = Open(argv[1]);
  if (o == -1) {
    putss("Error: open.\n");
    return -1;
  }

  char buffer[1];
  // buffer[1] = '\0';
  while(Read(buffer, 1, o)) {
    // putss("reading\n");
    Write(buffer, 1, 1);
  }

  Write("\n", 1, 1);
  
  Close(o);

  return 1;
}