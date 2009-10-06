#ifndef PDS_RECORDER
#define PDS_RECORDER

#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/Src.hh"

namespace Pds {

class Recorder : public Appliance {
public:
  Recorder(const char* fname);
  ~Recorder() {}
  Transition* transitions(Transition*);
  InDatagram* occurrences(InDatagram* in);
  InDatagram* events     (InDatagram* in);

private:
  FILE* _f;
  Pool* _pool;
  enum { SizeofPath=128 };
  char     _path[SizeofPath];
  enum { SizeofConfig=0x800000 };
  char     _config[SizeofConfig];
  Src      _src;
  unsigned _node;
  unsigned _beginrunerr;
};

}
#endif
