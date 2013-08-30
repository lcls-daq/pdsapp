#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <new>
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/acqiris.ddl.h"

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

typedef Pds::Acqiris::TdcDataV1 TdcType;

static const TypeId  _adcType (TypeId::Id_AcqWaveform,1);
static const TypeId  _adcCType(TypeId::Id_AcqConfig,1);
static const DetInfo _adcSrc  (0,
                               DetInfo::SxrEndstation,0,
                               DetInfo::Acqiris,0);

typedef Pds::Acqiris::DataDescV1 AdcType;
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
      _add_hit(t,TdcType::Chan1,TdcType::Chan2);
      _add_hit(t,TdcType::Chan3,TdcType::Chan4);

      new(alloc(4)) uint32_t((TdcType::Chan5<<28)|unsigned(t/TDC_SAMP));
      double d = MCP_TIME*ranuni();
      new(alloc(4)) uint32_t((TdcType::Chan6<<28)|unsigned((t+d)/TDC_SAMP));

      t += -log(ranuni())/ION_RATE;
    }
    unsigned nhits = reinterpret_cast<const uint32_t*>(next())-common-1;
    *common = ((TdcType::Common<<28)|nhits);
    xtc->extent = (nhits+1)*sizeof(uint32_t)+sizeof(Xtc);

    printf("%d hits : %g sec : %p %p\n",nhits,t,next(),common);
  }
private:
  void _add_hit(double t, TdcType::Source s1, TdcType::Source s2) {
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
    AdcData* t = const_cast<AdcData*>(this);
    AdcType* d = reinterpret_cast<AdcType*>(t);
    return 
      reinterpret_cast<char*>(d->nextChannel(adcCHoriz))-
      reinterpret_cast<char*>(t);
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

static Datagram* createTransition(Pool& pool, TransitionId::Value id)
{
  Transition tr(id, Env(0));
  Datagram* dg = new(&pool) Datagram(tr, _xtcType, Src(Level::Event));
  return dg;
}

int main(int argc,char **argv)
{

  FILE* f = fopen("junk.xtc","w");
  
  GenericPool* pool = new GenericPool(1<<18,1);

  Datagram* dg;
  Xtc*      xtc;

  dg = createTransition(*pool,TransitionId::Configure);
  dg->xtc.extent += (new (&dg->xtc) EbitTdcConfig)->extent;
  dg->xtc.extent += (new (&dg->xtc) EbitAdcConfig)->extent;
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginRun);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginCalibCycle);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::Enable);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  unsigned nevent=3;
  while (nevent--) {
    dg = createTransition(*pool,TransitionId::L1Accept);
    dg->xtc.extent += (new (&dg->xtc) EbitTdcData)->extent;
    dg->xtc.extent += (new (&dg->xtc) EbitAdcData)->extent;
    fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
    delete dg;
  }

  dg = createTransition(*pool,TransitionId::Disable);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndCalibCycle);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndRun);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::Unconfigure);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  fclose(f);

  return(0);
}
