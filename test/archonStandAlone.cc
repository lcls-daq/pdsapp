#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pds/archon/Driver.hh"

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("usage %s <filename>\n", argv[0]);
    return 1;
  }

  Pds::Archon::Driver drv = Pds::Archon::Driver("10.0.0.2", 4242);
  if(drv.configure(argv[1])) {
    printf("%llu\n", drv.time());
    sleep(5);
    printf("%llu\n", drv.time());
    sleep(5);
    printf("%llu\n", drv.time());
  }

  return 0;
}
