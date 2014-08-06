#include "pdsapp/control/PVMonitor.hh"
#include "pdsapp/control/PVRunnable.hh"
#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pds/config/CfgCache.hh"
#include "pds/service/CmdLineTools.hh"

#include "cadef.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

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

void usage(const char *cmd)
{
  printf("Usage: %s --db <path> --key <key> [-h]\n", cmd);
}

int main(int argc, char* argv[])
{
  const char* dbpath=0;
  unsigned key = 0;
  extern int optind;
  static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"key",  1, 0, 'k'},
    {"db",   1, 0, 'd'},
    {NULL,   0, NULL, 0}
  };
  bool helpFlag = false;
  bool parseErr = false;
  int c;
  int idx = 0;
  while ((c = getopt_long(argc, argv, "hd:k:", long_options, &idx)) != -1) {
    switch (c) {
    case 'h':
      helpFlag = true;
      break;
    case 'd':
      dbpath = optarg;
      break;
    case 'k':
      if (!Pds::CmdLineTools::parseUInt(optarg, key)) {
        parseErr = true;
      }
      break;
    case '?':
    default:
      parseErr = true;
    }
  }

  if (helpFlag) {
    usage(argv[0]);
    exit(0);
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    usage(argv[0]);
    exit(1);
  }

  if (parseErr) {
    printf("%s: argument parsing error\n", argv[0]);
    usage(argv[0]);
    exit(1);
  }

  if (dbpath==0 || key==0) {
    usage (argv[0]);
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
