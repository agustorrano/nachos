#include "userprog/syscall.h"

unsigned strlen (const char *s) 
{
  unsigned i;
  for (i = 0; s[i] != 0; i++);
  return i;
}

void puts (const char *s)
{
  Write(s, strlen(s), CONSOLE_OUTPUT);
}

void itoa (int n , char *str);