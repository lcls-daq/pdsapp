#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <list>
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/TypeId.hh"

using namespace Pds;

static void usage(const char* p)
{
}

static InDatagram* createTransition(Pool& pool, TransitionId::Value id)
{
  Transition tr(id, Env(0));
  CDatagram* dg = new(&pool) CDatagram(Datagram(tr, _xtcType, Src(Level::Event)));
  return dg;
}

typedef std::list<Pds::Appliance*> AppList;

int main(int argc,char **argv)
{
  int c;
  unsigned nevents = -1;
  const char* outxtcname = "out.xtc";
  AppList user_apps;

  while ((c = getopt(argc, argv, "ho:n:L:")) != -1) {
    switch(c) {
    case 'n':
      nevents = atoi(optarg);
      break;
    case 'o':
      outxtcname = optarg;
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
	  user_apps.push_back( c_user() );
	}
	break;
      }
    case 'h':
    default:
      usage(argv[0]);
      exit(0);
    };
  };
  
  FILE* f = fopen(outxtcname,"w");
  
  GenericPool* pool = new GenericPool(1<<18,1);

  InDatagram* dg;
  Xtc*      xtc;

  dg = createTransition(*pool,TransitionId::Configure);
  for(AppList::iterator it=user_apps.begin(); it!=user_apps.end(); it++)
    dg = (*it)->events(dg);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginRun);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginCalibCycle);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::Enable);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  while (nevents--) {
    dg = createTransition(*pool,TransitionId::L1Accept);
    for(AppList::iterator it=user_apps.begin(); it!=user_apps.end(); it++)
      dg = (*it)->events(dg);
    fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
    delete dg;
  }

  dg = createTransition(*pool,TransitionId::Disable);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndCalibCycle);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndRun);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::Unconfigure);
  fwrite(static_cast<Datagram*>(dg),sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  fclose(f);

  return(0);
}
