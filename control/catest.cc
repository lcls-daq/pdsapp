#include "pdsapp/control/PVMonitor.hh"
#include "pdsapp/control/PVRunnable.hh"
#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/config/CfgCache.hh"

#include "cadef.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Pds {
  class MyMonitor : public PVRunnable {
  public:
    MyMonitor() {}
    ~MyMonitor() {}
  public:
    void runnable_change(bool state)
    { printf("State : %s\n", state ? "OK" : "~OK"); }
  };

  class CfgControl : public CfgCache {
  public:
    CfgControl(const Src& src) : CfgCache( src, _controlConfigType, 0x1000 ) {}
  private:
    int _size (void* tc) const { return reinterpret_cast<ControlConfigType*>(tc)->_sizeof(); }
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

  CfgControl cfg(src);
  cfg.init(alloc);
  Transition tr(TransitionId::Configure, key);

  // const int size = 0x10000;
  // char* buffer   = new char[size];
  int sz         = cfg.fetch(&tr);
  if (sz<=0) { return -1;  }
  printf("Fetched 0x%x bytes configuration data\n",sz);
  printf("Current %p\n",cfg.current());

  MyMonitor mon;
  PVMonitor pvs(mon);

  int result = ca_context_create(ca_disable_preemptive_callback);
  if (result != ECA_NORMAL) {
    printf("CA error %s occurred while trying to start channel access.\n",
	   ca_message(result));
    return -1;
  }

  while(true) {

    if (cfg.changed()) {
      printf("Changed %p\n",cfg.current());
      pvs.configure(*reinterpret_cast<const ControlConfigType*>(cfg.current()));
    }

    printf("State : %s\n", pvs.runnable() ? "OK" : "~OK");

    ca_pend_event(3);

    cfg.next();
    if (cfg.changed())
      pvs.unconfigure();
  }

  ca_context_destroy();

  return 1;
}
