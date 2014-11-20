#include "report.h"

#include <stdio.h>
#include <fcntl.h>

int main (int argc, char**argv) {

  unsigned char *bytes;
  int len;
  int i;

  bytes = (unsigned char*)ReportDescriptor;
  len = sizeof(ReportDescriptor);

  for (i = 0; i < len; i++) {
    
    if (i > 0 && (i % 4) == 0)
      printf(" ");

    if (bytes[i] < 16)
      printf("0%X", bytes[i]);
    else
      printf("%X", bytes[i]);
  }

  
  return 0;
}
