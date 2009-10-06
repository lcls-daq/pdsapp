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
  _node    (0),
  _beginrunerr(0)
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
  case TransitionId::BeginRun:
    if (_beginrunerr) {
      in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
      _beginrunerr=0;
    }
    // deliberately "fall through" the case statement (no "break), so we
    // rewrite configure transition information every beginrun.
  default:  // write this transition
    fwrite(&(in->datagram()),sizeof(in->datagram()),1,_f);
    { struct iovec iov;
      int remaining = in->datagram().xtc.sizeofPayload();
      while(remaining) {
        int isize = iter->read(&iov,1,remaining);
        fwrite(iov.iov_base,iov.iov_len,1,_f);
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
  if (tr->id()==TransitionId::BeginRun) {
    RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
    // open the file, write configure, and this transition
    printf("run %d expt %d\n",rinfo.run(),rinfo.experiment());
    char fname[256];
    unsigned slice=0;
    unsigned chunk=0;
    sprintf(fname,"%s/e%d-r%d_s%d_c%d.xtc",
            _path,rinfo.experiment(),rinfo.run(),slice,chunk);
    _f=fopen(fname,"w");
    if (_f) {
      printf("Opened %s\n",fname);
      fwrite(_config, sizeof(Datagram) + 
             reinterpret_cast<const Datagram*>(_config)->xtc.sizeofPayload(),1,
	       
             _f);
    }
    else {
      printf("Error opening %s : %s\n",fname,strerror(errno));
      _beginrunerr++;
    }
  }
  return tr;
}

InDatagram* Recorder::occurrences(InDatagram* in) {
  return in;
}
