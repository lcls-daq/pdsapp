#include "Recorder.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/collection/Node.hh"
#include "pds/service/GenericPool.hh"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>

using namespace Pds;

Recorder::Recorder(const char* path) : 
  Appliance(), 
  _pool    (new GenericPool(sizeof(ZcpDatagramIterator),1)),
  _node    (0)
{
  struct stat st;
  if (stat(path,&st)) {
    printf("Cannot stat %s : %s\n",path,strerror(errno));
    printf("Using current working directory\n");
    strcpy(_path,".");
  }
  else if (S_ISDIR(st.st_mode))
    strcpy(_path,path);
  else
    strcpy(_path,dirname(const_cast<char*>(path)));

  printf("Using path: %s\n",_path);
}

InDatagram* Recorder::events(InDatagram* in) {

  InDatagramIterator* iter = in->iterator(_pool);

  switch(in->datagram().seq.service()) {
  case TransitionId::Map:
    _src = in->datagram().xtc.src;
  case TransitionId::Unmap:
  case TransitionId::Unconfigure:
    break;
  case TransitionId::Configure: // cache the configure result
    memcpy   (_config, &in->datagram(), sizeof(Datagram));
    iter->copy(_config+sizeof(Datagram), in->datagram().xtc.sizeofPayload());
    break;
  case TransitionId::BeginRun: // open the file, write configure, and this transition
    { char fname[256];
      char dtime[64];
      time_t tm = in->datagram().seq.clock().seconds();
      strftime(dtime,64,"%Y%m%d-%H%M%S",gmtime(&tm));
      sprintf(fname,"%s/%s-%d.xtc",_path,dtime,_node);
      _f=fopen(fname,"w");
      if (_f) {
	printf("Opened %s\n",fname);
	fwrite(_config, sizeof(Datagram) + 
	       reinterpret_cast<const Datagram*>(_config)->xtc.sizeofPayload(),1,
	       
	       _f);
      }
      else {
	printf("Error opening %s : %s\n",fname,strerror(errno));
	in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
      }
    }
  default:  // write this transition
    { struct iovec iov;
      int remaining = in->datagram().xtc.sizeofPayload();
      while(remaining) {
        int isize = iter->read(&iov,1,remaining);
        int fsize = fwrite(iov.iov_base,iov.iov_len,1,_f);
        remaining -= isize;
      }
      fflush(_f);
    }
    break;
  }
  if (in->datagram().seq.service()==TransitionId::EndRun) {
    fclose(_f);
    _f = 0;
  }

  delete iter;
  return in;
}

Transition* Recorder::transitions(Transition* tr) {
  if (tr->id()==TransitionId::Configure) {
    const Allocation& alloc = reinterpret_cast<const Allocate*>(tr)->allocation();
    for(unsigned k=0; k<alloc.nnodes(); k++) {
      const Node& n = *alloc.node(k);
      if (n.procInfo() == reinterpret_cast<const ProcInfo&>(_src)) {
	_node = k;
	break;
      }
    }
  }
  return tr;
}

InDatagram* Recorder::occurrences(InDatagram* in) {
  return in;
}
