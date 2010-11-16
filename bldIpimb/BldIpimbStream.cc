#include "BldIpimbStream.hh"   

#include "pds/utility/Appliance.hh"   
#include "pds/utility/InletWire.hh"
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

#include <sys/types.h>
#include <pthread.h>



namespace Pds {
  class IdleApp : public Appliance {
  public:
    IdleApp(BldIpimbStream& s) : _stream(s) {}
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
    BldIpimbStream& _stream;
  };

  class IdleControl : public Routine {
  public:
    IdleControl(BldIpimbStream& s) : _stream(s) {}
  public:
    void routine() { _stream.control(); }
  private:
    BldIpimbStream& _stream;
  };
};

using namespace Pds;

static const int MaxSize = sizeof(Allocate);

BldIpimbStream::BldIpimbStream(unsigned short port, const Src& src, char* ipimbConfigDb) :
  Stream   (0),
  _task    (new Task(TaskObject("BldIpimbStream"))),
  _pool    (MaxSize, 8),
  _src     (src),
  _main    (new IdleApp(*this)),
  _sem     (Semaphore::FULL),
  _wire    (0)
{
  sprintf(_msg.dbpath,ipimbConfigDb);  
  _msg.key = 0x1a;   //find a way to locate last saved run-key and then use it 

  _socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (_socket<0)
    printf("*** BldIpimbStream(): socket open failed : %s\n",strerror(errno));
  else {
    Ins control(port);     
    Sockaddr sa(control);
    if (::bind(_socket, sa.name(), sa.sizeofName())<0)
      printf("*** BldIpimbStream(): socket bind error : %s\n",strerror(errno));
    else {
      socklen_t len = sa.sizeofName();
      ::getsockname(_socket, sa.name(), &len);
      _ip = sa.get().address();
      printf("BldIpimbStream(): control bound to addr/port = %x/%d\n",
	     sa.get().address(), sa.get().portId());
    }
  }
}

BldIpimbStream::~BldIpimbStream()
{
  //if(_wire) delete _wire;
  _task->destroy();
}

void BldIpimbStream::enable()
{
  printf("BldIpimbStream::enable() begin\n");
  timespec delayT;
  delayT.tv_sec = 0; delayT.tv_nsec= (long int) 100e6;       
  
  Allocation allocatn("shared",_msg.dbpath,0);
  Node fake;
  const_cast<ProcInfo&>(fake.procInfo()) = ProcInfo(Level::Segment,0,0);
  allocatn.add(Node(Level::Segment,0)); 
  
  transition(Allocate  (allocatn));
  nanosleep(&delayT, NULL);
  transition(Transition(TransitionId::Configure      , _msg.key));
  transition(Transition(TransitionId::BeginRun       , Env(0)));
  transition(Transition(TransitionId::BeginCalibCycle, Env(0)));
  transition(Transition(TransitionId::Enable         , Env(0x80000000)));
  printf("BldIpimbStream::enable() complete\n");
}

void BldIpimbStream::disable()
{
  printf("BldIpimbStream::disable() begin\n");
  timespec delayT;
  delayT.tv_sec = 0; delayT.tv_nsec= (long int) 100e6;   

  transition(Transition(TransitionId::Disable      , Env(0)));
  transition(Transition(TransitionId::EndCalibCycle, Env(0)));
  transition(Transition(TransitionId::EndRun       , Env(0)));
  transition(Transition(TransitionId::Unconfigure  , Env(0)));
  nanosleep(&delayT, NULL);
  transition(Transition(TransitionId::Unmap        , Env(0)));
  printf("BldIpimbStream::disable() complete\n");
}

void BldIpimbStream::transition(const Transition& tr)
{ 
  if (_wire) {
    _wire->post(*new(&_pool) Transition(tr));
    if (tr.id()!=TransitionId::Unmap)
      _wire->post(*new(&_pool) CDatagram(Datagram(tr,_xtcType,_src)));
  } else {
    inlet()->post(new(&_pool) Transition(tr));
    if (tr.id()!=TransitionId::Unmap)
      inlet()->post(new(&_pool) CDatagram(Datagram(tr,_xtcType,_src)));	
  }
}

Appliance& BldIpimbStream::main() { return *_main; }

void BldIpimbStream::set_inlet_wire(InletWire* wire) { _wire=wire; }

void BldIpimbStream::start() { _task->call(new IdleControl(*this)); } 

void BldIpimbStream::control()
{
  enable();
  while(1) {
    if (::listen(_socket,5)<0)
      printf("Listen failed\n");
    else {
      Sockaddr name;
      socklen_t len = name.sizeofName();
	  printf("### Before Accept \n");
      int s = ::accept(_socket,name.name(),&len);
	  printf("### After Accept \n");	  
      if (s<0)
        printf("Accept failed\n");
      else {
        printf("new connection %d from %x/%d\n",s,name.get().address(), name.get().portId());	
        while (::read(s, &_msg, sizeof(_msg))==sizeof(_msg)) {
          _sem.take();
          printf("$$$ *** new config Data \n\n\n");
          disable();
          sleep(1);
          enable();
          _sem.give();
        }
        ::close(s);
      }
    }
  } 
}
