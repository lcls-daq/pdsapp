#include "EventOptions.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds;

const unsigned DefaultBufferSize = 0x100000;

EventOptions::EventOptions(int argc, char** argv) :
  platform(0),
  buffersize(DefaultBufferSize),
  arpsuidprocess(0),
  outfile(0),
  mode(Counter)
{
  int c;
  while ((c = getopt(argc, argv, "f:p:b:a:ed")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'b':
      buffersize = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) buffersize=DefaultBufferSize;
      break;
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = 0;
      break;
    case 'a':
      arpsuidprocess = optarg;
      break;
    case 'f':
      outfile = optarg;
      break;
    case 'e':
      mode = Decoder;
      break;
    case 'd':
      mode = Display;
      break;
    }
  }
}

int EventOptions::validate(const char* arg0) const
{
  if (!platform) {
    printf("Usage: %s -p <platform>\n"
	   "options: -e decodes each transition (no FSM is connected)\n"
	   "         -d displays transition summaries and eb statistics\n"
	   "         -b <buffer_size>\n"
           "         -a <arp_suid_executable>\n"
           "         -f <outputfilename>\n",
	   arg0);
    return 0;
  }
  return 1;
}
