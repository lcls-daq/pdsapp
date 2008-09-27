#include "EventOptions.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds;

const unsigned DefaultBufferSize = 0x100000;

EventOptions::EventOptions(int argc, char** argv) :
  partition(0),
  buffersize(DefaultBufferSize),
  id(-1),
  arpsuidprocess(0),
  mode(Counter)
{
  int c;
  while ((c = getopt(argc, argv, "p:i:b:a:ed")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'b':
      buffersize = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) buffersize=DefaultBufferSize;
      break;
    case 'p':
      partition = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) partition = 0;
      break;
    case 'i':
      id = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) id = -1;
      break;
    case 'a':
      arpsuidprocess = optarg;
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
  if (!partition || id < 0) {
    printf("Usage: %s -p <partition> -i <id>\n"
	   "options: -e decodes each transition (no FSM is connected)\n"
	   "         -d displays transition summaries and eb statistics\n"
	   "         -b <buffer_size>\n"
           "         -a <arp_suid_executable>\n",
	   arg0);
    return 0;
  }
  return 1;
}
