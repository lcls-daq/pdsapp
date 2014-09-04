#include "pds/service/CmdLineTools.hh"

#include "pds/client/FrameCompApp.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"

#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/utility/ToEventWireScheduler.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/QuartzConfigType.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/config/CsPadDataType.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/config/CsPad2x2DataType.hh"
#include "pdsdata/psddl/cspad.ddl.h"
#include "pds/config/ImpConfigType.hh"
#include "pds/config/ImpDataType.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/alias.ddl.h"

#include "pdsdata/psddl/epix.ddl.h"
#include "pdsdata/psddl/epixsampler.ddl.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <math.h>
#include <dlfcn.h>
#include <new>

static bool verbose = false;
static int fdrop = 0;
static int ndrop = 1;
static int ntime = 0;
static int nforward = 1;
static double ftime = 0;

static const unsigned sizeof_Section = 2*Pds::CsPad::ColumnsPerASIC*Pds::CsPad::MaxRowsPerASIC*sizeof(int16_t);

typedef std::list<Pds::Appliance*> AppList;

using namespace Pds;
using Pds::Camera::FrameV1;
using Pds::Alias::SrcAlias;

//
//#include "pds/config/EpixConfigType.hh"
//
static TypeId _EpixConfigType(TypeId::Id_EpixConfig,1);
typedef Epix::ConfigV1 EpixConfigType;

static TypeId _EpixDataType  (TypeId::Id_EpixElement,1);
typedef Epix::ElementV1 EpixDataType;

static TypeId _EpixSamplerConfigType(TypeId::Id_EpixSamplerConfig,1);
typedef EpixSampler::ConfigV1 EpixSamplerConfigType;

static TypeId _EpixSamplerDataType  (TypeId::Id_EpixSamplerElement,1);
typedef EpixSampler::ElementV1 EpixSamplerDataType;


class SimApp : public Appliance {
public:
  virtual ~SimApp() {}
  virtual size_t max_size() const = 0;
private:
  virtual void _execute_configure() = 0;
  virtual void _insert_configure (InDatagram*) = 0;
  virtual void _insert_event     (InDatagram*) = 0;
public:
  Transition* transitions(Transition* tr) 
  { 
    switch(tr->id()) {
    case TransitionId::Configure:
      _execute_configure();
      break;
    case TransitionId::L1Accept:
      break;
    default:
      break;
    }
    return tr; 
  }
  InDatagram* events     (InDatagram* dg) 
  {
    switch(dg->seq.service()) {
    case TransitionId::Configure:
      _insert_configure(dg);
      break;
    case TransitionId::L1Accept:
    {
      static int itime=0;
      if ((++itime)==ntime) {
        itime = 0;
        timeval ts = { int(ftime), int(drem(ftime,1)*1000000) };
        select( 0, NULL, NULL, NULL, &ts);
      }

      static int idrop=0;
      if (++idrop==fdrop) {
        idrop=0;
        return 0;
      }
      else if (idrop<fdrop && idrop>fdrop-ndrop)
        return 0;
      
      static int iforward=0;
      if (++iforward==nforward) {
        iforward=0;
        _insert_event(dg);
      }

      break;
    }
    default:
      break;
    }
    return dg; 
  }
};

class SimFrameV1 : public SimApp {
  enum { CfgSize = 0x1000 };
  enum { FexSize = 0x1000 };
  enum { EvtSize = 0x1000000 };
public:
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Opal1000:
    case DetInfo::Opal2000:
    case DetInfo::Opal4000:
    case DetInfo::Opal1600:
    case DetInfo::Opal8000:
    case DetInfo::TM6740:
    case DetInfo::Quartz4A150:
    case DetInfo::Fccd:
    case DetInfo::Fccd960:
      return true;
    default:
      break;
    }
    return false;
  }
public:
  SimFrameV1(const Src& src) 
  {
    _cfgpayload = new char[CfgSize];

    unsigned width,height,depth,offset;
    float    fdummy [32]; memset( fdummy,0,32*sizeof(float));
    uint16_t usdummy[32]; memset(usdummy,0,32*sizeof(uint16_t));
 
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Opal1000:
    case DetInfo::Opal2000:
    case DetInfo::Opal4000:
    case DetInfo::Opal1600:
    case DetInfo::Opal8000:
      _cfgtc = new(_cfgpayload) Xtc(_opal1kConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) Opal1kConfigType(32, 100, 
                                                               Opal1kConfigType::Twelve_bit,
                                                               Opal1kConfigType::x1,
                                                               Opal1kConfigType::None,
                                                               true,
                                                               false, 
                                                               false, 0, 0, 0))->_sizeof();
      width  = Pds::Opal1k::max_column_pixels(info);
      height = Pds::Opal1k::max_row_pixels(info);
      depth  = 12;
      offset = 32;
      break;
    case DetInfo::TM6740:
      _cfgtc = new(_cfgpayload) Xtc(_tm6740ConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) TM6740ConfigType(32, 32, 100, 100, false,
                                                               TM6740ConfigType::Ten_bit,
                                                               TM6740ConfigType::x1,
                                                               TM6740ConfigType::x1,
                                                               TM6740ConfigType::Linear))->_sizeof();
      width  = TM6740ConfigType::Column_Pixels;
      height = TM6740ConfigType::Row_Pixels;
      depth  = 10;
      offset = 32;
      break;
    case DetInfo::Quartz4A150:
      _cfgtc = new(_cfgpayload) Xtc(_quartzConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) QuartzConfigType(32, 100, 
                                                               QuartzConfigType::Eight_bit,
                                                               QuartzConfigType::x1,
                                                               QuartzConfigType::x1,
                                                               QuartzConfigType::None,
                                                               false,
                                                               false, 0, 0, 0))->_sizeof();
      width  = QuartzConfigType::Column_Pixels;
      height = QuartzConfigType::Row_Pixels;
      depth  = 8;
      offset = 32;
      break;
    case DetInfo::Fccd:
      _cfgtc = new(_cfgpayload) Xtc(_fccdConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) FccdConfigType(0, true, false, 0,
                                                             fdummy, usdummy))->_sizeof();
      width  = FccdConfigType::Trimmed_Column_Pixels;
      height = FccdConfigType::Trimmed_Row_Pixels;
      depth  = 16;
      offset = 0x1000;
      break;
    case DetInfo::Fccd960:
      _cfgtc = new(_cfgpayload) Xtc(_fccdConfigType,src);
      _cfgtc->extent += (new (_cfgtc->next()) FccdConfigType(0, true, false, 0,
                                                             fdummy, usdummy))->_sizeof();
      width  = 960;
      height = 960;
      depth  = 16;      // depth=16: corrected frame; depth=13: raw frame
      offset = 0x1000;
      break;
    default:
      printf("Unsupported camera %s\n",Pds::DetInfo::name(info.device()));
      exit(1);
    }

    //
    //  Create several random frames (constant offset + spread by quadrant)
    //
    unsigned evtsz = (sizeof(Xtc) + sizeof(FrameV1) + width*height*((depth+7)/8));
    unsigned evtst = (evtsz + 3)&~3;
    _evtpayload = new char[NBuffers*evtst];
    for(unsigned i=0; i<NBuffers; i++) {
      _evttc[i] = new(_evtpayload+i*evtst) Xtc(TypeId(TypeId::Id_Frame,1),src);
      FrameV1* f = new(_evttc[i]->next()) FrameV1(width, height, depth, offset);
      if (depth<=8) {
        ndarray<const uint8_t, 2> idata = f->data8();
        ndarray<uint8_t, 2> fdata = make_ndarray(const_cast<uint8_t*>(idata.data()), idata.shape()[0], idata.shape()[1]);
        unsigned i;
        for(i=0; i<f->height()/2; i++) {
          unsigned j;
          { const int mean   = offset/2;
            const int spread = offset/2;
            for(j=0; j<f->width()/2; j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
          { const int mean   = offset;
            const int spread = offset;
            for(; j<f->width(); j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
        }
        for(; i<f->height(); i++) {
          unsigned j;
          { const int mean   = offset;
            const int spread = offset;
            for(j=0; j<f->width()/2; j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
          { const int mean   = offset/2;
            const int spread = offset/2;
            for(; j<f->width(); j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
        }
      }
      else if (depth<=16) {
        ndarray<const uint16_t, 2> idata = f->data16();
        ndarray<uint16_t, 2> fdata = make_ndarray(const_cast<uint16_t*>(idata.data()), idata.shape()[0], idata.shape()[1]);
        unsigned i;
        for(i=0; i<f->height()/2; i++) {
          unsigned j;
          { const int mean   = offset/2;
            const int spread = offset/2;
            for(j=0; j<f->width()/2; j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
          { const int mean   = offset;
            const int spread = offset;
            for(; j<f->width(); j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
        }
        for(; i<f->height(); i++) {
          unsigned j;
          { const int mean   = offset;
            const int spread = offset;
            for(j=0; j<f->width()/2; j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
          { const int mean   = offset/2;
            const int spread = offset/2;
            for(; j<f->width(); j++)
              fdata[i][j] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
        }
      }
      else
        ;
      _evttc[i]->extent += (f->_sizeof()+3)&~3;
    }

    _fexpayload = new char[FexSize];
    _fextc = new(_fexpayload) Xtc(_frameFexConfigType,src);
    new(_fextc->next()) FrameFexConfigType(FrameFexConfigType::FullFrame,
                                           1,
                                           FrameFexConfigType::NoProcessing,
                                           Pds::Camera::FrameCoord(0,0),
                                           Pds::Camera::FrameCoord(0,0),
                                           0, 0, 0);
    _fextc->extent += sizeof(FrameFexConfigType);
  }
  ~SimFrameV1() 
  {
    delete[] _cfgpayload;
    delete[] _fexpayload;
    delete[] _evtpayload;
  }
private:
  void _execute_configure()
  {
    _ibuffer = 0;
  }
  void _insert_configure(InDatagram* dg)
  {
    dg->insert(*_cfgtc,_cfgtc->payload());
    dg->insert(*_fextc,_fextc->payload());
  }
  void _insert_event(InDatagram* dg)
  {
    dg->insert(*_evttc[_ibuffer],_evttc[_ibuffer]->payload());
    if (++_ibuffer==NBuffers) _ibuffer=0;
  }
public:
  size_t max_size() const { return _evttc[0]->sizeofPayload(); }
private:
  char* _cfgpayload;
  char* _fexpayload;
  char* _evtpayload;
  Xtc*  _cfgtc;
  Xtc*  _fextc;

  enum { NBuffers=16 };
  Xtc*  _evttc[NBuffers];
  unsigned _ibuffer;
};


class SimCspad : public SimApp {
  enum { CfgSize = sizeof(CsPadConfigType)+sizeof(Xtc) };
public:
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Cspad:
      return true;
    default:
      break;
    }
    return false;
  }
public:
  SimCspad(const Src& src) 
  {
    _cfgpayload = new char[CfgSize];
    _cfgtc = new(_cfgpayload) Xtc(_CsPadConfigType,src);

    Pds::CsPad::ProtectionSystemThreshold pt[4];
    Pds::CsPad::ConfigV3QuadReg quads[4];

    new (_cfgtc->alloc(sizeof(CsPadConfigType)))
      CsPadConfigType(0, 0, 40, 
                      pt, 0,
                      0, 1, 0, 0, 8*sizeof_Section,
                      0, 0, 0xffffffff, 0xf, 0xffffffff,
                      quads);
    
    const size_t sz = sizeof(CsPad::ElementV1)+8*sizeof_Section+sizeof(uint32_t);
    unsigned evtsz = 4*sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];
    memset(_evtpayload, 0, NBuffers*evtst);

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(TypeId(TypeId::Id_CspadElement,1),src);
      for(unsigned i=0; i<4; i++) {
        CsPad::ElementV1* q = new (_evttc[b]->alloc(sz)) CsPad::ElementV1;
        //  Initialize the header
        memset(q, 0, sizeof(*q));
        //  Set the quad number
        reinterpret_cast<uint32_t*>(q)[1] = i<<24;
        //  Set the payload
        uint16_t* p = reinterpret_cast<uint16_t*>(q+1);
#ifdef TESTPATTERN
        for(unsigned j=0; j<8; j++)
          for(unsigned k=0; k<Pds::CsPad::ColumnsPerASIC; k++)
            for(unsigned m=0; m<Pds::CsPad::MaxRowsPerASIC*2; m++)
              *p++ = 0x150 + i*0x40 + k*b/5 + m;
#else
        uint16_t off = rand()&0x2fff;
        for(unsigned j=0; j<8; j++)
          for(unsigned k=0; k<Pds::CsPad::ColumnsPerASIC; k++)
            for(unsigned m=0; m<Pds::CsPad::MaxRowsPerASIC*2; m++)
              *p++ = (rand()&0x03ff)+off;
              //*p++ = ((m<<8) + (k<<0))&0x3fff;
#endif
        // trailer word
        *p++ = 0;
        *p++ = 0;
      }
    }
  }
  ~SimCspad() 
  {
    delete[] _cfgpayload;
    delete[] _evtpayload;
  }
private:
  void _execute_configure() { _ibuffer=0; }
  void _insert_configure(InDatagram* dg)
  {
    dg->insert(*_cfgtc,_cfgtc->payload());
  }
  void _insert_event(InDatagram* dg)
  {
    dg->insert(*_evttc[_ibuffer],_evttc[_ibuffer]->payload());
    if (++_ibuffer == NBuffers) _ibuffer=0;
  }
public:
  size_t max_size() const { return _evttc[0]->sizeofPayload(); }
private:
  char* _cfgpayload;
  char* _evtpayload;
  Xtc*  _cfgtc;
  enum { NBuffers=16 };
  Xtc*  _evttc[NBuffers];
  unsigned _ibuffer;
};


class SimCspad140k : public SimApp {
  enum { CfgSize = sizeof(CsPad2x2ConfigType)+sizeof(Xtc) };
public:
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Cspad2x2:
      return true;
    default:
      break;
    }
    return false;
  }
public:
  SimCspad140k(const Src& src) 
  {
    _cfgpayload = new char[CfgSize];
    _cfgtc = new(_cfgpayload) Xtc(_CsPad2x2ConfigType,src);

    Pds::CsPad2x2::ProtectionSystemThreshold pt(-1U,-1U);
    Pds::CsPad2x2::CsPad2x2ReadOnlyCfg       ro;
    Pds::CsPad2x2::CsPad2x2DigitalPotsCfg    dpots;

    unsigned shape[2] = { Pds::CsPad2x2::ColumnsPerASIC, 
                          Pds::CsPad2x2::MaxRowsPerASIC };
    ndarray<uint16_t,2> gm(shape);
    for(unsigned x=32; x<40; x++)
      for(unsigned y=32; y<40; y++)
        gm[x][y] = 0x3;
    Pds::CsPad2x2::CsPad2x2GainMapCfg        gainmap(gm.data());

    Pds::CsPad2x2::ConfigV2QuadReg quad(0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0,
                                        0, 0, 0, ro,
                                        dpots, gainmap);

    new (_cfgtc->alloc(sizeof(CsPad2x2ConfigType)))
      CsPad2x2ConfigType(0, pt, 0,
                         0, 0, 0, 0,
                         2*sizeof_Section,
                         0, 0xf, 0xf,
                         quad);
    
    const size_t sz = CsPad2x2DataType::_sizeof()+sizeof(uint32_t);
    unsigned evtsz = sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_CsPad2x2DataType,src);
      CsPad2x2DataType* q = new (_evttc[b]->alloc(sz)) CsPad2x2DataType;
      //  Set the quad number
      reinterpret_cast<uint32_t*>(q)[1] = 0;
      //  Set the payload
      ndarray<const int16_t,3> a = q->data();
      uint16_t* p = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(a.data()));
      uint16_t* e = p+a.size();
      unsigned o = 0x150 + ((rand()>>8)&0x7f);
      while(p < e)
        *p++ = (o + ((rand()>>8)&0x3f))&0x3fff;
    }
  }
  ~SimCspad140k() 
  {
    delete[] _cfgpayload;
    delete[] _evtpayload;
  }
private:
  void _execute_configure() { _ibuffer=0; }
  void _insert_configure(InDatagram* dg)
  {
    dg->insert(*_cfgtc,_cfgtc->payload());
  }
  void _insert_event(InDatagram* dg)
  {
    dg->insert(*_evttc[_ibuffer],_evttc[_ibuffer]->payload());
    if (++_ibuffer == NBuffers) _ibuffer=0;
  }
public:
  size_t max_size() const { return _evttc[0]->sizeofPayload(); }
private:
  char* _cfgpayload;
  char* _evtpayload;
  Xtc*  _cfgtc;
  enum { NBuffers=16 };
  Xtc*  _evttc[NBuffers];
  unsigned _ibuffer;
};


class SimImp : public SimApp {
  enum { CfgSize = sizeof(ImpConfigType)+sizeof(Xtc) };
public:
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Imp:
      return true;
    default:
      break;
    }
    return false;
  }
public:
  SimImp(const Src& src) 
  {
    _cfgpayload = new char[CfgSize];
    _cfgtc = new(_cfgpayload) Xtc(_ImpConfigType,src);
    ImpConfigType* cfg = new (_cfgtc->alloc(sizeof(ImpConfigType)))
      ImpConfigType(Pds::ImpConfig::defaultValue(ImpConfigType::Range),
                    Pds::ImpConfig::defaultValue(ImpConfigType::Cal_range),
                    Pds::ImpConfig::defaultValue(ImpConfigType::Reset),
                    Pds::ImpConfig::defaultValue(ImpConfigType::Bias_data),
                    Pds::ImpConfig::defaultValue(ImpConfigType::Cal_data),
                    Pds::ImpConfig::defaultValue(ImpConfigType::BiasDac_data),
                    Pds::ImpConfig::defaultValue(ImpConfigType::Cal_strobe),
                    Pds::ImpConfig::defaultValue(ImpConfigType::NumberOfSamples),
                    Pds::ImpConfig::defaultValue(ImpConfigType::TrigDelay),
                    Pds::ImpConfig::defaultValue(ImpConfigType::Adc_delay));
    
    const size_t sz = sizeof(ImpDataType)+Pds::ImpConfig::get(*cfg,ImpConfigType::NumberOfSamples)*sizeof(Pds::Imp::Sample)+sizeof(uint32_t);
    unsigned evtsz = sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_ImpDataType,src);
      ImpDataType* q = new (_evttc[b]->alloc(sz)) ImpDataType;
      //  Set the payload
      uint16_t* p = reinterpret_cast<uint16_t*>(q+1);
      uint16_t* e = p + Pds::ImpConfig::get(*cfg,ImpConfigType::NumberOfSamples)*Pds::Imp::Sample::channelsPerDevice;
      unsigned o = 0x150 + ((rand()>>8)&0x7f);
      printf("SimImp buffer %d offset %d\n",b,o);
      while(p < e)
        *p++ = (o + ((rand()>>8)&0x3f))&0x3fff;
    }
  }
  ~SimImp() 
  {
    delete[] _cfgpayload;
    delete[] _evtpayload;
  }
private:
  void _execute_configure() { _ibuffer=0; }
  void _insert_configure(InDatagram* dg)
  {
    dg->insert(*_cfgtc,_cfgtc->payload());
  }
  void _insert_event(InDatagram* dg)
  {
    dg->insert(*_evttc[_ibuffer],_evttc[_ibuffer]->payload());
    if (++_ibuffer == NBuffers) _ibuffer=0;
  }
public:
  size_t max_size() const { return _evttc[0]->sizeofPayload(); }
private:
  char* _cfgpayload;
  char* _evtpayload;
  Xtc*  _cfgtc;
  enum { NBuffers=16 };
  Xtc*  _evttc[NBuffers];
  unsigned _ibuffer;
};


#if 1
class SimEpixBase : public SimApp {
public:
  SimEpixBase(const Src& src, unsigned AsicsPerColumn, unsigned AsicsPerRow, unsigned Rows, unsigned Columns) 
  {
    const unsigned Asics = AsicsPerRow*AsicsPerColumn;
    const unsigned CfgSize = sizeof(Epix::ConfigV1)
      +Asics*Epix::AsicConfigV1::_sizeof()          // AsicsConfigV1
      +2*Asics*Rows*((Columns+31)/32)*sizeof(uint32_t) // PixelArrays [Test,Mask]
      +sizeof(Xtc);

    _cfgpayload = new char[CfgSize];
    _cfgtc = new(_cfgpayload) Xtc(_EpixConfigType,src);

    Epix::AsicConfigV1 asics[Asics];
    uint32_t* testarray = new uint32_t[Asics*Rows*(Columns+31)/32];
    memset(testarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
    uint32_t* maskarray = new uint32_t[Asics*Rows*(Columns+31)/32];
    memset(maskarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
    unsigned asicMask = src.phy()&0xf;

    EpixConfigType* cfg = 
      new (_cfgtc->next()) EpixConfigType( 0, 1, 0, 1, 0, // version
                                           0, 0, 0, 0, 0, // asicAcq
                                           0, 0, 0, 0, 0, // asicGRControl
                                           0, 0, 0, 0, 0, // asicR0Control
                                           0, 0, 0, 0, 0, // asicR0ToAsicAcq
                                           1, 0, 0, 0, 0, // adcClkHalfT
                                           0, 0, 0, 0, 0, // digitalCardId0
                                           AsicsPerRow, AsicsPerColumn, 
                                           Rows, Columns, 
                                           0x200000, // 200MHz
                                           asicMask,
                                           asics, testarray, maskarray );
                                         
    _cfgtc->alloc(cfg->_sizeof());
    if (_cfgtc->extent != CfgSize) {
      printf("EpixConfigType size mismatch [%d/%d]\n",
             _cfgtc->extent, CfgSize);
                                                        
      printf("  Asics %d  Rows %d  Columns %d\n",Asics,Rows,Columns);
      printf("  sizeof(ConfigV1)\t%zd\n", sizeof(Epix::ConfigV1));
      printf("  sizeof(AsicConfigV1)\t%d\n", Epix::AsicConfigV1::_sizeof());
      printf("  sizeof(PixelArray)\t%zd\n", Rows*(Columns+31)/32*sizeof(uint32_t));
      printf("  AsicsPerRow %d  AsicsPerCol %d  RowsPerAsic %d  PixelsPerAsicRow %d\n",
             cfg->numberOfAsicsPerRow(),
             cfg->numberOfAsicsPerColumn(),
             cfg->numberOfRowsPerAsic(),
             cfg->numberOfPixelsPerAsicRow());
      abort();
    }

    const size_t sz = EpixDataType::_sizeof(*cfg);
    unsigned evtsz = sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_EpixDataType,src);
      EpixDataType* q = new (_evttc[b]->alloc(sz)) EpixDataType;
      //  Set the quad number
      reinterpret_cast<uint32_t*>(q)[1] = 0;
      //  Set the payload
      ndarray<const uint16_t,2> a = q->frame(*cfg);
      uint16_t* p = const_cast<uint16_t*>(a.data());
      uint16_t* e = p+a.size();
      unsigned o = 0x150 + ((rand()>>8)&0x7f);
      while(p < e)
        *p++ = (o + ((rand()>>8)&0x3f))&0x3fff;
    }
  }
  ~SimEpixBase() 
  {
    delete[] _cfgpayload;
    delete[] _evtpayload;
  }
private:
  void _execute_configure() { _ibuffer=0; }
  void _insert_configure(InDatagram* dg)
  {
    dg->insert(*_cfgtc,_cfgtc->payload());
  }
  void _insert_event(InDatagram* dg)
  {
    dg->insert(*_evttc[_ibuffer],_evttc[_ibuffer]->payload());
    if (++_ibuffer == NBuffers) _ibuffer=0;
  }
public:
  size_t max_size() const { return _evttc[0]->sizeofPayload(); }
private:
  char* _cfgpayload;
  char* _evtpayload;
  Xtc*  _cfgtc;
  enum { NBuffers=16 };
  Xtc*  _evttc[NBuffers];
  unsigned _ibuffer;
};

class SimEpix100 : public SimEpixBase {
  enum { Columns = 2*96, Rows = 2*88 };
public:
  SimEpix100(const Src& src) : SimEpixBase(src, 2, 2, Rows, Columns) {}
public:
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Epix:
      return (info.detId()%2)==0;
    default:
      break;
    }
    return false;
  }
};

class SimEpix10k : public SimEpixBase {
  enum { Columns = 96, Rows = 88 };
public:
  SimEpix10k(const Src& src) : SimEpixBase(src, 2, 2, Rows, Columns) {}
public:
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::Epix:
      return (info.detId()%2)==1;
    default:
      break;
    }
    return false;
  }
};
#endif

class SimEpixSampler : public SimApp {
public:
  SimEpixSampler(const Src& src)
  {
    const unsigned Channels=16;
    const unsigned Samples =1024;
    const unsigned BaseClkFreq=100000000;

    const unsigned CfgSize = sizeof(EpixSamplerConfigType)+sizeof(Xtc);

    _cfgpayload = new char[CfgSize];
    _cfgtc = new(_cfgpayload) Xtc(_EpixSamplerConfigType,src);

    EpixSamplerConfigType* cfg = 
      new (_cfgtc->next()) EpixSamplerConfigType( 0, 0, 0, 0, 1,
                                                  0, 0, 0, 0, 0, 
                                                  Channels, Samples, 
                                                  BaseClkFreq, 0 );
                                         
    _cfgtc->alloc(cfg->_sizeof());
    if (_cfgtc->extent != CfgSize) {
      printf("EpixSamplerConfigType size overrun [%d/%d]\n",
             _cfgtc->extent, CfgSize);
      abort();
    }

    const size_t sz = EpixSamplerDataType::_sizeof(*cfg);
    unsigned evtsz = sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_EpixSamplerDataType,src);
      EpixSamplerDataType* q = new (_evttc[b]->alloc(sz)) EpixSamplerDataType;
      //  Set the quad number
      reinterpret_cast<uint32_t*>(q)[1] = 0;
      //  Set the payload
      ndarray<const uint16_t,2> a = q->frame(*cfg);
      uint16_t* p = const_cast<uint16_t*>(a.data());
      uint16_t* e = p+a.size();
      unsigned o = 0x150 + ((rand()>>8)&0x7f);
      while(p < e)
        *p++ = (o + ((rand()>>8)&0x3f))&0x3fff;
    }
  }
  ~SimEpixSampler() 
  {
    delete[] _cfgpayload;
    delete[] _evtpayload;
  }
  static bool handles(const Src& src) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    switch(info.device()) {
    case DetInfo::EpixSampler:
      return true;
    default:
      break;
    }
    return false;
  }
private:
  void _execute_configure() { _ibuffer=0; }
  void _insert_configure(InDatagram* dg)
  {
    dg->insert(*_cfgtc,_cfgtc->payload());
  }
  void _insert_event(InDatagram* dg)
  {
    dg->insert(*_evttc[_ibuffer],_evttc[_ibuffer]->payload());
    if (++_ibuffer == NBuffers) _ibuffer=0;
  }
public:
  size_t max_size() const { return _evttc[0]->sizeofPayload(); }
private:
  char* _cfgpayload;
  char* _evtpayload;
  Xtc*  _cfgtc;
  enum { NBuffers=16 };
  Xtc*  _evttc[NBuffers];
  unsigned _ibuffer;
};


//
//  Implements the callbacks for attaching/dissolving.
//  Appliances can be added to the stream here.
//
class SegTest : public EventCallback, public SegWireSettings {
public:
  SegTest(Task*                 task,
          unsigned              platform,
          const Src&            src,
          const AppList&        user_apps,
          bool                  lCompress,
          unsigned              nCompressThreads,
          bool                  lTriggered,
          unsigned              module,
          unsigned              channel,
          const char *          aliasName = NULL) :
    _task     (task),
    _platform (platform),
    _user_apps(user_apps),
    _triggered(lTriggered),
    _module   (module),
    _channel  (channel)
  {
    if (SimFrameV1::handles(src))
      _app = new SimFrameV1(src);
    else if (SimCspad::handles(src))
      _app = new SimCspad(src);
    else if (SimCspad140k::handles(src))
      _app = new SimCspad140k(src);
    else if (SimImp::handles(src))
      _app = new SimImp(src);
#if 1
    else if (SimEpix100::handles(src))
      _app = new SimEpix100(src);
    else if (SimEpix10k::handles(src))
      _app = new SimEpix10k(src);
#endif
    else if (SimEpixSampler::handles(src))
      _app = new SimEpixSampler(src);
    else {
      const DetInfo& printInfo = static_cast<const DetInfo&>(src);
      fprintf(stderr,"Unsupported camera %s\n",Pds::DetInfo::name(printInfo.device()));
      exit(1);
    }

    if (lCompress)
      _user_apps.push_front(new FrameCompApp(_app->max_size()*2,nCompressThreads));

    _sources.push_back(src);

    if (aliasName) {
      SrcAlias tmpAlias(src, aliasName);
      _aliases.push_back(tmpAlias);
    }
  }

  virtual ~SegTest()
  {
    delete _app;

    for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
      delete (*it);

    _task->destroy();
  }

public:    
  // Implements SegWireSettings
  void connect (InletWire&, StreamParams::StreamType, int) {}
  const std::list<Src>& sources() const { return _sources; }
  const std::list<SrcAlias>* pAliases() const
  {
    return (_aliases.size() > 0) ? &_aliases : NULL;
  }
  bool     is_triggered() const { return _triggered; }
  unsigned module      () const { return _module; }
  unsigned channel     () const { return _channel; }
private:
  // Implements EventCallback
  void attached(SetOfStreams& streams)
  {
    printf("SegTest connected to platform 0x%x\n", _platform);

    Stream* frmk = streams.stream(StreamParams::FrameWork);

    for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
      (*it)->connect(frmk->inlet());

    _app->connect(frmk->inlet());
  }
  void failed(Reason reason)
  {
    static const char* reasonname[] = { "platform unavailable", 
                                        "crates unavailable", 
                                        "fcpm unavailable" };
    printf("SegTest: unable to allocate crates on platform 0x%x : %s\n", 
           _platform, reasonname[reason]);
    delete this;
  }
  void dissolved(const Node& who) { delete this; }
    
private:
  Task*          _task;
  unsigned       _platform;
  std::list<Src> _sources;
  std::list<SrcAlias> _aliases;
  SimApp*        _app;
  AppList        _user_apps;
  bool           _triggered;
  unsigned       _module;
  unsigned       _channel;
};

void printUsage(char* s) {
  printf( "Usage: %s [-h] -i <detinfo> -p <platform>[,<Mod>,<Ch>]\n"
          "    -h          Show usage\n"
          "    -p          Set platform id [required], EVR Mod/Ch\n"
          "    -i          Set device info [required]\n"
          "                    integer/integer/integer/integer or string/integer/string/integer\n"
          "                    (e.g. XppEndStation/0/Opal1000/1 or 22/0/3/1)\n"
          "    -u <alias>  Set device alias\n"
          "    -v          Toggle verbose mode\n"
          "    -C <N>[,<T>] Compress frames and add uncompressed frame every N events (using T threads)\n"
          "    -O          Use OpenMP\n"
          "    -D <F>[,<N>] Drop N events (default 1) out of every F events\n"
          "    -T <S>,<N>  Delay S seconds every N events\n"
          "    -P <N>      Only forward payload every N events\n",
          s
          );
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  const unsigned NO_PLATFORM = unsigned(-1UL);
  unsigned platform = NO_PLATFORM;
  bool lCompress = false;
  unsigned            compressThreads     = 0;
  bool lTriggered = false;
  bool lDetInfoSet = false;
  unsigned module=0, channel=0;
  unsigned int uuu;
  bool parseValid=true;

  DetInfo info;
  AppList user_apps;

  extern char* optarg;
  extern int optind;
  char* endPtr;
  char* uniqueid = (char *)NULL;
  int c;
  while ( (c=getopt( argc, argv, "i:p:vC:OD:T:L:P:S:u:h")) != EOF ) {
    switch(c) {
    case 'i':
      lDetInfoSet = true;
      parseValid &= CmdLineTools::parseDetInfo(optarg,info);
      break;
    case 'p':
      switch (CmdLineTools::parseUInt(optarg,platform,module,channel)) {
      case 1:  lTriggered = false;  break;
      case 3:  lTriggered = true;   break;
      default:
        printf("%s: option `-p' parsing error\n", argv[0]);
        parseValid = false;
        break;
      }
      break;
    case 'v':
      verbose = !verbose;
      FrameCompApp::setVerbose(verbose);
      printf("Verbose mode now %s\n", verbose ? "true" : "false");
      break;
    case 'C':
      lCompress = true;
      endPtr = index(optarg, ',');
      if (endPtr)
        *endPtr = '\0';
      parseValid &= CmdLineTools::parseUInt(optarg, uuu);
      FrameCompApp::setCopyPresample(uuu);
      if (endPtr)
        parseValid &= CmdLineTools::parseUInt(endPtr+1,compressThreads);
      break;
    case 'O':
      FrameCompApp::useOMP(true);
      break;
    case 'D':
      endPtr = index(optarg, ',');
      if (endPtr)
        *endPtr = '\0';
      parseValid &= CmdLineTools::parseInt(optarg, fdrop);
      if (endPtr)
        parseValid &= CmdLineTools::parseInt(endPtr+1, ndrop);
      break;
    case 'T':
      endPtr = index(optarg, ',');
      if (endPtr) {
        *endPtr = '\0';
        parseValid &= CmdLineTools::parseInt(optarg, ntime);
        parseValid &= CmdLineTools::parseDouble(endPtr+1, ftime);
      } else {
        parseValid = false; // error: missing comma
      }
      break;
    case 'L':
      { for(const char* p = strtok(optarg,","); p!=NULL; p=strtok(NULL,",")) {
          printf("dlopen %s\n",p);

          void* handle = dlopen(p, RTLD_LAZY);
          if (!handle) {
            printf("dlopen failed : %s\n",dlerror());
            break;
          }

          // reset errors
          const char* dlsym_error;
          dlerror();

          // load the symbols
          create_app* c_user = (create_app*) dlsym(handle, "create");
          if ((dlsym_error = dlerror())) {
            fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
            break;
          }
          user_apps.push_back( c_user() );
        }
        break;
      }
    case 'P':
      parseValid &= CmdLineTools::parseInt(optarg, nforward);
      break;
    case 'S':
      parseValid &= CmdLineTools::parseUInt(optarg, uuu);
      ToEventWireScheduler::setMaximum(uuu);
      break;
    case 'u':
      if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
        printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
      } else {
        uniqueid = optarg;
      }
      break;
    case 'h':
      printUsage(argv[0]);
      return 0;
    default:
    case '?':
      printUsage(argv[0]);
      return 1;
    }
  }

  if (platform == NO_PLATFORM) {
    printf("%s: platform required\n",argv[0]);
    parseValid = false;
  }

  if (!lDetInfoSet) {
    printf("%s: detinfo required\n",argv[0]);
    parseValid = false;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    parseValid = false;
  }

  if (!parseValid) {
    printUsage(argv[0]);
    return 1;
  }

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());
  SegTest* segtest = new SegTest(task, 
                                 platform, 
                                 info,
                                 user_apps,
                                 lCompress,
                                 compressThreads,
                                 lTriggered, module, channel,
                                 uniqueid);

  SegmentLevel* segment = new SegmentLevel(platform, 
             *segtest,
             *segtest, 
             NULL);
  if (segment->attach()) {
    task->mainLoop();
  }

  segment->detach();

  return 0;
}
