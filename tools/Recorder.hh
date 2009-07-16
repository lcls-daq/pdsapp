#ifndef PDS_RECORDER
#define PDS_RECORDER

#include "pds/utility/Appliance.hh"

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
};

}
#endif
