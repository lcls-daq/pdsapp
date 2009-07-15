#include "pdsapp/control/RunMonitor.hh"
#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/config/CfgClientNfs.hh"

#include <errno.h>

namespace Pds {
  class MyMonitor : public RunMonitor {
  public:
    MyMonitor() {}
    ~MyMonitor() {}
  public:
    void state_changed(RunMonitor::State state) 
    { printf("State : %s\n", state==RunMonitor::OK ? "OK" : "~OK"); }
  };
};

using namespace Pds;

int main(int argc, char* argv[])
{
  const char* dbpath=0;
  unsigned key = 0;
  for(int iarg = 1; iarg < argc; iarg++) {
    if (strcmp(argv[iarg],"--db")==0)
      dbpath = argv[++iarg];
    else if (strcmp(argv[iarg],"--key")==0)
      key = strtoul(argv[++iarg],NULL,0);
  }

  if (dbpath==0 || key==0) {
    printf("Usage: %s --db <path> --key <key>\n",argv[0]);
    return -1;
  }
  else
    printf("Searching %s for key %x\n",dbpath,key);

  ProcInfo src(Pds::Level::Control, 0, 0);
  Allocation alloc("", dbpath, 0);

  CfgClientNfs cfg(src);
  cfg.initialize(alloc);
  Transition tr(TransitionId::Configure,
		key);

  const int size = 0x10000;
  char* buffer = new char[size];
  int sz = cfg.fetch(tr, _controlConfigType, buffer);
  if (sz<=0) {
    printf("Error fetching configuration with address %p : %s\n",buffer,strerror(errno));
    return -1;
  }
  printf("Fetched 0x%x bytes configuration data\n",sz);

  MyMonitor mon;
  mon.configure(reinterpret_cast<const ControlConfigType&>(*buffer));

  while(true) {

    printf("State : %s\n", mon.state()==RunMonitor::OK ? "OK" : "~OK");

    int maxlen = 128;
    char line[maxlen];
    char* result = fgets(line, maxlen, stdin);
    if (!result) break;

  }

  return 1;
}
