#ifndef Pds_CamDisplay_hh
#define Pds_CamDisplay_hh

#include "pds/utility/Appliance.hh"
#include "pds/client/XtcIterator.hh"

#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/ClockTime.hh"

namespace Pds {

  class MonServerManager;
  class MonEntryImage;
  class MonEntryTH1F;
  class MonFex;

  class CamDisplay : public Appliance, XtcIterator {
  public:
    CamDisplay(const char* name,
	       unsigned detectorId,
	       MonServerManager& monsrv);
    ~CamDisplay();

    int process(const Xtc& xtc,
		InDatagramIterator* iter);

    Transition* transitions(Transition* in) { return in; }
    InDatagram* occurrences(InDatagram* in) { return in; }
    InDatagram* events     (InDatagram* in);

  private:
    unsigned          _detectorId;
    GenericPool       _iter;
    MonServerManager& _monsrv;
    ClockTime         _now;
    MonEntryImage*    _image;
    MonFex*           _fex;
  };
}

#endif
