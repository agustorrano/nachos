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

void reverse (char str[], int length)
{
  int start = 0;
  int end = length - 1;
  while (start < end) {
    char temp = str[start];
    str[start] = str[end];
    str[end] = temp;
    end--;
    start++;
  }
}

void itoa (int n , char *str) 
{
  if (n == 0) {
    *str = '0';
    *str++ = '\0';
    return;
  }

  int flagNegative = 0;
  if (n < 0) {
    flagNegative = 1;
    n = -n;
  }
  
  int i = 0;
  while (n != 0) {
    int c = n % 10;
    str[i++] = c + '0';
    n = n / 10;
  }
  str[i] = '\0';
  if (flagNegative) str[i++] = '-';
  reverse(str, i);
  return;
}