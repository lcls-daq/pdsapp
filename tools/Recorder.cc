#include "Recorder.hh"
#include "PnccdShuffle.hh"
#include "CspadShuffle.hh"
#include "StripTransient.hh"
#include "EventOptions.hh"
#include "pdsdata/index/XtcIterL1Accept.hh"
#include "pdsdata/index/SmlDataIterL1Accept.hh"
#include "pds/collection/Node.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/xtc/CDatagramIterator.hh"

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
#include <ctype.h>
#include <vector>

#define DELAY_XFER

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

    
using namespace Pds;
using SmlData::SmlDataIterL1Accept;
using SmlData::XtcObj;
using std::vector;


static int call(char *cmd)
{
  int rv;

  printf("calling: %s\n", cmd);
  rv = system(cmd);
  if (errno != 0) {
    perror("Recorder:: system");
  }
  if (rv != 0) {
    fprintf(stderr, " *** system call '%s' returned %d ***\n", cmd, rv);
  }
  
  return 0;
}

static void local_mkdir (const char * path)
{
  struct stat64 buf;

  if (path && (stat64(path, &buf) != 0)) {
    if (mkdir(path, 0777)) {
      perror("Recorder:: mkdir");
    }
  }
}

static void local_mkdir_with_acls (const char * path, const char *expname)
{
  struct stat64 buf;

  if (path && (stat64(path, &buf) != 0)) {
    if (mkdir(path, 0770)) {
      perror("Recorder:: mkdir");
    } else {
      // set ACLs
      char cmdbuf[200];
      snprintf(cmdbuf, sizeof(cmdbuf), "setfacl -m group:ps-data:rx %s", path);
      call(cmdbuf);
      snprintf(cmdbuf, sizeof(cmdbuf), "setfacl -d -m group:ps-data:rx %s", path);
      call(cmdbuf);
      snprintf(cmdbuf, sizeof(cmdbuf), "setfacl -m group:%s:rx %s", expname, path);
      call(cmdbuf);
      snprintf(cmdbuf, sizeof(cmdbuf), "setfacl -d -m group:%s:rx %s", expname, path);
      call(cmdbuf);
    }
  }
}

Recorder::Recorder(const char* path, unsigned int sliceID, uint64_t chunkSize, bool delay_xfer, OfflineClient *offlineclient, const char* expname, unsigned uSizeThreshold) : 
  Appliance(), 
  _pool    (new GenericPool(sizeof(CDatagramIterator),1)),
  _node    (0),
  _sliceID (sliceID),
  _beginrunerr(0),
  _path_error(false),
  _write_error(false),
  _sdf_write_error(false),
  _chunk_requested(false),
  _chunk(0),
  _chunkSize(chunkSize),
  _delay_xfer(delay_xfer),
  _experiment(0),
  _expname(expname),
  _run(0),
  _occPool(new GenericPool(sizeof(DataFileOpened),5)),
  _offlineclient(offlineclient),
  _open_data_file_error(false),
  _uSizeThreshold(uSizeThreshold)
{
  struct stat64 st;

  if (stat64(path,&st)) {
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

  if (gethostname(_host_name, sizeof(_host_name))) {
    // error
    perror("gethostname");
    strncpy(_host_name, "unknown", sizeof(_host_name)-1);
  }
  printf("Host name: %s\n", _host_name);
}

InDatagram* Recorder::events(InDatagram* in) {
  PnccdShuffle::shuffle(in->datagram());
  if (!CspadShuffle::shuffle(reinterpret_cast<Dgram&>(in->datagram()))) {
    post(new(_occPool) UserMessage("Corrupt CSPAD data.  Recommend reboot of CSPAD host"));
    post(new(_occPool) Occurrence(OccurrenceId::ClearReadout));
  }
  StripTransient::process(reinterpret_cast<Dgram&>(in->datagram()));

  InDatagramIterator* iter = in->iterator(_pool);

  switch(in->datagram().seq.service()) {
  case TransitionId::Map:
    _src = in->datagram().xtc.src;
  case TransitionId::Unmap:
  case TransitionId::Unconfigure:
    break;
  case TransitionId::Configure: // cache the configure result
    {
       // Cache configure transition for xtc file
       memcpy   (_config, &in->datagram(), sizeof(Datagram));
       iter->copy(_config+sizeof(Datagram), in->datagram().xtc.sizeofPayload());
       
       // Add SmlData::ConfigV1 to configure datagram for smldata index file
       XtcObj   xtcIndexConfig;
       uint32_t sizeofPayloadOrg = in->datagram().xtc.sizeofPayload();
       new ((char*)&xtcIndexConfig)  Xtc             (); // set damage to 0
       new (xtcIndexConfig.configV1) SmlData::ConfigV1 (_uSizeThreshold);

       xtcIndexConfig.xtc.src      = Src(Level::Recorder);
       xtcIndexConfig.xtc.contains = TypeId(TypeId::Id_SmlDataConfig, 1);
       xtcIndexConfig.xtc.extent   = sizeof(Xtc) + sizeof(SmlData::ConfigV1);
       in->datagram().xtc.extent += xtcIndexConfig.xtc.extent;
       
       // Cache the extended configure transition for smldata index file
       memcpy (_smlconfig, &in->datagram(), sizeof(Datagram));
       memcpy (_smlconfig+sizeof(Datagram),(char*)&xtcIndexConfig, xtcIndexConfig.xtc.extent);
       memcpy (_smlconfig+sizeof(Datagram)+xtcIndexConfig.xtc.extent,in->datagram().xtc.payload(),sizeofPayloadOrg);
    }
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
    if (_path_error || _open_data_file_error) {
      // use flag to avoid flood of occurrences
      if (!_write_error) {
        _write_error = true;
        _postDataFileError();
      }
    } else if (_f) {
      struct stat64 st;
      
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
        if ((rv = fstat64(fileno(_f), &st)) != 0) {
          perror("fstat");
          // error
          in->datagram().xtc.damage.increase(1<<Damage::UserDefined);
        }

        /////////////// Index file ////////////////////////////

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

        /////////////// Small data file ////////////////////////////

        // Write to small data file
        if (_sdf) {
           std::vector<XtcObj>  xtcObjPool;
           unsigned             lquiet(1);
           uint32_t             dgramOffset = sizeof(in->datagram());
           if (in->datagram().seq.service() == TransitionId::L1Accept) {
              xtcObjPool.clear();
              // Iterate over datagram
              SmlDataIterL1Accept iterL1AcceptSml(&(in->datagram().xtc), 0, 
                                                  i64Offset + dgramOffset, 
                                                  dgramOffset, 
                                                  _uSizeThreshold, 
                                                  xtcObjPool, lquiet);
              iterL1AcceptSml.iterate();
              
              typedef std::vector<SmlDataIterL1Accept::XtcInfo> XtcInfoList;
              XtcInfoList& xtcInfoList = iterL1AcceptSml.xtcInfoList();
              
              // Write datagram header to small data file
              if (_writeSmallDataFile( &in->datagram(), sizeof(in->datagram()) - sizeof(Xtc) ) != 0) {
                 _sdf_write_error=true; 
                 printf("ERROR:  Failed to write to small data index file\n");
              }
              
              // Create OrigDgramOffsetV1 
              XtcObj xtcIndexOrigDgramOffset;
              new ((char*)&xtcIndexOrigDgramOffset) Xtc           (); // set damage to 0
              new (xtcIndexOrigDgramOffset.origDgramOffsetV1)   SmlData::OrigDgramOffsetV1  (i64Offset, in->datagram().xtc.extent);

              xtcIndexOrigDgramOffset.xtc.src      = Src(Level::Recorder);
              xtcIndexOrigDgramOffset.xtc.contains = TypeId(TypeId::Id_SmlDataOrigDgramOffset, 1);
              xtcIndexOrigDgramOffset.xtc.extent   = sizeof(Xtc) + sizeof(SmlData::OrigDgramOffsetV1);
              xtcInfoList[0].uSize    += xtcIndexOrigDgramOffset.xtc.extent;
              
              // Loop over xtcInfoList
              // Write data to small data file:  Original xtc written if size < threshold; Proxy written otherwise
              for (size_t i=0; i < xtcInfoList.size(); ++i) {
                 Xtc* pOrgXtc = (Xtc*)((char*)&(in->datagram()) + (long) (xtcInfoList[i].i64Offset - i64Offset));
                 Xtc* pXtc    = (xtcInfoList[i].iPoolIndex == -1? pOrgXtc : &xtcObjPool[xtcInfoList[i].iPoolIndex].xtc);
                 pXtc->extent       = xtcInfoList[i].uSize;
                 uint32_t sizeWrite = ((i == xtcInfoList.size()-1 || xtcInfoList[i].depth >= xtcInfoList[i+1].depth) 
                                       ? pXtc->extent : sizeof(Xtc));
                 if(_writeSmallDataFile(pXtc, sizeWrite) != 0) {
                    _sdf_write_error=true;
                    printf("ERROR: failed to write to small data index file\n");
                 }
                 if (i == 0 && _writeSmallDataFile((char*)&xtcIndexOrigDgramOffset, xtcIndexOrigDgramOffset.xtc.extent) != 0) {
                    _sdf_write_error=true;
                    printf("ERROR:  failed to write to small data index file\n");
                 }
              } // loop over xtcInfoList
           } // _sdf L1Accept transition
           else {
              if (_writeSmallDataFile(&(in->datagram()),sizeof(in->datagram())) != 0) {
                 // error
                 _sdf_write_error=true;
                 printf("ERROR:  failed to write datagram header to small data index file in non-L1Accept transition\n");
              } else {
                 if (_writeSmallDataFile(in->datagram().xtc.payload(), in->datagram().xtc.sizeofPayload()) != 0) {
                    //error
                    _sdf_write_error=true;
                    printf("ERROR:  failed to write payload to small data index file in non-L1Accept transition\n");
                 }
              }
           } //_sdf all other transitions

           // flush the current small data output file
           if (_flushSmallDataFile() != 0) {
              // error
              printf("ERROR:  failed to flush small data index file \n");
           }
        } // if (_sdf)
        
        ///////////////////////////////////////////

      } //(_writeOutputFile(&(in->datagram()),sizeof(in->datagram()),1) successful
    } // if (_f)
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
  struct stat64 st;
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
      // ignore file writing errors when not recording
      _path_error = false;
      _open_data_file_error = false;
    }
    else {
      RunInfo& rinfo = *reinterpret_cast<RunInfo*>(tr);
      _experiment = rinfo.experiment();
      _run = rinfo.run();
      _chunk = 0;
      // open the file, write configure, and this transition
      printf("run %d exp# %d ",_run,_experiment);
      if (_expname) {
        printf("expname %s ", _expname);
      }
      if (_chunkSize < ULLONG_MAX) {
        printf("chunk_size %zu", ssize_t(_chunkSize));
      }
      printf("\n");

      if (_path_error) {
        printf("Error opening output file : failed to stat output path: %s\n", _path);
        _beginrunerr++;
      }
      else {
        // create directory
        if (_expname && isalpha(_expname[0])) {
          sprintf(_fname,"%s/%s", _path,_expname);
          if (_offlineclient) {
            // fast feedback directory: add ACLs
            local_mkdir_with_acls(_fname,_expname);
          } else {
            // data directory: no ACLs needed
            local_mkdir(_fname);
          }
          sprintf(_fname,"%s/%s/xtc", _path,_expname);
        } else {
          sprintf(_fname,"%s/e%d", _path,_experiment);
        }
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
          if (_writeSmallDataFile(_smlconfig, sizeof(Datagram) + 
                             reinterpret_cast<const Datagram*>(_smlconfig)->xtc.sizeofPayload()) != 0) {
             // error
             _sdf_write_error=true;
             printf("ERROR:  Did not write config transition to small data file\n");
          }
        }
      }
    }
  }
  else if (tr->id()==TransitionId::Enable && _f &&
           fstat64(fileno(_f), &st) == 0 && 
           ((uint64_t)st.st_size >= _chunkSize/2)) {
    // chunking: close the current output file and open the next one
    ++_chunk;     // should _chunk have an upper limit? Why, yes, it should!
    if (_chunk >= 99) {
       // trap error so we never go above chunk 99
       _beginrunerr++;
    }
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

  if (_expname && isalpha(_expname[0])) {
    sprintf(_fname,"%s/%s/xtc/e%d-r%04d-s%02d-c%02d.xtc",
      _path, _expname, _experiment, _run, _sliceID, _chunk);
  } else {
    sprintf(_fname,"%s/e%d/e%d-r%04d-s%02d-c%02d.xtc",
      _path, _experiment, _experiment, _run, _sliceID, _chunk);
  }
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
    if (!_delay_xfer) {
      if ( (rv = rename(_fnamerunning, _fname)) ) {
        perror(_fname);
        return rv;
      }
    }

    //    rv = 0;
    //  Set disk buffering as a multiple of RAID stripe size (256kB)
    rv = setvbuf(_f, NULL, _IOFBF, 4*1024*1024);
    if (verbose) {
      printf("Opened %s\n",_fname);
    }
    if (_offlineclient) {
      if (_experiment != 0) {
        // fast feedback
        std::string hostname = _host_name;
        std::string filename = _fname;
        // ffb=true
        _offlineclient->reportOpenFile(_experiment, _run, (int)_sliceID, (int)_chunk, hostname, filename, true);
      }
    } else {
      post(new(_occPool) DataFileOpened(_experiment,_run,_sliceID,_chunk,_host_name,_fname));
    }
  }
  else {
    _open_data_file_error = true;
    if (verbose) {
      printf("Error opening file %s : %s\n",_fnamerunning,strerror(errno));
    }
  }

  /*
   * Open small data index file 
   */
  if (_expname && isalpha(_expname[0])) {
    sprintf(_sdfname,"%s/%s/xtc/smalldata", _path,_expname);
    local_mkdir(_sdfname);
    printf("Created smalldata directory: %s\n", _sdfname);
    sprintf(_sdfname,"%s/%s/xtc/smalldata/e%d-r%04d-s%02d-c%02d.smd.xtc",
      _path, _expname, _experiment, _run, _sliceID, _chunk);
  } else {
    sprintf(_sdfname,"%s/e%d/smalldata", _path, _experiment);
    local_mkdir(_sdfname);
    printf("Created smldata directory: %s\n", _sdfname);
    sprintf(_sdfname,"%s/e%d/smalldata/e%d-r%04d-s%02d-c%02d.smd.xtc",
      _path, _experiment, _experiment, _run, _sliceID, _chunk);
  }
  sprintf(_sdfnamerunning,"%s.inprogress",_sdfname);
  _sdf=fopen(_sdfnamerunning,"wx"); // x: if the file already exists, fopen() fails
  printf("Opened smldata file: %s\n", _sdfnamerunning);
  if (_sdf) {
    int rc;
    do { rc = fcntl(fileno(_sdf), F_SETLKW, &flk); }
    while(rc<0 && errno==EINTR);
    if (rc<0) {
      perror(_sdfnamerunning);
      return rv;
    }
    if (!_delay_xfer) {
      if ( (rv |= rename(_sdfnamerunning, _sdfname)) ) {
        perror(_sdfname);
        return rv;
      }
    }
    
    //  rv = 0;
    //  Set disk buffering as a multiple of RAID stripe size (256kB)
    rv |= setvbuf(_sdf, NULL, _IOFBF, 4*1024*1024);
    printf("Opened %s\n",_sdfname); 
    if (verbose) {
      printf("Opened %s\n",_sdfname); 
    }
    // JBT - do not register open file with offline client until offline client API contains file type
//    if (_offlineclient) {
//      if (_experiment != 0) {
//        // fast feedback
//        std::string hostname = _host_name;
//        std::string sdfilename = _sdfname;
//        // ffb=true
//        // Don't report smldata file name until offline client API changes to include file type
//        //        _offlineclient->reportOpenFile(_experiment, _run, (int)_sliceID, (int)_chunk, hostname, sdfilename, true);
//      }
//    } 
  } // if (_sdf)
  else {
    _open_data_file_error = true;
      printf("Error opening smldata file %s : %s\n",_sdfnamerunning,strerror(errno));
    if (verbose) {
      printf("Error opening smldata file %s : %s\n",_sdfnamerunning,strerror(errno));
    }
  }
  
  
  /*
   * Initialize/Reset the index list 
   */
  if (_expname && isalpha(_expname[0])) {
    sprintf(_indexfname,"%s/%s/xtc/index", _path,_expname);
  } else {
    sprintf(_indexfname,"%s/e%d/index", _path,_experiment);
  }
  local_mkdir(_indexfname);
  
  _indexList.reset();
  _indexList.setXtcFilename(_fname);   
  if (_expname && isalpha(_expname[0])) {
    sprintf(_indexfname,"%s/%s/xtc/index/e%d-r%04d-s%02d-c%02d.xtc.idx",
      _path, _expname, _experiment, _run, _sliceID, _chunk); 
  } else {
    sprintf(_indexfname,"%s/e%d/index/e%d-r%04d-s%02d-c%02d.xtc.idx",
      _path, _experiment, _experiment, _run, _sliceID, _chunk); 
  }
  
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
// Recorder::_writeSmalllDataFile - write smldata file
//
// RETURNS: 0 on success, otherwise -1.
//
int Recorder::_writeSmallDataFile(const void *ptr, size_t size) 
{
  int rv = -1;
  size_t count = 1;
  if (_sdf) {
     if (fwrite(ptr, size, count, _sdf) == count) {
        // success
        rv = 0;
     } else if (!_write_error) {
        // error
        // use flag to avoid flood of occurrences
        _sdf_write_error = true;
        perror("fwrite");
        printf("ERROR:   in _writeSmallDataFile\n");
     }
  } // if (_sdf)

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
    } else if (!_write_error) {
      // error
      _write_error = true;
      perror("fflush");
      _postDataFileError();
    }
  }
  return (rv);
}

//
// Recorder::_flushSmallDataFile - flush output file
//
// RETURNS: 0 on success, otherwise -1.
//

int Recorder::_flushSmallDataFile() {
  int rv = -1;

  // If error flushing small data file, print error message, but do not increment damage
  if (_sdf) {
    if (fflush(_sdf) == 0) {
      // success
       rv = 0;
    } else if (!_write_error) {
      // error
       _sdf_write_error=true;
      perror("fflush");
      printf("ERROR:  Did not successfully flush small data file \n");
      //      _postDataFileError();
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
     *   so it is ready for data mover to transfer it
     */
    if ( _indexfname[0] != 0 )
    {
      _indexList.finishList();  
      
      printf( "Writing index file %s\n", _indexfname );          
      int fdIndex = open(_indexfname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
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
    if (_delay_xfer) {
      if ( (rv = rename(_fnamerunning, _fname)) ) {
        perror(_fname);
      }
    }
    if (fclose(_f) == 0) {
      // success
      rv = 0;           
    } else {
      // error
      perror("fclose");
      _postDataFileError();
    }

    // Close small data index file
    if (_sdf) {
      if (_delay_xfer) {
        if ( (rv |= rename(_sdfnamerunning, _sdfname)) ) {
          perror(_sdfname);
        }
      }
      if (fclose(_sdf) != 0) {
        perror("fclose");
      }
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
