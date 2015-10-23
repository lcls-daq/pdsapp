#include "pds/xtc/CDatagram.hh"
#include "pds/utility/DmaEngine.hh"
#include "EvrBldServer.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

EvrBldServer::EvrBldServer(const Src& client, Inlet& inlet) :
  EvrServer(Ins(), 
            client, 
            inlet, 
            1),
  _xtc(TypeId(TypeId::Id_EvrData,0),client)  
{
  if (::pipe(_pipefd))
    printf("*** EvrBldServer() Pipe Create Error : %s\n", strerror(errno));
  fd(_pipefd[0]);
  _evrDatagram = (EvrDatagram*) new char[sizeof(EvrDatagram)];  
}

int EvrBldServer::sendEvrEvent(EvrDatagram* evrDatagram)
{
  ::write(_pipefd[1],(char*)evrDatagram,sizeof(EvrDatagram));
  return 0;
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
  int length = ::read(_pipefd[0],(char*)_evrDatagram,sizeof(EvrDatagram));
  if(length != sizeof(EvrDatagram)) printf("*** EvrBldServer::fetch() : EvrDatagram not received in full \n");
  _count = _evrDatagram->evr;
  // printf("In EvrBldServer::fetch() cnt = %u fid Id = %x  tick = %d \n",_count, _evrDatagram->seq.stamp().fiducials(), _evrDatagram->seq.stamp().ticks() );

  int payloadSize = sizeof(EvrDatagram);	
  _xtc.extent = payloadSize+sizeof(Xtc);
  memcpy(payload, &_xtc, sizeof(Xtc));
  memcpy(payload+sizeof(Xtc), (char*)_evrDatagram, sizeof(EvrDatagram));
  return payloadSize+sizeof(Xtc);

}

