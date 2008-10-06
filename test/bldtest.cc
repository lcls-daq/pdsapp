#include "pds/utility/Transition.hh"

#include "pds/service/Client.hh"
#include "pds/service/LinkedList.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/xtc/BldDatagram.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <stdio.h>

namespace Pds {
  class BldService : public LinkedList<BldService> {
  public:
    BldService(unsigned id);
    virtual ~BldService();
    
    void send(const Sequence&);
  private:
    Client   _outlet;
    Ins      _dst;
    BldDatagram _datagram;
    char        _payload[32];
  };

  class BldDriver {
  public:
    BldDriver();
    ~BldDriver();
  public:
    void add (BldService*);
    void send(unsigned);
  private:
    LinkedList<BldService> _service;
  };
}

using namespace Pds;

static const unsigned MaxPayload = sizeof(Transition);
static const unsigned ConnectTimeOut = 500; // 1/2 second

BldService::BldService(unsigned id) :
  _outlet(sizeof(_datagram),
	  sizeof(_payload)),
  _dst(StreamPorts::bld(id)),
  _datagram(Sequence(),sizeof(_payload))
{
  for(unsigned k=0; k<sizeof(_payload); k++)
    _payload[k] = id&0xff;
}

BldService::~BldService() 
{
}

void BldService::send(const Sequence& seq)
{
  _datagram.sequence() = seq;
  _outlet.send(reinterpret_cast<char*>(&_datagram),
	       _payload, 
	       sizeof(_payload), 
	       _dst);
}

BldDriver::BldDriver()
{
}

BldDriver::~BldDriver()
{
  while( _service.forward() != _service.empty() )
    delete _service.remove();
}

void BldDriver::add (BldService* srv)
{
  _service.insert(srv);
}

void BldDriver::send(unsigned rate)
{
  unsigned ns = (1<<30)/rate;
  ns = ns>999999999 ? 999999999 : ns;
  timespec td;
  td.tv_sec  = 0;
  td.tv_nsec = ns;

  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  printf("Starting time    %08x/%08x\n",tp.tv_sec,tp.tv_nsec);
  printf("Sending interval %08x/%08x\n",td.tv_sec,td.tv_nsec);

  unsigned _pulseId=0;

  while(1) {

    nanosleep( &td, 0 );

    clock_gettime(CLOCK_REALTIME, &tp);
    unsigned  sec = tp.tv_sec;
    unsigned nsec = tp.tv_nsec & ~(0x7FFFFF);
    unsigned pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
    if (pulseId != _pulseId) {
      Sequence seq(Sequence::Event,
		   Sequence::L1Accept,
		   ClockTime(sec,nsec),
		   0, pulseId);
      BldService* srv = _service.forward();
      while( srv != _service.empty() ) {
	srv->send(seq);
	srv = srv->forward();
      }
      _pulseId = pulseId;
    }
  }
}


int main(int argc, char** argv)
{
  int nid = argc-2;
  if (nid<1) {
    printf("%s usage: <rate(Hz)> <id0> <id1> ...\n", argv[0]);
    return 0;
  }

  char* end;
  unsigned rate = strtoul(argv[1], &end, 0);
  BldDriver* driver = new BldDriver;
  for(int k=0; k<nid; k++) {
    unsigned id = strtoul(argv[k+2], &end, 0);
    driver->add(new BldService(id));
  }
  driver->send(rate);

  delete driver;

  return 0;
}
