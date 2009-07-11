#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include "pds/xtc/Datagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/TypeId.hh"

using namespace Pds;

class MyPVData : public Xtc {
public:
  MyPVData(unsigned value) : Xtc(TypeId(TypeId::Any,Version),Src(Level::Recorder)) {
    alloc(sizeof(*this)-sizeof(Xtc));
    _value = value;
  }
  unsigned value() const {return _value;}
private:
  enum {Version=1};
  unsigned _value;
};

int main(int argc,char **argv)
{

  FILE* f = fopen("junk.xtc","w");
  
  GenericPool* pool = new GenericPool(10000,1);

  unsigned nevent=3;
  const unsigned version=1;
  while (nevent--) {
    TypeId contains(TypeId::Any,version);
    Src src;
    Datagram& dg = *new(pool) Datagram(contains,src);
    // ideally this should be a constructor argument to Dgram
    // dg.sequence=clock_gettime();

    unsigned nvar=2;
    while (nvar--) new(&dg.xtc) MyPVData(nvar);
    fwrite(&dg,sizeof(Dgram)+dg.xtc.sizeofPayload(),1,f);
    delete &dg;
  }

  fclose(f);

  return(0);
}
