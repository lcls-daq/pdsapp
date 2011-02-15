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
      char pathname[128];
      sprintf(pathname,"%s/e*/*.xtc*",_path);
      glob_t g;
      glob(pathname,0,0,&g);
      printf("Found %d files in path %s\n",g.gl_pathc,pathname);
      time_t now = time(0);
      for(unsigned i=0; i<g.gl_pathc; i++) {
	struct stat s;
	if (!stat(g.gl_pathv[i],&s)) {
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
      delete this;
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
	if (tr->size()==sizeof(Transition))
	  return new(&_pool) RunInfo(tr->env().value(),0);
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
				     unsigned      lifetime_sec) :
  _task   (task),
  _options(options),
  _cleanup_task(new Task("cleanup")),
  _lifetime_sec(lifetime_sec)
{
}

ParasiticRecorder::~ParasiticRecorder()
{
  _cleanup_task->destroy();
  _task->destroy();
}

void ParasiticRecorder::attached(SetOfStreams& streams)
{
  printf("ParasiticRecorder connected to platform.\n");
  
  Stream* frmk = streams.stream(StreamParams::FrameWork);

  if (_options.outfile) {
    (new RecorderQ   (_options.outfile, _options.sliceID, _options.chunkSize, true))->connect(frmk->inlet());
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
