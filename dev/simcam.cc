#include "pdsapp/dev/CmdLineTools.hh"

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
#include "pdsdata/cspad/ElementIterator.hh"
#include "pds/config/ImpConfigType.hh"
#include "pds/config/ImpDataType.hh"

#include "pds/service/Task.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/SrcAlias.hh"
#include "pdsdata/camera/FrameV1.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <dlfcn.h>
#include <new>

static bool verbose = false;
static int ndrop = 0;
static int ntime = 0;
static int nforward = 1;
static double ftime = 0;

typedef std::list<Pds::Appliance*> AppList;

using namespace Pds;
using Pds::Camera::FrameV1;

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
      if (++idrop==ndrop) {
	idrop=0;
	return 0;
      }
      
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
							       false))->size();
      width  = Opal1kConfigType::max_column_pixels(info);
      height = Opal1kConfigType::max_row_pixels(info);
      depth  = 12;
      offset = 32;
      break;
    case DetInfo::TM6740:
      _cfgtc = new(_cfgpayload) Xtc(_tm6740ConfigType,src);
      _cfgtc->extent += sizeof(*new (_cfgtc->next()) TM6740ConfigType(32, 32, 100, 100, false,
                                                                      TM6740ConfigType::Ten_bit,
                                                                      TM6740ConfigType::x1,
                                                                      TM6740ConfigType::x1,
                                                                      TM6740ConfigType::Linear));
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
                                                               false))->size();
      width  = QuartzConfigType::Column_Pixels;
      height = QuartzConfigType::Row_Pixels;
      depth  = 8;
      offset = 32;
      break;
    case DetInfo::Fccd:
      _cfgtc = new(_cfgpayload) Xtc(_fccdConfigType,src);
      _cfgtc->extent += sizeof(*new (_cfgtc->next()) FccdConfigType(0, true, false, 0,
								    0, 0, 0, 0, 0, 0,
								    0, 0, 0, 0, 0, 0,
								    0, 0, 0, 0, 0,
								    0, 0, 0, 0, 0,
								    0, 0, 0, 0, 0,
								    0, 0, 0, 0, 0));
      width  = FccdConfigType::Trimmed_Column_Pixels;
      height = FccdConfigType::Trimmed_Row_Pixels;
      depth  = 16;
      offset = 0;
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
	unsigned i,k=0;
	for(i=0; i<f->height()/2; i++) {
	  unsigned j;
	  { const int mean   = offset/2;
	    const int spread = offset/2;
	    for(j=0; j<f->width()/2; j++,k++)
	      const_cast<uint8_t*>(f->data())[k] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
	  { const int mean   = offset;
	    const int spread = offset;
	    for(; j<f->width(); j++,k++)
	      const_cast<uint8_t*>(f->data())[k] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
	}
	for(; i<f->height(); i++) {
	  unsigned j;
	  { const int mean   = offset;
	    const int spread = offset;
	    for(j=0; j<f->width()/2; j++,k++)
	      const_cast<uint8_t*>(f->data())[k] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
	  { const int mean   = offset/2;
	    const int spread = offset/2;
	    for(; j<f->width(); j++,k++)
	      const_cast<uint8_t*>(f->data())[k] = (offset + (rand()%spread) + (mean-spread/2))&0xff; }
	}
      }
      else if (depth<=16) {
	unsigned i,k=0;
	for(i=0; i<f->height()/2; i++) {
	  unsigned j;
	  { const int mean   = offset/2;
	    const int spread = offset/2;
	    for(j=0; j<f->width()/2; j++,k++)
	      reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(f->data()))[k] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
	  { const int mean   = offset;
	    const int spread = offset;
	    for(; j<f->width(); j++,k++)
	      reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(f->data()))[k] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
	}
	for(; i<f->height(); i++) {
	  unsigned j;
	  { const int mean   = offset;
	    const int spread = offset;
	    for(j=0; j<f->width()/2; j++,k++)
	      reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(f->data()))[k] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
	  { const int mean   = offset/2;
	    const int spread = offset/2;
	    for(; j<f->width(); j++,k++)
	      reinterpret_cast<uint16_t*>(const_cast<uint8_t*>(f->data()))[k] = (offset + (rand()%spread) + (mean-spread/2))&0xffff; }
	}
      }
      else
	;
      _evttc[i]->extent += (f->data_size()+sizeof(FrameV1)+3)&~3;
    }

    _fexpayload = new char[FexSize];
    _fextc = new(_fexpayload) Xtc(_frameFexConfigType,src);
    new(_fextc->next()) FrameFexConfigType(FrameFexConfigType::FullFrame,
					   1,
					   FrameFexConfigType::NoProcessing,
					   Pds::Camera::FrameCoord(0,0),
					   Pds::Camera::FrameCoord(0,0),
					   0,
					   0, NULL);
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
    new (_cfgtc->alloc(sizeof(CsPadConfigType)))
      CsPadConfigType(0, 40, 0, 1, 0, 0, 8*sizeof(CsPad::Section),
		      0, 0, 0xffffffff, 0xf, 0xffffffff);
    
    const size_t sz = sizeof(CsPadDataType)+8*sizeof(CsPad::Section)+sizeof(uint32_t);
    unsigned evtsz = 4*sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_CsPadDataType,src);
      for(unsigned i=0; i<4; i++) {
	CsPadDataType* q = new (_evttc[b]->alloc(sz)) CsPadDataType;
	//  Set the quad number
	reinterpret_cast<uint32_t*>(q)[1] = i<<24;
	//  Set the payload
	uint16_t* p = reinterpret_cast<uint16_t*>(q+1);
	uint16_t* e = p;
	for(unsigned j=0; j<8; j++) {
	  e += sizeof(CsPad::Section)/sizeof(uint16_t);
	  unsigned o = 0x150 + ((rand()>>8)&0x7f);
	  while(p < e)
	    *p++ = (o + ((rand()>>8)&0x3f))&0x3fff;
	}
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
    new (_cfgtc->alloc(sizeof(CsPad2x2ConfigType)))
      CsPad2x2ConfigType(0, 0, 0, 0, 2*sizeof(CsPad::Section),
			 0, 0xf, 0xf);
    
    const size_t sz = sizeof(CsPad2x2DataType)+sizeof(uint32_t);
    unsigned evtsz = sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_CsPad2x2DataType,src);
      CsPad2x2DataType* q = new (_evttc[b]->alloc(sz)) CsPad2x2DataType;
      //  Set the quad number
      reinterpret_cast<uint32_t*>(q)[1] = 0;
      //  Set the payload
      uint16_t* p = reinterpret_cast<uint16_t*>(&q->pair[0][0]);
      uint16_t* e = reinterpret_cast<uint16_t*>(q+1);
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
      ImpConfigType(ImpConfigType::defaultValue(ImpConfigType::Range),
		    ImpConfigType::defaultValue(ImpConfigType::Cal_range),
		    ImpConfigType::defaultValue(ImpConfigType::Reset),
		    ImpConfigType::defaultValue(ImpConfigType::Bias_data),
		    ImpConfigType::defaultValue(ImpConfigType::Cal_data),
		    ImpConfigType::defaultValue(ImpConfigType::BiasDac_data),
		    ImpConfigType::defaultValue(ImpConfigType::Cal_strobe),
		    ImpConfigType::defaultValue(ImpConfigType::NumberOfSamples),
		    ImpConfigType::defaultValue(ImpConfigType::TrigDelay),
		    ImpConfigType::defaultValue(ImpConfigType::Adc_delay));
    
    const size_t sz = sizeof(ImpDataType)+cfg->get(ImpConfigType::NumberOfSamples)*sizeof(Pds::Imp::Sample)+sizeof(uint32_t);
    unsigned evtsz = sz + sizeof(Xtc);
    unsigned evtst = (evtsz+3)&~3;
    _evtpayload = new char[NBuffers*evtst];

    for(unsigned b=0; b<NBuffers; b++) {
      _evttc[b] = new(_evtpayload+b*evtst) Xtc(_ImpDataType,src);
      ImpDataType* q = new (_evttc[b]->alloc(sz)) ImpDataType;
      //  Set the payload
      uint16_t* p = reinterpret_cast<uint16_t*>(q+1);
      uint16_t* e = p + cfg->get(ImpConfigType::NumberOfSamples)*Pds::Imp::channelsPerDevice;
      unsigned o = 0x150 + ((rand()>>8)&0x7f);
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
          const char *          aliasName = NULL) :
    _task     (task),
    _platform (platform),
    _user_apps(user_apps)
  {
    if (SimFrameV1::handles(src))
      _app = new SimFrameV1(src);
    else if (SimCspad::handles(src))
      _app = new SimCspad(src);
    else if (SimCspad140k::handles(src))
      _app = new SimCspad140k(src);
    else if (SimImp::handles(src))
      _app = new SimImp(src);

    if (lCompress)
      _user_apps.push_front(new FrameCompApp(_app->max_size()));

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
};

void printUsage(char* s) {
  printf( "Usage: %s [-h] -i <detinfo> -p <platform>\n"
	  "    -h          Show usage\n"
	  "    -p          Set platform id           [required]\n"
	  "    -i          Set device info           [required]\n"
	  "                    integer/integer/integer/integer or string/integer/string/integer\n"
	  "                    (e.g. XppEndStation/0/Opal1000/1 or 22/0/3/1)\n"
	  "    -u <alias>  Set device alias\n"
	  "    -v          Toggle verbose mode\n"
	  "    -C <N>      Compress frames and add uncompressed frame every N events\n"
	  "    -O          Use OpenMP\n"
	  "    -D <N>      Drop every N events\n"
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

  DetInfo info;
  AppList user_apps;

  extern char* optarg;
  char* endPtr;
  char* uniqueid = (char *)NULL;
  int c;
  while ( (c=getopt( argc, argv, "i:p:vC:OD:T:L:P:S:u:h")) != EOF ) {
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        printUsage(argv[0]);
        return -1;
      }
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'v':
      verbose = !verbose;
      printf("Verbose mode now %s\n", verbose ? "true" : "false");
      break;
    case 'C':
      lCompress = true;
      FrameCompApp::setCopyPresample(strtoul(optarg, NULL, 0));
      break;
    case 'O':
      FrameCompApp::useOMP(true);
      break;
    case 'D':
      ndrop = strtoul(optarg, NULL, 0);
      break;
    case 'T':
      ntime = strtoul(optarg, &endPtr, 0);
      ftime = strtod (endPtr+1, &endPtr);
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
      nforward = strtoul(optarg,NULL,0);
      break;
    case 'S':
      ToEventWireScheduler::setMaximum(strtoul(optarg,NULL,0));
      break;
    case 'u':
      uniqueid = optarg;
      break;
    case 'h':
      printUsage(argv[0]);
      return 0;
    default:
      printf("Option %c not understood\n", (char) c&0xff);
      printUsage(argv[0]);
      return 1;
    }
  }

  if (platform == NO_PLATFORM) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

  Task* task = new Task(Task::MakeThisATask);
  Node node(Level::Source,platform);
  info = DetInfo(node.pid(), info.detector(), info.detId(), info.device(), info.devId());
  SegTest* segtest = new SegTest(task, 
                                 platform, 
                                 info,
                                 user_apps,
                                 lCompress,
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
