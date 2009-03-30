#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/acqiris/ConfigV1.hh"
#include "pdsdata/acqiris/DataDescV1.hh"

#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonEntryTH1F.hh"

#include "pds/service/Semaphore.hh"

#include "AcqDisplay.hh"

#include <time.h>
#include <stdio.h>
#include <string.h>

using namespace Pds;

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

void DisplayConfig::request(Src& src) {
  if (_numsource>=MaxSrc) throw("DisplayConfig::request: Too many sources\n");
  _src[_numsource]=src;
  _numsource++;
}

void DisplayConfig::add(const Src& src, unsigned channel, MonEntry* entry) {
  for (unsigned i=0;i<_numsource;i++) {
    if (src.phy()==_src[i].phy()) {
      if (channel>=MaxChan) throw("DisplayConfig::add: Too many channels");
      _entry[i][channel]=entry;
      _group.add(entry);
    }
  }
}

DisplayConfig::DisplayConfig (char* groupName) :
    _groupName(groupName),_group(*new MonGroup(groupName)),_numentry(0),_numsource(0)
{}

AcqDisplay::AcqDisplay(DisplayConfig& dispConfig) :
  _monsrv(MonPort::Mon),_dispConfig(dispConfig)
{	
  _monsrv.cds().add(&_dispConfig.group());
  _monsrv.serve();
  _config = new AcqDisplayConfigAction(*this);
  callback(TransitionId::Configure, _config);
  _l1 = new AcqDisplayL1Action(*this,_config->acqcfg());
  callback(TransitionId::L1Accept, _l1);
}

AcqDisplay::~AcqDisplay() {
  delete _config;
  delete _l1;
}

//**********************************************************************

AcqDisplayConfigAction::~AcqDisplayConfigAction() {}

AcqDisplayConfigAction::AcqDisplayConfigAction(AcqDisplay& disp) :
  _disp(disp), _iter(sizeof(ZcpDatagramIterator),1) {}

Transition* AcqDisplayConfigAction::fire(Transition* tr) {
  return tr;
}

InDatagram* AcqDisplayConfigAction::fire(InDatagram* dg) {
  InDatagramIterator* in_iter = dg->iterator(&_iter);
  iterate(dg->datagram().xtc,in_iter);
  delete in_iter;
  return dg;
}

int AcqDisplayConfigAction::process(const Xtc& xtc,
                                         InDatagramIterator* iter)
{
  if (xtc.contains.id()==TypeId::Id_Xtc)
    return iterate(xtc,iter);

  if (_disp.config().requested(xtc.src) && (xtc.contains.id() == TypeId::Id_AcqConfig)) {
    Acqiris::ConfigV1& config = (*(Acqiris::ConfigV1*)(xtc.payload()));
    memcpy(&_config,&config,sizeof(Acqiris::ConfigV1));
    Acqiris::HorizV1& horiz = config.horiz();
    
    unsigned channelMask = config.channelMask();
    unsigned chnum=0;
    while (channelMask) {
      MonDescTH1F desc("Acqiris","Time [s]","Voltage [V]",horiz.nbrSamples(),
                       0.0,horiz.sampInterval()*horiz.nbrSamples());
      _disp.config().add(xtc.src,chnum,new MonEntryTH1F(desc));
      chnum++;
      channelMask&=(channelMask-1);
    }
  }
  int advance = 0;
  return advance;
}

//**********************************************************************

AcqDisplayL1Action::~AcqDisplayL1Action() {}

AcqDisplayL1Action::AcqDisplayL1Action(AcqDisplay& disp, Acqiris::ConfigV1& config) :
  _disp(disp),
  _iter(sizeof(ZcpDatagramIterator),1),
  _config(config)
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

  if (_disp.config().requested(xtc.src) && (xtc.contains.id() == TypeId::Id_AcqWaveform)) {
    Acqiris::DataDescV1* ddesc = (Acqiris::DataDescV1*)(xtc.payload());
    Acqiris::HorizV1& hcfg = _config.horiz();
    MonCds& cds = _disp.monsrv().cds();

    cds.payload_sem().take();  // make image update atomic

    // this constants reflects two things:
    // 1) the fact that 512 is the full-scale acqiris value,
    //    since the value read out is a signed 10-bit number
    // 2) the 10-bit acqiris data is shifted left by 6 bits,
    //    presumably to take advantage of improved performance
    //    using signed arithmetic.
    const unsigned normalize=
      (2<<Acqiris::DataDescV1::NumberOfBits)*(1<<NumberOfBits);
    for (unsigned i=0;i<_config.nbrChannels();i++) {
      uint16_t* data = ddesc->waveform(hcfg);
      float slope = _config.vert(i).fullScale()/(float)(normalize);
      float offset = _config.vert(i).offset();
      MonEntryTH1F* entry = (MonEntryTH1F*)(_disp.config().entry(xtc.src,i));
      unsigned nbrSamples = hcfg.nbrSamples();
      for (unsigned j=0;j<nbrSamples;j++) {
        double val = data[j]*slope-offset;
        entry->content(val,j);
      }
      entry->time(_now);
      ddesc = ddesc->nextChannel(hcfg);
    }

    cds.payload_sem().give();  // make image update atomic
  }
  int advance = 0;
  return advance;
}
