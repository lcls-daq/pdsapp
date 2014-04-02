#include "pds/utility/Appliance.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/psddl/acqiris.ddl.h"

#include <math.h>

namespace Pds {
  class AcqWriter : public Appliance {
  public:
    AcqWriter() {}
    ~AcqWriter() {}
  public:
    InDatagram* event(InDatagram*);
  private:
  };
};


using namespace Pds;

static const double TDC_SAMP    =50e-12; // TDC resolution
static const double ION_RATE    =200e3;  // Rate of ion hits (Hz)
static const double ION_CUTOFF  =8e-3;   // Ion hit cutoff time since LCLS pulse (sec)
static const double DRIFT_TIME  =100e-9; // Drift tube half-length
static const double MCP_TIME    =100e-9;

static const TypeId  _tdcType(TypeId::Id_AcqTdcData,1);
static const DetInfo _tdcSrc (0,
                              DetInfo::SxrEndstation,0,
                              DetInfo::AcqTDC,0);

typedef Pds::Acqiris::TdcDataV1  TdcType;
typedef Pds::Acqiris::TdcDataV1_Item TdcItem;

static const TypeId  _adcType (TypeId::Id_AcqWaveform,1);
static const TypeId  _adcCType(TypeId::Id_AcqConfig,1);
static const DetInfo _adcSrc  (0,
                               DetInfo::SxrEndstation,0,
                               DetInfo::Acqiris,0);

typedef Pds::Acqiris::DataDescV1Elem AdcType;
typedef Pds::Acqiris::ConfigV1   AdcCType;

static Pds::Acqiris::TrigV1  adcCTrig (Pds::Acqiris::TrigV1::DC,
                                       -1,  // external trigger input
                                       Pds::Acqiris::TrigV1::Positive,
                                       1.0); // 1 Volt trigger level

static Pds::Acqiris::HorizV1 adcCHoriz(20.e-9,
                                       0.,
                                       1000,
                                       1);

static Pds::Acqiris::VertV1  adcCVert[] = 
  { Pds::Acqiris::VertV1(5., 
                         0.,
                         Pds::Acqiris::VertV1::DC50ohm,
                         Pds::Acqiris::VertV1::None),
    Pds::Acqiris::VertV1(5., 
                         0., 
                         Pds::Acqiris::VertV1::DC50ohm,
                         Pds::Acqiris::VertV1::None) };


static double ranuni() { return double(random())/double(RAND_MAX); }

class EbitTdcConfig : public Xtc {
public:
  EbitTdcConfig() :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
  }
};

class EbitAdcConfig : public Xtc {
public:
  EbitAdcConfig() :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    Xtc* xtc = new(this) Xtc(_adcCType,_adcSrc);
    new(xtc->alloc(sizeof(AdcCType)))
      AdcCType(1, 0x3, 1, adcCTrig, adcCHoriz, adcCVert);
    extent += xtc->extent;
  }
};

class EbitTdcData : public Xtc {
public:
  EbitTdcData() :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    Xtc* xtc = new(this) Xtc(_tdcType,_tdcSrc);
    uint32_t* common = new(alloc(4)) uint32_t;
    double t = -log(ranuni())/ION_RATE;
    while(t < ION_CUTOFF) {
      _add_hit(t,TdcItem::Chan1,TdcItem::Chan2);
      _add_hit(t,TdcItem::Chan3,TdcItem::Chan4);

      new(alloc(4)) uint32_t((TdcItem::Chan5<<28)|unsigned(t/TDC_SAMP));
      double d = MCP_TIME*ranuni();
      new(alloc(4)) uint32_t((TdcItem::Chan6<<28)|unsigned((t+d)/TDC_SAMP));

      t += -log(ranuni())/ION_RATE;
    }
    unsigned nhits = reinterpret_cast<const uint32_t*>(next())-common-1;
    *common = ((Pds::Acqiris::TdcChannel::Common<<28)|nhits);
    xtc->extent = (nhits+1)*sizeof(uint32_t)+sizeof(Xtc);

    printf("%d hits : %g sec : %p %p\n",nhits,t,next(),common);
  }
private:
  void _add_hit(double t, TdcItem::Source s1, TdcItem::Source s2) {
    double d = DRIFT_TIME*ranuni();
    new(alloc(4)) uint32_t((s1<<28)|unsigned((t+d)/TDC_SAMP));
    new(alloc(4)) uint32_t((s2<<28)|unsigned((t+DRIFT_TIME-d)/TDC_SAMP));
  }
};

class AdcData {
public:
  AdcData(const Pds::Acqiris::VertV1& vert) 
  {
    char* p = reinterpret_cast<char*>(this);
    new(p) uint32_t(adcCHoriz.nbrSamples());   p+=4;
    new(p) uint32_t(0);                        p+=4;
    new(p) double  (adcCHoriz.sampInterval()); p+=8;
    new(p) double  (vert.fullScale());         p+=8;
    new(p) double  (vert.offset());            p+=8;
    new(p) uint32_t(1);                        p+=4;
    new(p) uint32_t(1);                        p+=4;
    new(p) uint32_t(1);                        p+=4;
    new(p) uint32_t(1);                        p+=4;
    new(p) uint32_t(); // data size            p+=4;
    new(p) uint32_t(); // reserved             p+=4;
    new(p) double  (); // reserved             p+=8;

    new(p) double  (0); // horpos              p+=8;
    new(p) uint32_t(0); // timestamp lo        p+=4;
    new(p) uint32_t(0); // timestamp hi        p+=4;

    const double dt = adcCHoriz.sampInterval();
    const double risetime  = 100e-9;
    const double falltime  = 1e-6;
    const double hitTime   = 100e-9;
    const double peakTime  = 300e-9;
    const unsigned HITSMP  = unsigned(hitTime/dt);
    const unsigned PEAKSMP = unsigned(peakTime/dt);
    const unsigned NSMP    = adcCHoriz.nbrSamples();

    double amp = ranuni()*2;
    int16_t* wf = new(p) int16_t;
    unsigned i=0;
    while(i < HITSMP)
      wf[i++] = _convert(vert,0);
    while(i < PEAKSMP) {
      wf[i] = _convert(vert,amp*(1-exp(dt*double(i-HITSMP)/risetime)));
      i++;
    }
    while(i < NSMP) {
      wf[i] = _convert(vert,amp*(1-exp(dt*double(PEAKSMP-HITSMP)/risetime))
                       *exp(dt*double(i-PEAKSMP)/falltime));
      i++;
    }
  }
public:
  unsigned size() const 
  {
    AdcCType cfg(1, 0x3, 1, adcCTrig, adcCHoriz, adcCVert);

    AdcData* t = const_cast<AdcData*>(this);
    AdcType* d = reinterpret_cast<AdcType*>(t);
    return d->_sizeof(cfg);
  }
private:
  int _convert(const Pds::Acqiris::VertV1& _vert,
               double v) 
  { return int(v+_vert.offset())/_vert.slope(); }
};

class EbitAdcData : public Xtc {
public:
  EbitAdcData() : 
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    Xtc* txtc = new(this) Xtc(_adcType,_adcSrc);

    AdcData* d0 = new (txtc->next()) AdcData(adcCVert[0]);
    txtc->alloc(d0->size());

    AdcData* d1 = new (txtc->next()) AdcData(adcCVert[1]);
    txtc->alloc(d1->size());

    extent += txtc->extent;
  }
};

InDatagram* Pds::AcqWriter::event(InDatagram* dg)
{
  switch (dg->seq.service()) {
  case TransitionId::L1Accept:
    dg->xtc.extent += (new (&dg->xtc) EbitTdcData)->extent;
    dg->xtc.extent += (new (&dg->xtc) EbitAdcData)->extent;
    break;
  case TransitionId::Configure:
    dg->xtc.extent += (new (&dg->xtc) EbitTdcConfig)->extent;
    dg->xtc.extent += (new (&dg->xtc) EbitAdcConfig)->extent;
    break;
  default:
    break;
  }
  return dg;
}
