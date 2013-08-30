#ifndef Pds_WaveformDisplay_hh
#define Pds_WaveformDisplay_hh

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/psddl/acqiris.ddl.h"
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
    DisplayConfig(const char* groupNameModifier, MonCds& cds);
    ~DisplayConfig();
    void reset();
    void request(const Src& src,const Acqiris::ConfigV1&);
    unsigned requested(const Src& src);
    MonEntry* entry(const Src& src,unsigned channel);
    void add(const Src& src, unsigned channel, MonEntry* entry);
    MonCds&   cds  () { return _cds; }
    MonGroup& group(unsigned i) {return *_group[i];}
    Acqiris::ConfigV1* acqcfg(const Src&);
  private:
    enum {MaxSrc=6};
    enum {MaxChan=6};
    MonCds& _cds;
    //    unsigned  _numentry;
    unsigned  _numsource;
    MonGroup* _group[MaxSrc];
    MonEntry* _entry[MaxSrc][MaxChan];
    Src       _src[MaxSrc];
    Acqiris::ConfigV1 _config[MaxSrc];
    const char* _groupNameModifier;
    char      _groupNameBuffer[128];
  };

  class AcqDisplayConfigAction;
  class AcqDisplayL1Action;

  class AcqDisplay : public Fsm {
  public:
    AcqDisplay(MonServerManager& cds);
    DisplayConfig& config() {return _dispConfig;}
    DisplayConfig& configProfile() {return _dispConfigProfile;}
    ~AcqDisplay();
  private:
    DisplayConfig     _dispConfig;
    DisplayConfig     _dispConfigProfile;
    AcqDisplayConfigAction* _config;
    AcqDisplayL1Action* _l1;
  };

  class AcqDisplayConfigAction : public Action, public XtcIterator {
  public:
    AcqDisplayConfigAction(MonServerManager& monsrv,DisplayConfig& disp, DisplayConfig& dispprofile);
    ~AcqDisplayConfigAction();
    Transition* fire(Transition* tr);
    InDatagram* fire(InDatagram* dg);
    int process(const Xtc& xtc,InDatagramIterator* iter);
  private:
    MonServerManager& _monsrv;
    DisplayConfig& _disp;
    DisplayConfig& _dispprofile;
    GenericPool    _iter;
  };

  class AcqDisplayL1Action : public Action, public XtcIterator {
  public:
    AcqDisplayL1Action(DisplayConfig& disp, DisplayConfig& dispprofile);
    ~AcqDisplayL1Action();
    Transition* fire(Transition* tr);
    InDatagram* fire(InDatagram* dg);
    int process(const Xtc& xtc,InDatagramIterator* iter);
  private:
    DisplayConfig& _disp;
    DisplayConfig& _dispprofile;
    GenericPool    _iter;
    ClockTime      _now;
  };

}

#endif
