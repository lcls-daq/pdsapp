#ifndef Pds_IdleStream_hh
#define Pds_IdleStream_hh

// stream to attach BLD IPIMB appliances
#include "pdsapp/blv/IdleControlMsg.hh" 
#include "pds/utility/Stream.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Semaphore.hh"
#include "pdsdata/xtc/Src.hh"

namespace Pds {

  class Appliance;
  class Task;
  class InletWire;

  class BldIpimbStream : public Stream {
  public:
    BldIpimbStream(unsigned short port, const Src& src, char* ipimbConfigDb);
    ~BldIpimbStream();
  public:
    Appliance& main();  // appliance to attach to mainstream
	void       set_inlet_wire(InletWire*);
  public:
    void       control();
    void       start  ();
    unsigned   ip     () const { return _ip; }
  public:
    void       enable();
    void       disable();
  private:
    friend class IdleApp;
    void       transition(const Transition&);
  private:
    Task*         _task;  // handle this stream in its own thread
    GenericPool   _pool;
    Allocation    _allocatn;
    Src           _src;
    IdleControlMsg _msg;
    int           _socket;
    unsigned      _ip;
    Appliance*    _main;
    Semaphore     _sem;
	InletWire*    _wire;
  };
};

#endif  
