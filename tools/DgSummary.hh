#ifndef Pds_DgSummary_hh
#define Pds_DgSummary_hh

#include "pds/utility/Appliance.hh"
#include "pds/client/XtcIterator.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {
  class SummaryDg;

  class DgSummary : public Appliance,
		    public XtcIterator {
  public:
    DgSummary();
    ~DgSummary();
  public:
    Transition* transitions(Transition* tr);
    InDatagram* events     (InDatagram* dg);
  public:
    int process(const Xtc&, InDatagramIterator*);
  private:
    SummaryDg*  _out;
    GenericPool _dgpool;
    GenericPool _itpool;
  };
};

#endif
