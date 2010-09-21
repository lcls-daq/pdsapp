#include "Recorder.hh"
#include "PnccdShuffle.hh"
#include "CspadShuffle.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/collection/Node.hh"
#include "pds/service/GenericPool.hh"

#include "pds/utility/Occurrence.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

    
using namespace Pds;

static void local_mkdir (const char * path)
{
  struct stat buf;

  if (path && (stat(path, &buf) != 0)) {
    if (mkdir(path, 0777)) {
      perror("Recorder:: mkdir");
    }
  }
}

Recorder::Recorder(const char* path, unsigned int sliceID, uint64_t chunkSize) : 
  Appliance(), 
  _pool    (new GenericPool(sizeof(ZcpDatagramIterator),1)),
  _node    (0),
  _sliceID (sliceID),
  _beginrunerr(0),
  _path_error(false),
  _chunk(0),
  _chunkSize(chunkSize),
  _experiment(0),
  _run(0),
  _occPool(new GenericPool(sizeof(DataFileOpened),4))
{
  struct stat st;

  if (stat(path,&st)) {
    printf("Cannot stat %s : %s\n",path,strerror(errno));
    printf("Error: Data will not be recorded.\n");
    _path_error = true;
    _path[0] = '\0';  // cannot stat path, so _path is empty
  }
  else if (S_ISDIR(st.st_mode))
    strcpy(_path,path);
  else
    strcpy(_path,dirname(const_cast<char*>(path)));

  if (_path_error) {
    printf("No output path\n");
  } else {
    printf("Using path: %s\n",_path);
  }

  mode_t newmask = S_IWOTH;
  mode_t oldmask = umask(newmask);
  printf("Changed umask from %o to %o\n",oldmask,newmask);
}

InDatagram* Recorder::events(InDatagram* in) {

  PnccdShuffle::shuffle(in->datagram());
  CspadShuffle::shuffle(in->datagram());

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
  default:  // write this transition
    if (_f) {
      struct stat st;
      fwrite(&(in->datagram()),sizeof(in->datagram()),1,_f);
      { struct iovec iov;
        int rv;
        int remaining = in->datagram().xtc.sizeofPayload();
        while(remaining) {
          int isize = iter->read(&iov,1,remaining);
          fwrite(iov.iov_base,iov.iov_len,1,_f);
          remaining -= isize;
        }
        if ((rv = fstat(fileno(_f), &st)) != 0) {
          perror("fstat");
        }
        if ((0 == rv) && ((uint64_t)st.st_size >= _chunkSize)) {
          // chunking: close the current output file and open the next one
          ++_chunk;     // should _chunk have an upper limit?
          fclose(_f);
          if (_openOutputFile(true) != 0) {
            in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
          }
        }
        else {
          // flush the current output file
          fflush(_f);
        }
      }
    }
    break;
  }
  if (in->datagram().seq.service()==TransitionId::EndRun) {
    if (_f) {
      fclose(_f);
    }
    _f = 0;
  }

  delete iter;
  return in;
}

Transition* Recorder::transitions(Transition* tr) {
  if (tr->id()==TransitionId::Map) {
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
    if (tr->size() == sizeof(Transition)) {  // No RunInfo
      _f = 0;
      printf("No RunInfo.  Not recording.\n");
    }
    else {
      RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
      _experiment = rinfo.experiment();
      _run = rinfo.run();
      _chunk = 0;
      // open the file, write configure, and this transition
      printf("run %d expt %d ",_run,_experiment);
      if (_chunkSize < ULLONG_MAX) {
        printf("chunk_size %llu", _chunkSize);
      }
      printf("\n");

      if (_path_error) {
        printf("Error opening output file : failed to stat output path\n");
        _beginrunerr++;
      }
      else {
        // create directory
        sprintf(_fname,"%s/e%d", _path,_experiment);
        local_mkdir(_fname);
        // open output file
        if (_openOutputFile(true) != 0) {
          // error
          _beginrunerr++;
        }
        else {
          fwrite(_config, sizeof(Datagram) + 
             reinterpret_cast<const Datagram*>(_config)->xtc.sizeofPayload(),1,
             _f);
        }
      }
    }
  }
  return tr;
}

InDatagram* Recorder::occurrences(InDatagram* in) {
  return in;
}

//
// Recorder::_openOutputFile - open output file
//
// RETURNS: 0 on success, otherwise -1.
//
int Recorder::_openOutputFile(bool verbose) {
  int rv = -1;

  struct flock flk;
  flk.l_type   = F_WRLCK;
  flk.l_whence = SEEK_SET;
  flk.l_start  = 0;
  flk.l_len    = 0;

  sprintf(_fname,"%s/e%d/e%d-r%04d-s%02d-c%02d.xtc",
    _path, _experiment, _experiment, _run, _sliceID, _chunk);
  sprintf(_fnamerunning,"%s.inprogress",_fname);
  _f=fopen(_fnamerunning,"wx"); // x: if the file already exists, fopen() fails
  if (_f) {
    int rc;
    do { rc = fcntl(fileno(_f), F_SETLKW, &flk); }
    while(rc<0 && errno==EINTR);
    if (rc<0) {
      perror(_fnamerunning);
      return rv;
    }
    if ( (rv = rename(_fnamerunning, _fname)) ) {
      perror(_fname);
      return rv;
    }
    //    rv = 0;
    //  Set disk buffering as a multiple of RAID stripe size (256kB)
    rv = setvbuf(_f, NULL, _IOFBF, 4*1024*1024);
    if (verbose) {
      printf("Opened %s\n",_fname);
    }
    post(new(_occPool) DataFileOpened(_experiment,_run,_sliceID,_chunk));
  }
  else {
    if (verbose) {
      printf("Error opening file %s : %s\n",_fnamerunning,strerror(errno));
    }
  }
  return rv;
}

