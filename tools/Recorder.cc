#include "Recorder.hh"
#include "PnccdShuffle.hh"
#include "CspadShuffle.hh"
#include "pdsdata/index/XtcIterL1Accept.hh"
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
  _write_error(false),
  _chunk_requested(false),
  _chunk(0),
  _chunkSize(chunkSize),
  _experiment(0),
  _run(0),
  _occPool(new GenericPool(sizeof(DataFileOpened),5))
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
  
  memset( _indexfname, 0, sizeof(_indexfname) );
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
  case TransitionId::Enable:
    if (_beginrunerr) {
      in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
      _beginrunerr=0;
    }
    _write_error=false;
    _chunk_requested=false;
  default:  // write this transition
    if (_f) {
      struct stat st;
      
      int64_t i64Offset = lseek64( fileno(_f), 0, SEEK_CUR);
      if (_writeOutputFile(&(in->datagram()),sizeof(in->datagram()),1) != 0) {
        // error
        in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
      } else {
        struct iovec iov;
        int rv;
        int remaining = in->datagram().xtc.sizeofPayload();
        while(remaining) {
          int isize = iter->read(&iov,1,remaining);
          if (_writeOutputFile(iov.iov_base,iov.iov_len,1) != 0) {
            // error
            in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
            break;
          }
          remaining -= isize;
        }
        if ((rv = fstat(fileno(_f), &st)) != 0) {
          perror("fstat");
          // error
          in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
        }

        if ( _indexfname[0] != 0 )
        {
          if (in->datagram().seq.service() == TransitionId::L1Accept)
          {
            bool bInvalidNodeData = false;
            _indexList.startNewNode( (const Pds::Dgram&) in->datagram(), i64Offset, bInvalidNodeData);
            
            if ( !bInvalidNodeData )
            {            
              Index::XtcIterL1Accept iterL1Accept(&(in->datagram().xtc), 0,
                i64Offset + sizeof(Xtc) + sizeof(in->datagram()) - sizeof(in->datagram().xtc),
                _indexList);           
              iterL1Accept.iterate();
                    
              bool bPrintNode = false;
              _indexList.finishNode(bPrintNode);
            }
          } // if (in->datagram().seq.service() == TransitionId::L1Accept)  
          else if (in->datagram().seq.service() == TransitionId::BeginCalibCycle)
          {
            _indexList.addCalibCycle(i64Offset, in->datagram().seq.clock().seconds(), in->datagram().seq.clock().nanoseconds() );
          }          
        } // if ( _indexfname[0] != 0 )
        
        if ((0 == rv) && ((uint64_t)st.st_size >= _chunkSize)) {
          _requestChunk();
        }

        // flush the current output file
        if (_flushOutputFile() != 0) {
          // error
          in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
        }
      }
    }
    break;
  }
  if (in->datagram().seq.service()==TransitionId::EndRun) {
    if (_f && (_closeOutputFile() != 0)) {
      // error
      in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
    }
    _f = 0;
  }

  delete iter;
  return in;
}

Transition* Recorder::transitions(Transition* tr) {
  struct stat st;
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
  else if (tr->id()==TransitionId::BeginRun) {
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
          if (_writeOutputFile(_config, sizeof(Datagram) + 
              reinterpret_cast<const Datagram*>(_config)->xtc.sizeofPayload(),1) != 0) {
            // error
            _beginrunerr++;
          }
        }
      }
    }
  }
  else if (tr->id()==TransitionId::Enable && _f &&
           fstat(fileno(_f), &st) == 0 && 
           ((uint64_t)st.st_size >= _chunkSize/2)) {
    // chunking: close the current output file and open the next one
    ++_chunk;     // should _chunk have an upper limit?
    if (_closeOutputFile() != 0) {
      // error
      _beginrunerr++;
    }
    if (_openOutputFile(true) != 0) {
      _beginrunerr++;
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
  
  /*
   * Initialize/Reset the index list 
   */
  sprintf(_indexfname,"%s/e%d/index", _path,_experiment);
  local_mkdir(_indexfname);
  
  _indexList.reset();
  _indexList.setXtcFilename(_fname);   
  sprintf(_indexfname,"%s/e%d/index/e%d-r%04d-s%02d-c%02d.xtc.idx",
    _path, _experiment, _experiment, _run, _sliceID, _chunk); 
  
  return rv;
}

//
// Recorder::_postDataFileError -
//
// RETURNS: 0
//
int Recorder::_postDataFileError()
{
  post(new(_occPool) DataFileError(_experiment,_run,_sliceID,_chunk));
  return (0);
}

//
// Recorder::_writeOutputFile - write output file
//
// RETURNS: 0 on success, otherwise -1.
//
int Recorder::_writeOutputFile(const void *ptr, size_t size, size_t nmemb) {
  int rv = -1;

  if (_f) {
    if (fwrite(ptr, size, nmemb, _f) == nmemb) {
      // success
      rv = 0;
    } else if (!_write_error) {
      // error
      // use flag to avoid flood of occurrences
      _write_error = true;
      perror("fwrite");
      _postDataFileError();
    }
  }

  return (rv);
}

//
// Recorder::_flushOutputFile - flush output file
//
// RETURNS: 0 on success, otherwise -1.
//

int Recorder::_flushOutputFile() {
  int rv = -1;

  if (_f) {
    if (fflush(_f) == 0) {
      // success
      rv = 0;
    } else {
      // error
      perror("fflush");
      _postDataFileError();
    }
  }

  return (rv);
}

//
// Recorder::_closeOutputFile - close output file
//
// RETURNS: 0 on success, otherwise -1.
//

int Recorder::_closeOutputFile() {
  int rv = -1;

  if (_f) {
    
    /*
     * generate index file
     * 
     * Note: index file is generated before xtc file is closed,
     *   so it is okay for data mover to transfer it
     */
    if ( _indexfname[0] != 0 )
    {
      _indexList.finishList();  
      
      printf( "Writing index file %s\n", _indexfname );          
      int fdIndex = open(_indexfname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); //!! debug
      if ( fdIndex == -1 )
        printf( "Recorder::_closeOutputFile(): Open index file %s failed (%s)\n", _indexfname, strerror(errno) );
      else {
        _indexList.writeToFile(fdIndex);    
        ::close(fdIndex);
        
        int iVerbose = 0;
        _indexList.printList(iVerbose);          
      }  
      _indexfname[0] = 0;
    }    
    
    if (fclose(_f) == 0) {
      // success
      rv = 0;           
    } else {
      // error
      perror("fclose");
      _postDataFileError();
    }
  }

  return (rv);
}

//
// Recorder::_requestChunk - request a transition to allow 
//    file close and open of a new chunk
//
// RETURNS: 0 on success, otherwise -1.
//

int Recorder::_requestChunk() {
  int rv = -1;

  if (!_chunk_requested) {
    _chunk_requested = true;
    Occurrence* occ = new(_occPool) Occurrence(OccurrenceId::RequestPause);
    if (occ) {
      post(occ);
      rv = 0;
    }        
  }
  return rv;
}
