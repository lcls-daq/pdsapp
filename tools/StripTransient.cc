#include "pdsapp/tools/StripTransient.hh"

#include "pds/client/XtcStripper.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/xtc/Datagram.hh"

#include <stdio.h>

//#define DBUG

using namespace Pds;

class TStripper : public XtcStripper {
public:
  enum Status {Stop, Continue};
  TStripper(Xtc* root, uint32_t*& pwrite) : XtcStripper(root,pwrite) {}
  ~TStripper() {}

protected:
  void process(Xtc* xtc) {
    if (xtc->contains.value() == _transientXtcType.value()) {
#ifdef DBUG
      printf("Stripping %08x.%08x\n",xtc->src.log(),xtc->src.phy());
#endif
      return;
    }
    else
      XtcStripper::process(xtc);
  }
};

bool StripTransient::process(Dgram& dg) 
{
  uint32_t* pdg = reinterpret_cast<uint32_t*>(&(dg.xtc));
  TStripper iter(&(dg.xtc),pdg);
  iter.iterate();
  return true;
}
