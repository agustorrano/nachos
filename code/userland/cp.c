#include "lib.c"
#define FILE_NAME_MAX_LEN 9 // esta definido pero no se como incluirlo
                            // en directory_entry.hh
int main(int argc, char *argv[])
{
  // Tiene que haber al menos dos argumuentos
  if (argc < 2) {
    putss("Error: invalid number of arguments.\n");
    return -1;
  }
  char* fileName = argv[0];
  char* copyName = argv[1];
  
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
  char buffer[FILE_NAME_MAX_LEN];
  int bytesRead = Read(buffer, FILE_NAME_MAX_LEN, of);
  Write(buffer, bytesRead, oc);
  Close(of);
  Close(oc);
  return 0;
}