
#include <stdio.h>

#include "Recorder.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"

using namespace Pds;

Recorder::Recorder(const char* fname) : Appliance(), _pool(new GenericPool(sizeof(ZcpDatagramIterator),1)) {
  printf("Opening file: %s\n",fname);
  _f=fopen(fname,"w");
}

InDatagram* Recorder::events(InDatagram* in) {
  InDatagramIterator& iter = *in->iterator(_pool);
  struct iovec iov;
  iter.read(&iov,0,0x800000);
  printf("Iterator found %x bytes at %p dg at %p next %p\n",iov.iov_len,iov.iov_base,&(in->datagram()),
         in->datagram().xtc.next());
  unsigned size = fwrite(&(in->datagram()),sizeof(in->datagram()),1,_f);
  size = fwrite(iov.iov_base,iov.iov_len,1,_f);
  fflush(_f);
  delete &iter;
  return in;
}

Transition* Recorder::transitions(Transition* tr) {
  printf("Received transition\n");
  return tr;
}

InDatagram* Recorder::occurrences(InDatagram* in) {
  return in;
}
