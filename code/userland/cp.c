#include "lib.c"

int main(int argc, char *argv[])
{
  // Tiene que haber al menos dos argumuentos
  if (argc < 3) {
    putss("Error: invalid number of arguments.\n");
    return -1;
  }
  char* fileName = argv[1];
  char* copyName = argv[2];
  
  OpenFileId of = Open(fileName);
  if (of == -1) {
    putss("Error: open.\n");
    return -1;
  }
  
  int r = Create(copyName);
  if (r == -1) {
    putss("Error: create.\n");
    return -1;
  }
  OpenFileId oc = Open(copyName);
  if (oc == -1) {
    putss("Error: open.\n");
    return -1;
  }
  char buffer[1];
  while (Read(buffer, 1, of) > 0)
    Write(buffer, 1, oc);
  
  Close(of);
  Close(oc);
  return 1;
}