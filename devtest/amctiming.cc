#include "pds/cphw/AmcTiming.hh"

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include <new>

using namespace Pds::Cphw;

void usage(const char* p) {
  printf("Usage: %s [options]\n",p);
  printf("Options: -a <ip address, dotted notation>\n");
}

int main(int argc, char* argv[])
{
  extern char* optarg;
  char* endptr;
  char c;

  const char* ip = "192.168.2.10";
  unsigned short port = 8192;
  bool lReset = false;
  bool lInternal = false;
  int alignTarget = -1;

  while ( (c=getopt( argc, argv, "a:rRt:h")) != EOF ) {
    switch(c) {
    case 'a':
      ip = optarg; break;
      break;
    case 'r':
      lReset = true;
      lInternal = false;
      break;
    case 'R':
      lReset = true;
      lInternal = true;
      break;
    case 't':
      alignTarget = strtoul(optarg,NULL,0);
      break;
    default:
      usage(argv[0]);
      return 0;
    }
  }

  Pds::Cphw::Reg::set(ip, port, 0);

  Pds::Cphw::AmcTiming* t = new((void*)0) Pds::Cphw::AmcTiming;
  printf("buildStamp %s\n",t->version.buildStamp().c_str());

  if (alignTarget >= 0) {
    t->setRxAlignTarget(alignTarget);
    t->bbReset();
  }

  if (lReset) {
    t->xbar.setOut( XBar::FPGA, lInternal ? XBar::FPGA : XBar::RTM0 );
    t->setLCLS();
    usleep(1000);
    t->resetStats();
    usleep(1000);
  }
  t->xbar.dump();
  t->dumpStats();
  t->dumpRxAlign();

  return 0;
}
