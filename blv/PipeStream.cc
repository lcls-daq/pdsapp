#include "pdsapp/blv/PipeStream.hh"

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

//#define DBUG

using namespace Pds;

static const int MaxSize = 32*1024;

PipeStream::PipeStream(const Src&     src,
                       int            read_fd) :
  Stream   (0),
  _read_fd (read_fd),
  _task    (new Task(TaskObject("pipestr"))),
  _pool    (MaxSize, 8),
  _src     (src),
  _wire    (0)
{
}

PipeStream::~PipeStream()
{
  _task->destroy();
}

void PipeStream::set_inlet_wire(InletWire* wire) { _wire=wire; }

void PipeStream::_read(char*& buff, int remaining)
{
  while(remaining) {
    int len = ::read(_read_fd, buff, remaining);
    if (len == -1) continue;
    remaining -= len;
    buff += len;
  }
}

void PipeStream::start()
{
  while(1) {
    char* buff = (char*)_pool.alloc(0);
    Transition* tr = reinterpret_cast<Transition*>(buff);

    _read(buff,sizeof(Transition));

    if (tr->size() > sizeof(Transition))
      _read(buff,tr->size()-sizeof(Transition));

#ifdef DBUG
    printf("PipeStream read %08x.%08x [%p : %d]\n",
           reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[0],
           reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[1],
           this, _read_fd);
#endif

    if (tr->phase() == Transition::Record) {
      CDatagram* dg = new(&_pool) CDatagram(Datagram(*tr, _xtcType, _src));
      delete tr;
      _wire->post( *dg );
    }
    else
      _wire->post( *tr );
  }
}
