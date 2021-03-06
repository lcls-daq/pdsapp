#include "pds/service/CmdLineTools.hh"
#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/SxrSpectrum.hh"
#include "pdsapp/monobs/IpimbHandler.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace Pds;
using namespace PdsCas;

void usage(const char* p)
{
  printf("Usage: %s -n <PV (SXR:EXS)> -d <DetInfo (0x0b000300)> %s\n",
         p, ShmClient::options());
}

int main(int argc, char* argv[])
{
  ShmClient client;

  const unsigned NONE = unsigned(-1UL);
  const char* pvName = 0;
  unsigned detinfo   = NONE;

  int c;
  extern char* optarg;
  extern int optind;
  char opts[128];
  sprintf(opts,"?hi:n:d:%s",client.opts());
  while ((c = getopt(argc, argv, opts)) != -1) {
    switch (c) {
    case '?':
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'n':
      pvName = optarg;
      break;
    case 'd':
      if (!CmdLineTools::parseUInt(optarg, detinfo)) {
        usage(argv[0]);
        exit(1);
      }
      break;
    default:
      if (!client.arg(c,optarg)) {
        usage(argv[0]);
        exit(1);
      }
      break;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    usage(argv[0]);
    exit(1);
  }

  if (pvName==0 || detinfo==NONE || !client.valid()) {
    usage(argv[0]);
    exit(1);
  }
  
  client.insert(new SxrSpectrum(pvName,detinfo));
  client.insert(new IpimbHandler("SXR:USR:DAQ",
                                 Pds::DetInfo(-1, 
                                              Pds::DetInfo::SxrBeamline, 0,
                                              Pds::DetInfo::Ipimb,1)));
  fprintf(stderr, "client returned: %d\n", client.start());
}
