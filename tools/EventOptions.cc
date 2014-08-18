#include "EventOptions.hh"
#include "pds/utility/Appliance.hh"
#include "pds/client/L3FilterDriver.hh"
#include "pds/client/L3FilterThreads.hh"
#include "pds/collection/Route.hh"
#include "pdsdata/app/L3FilterModule.hh"
#include "pdsdata/xtc/ProcInfo.hh"

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

static void load_appliance(char*       arg,
                           Appliance*& apps)
{
  for(const char* p = strtok(arg,","); p!=NULL; p=strtok(NULL,",")) {
    printf("dlopen %s\n",p);

    void* handle = dlopen(p, RTLD_LAZY);
    if (!handle) {
      printf("dlopen failed : %s\n",dlerror());
    }
    else {
      // reset errors
      const char* dlsym_error;
      dlerror();

      // load the symbols
      create_app* c_user = (create_app*) dlsym(handle, "create");
      if ((dlsym_error = dlerror())) {
        fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
      }
      else {
        if (apps != NULL)
          c_user()->connect(apps);
        else
          apps = c_user();
      }
    }
  }
}

static void load_filter(char*       arg,
                        Appliance*& apps)
{
  {
    std::string p(arg);

    unsigned n=0;
    size_t posn = p.find(",");
    if (posn!=std::string::npos) {
      n = strtoul(p.substr(posn+1).c_str(),NULL,0);
      p = p.substr(0,posn);
    }
        
    printf("dlopen %s\n",p.c_str());

    void* handle = dlopen(p.c_str(), RTLD_LAZY);
    if (!handle) {
      printf("dlopen failed : %s\n",dlerror());
    }
    else {
      // reset errors
      const char* dlsym_error;
      dlerror();

      // load the symbols
      create_m* c_user = (create_m*) dlsym(handle, "create");
      if ((dlsym_error = dlerror())) {
        fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
      }
      else {
        L3FilterThreads* driver = new L3FilterThreads(c_user, n);
        if (apps != NULL)
          driver->connect(apps);
        else
          apps = driver;
      }
    }
  }
}


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
  apps   (NULL),
  slowReadout (0)
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
  apps   (NULL),
  slowReadout (0)
{
  int c;
  while ((c = getopt(argc, argv, opt_string())) != -1) {
    parse_opt(c);
  }
  printf("(%p) apps = %p\n",this,this->apps);
}

const char* EventOptions::opt_string()
{
  return "f:p:b:a:s:c:n:edDE:L:F:w:V";
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
  case 'w':
    slowReadout = strtoul(optarg, &endPtr, 0);
    if (errno != 0 || endPtr == optarg) slowReadout = 0;
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
    load_appliance(optarg, apps);
    break;
  case 'F':
    load_filter   (optarg, apps);
    break;
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
     "         -L <plugin>\n"
     "         -F <L3 filter plugin>\n"
     "         -w <0/1> : enable slow readout support\n"
     " Use \'-F VETO[plugin]\' to enable veto\n",
     arg0);
    return 0;
  }
  return 1;
}
