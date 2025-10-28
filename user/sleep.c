#include "kernel/types.h"
#include "user/user.h"

void 
main(int argc, char *argv[])
{
  int n;
  if (argc != 2) {
    fprintf(2, "usage: sleep ticks \n");
    exit(1);
  }
  n = atoi(argv[1]);
  pause(n);  

  exit(0);
}
