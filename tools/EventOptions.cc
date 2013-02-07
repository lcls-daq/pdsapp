#include "EventOptions.hh"
#include "pds/utility/Appliance.hh"

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

using namespace Pds;

const unsigned DefaultBuffers    = 0;
const unsigned DefaultBufferSize = 0x1000000;
const uint64_t DefaultChunkSize = ULLONG_MAX;

EventOptions::EventOptions() :
  platform((unsigned)-1),
  sliceID(0),
  nbuffers  (DefaultBuffers),
  buffersize(DefaultBufferSize),
  arpsuidprocess(0),
  outfile(0),
  mode(Counter),
  chunkSize(DefaultChunkSize),
  delayXfer(false),
  expname(NULL),
  apps   (NULL)
{
  printf("(%p) apps = %p\n",this,this->apps);
}

EventOptions::EventOptions(int argc, char** argv) :
  platform((unsigned)-1),
  sliceID(0),
  nbuffers  (DefaultBuffers),
  buffersize(DefaultBufferSize),
  arpsuidprocess(0),
  outfile(0),
  mode(Counter),
  chunkSize(DefaultChunkSize),
  delayXfer(false),
  expname(NULL),
  apps   (NULL)
{
  int c;
  while ((c = getopt(argc, argv, opt_string())) != -1) {
    parse_opt(c);
  }
  printf("(%p) apps = %p\n",this,this->apps);
}

const char* EventOptions::opt_string() 
{
  return "f:p:b:a:s:c:n:edDE:L:";
}

bool        EventOptions::parse_opt (int c)
{
  errno = 0;
  char* endPtr;
  switch (c) {
  case 'n':
    nbuffers   = strtoul(optarg, &endPtr, 0);
    break;
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
  case 'E':
    expname = optarg;
    break;
  case 'c':
    errno = 0;
    chunkSize = strtoull(optarg, &endPtr, 0);
    if (errno != 0 || endPtr == optarg) chunkSize = DefaultChunkSize;
    break;
  case 'L':
    { for(const char* p = strtok(optarg,","); p!=NULL; p=strtok(NULL,",")) {
	printf("dlopen %s\n",p);

	void* handle = dlopen(p, RTLD_LAZY);
	if (!handle) {
	  printf("dlopen failed : %s\n",dlerror());
	  break;
	}

	// reset errors
	const char* dlsym_error;
	dlerror();

	// load the symbols
	create_app* c_user = (create_app*) dlsym(handle, "create");
	if ((dlsym_error = dlerror())) {
	  fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
	  break;
	}
	if (apps != NULL)
	  c_user()->connect(apps);
	else
	  apps = c_user();
      }
      break;
    }
  default:
    return false;
  }
  
  return true;
}


int EventOptions::validate(const char* arg0) const
{
  if (platform==(unsigned)-1) {
    printf("Usage: %s -p <platform>\n"
	   "options: -e decodes each transition (no FSM is connected)\n"
	   "         -d displays transition summaries and eb statistics\n"
	   "         -D delay transfer to offline until file is closed\n"
	   "         -n <nbuffers>\n"
	   "         -b <buffer_size>\n"
           "         -a <arp_suid_executable>\n"
           "         -f <outputfilename>\n"
           "         -c <chunk_size>\n"
           "         -s <slice_ID>\n"
           "         -E <experimentname>\n"
           "         -L <plugin>\n",
	   arg0);
    return 0;
  }
  return 1;
}
