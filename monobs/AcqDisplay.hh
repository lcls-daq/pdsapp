#ifndef Pds_WaveformDisplay_hh
#define Pds_WaveformDisplay_hh

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/acqiris/ConfigV1.hh"
#include "pds/mon/MonServerManager.hh"

namespace Pds {

  class InDatagramIterator;
  class Xtc;
  class Transition;
  class InDatagram;
  class MonEntry;
  class MonGroup;
  class MonServerManager;

  class DisplayConfig {
  public:
    DisplayConfig(char* groupName);
    ~DisplayConfig();
    void request(const Src& src);
    unsigned requested(const Src& src);
    MonEntry* entry(const Src& src,unsigned channel);
    void add(const Src& src, unsigned channel, MonEntry* entry);
    MonGroup& group() {return _group;}
  private:
    enum {MaxSrc=6};
    enum {MaxChan=8};
    char*     _groupName;
    MonGroup& _group;
    unsigned  _numentry;
    unsigned  _numsource;
    MonEntry* _entry[MaxSrc][MaxChan];
    Src       _src[MaxSrc];
  };

  class AcqDisplayConfigAction;
  class AcqDisplayL1Action;

  class AcqDisplay : public Fsm {
  public:
    AcqDisplay(DisplayConfig& dispConfig,
	       MonServerManager& monsrv);
    DisplayConfig& config() {return _dispConfig;}
    MonServerManager& monsrv() {return _monsrv;}
    ~AcqDisplay();
  private:
    MonServerManager& _monsrv;
    DisplayConfig& _dispConfig;
    AcqDisplayConfigAction* _config;
    AcqDisplayL1Action* _l1;
  };

  class AcqDisplayConfigAction : public Action, public XtcIterator {
  public:
    AcqDisplayConfigAction(AcqDisplay& disp);
    ~AcqDisplayConfigAction();
    Transition* fire(Transition* tr);
    InDatagram* fire(InDatagram* dg);
    Acqiris::ConfigV1& acqcfg() {return _config;}
    int process(const Xtc& xtc,InDatagramIterator* iter);
  private:
    AcqDisplay& _disp;
    GenericPool _iter;
    Acqiris::ConfigV1 _config;
  };

  class AcqDisplayL1Action : public Action, public XtcIterator {
  public:
    AcqDisplayL1Action(AcqDisplay& disp, Acqiris::ConfigV1& config);
    ~AcqDisplayL1Action();
    Transition* fire(Transition* tr);
    InDatagram* fire(InDatagram* dg);
    int process(const Xtc& xtc,InDatagramIterator* iter);
  private:
    AcqDisplay& _disp;
    GenericPool _iter;
    ClockTime   _now;
    Acqiris::ConfigV1& _config;
  };

}

#endif
