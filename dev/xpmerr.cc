
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>
#include <new>

#include "pds/xpm/Module.hh"
#include "pds/xpm/RingBuffer.hh"

#include <string>

static inline double dtime(timespec& tsn, timespec& tso)
{
  return double(tsn.tv_sec-tso.tv_sec)+1.e-9*(double(tsn.tv_nsec)-double(tso.tv_nsec));
}

extern int optind;

using namespace Xpm;

void usage(const char* p) {
  printf("Usage: %s [-a <IP addr (dotted notation)>]\n",p);
}

void sigHandler( int signal ) {
  Xpm::Module* m = new(0) Xpm::Module;
  m->setL0Enabled(false);
  ::exit(signal);
}

int main(int argc, char** argv) {

  extern char* optarg;

  int c;
  bool lUsage = false;

  unsigned ip = 0xc0a8020a;
  unsigned short port = 8192;
  int fixedRate=-1;

  while ( (c=getopt( argc, argv, "a:F:h")) != EOF ) {
    switch(c) {
    case 'a':
      ip = ntohl(inet_addr(optarg)); break;
      break;
    case 'F':
      fixedRate = atoi(optarg);
      break;
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (lUsage) {
    usage(argv[0]);
    exit(1);
  }

  Reg::set(ip, port, 0x80000000);

  Xpm::Module* m = new(0) Xpm::Module;
  
  while(1) {
    unsigned rx = m->rxLinkStat();
    unsigned tx = m->txLinkStat();
    printf("rx/tx Status: %08x/%08x \t %c %c %04x %c\n", 
           rx,tx,
           ((rx>>31)&1)?'.':'+',
           ((rx>>30)&1)?'.':'+',
           ((rx>>16)&0x3fff),
           ((rx>>0)&1)?'.':'+');
    usleep(100000);
  }

  return 0;
}
