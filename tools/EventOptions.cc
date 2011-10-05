#include "EventOptions.hh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

using namespace Pds;

const unsigned DefaultBufferSize = 0x100000;
const uint64_t DefaultChunkSize = ULLONG_MAX;

EventOptions::EventOptions() :
  platform((unsigned)-1),
  sliceID(0),
  buffersize(DefaultBufferSize),
  arpsuidprocess(0),
  outfile(0),
  mode(Counter),
  chunkSize(DefaultChunkSize),
  delayXfer(false)
{
}

EventOptions::EventOptions(int argc, char** argv) :
  platform((unsigned)-1),
  sliceID(0),
  buffersize(DefaultBufferSize),
  arpsuidprocess(0),
  outfile(0),
  mode(Counter),
  chunkSize(DefaultChunkSize),
  delayXfer(false)
{
  int c;
  while ((c = getopt(argc, argv, "f:p:b:a:s:c:edD")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'b':
      buffersize = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) buffersize=DefaultBufferSize;
      break;
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = (unsigned)-1;
      break;
    case 's':
      sliceID = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) sliceID = 0;
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
    case 'D':
      delayXfer = true;
      break;
    case 'c':
      errno = 0;
      chunkSize = strtoull(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) chunkSize = DefaultChunkSize;
      break;
    }
  }
}

int EventOptions::validate(const char* arg0) const
{
  if (platform==(unsigned)-1) {
    printf("Usage: %s -p <platform>\n"
	   "options: -e decodes each transition (no FSM is connected)\n"
	   "         -d displays transition summaries and eb statistics\n"
	   "         -D delay transfer to offline until file is closed\n"
	   "         -b <buffer_size>\n"
           "         -a <arp_suid_executable>\n"
           "         -f <outputfilename>\n"
           "         -c <chunk_size>\n"
           "         -s <slice_ID>\n",
	   arg0);
    return 0;
  }
  return 1;
}
