#include "lib.c"

int main(int argc, char *argv[])
{
  // Tiene que haber al menos dos argumuentos
  if (argc < 2) {
    putss("Error: invalid number of arguments.\n");
    return -1;
  }

  for (int i = 1; i < argc; i++) {
    if (Remove(argv[i]) < 0) {
      putss("Error: remove.\n");
    }
  }

  return 0;
}