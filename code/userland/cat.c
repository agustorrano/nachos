#include "lib.c"

int main(int argc, char *argv[])
{
  // Tiene que haber dos argumuentos
  // argv[0] es el programa y argv[1] es el archivo que le pasamos a cat
  if (argc != 2) {
    puts("Error: invalid number of arguments.\n");
    return -1;
  }

  OpenFileId o = Open(argv[1]);
  if (o == -1) {
    puts("Error: open.\n");
    return -1;
  }

  char buffer[1];
  while(Read(buffer, 1, o) > 0)
    puts(buffer);
  
  Close(o);

  return 0;
}