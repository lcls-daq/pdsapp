#include "pdsapp/blv/IdleStream.hh"

#include "pds/utility/Appliance.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pdsdata/xtc/TransitionId.hh"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>

namespace Pds {
  class IdleApp : public Appliance {
  public:
    IdleApp(IdleStream& s) : _stream(s) {}
  public:
    Transition* transitions(Transition* tr) { 
      if (tr->id()==TransitionId::Map) {
	_stream._sem.take();
	_stream.disable();
      }
      else if (tr->id()==TransitionId::Unmap) {
	post(tr);
	_stream.enable();
	_stream._sem.give();
	return (Transition*)DontDelete;
      }
      return tr; 
    }
    InDatagram* events     (InDatagram* dg) { 
      return dg; 
    }
  private:
    IdleStream& _stream;
  };

  class IdleControl : public Routine {
  public:
    IdleControl(IdleStream& s) : _stream(s) {}
  public:
    void routine() { _stream.control(); }
  private:
    IdleStream& _stream;
  };
};

using namespace Pds;

static const int MaxSize = sizeof(Allocate);

IdleStream::IdleStream(unsigned short port,
		       const Src&     src) :
  Stream   (0),
  _task    (new Task(TaskObject("idlestr"))),
  _pool    (MaxSize, 8),
  _src     (src),
  _main    (new IdleApp(*this)),
  _sem     (Semaphore::FULL)
{
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (_socket<0)
    printf("IdleStream socket failed : %s\n",strerror(errno));

  else {
    Ins control(port);
    Sockaddr sa(control);
    if (::bind(_socket, sa.name(), sa.sizeofName())<0)
      printf("IdleStream socket bind error : %s\n",strerror(errno));
    else {
      socklen_t len = sa.sizeofName();
      ::getsockname(_socket, sa.name(), &len);
      _ip = sa.get().address();
      printf("IdleStream control bound to %x/%d\n",
	     sa.get().address(), sa.get().portId());
    }
  }
}

IdleStream::~IdleStream()
{
  _task->destroy();
}

void IdleStream::enable()
{
  printf("IdleStream::enable begin\n");
  
  Allocation allocatn("shared",_msg.dbpath,0);
  Node fake;
  const_cast<ProcInfo&>(fake.procInfo()) = ProcInfo(Level::Segment,0,0);
  allocatn.add(fake);                   // Add a fake EVR master
  allocatn.add(Node(Level::Segment,0)); // Add this process/node

  transition(Allocate  (allocatn));
  transition(Transition(TransitionId::Configure      , _msg.key));
  transition(Transition(TransitionId::BeginRun       , Env(0)));
  transition(Transition(TransitionId::BeginCalibCycle, Env(0)));
  transition(Transition(TransitionId::Enable         , Env(0x80000000)));
  printf("IdleStream::enable complete\n");
}

void IdleStream::disable()
{
  printf("IdleStream::disable begin\n");
  transition(Transition(TransitionId::Disable      , Env(0)));
  transition(Transition(TransitionId::EndCalibCycle, Env(0)));
  transition(Transition(TransitionId::EndRun       , Env(0)));
  transition(Transition(TransitionId::Unconfigure  , Env(0)));
  transition(Transition(TransitionId::Unmap        , Env(0)));
  printf("IdleStream::disable complete\n");
}

void IdleStream::transition(const Transition& tr)
{
  inlet()->post(new(&_pool) Transition(tr));
  if (tr.id()!=TransitionId::Unmap)
    inlet()->post(new(&_pool) CDatagram(Datagram(tr,_xtcType,_src)));
}

Appliance& IdleStream::main() { return *_main; }

void IdleStream::start()
{
  _task->call(new IdleControl(*this));
}

void IdleStream::control()
{
  enable();
  while(1) {
    if (::listen(_socket,5)<0)
      printf("Listen failed\n");
    else {
      Sockaddr name;
      socklen_t len = name.sizeofName();
      int s = ::accept(_socket,name.name(),&len);
      if (s<0)
	printf("Accept failed\n");
      else {
	printf("new connection %d from %x/%d\n",
	       s, name.get().address(), name.get().portId());
	
	while (::read(s, &_msg, sizeof(_msg))==sizeof(_msg)) {
	  _sem.take();
	  disable();
	  enable();
	  _sem.give();
	}
	::close(s);
      }
    }
  }
}
