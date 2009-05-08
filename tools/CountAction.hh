#include <stdio.h>

#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"

namespace Pds {

class CountAction: public Appliance {
public:
  CountAction() : _counter(0), _transitions(0) {}
  ~CountAction()
  {
    printf("CountAction: %.5d transitions\n"
	   "             %.5d events\n",
	   _transitions, _counter);
  }
  virtual Transition* transitions(Transition* in)
  {
    ++_transitions;
    printf ("transition %d\n", _transitions);
    return in;
  }

  virtual InDatagram* events(InDatagram* in) 
  {
    const Datagram& dg = in->datagram();
    if (_seq.stamp() >= dg.seq.stamp()) 
      printf("%x/%x was out of order. Last was %x/%x. Size %i\n",
	     dg.seq.stamp().fiducials(), dg.seq.stamp().ticks(), 
             _seq.stamp().fiducials(), _seq.stamp().ticks(), 
             dg.xtc.sizeofPayload());
    _seq = dg.seq;
    if (!(++_counter%1000)){
      printf ("nevents      = %d\n", _counter);
      printf ("current size = %d\n\n", dg.xtc.sizeofPayload());
    }
    return in;
  }
  virtual InDatagram* occurrences(InDatagram* dg) { return dg; }
private:
  unsigned _counter;
  unsigned _transitions;
  Sequence _seq;
};
}
