#include "pdsapp/blv/PipeApp.hh"

#include <unistd.h>

//#define DBUG

using namespace Pds;

static char buff[32*1024];

PipeApp::PipeApp(int read_fd, int write_fd) :
  _read_fd  (read_fd),
  _write_fd (write_fd)
{
}

InDatagram* PipeApp::events     (InDatagram* dg)
{
#ifdef DBUG
  printf("PipeApp::ev write %08x.%08x [%p : %d]\n",
         reinterpret_cast<const uint32_t*>(&dg->datagram().seq.stamp())[0],
         reinterpret_cast<const uint32_t*>(&dg->datagram().seq.stamp())[1],
         this, _write_fd);
#endif
  Transition tr(dg->datagram().seq.service(),
                Transition::Record,
                dg->datagram().seq,
                dg->datagram().env);
  ::write(_write_fd, (char*)&tr , tr.size());
  ::read (_read_fd , (char*)buff, tr.size());
#ifdef DBUG
  Transition* itr = (Transition*)buff;
  printf("PipeApp::ev read %08x.%08x [%p : %d]\n",
         reinterpret_cast<const uint32_t*>(&itr->sequence().stamp())[0],
         reinterpret_cast<const uint32_t*>(&itr->sequence().stamp())[1],
         this, _read_fd);
#endif
  return dg;
}

Transition* PipeApp::transitions(Transition* tr)
{
#ifdef DBUG
  printf("PipeApp::tr wr %08x.%08x [%p : %d]\n",
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[0],
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[1],
         this, _read_fd);
#endif
  ::write(_write_fd, (char*)tr  , tr->size());
  ::read (_read_fd , (char*)buff, tr->size());
#ifdef DBUG
  printf("PipeApp::tr read %08x.%08x [%p : %d]\n",
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[0],
         reinterpret_cast<const uint32_t*>(&tr->sequence().stamp())[1],
         this, _read_fd);
#endif
  return tr;
}
