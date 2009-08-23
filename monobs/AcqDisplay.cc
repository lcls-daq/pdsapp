#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/acqiris/ConfigV1.hh"
#include "pdsdata/acqiris/DataDescV1.hh"

#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescWaveform.hh"
#include "pds/mon/MonEntryWaveform.hh"
#include "pds/mon/MonDescProf.hh"
#include "pds/mon/MonEntryProf.hh"

#include "pds/service/Semaphore.hh"

#include "AcqDisplay.hh"

#include <time.h>
#include <stdio.h>
#include <string.h>

namespace Pds {

  class AcqDisplayUnconfig : public Action {
  public:
    AcqDisplayUnconfig(MonServerManager& monsrv,
		       DisplayConfig& dc, 
		       DisplayConfig& dcprof) : _monsrv(monsrv), _dc(dc),_dcprof(dcprof) {}
    ~AcqDisplayUnconfig() {}
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* dg) {
      _monsrv.dontserve();
      _dc.reset(); _dcprof.reset();
      _monsrv.serve();
      return dg; }
  private:
    MonServerManager& _monsrv;
    DisplayConfig& _dc;
    DisplayConfig& _dcprof;
  };
};

using namespace Pds;

DisplayConfig::DisplayConfig (char* groupNameModifier, MonCds& cds) :
  _cds(cds),_numsource(0),_groupNameModifier(groupNameModifier)
{}

DisplayConfig::~DisplayConfig() {}

unsigned DisplayConfig::requested(const Src& src) {
  for (unsigned i=0;i<_numsource;i++) if (src.phy()==_src[i].phy()) return 1;
  return 0;
}

MonEntry* DisplayConfig::entry(const Src& src, unsigned channel) {
  for (unsigned i=0;i<_numsource;i++) {
    if (src.phy()==_src[i].phy()) return _entry[i][channel];
  }
  throw("DisplayConfig::entry: Source/channel not found");
}

void DisplayConfig::reset() {
  for (unsigned i=0;i<_numsource;i++) {
    _cds.remove(_group[i]);
    delete _group[i];
  }
  _numsource=0;
}

void DisplayConfig::request(const Src& src, const Acqiris::ConfigV1& config) {
  if (_numsource>=MaxSrc) throw("DisplayConfig::request: Too many sources\n");
  sprintf(_groupNameBuffer,"%s: %s",DetInfo::name(reinterpret_cast<const DetInfo&>(src)),_groupNameModifier);
  _group[_numsource]=new MonGroup(_groupNameBuffer);
  _cds.add(_group[_numsource]);
  _src[_numsource]=src;
  memcpy(&_config[_numsource],&config,sizeof(Acqiris::ConfigV1));
  _numsource++;
}

void DisplayConfig::add(const Src& src, unsigned channel, MonEntry* entry) {
  for (unsigned i=0;i<_numsource;i++) {
    if (src.phy()==_src[i].phy()) {
      if (channel>=MaxChan) throw("DisplayConfig::add: Too many channels");
      _entry[i][channel]=entry;
      _group[i]->add(entry);
    }
  }
}

Acqiris::ConfigV1* DisplayConfig::acqcfg(const Src& src)
{
  for (unsigned i=0;i<_numsource;i++)
    if (src.phy()==_src[i].phy())
      return &_config[i];
  return 0;
}

AcqDisplay::AcqDisplay(MonServerManager& monsrv) :
  _dispConfig       ("Waveform",monsrv.cds()),
  _dispConfigProfile("Profile" ,monsrv.cds())
{	
  _config = new AcqDisplayConfigAction(monsrv, _dispConfig,_dispConfigProfile);
  callback(TransitionId::Configure, _config);
  callback(TransitionId::Unconfigure, new AcqDisplayUnconfig(monsrv, _dispConfig,_dispConfigProfile));
  _l1 = new AcqDisplayL1Action(_dispConfig,_dispConfigProfile);
  callback(TransitionId::L1Accept, _l1);
}

AcqDisplay::~AcqDisplay() {
  delete _config;
  delete _l1;
}

//**********************************************************************

AcqDisplayConfigAction::~AcqDisplayConfigAction() {}

AcqDisplayConfigAction::AcqDisplayConfigAction(MonServerManager& monsrv,DisplayConfig& disp, DisplayConfig& dispprofile) :
  _monsrv(monsrv), _disp(disp), _dispprofile(dispprofile), _iter(sizeof(ZcpDatagramIterator),1) {}

Transition* AcqDisplayConfigAction::fire(Transition* tr) {
  return tr;
}

InDatagram* AcqDisplayConfigAction::fire(InDatagram* dg) {
  _monsrv.dontserve();
  InDatagramIterator* in_iter = dg->iterator(&_iter);
  iterate(dg->datagram().xtc,in_iter);
  delete in_iter;
  _monsrv.serve();
  return dg;
}

int AcqDisplayConfigAction::process(const Xtc& xtc,
				    InDatagramIterator* iter)
{
  if (xtc.contains.id()==TypeId::Id_Xtc)
    return iterate(xtc,iter);

  if (!_disp.requested(xtc.src) && (xtc.contains.id() == TypeId::Id_AcqConfig)) {
    Acqiris::ConfigV1& config = (*(Acqiris::ConfigV1*)(xtc.payload()));
    _disp.request(xtc.src,config);
    _dispprofile.request(xtc.src,config);
    Acqiris::HorizV1& horiz = config.horiz();
    
    unsigned channelMask = config.channelMask();
    unsigned chnum=0;
    char buff[64];
    while (channelMask) {
      sprintf(buff,"Channel %d",chnum);
      MonDescWaveform desc(buff,"Time [s]","Voltage [V]",horiz.nbrSamples(),
                       0.0,horiz.sampInterval()*horiz.nbrSamples());
      _disp.add(xtc.src,chnum,new MonEntryWaveform(desc));
      MonDescProf     profdesc(buff,"Time [s]","Avg Voltage [V]",horiz.nbrSamples(),
			       0.0,horiz.sampInterval()*horiz.nbrSamples(),0);
      _dispprofile.add(xtc.src,chnum,new MonEntryProf(profdesc));
      chnum++;
      channelMask&=(channelMask-1);
    }
  }
  int advance = 0;
  return advance;
}

//**********************************************************************

AcqDisplayL1Action::~AcqDisplayL1Action() {}

AcqDisplayL1Action::AcqDisplayL1Action(DisplayConfig& disp, DisplayConfig& dispprofile) :
  _disp(disp),_dispprofile(dispprofile),
  _iter(sizeof(ZcpDatagramIterator),1)
{}

Transition* AcqDisplayL1Action::fire(Transition* tr) {
  return tr;
}

InDatagram* AcqDisplayL1Action::fire(InDatagram* in) {
  _now = in->datagram().seq.clock();
  InDatagramIterator* in_iter = in->iterator(&_iter);
  iterate(in->datagram().xtc,in_iter);
  delete in_iter;
  return in;
}

int AcqDisplayL1Action::process(const Xtc& xtc,
				InDatagramIterator* iter)
{
  if (xtc.contains.id()==TypeId::Id_Xtc)
    return iterate(xtc,iter);

  if (_disp.requested(xtc.src) && (xtc.contains.id() == TypeId::Id_AcqWaveform)) {
    Acqiris::DataDescV1* ddesc = (Acqiris::DataDescV1*)(xtc.payload());
    const Acqiris::ConfigV1& config = *_disp.acqcfg(xtc.src);
    const Acqiris::HorizV1& hcfg = config.horiz();
    MonCds& cds = _disp.cds();

    cds.payload_sem().take();  // make image update atomic

    for (unsigned i=0;i<config.nbrChannels();i++) {
      const int16_t* data = ddesc->waveform(hcfg);
      data += ddesc->indexFirstPoint();
      float slope = config.vert(i).slope();
      float offset = config.vert(i).offset();
      MonEntryWaveform* entry = (MonEntryWaveform*)(_disp.entry(xtc.src,i));
      MonEntryProf* profentry = (MonEntryProf*)(_dispprofile.entry(xtc.src,i));
      unsigned nbrSamples = hcfg.nbrSamples();
      for (unsigned j=0;j<nbrSamples;j++) {
        int16_t swap = (data[j]&0xff<<8) | (data[j]&0xff00>>8);
        double val = swap*slope-offset;
        entry->content(val,j);
        profentry->addy(val,j);
      }
      entry->time(_now);
      profentry->time(_now);
      ddesc = ddesc->nextChannel(hcfg);
    }

    cds.payload_sem().give();  // make image update atomic
  }
  int advance = 0;
  return advance;
}
