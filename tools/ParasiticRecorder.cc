#include "ParasiticRecorder.hh"
#include "EventOptions.hh"

#include "pds/management/PartitionMember.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/client/Decoder.hh"
#include "CountAction.hh"
#include "StatsTree.hh"
#include "pds/utility/EbDump.hh"
#include "RecorderQ.hh"
#include "pds/service/Task.hh"

#include <stdio.h>
#include <sys/stat.h>
#include <glob.h>

namespace Pds {
  class CleanupRoutine : public Routine {
  public:
    CleanupRoutine(const char* path,
		   unsigned    lifetime_sec) :
      _path        (path),
      _lifetime_sec(lifetime_sec) {}
  public:
    void routine() 
    { 
      //
      //  Cleanup e0-* files only
      //
      char pathname[128];
      // old style paths
      sprintf(pathname,"%s/e*/e*.xtc*",_path);
      remove(pathname);
      sprintf(pathname,"%s/e*/index/e*.idx*",_path);
      remove(pathname);
      // new style paths
      sprintf(pathname,"%s/*/xtc/e*.xtc*",_path);
      remove(pathname);
      sprintf(pathname,"%s/*/xtc/index/e*.idx*",_path);
      remove(pathname);
      delete this;
    }
  private:
    void remove(const char* pathname)
    {
      glob_t g;
      glob(pathname,0,0,&g);
      printf("Found %zd files in path %s\n",g.gl_pathc,pathname);
      time_t now = time(0);
      for(unsigned i=0; i<g.gl_pathc; i++) {
	struct stat64 s;
	if (!stat64(g.gl_pathv[i],&s)) {
	  if (S_ISREG(s.st_mode) && (s.st_mtime + _lifetime_sec < (unsigned)now)) {
	    printf("Removing %s : expired %lu (%lu)\n",
		   g.gl_pathv[i],s.st_mtime + _lifetime_sec,now);
	    unlink(g.gl_pathv[i]);
	  }
	  else {
	    printf("Retaining %s : expires %lu (%lu)\n",
		   g.gl_pathv[i],s.st_mtime + _lifetime_sec,now);
	  }
	}
      }
      globfree(&g);
    }
  private:
    const char* _path;
    unsigned    _lifetime_sec;
  };

  class OfflineProxy : public Appliance {
  public:
    OfflineProxy(Task*       task,
		 const char* path,
		 unsigned    lifetime_sec) : 
      _task        (task), 
      _pool        (sizeof(RunInfo),1),
      _path        (path),
      _lifetime_sec(lifetime_sec) {}
  public:
    Transition* transitions(Transition* tr) 
    {
      if (tr->id()==TransitionId::BeginRun) {
	_task->call(new CleanupRoutine(_path,_lifetime_sec)); 
        return new(&_pool) RunInfo(std::list<SegPayload>(), tr->env().value(),0);
      }
      return tr; 
    }
    InDatagram* events(InDatagram* dg) { return dg; }
  private:
    Task*       _task;
    GenericPool _pool;
    const char* _path;
    unsigned    _lifetime_sec;
  };
};
      
using namespace Pds;


ParasiticRecorder::ParasiticRecorder(Task*         task,
				     EventOptions& options,
				     unsigned      lifetime_sec,
                                     const char *  partition,
                                     const char *  offlinerc) :
  _task   (task),
  _options(options),
  _cleanup_task(new Task("cleanup")),
  _lifetime_sec(lifetime_sec),
  _partition(partition),
  _offlinerc(offlinerc),
  _offlineclient(NULL)
{
  if (_offlinerc) {
    _expname = options.expname;
    PartitionDescriptor pd(_partition);
    if (pd.valid()) {
      if (_expname) {
        // experiment name specified by caller
        _offlineclient = new OfflineClient(_offlinerc, pd, _expname, true);
      } else {
        // current experiment retrieved from database
        _offlineclient = new OfflineClient(_offlinerc, pd, true);
      }
    } else {
      printf("%s: partition '%s' not valid\n", __FUNCTION__, _partition);
    }
  }
}

ParasiticRecorder::~ParasiticRecorder()
{
  _cleanup_task->destroy();
  _task->destroy();
  if (_offlinerc && _offlineclient) {
    delete _offlineclient;
  }
}

void ParasiticRecorder::attached(SetOfStreams& streams)
{
  printf("ParasiticRecorder connected to platform.  ");

  if (_offlineclient) {
    printf("Offline client is initialized.\n");
  } else {
    printf("Offline client is NOT initialized.\n");
  }
  
  Stream* frmk = streams.stream(StreamParams::FrameWork);

  if (_options.outfile) {
    (new RecorderQ   (_options.outfile, _options.sliceID, _options.chunkSize, _options.delayXfer, false, _offlineclient, _options.expname))->connect(frmk->inlet());
    (new OfflineProxy(_cleanup_task, _options.outfile, _lifetime_sec))->connect(frmk->inlet());
  }

  switch (_options.mode) {
  case EventOptions::Counter:
    {
      (new CountAction)->connect(frmk->inlet());
      break;
    }
  case EventOptions::Decoder:
    {
      (new Decoder(Level::Event))->connect(frmk->inlet());
      break;
    }
  case EventOptions::Display:
    {
      (new StatsTree())->connect(frmk->inlet());
      (new EbDump(static_cast<EbBase*>(streams.wire())))->connect(frmk->inlet());
      break;
    }
  }
}

void ParasiticRecorder::failed(Reason reason) 
{
  printf("ParasiticRecorder: unable to attach to platform\n");
}

void ParasiticRecorder::dissolved(const Node& who) 
{
  const unsigned userlen = 12;
  char username[userlen];
  Node::user_name(who.uid(),username,userlen);

  const unsigned iplen = 64;
  char ipname[iplen];
  Node::ip_name(who.ip(),ipname, iplen);
  
  printf("ParasiticRecorder: platform 0x%x dissolved by user %s, pid %d, on node %s", 
          who.platform(), username, who.pid(), ipname);
}
