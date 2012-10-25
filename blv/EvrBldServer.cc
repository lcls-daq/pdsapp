#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/utility/DmaEngine.hh"
#include "EvrBldServer.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

//#define DBUG

using namespace Pds;

EvrBldServer::EvrBldServer(const Src& client,
                           int        read_fd,
                           InletWire& inlet) :
  EvrServer(Ins(),
            client,
            inlet,
            1),
  _xtc     (TypeId(TypeId::Id_EvrData,0),client),
  _hinput  ("EvrInput"),
  _hfetch  ("EvrFetch")
{
#ifdef DBUG
  _tfetch.tv_sec = _tfetch.tv_nsec = 0;
#endif

  fd(read_fd);
  _evrDatagram = (EvrDatagram*) new char[sizeof(EvrDatagram)];  
}

//Eb Interface
void EvrBldServer::dump(int detail) const
{
  printf("In EvrBldServer::dump() detail = %d \n)",detail);
}

bool EvrBldServer::isValued() const
{
  return true;
}

const Src& EvrBldServer::client() const
{
  return _xtc.src; 
}

//  EbSegment interface
const Xtc& EvrBldServer::xtc() const 
{
  return _xtc;
}	

bool EvrBldServer::more() const 
{ 
  return false; 
}

unsigned EvrBldServer::offset() const 
{
  return 0;
}

unsigned EvrBldServer::length() const 
{
  return _xtc.extent;
}


//  Server interface
int EvrBldServer::pend(int flag) 
{
  return -1;
}

int EvrBldServer::fetch(char* payload, int flags)
{	
#ifdef DBUG
  timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  if (_tfetch.tv_sec)
    _hinput.accumulate(ts,_tfetch);
  _tfetch = ts;
#endif

  int length = ::read(fd(),(char*)_evrDatagram,sizeof(EvrDatagram));
  if(length != sizeof(EvrDatagram)) printf("*** EvrBldServer::fetch() : EvrDatagram not received in full  (%d/%u)\n", length,sizeof(EvrDatagram));
  _count = _evrDatagram->evr; 
#ifdef DBUG
  printf("In EvrBldServer::fetch() [%p] cnt = %u fid Id = %x  tick = %d \n",
         payload, _count, _evrDatagram->seq.stamp().fiducials(), _evrDatagram->seq.stamp().ticks() );
#endif
  int payloadSize = sizeof(EvrDatagram);	
  _xtc.extent = payloadSize+sizeof(Xtc);
  memcpy(payload, &_xtc, sizeof(Xtc));
  memcpy(payload+sizeof(Xtc), (char*)_evrDatagram, sizeof(EvrDatagram));

#ifdef DBUG
  clock_gettime(CLOCK_REALTIME,&ts);
  _hfetch.accumulate(ts,_tfetch);

  _hinput.print(_count+1);
  _hfetch.print(_count+1);
#endif

  return payloadSize+sizeof(Xtc);
}

int EvrBldServer::fetch(ZcpFragment& zf, int flags)
{
  return 0;
}

