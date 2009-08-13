#include "CamDisplay.hh"

#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonEntryImage.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescImage.hh"
#include "pds/mon/MonDescTH1F.hh"

#include "pds/client/Action.hh"
#include "pds/client/XtcIterator.hh"

#include "pds/service/Semaphore.hh"
#include "pds/service/GenericPool.hh"

#include "pds/camera/FrameType.hh"
#include "pds/camera/TwoDGaussianType.hh"

#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"

#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"

#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <map>

#include <time.h>
#include <math.h>

namespace Pds {

  enum { BinShift=1 };
  enum { FexMeanShift =3 };
  enum { FexWidthShift=2 };

  class CamFex {
  public:
    CamFex(MonGroup& group, unsigned width, unsigned height, unsigned depth) {
      int nb = 128;
      float maxi = (1<<depth)*width*height;
      group.add(integral = 
		new MonEntryTH1F(MonDescTH1F("Integral",
					     "Counts","Events",
					     nb,0.,maxi)));
      group.add(logintegral = 
		new MonEntryTH1F(MonDescTH1F("Log10Integral",
					     "Log10(Counts)","Events",
					     nb,0.,log10(maxi))));
      nb = width>>FexMeanShift;
      group.add(meanx = 
		new MonEntryTH1F(MonDescTH1F("MeanX",
					     "Pixel","Events",
					     nb,0.,float(nb<<FexMeanShift))));
      nb = height>>FexMeanShift;
      group.add(meany = 
		new MonEntryTH1F(MonDescTH1F("MeanY",
					     "Pixel","Events",
					     nb,0.,float(nb<<FexMeanShift))));
      nb = (width > height ? width : height)>>(FexWidthShift+1);
      group.add(major = 
		new MonEntryTH1F(MonDescTH1F("Major",
					     "Pixels","Events",
					     nb,0.,float(nb<<FexWidthShift))));
      group.add(minor = 
		new MonEntryTH1F(MonDescTH1F("Minor",
					     "Pixels","Events",
					     nb,0.,float(nb<<FexWidthShift))));
      group.add(tilt = 
		new MonEntryTH1F(MonDescTH1F("Tilt",
					     "Radians","Events",
					     128,-M_PI_2,M_PI_2)));
    }

    ~CamFex() {
    }
    int update(InDatagramIterator& iter,
	       const ClockTime& now) {
      TwoDGaussianType gss;
      int advance = iter.copy(&gss, sizeof(TwoDGaussianType));
      integral->addcontent(1.,double(gss.integral()));
      logintegral->addcontent(1.,log10(double(gss.integral())));
      meanx   ->addcontent(1.,unsigned(gss.xmean())>>FexMeanShift);
      meany   ->addcontent(1.,unsigned(gss.ymean())>>FexMeanShift);
      major   ->addcontent(1.,unsigned(gss.major_axis_width())>>FexWidthShift);
      minor   ->addcontent(1.,unsigned(gss.minor_axis_width())>>FexWidthShift);
      tilt    ->addcontent(1.,gss.major_axis_tilt());
      integral->time(now);
      logintegral->time(now);
      meanx   ->time(now);
      meany   ->time(now);
      major   ->time(now);
      minor   ->time(now);
      tilt    ->time(now);
      return advance;
    }
  private:
    MonEntryTH1F* integral;
    MonEntryTH1F* logintegral;
    MonEntryTH1F* meanx;
    MonEntryTH1F* meany;
    MonEntryTH1F* major;
    MonEntryTH1F* minor;
    MonEntryTH1F* tilt;
  };

  class DisplayGroup {
  public:
    DisplayGroup(const char* name,
		 unsigned width,
		 unsigned height,
		 unsigned depth,
		 MonServerManager& monsrv) :
      _sem(monsrv.cds().payload_sem()) 
    {
      char name_buffer[64];
      { sprintf(name_buffer,"%s Image",name);
	MonGroup* group = new MonGroup(name_buffer);
	monsrv.cds().add(group);
	MonDescImage desc("Image",width>>BinShift,height>>BinShift,1<<BinShift,1<<BinShift);
	group->add(_image = new MonEntryImage(desc)); }
      { sprintf(name_buffer,"%s Fex",name);
	MonGroup* group = new MonGroup(name_buffer);
	monsrv.cds().add(group);
	_fex = new CamFex(*group,width,height,depth); }
    }
    ~DisplayGroup() { delete _image; delete _fex; }
  public:
    int  update_frame(InDatagramIterator& iter,
		      const ClockTime& now) {
      _sem.take();

      //  copy the frame header
      FrameType frame;
      int advance = iter.copy(&frame, sizeof(FrameType));

      unsigned remaining = frame.data_size();
      unsigned short offset = frame.offset();
      iovec iov;
      unsigned ix=0,iy=0;
      MonEntryImage& image = *_image;

      //  Ignoring the possibility of fragmenting on an odd-byte
      while(remaining) {
	int len = iter.read(&iov,1,remaining);
	remaining -= len;
	const unsigned short* w = (const unsigned short*)iov.iov_base;
	const unsigned short* end = w + (len>>1);
	while(w < end) {
	  if (*w > offset)
	    image.addcontent(*w - offset,ix>>BinShift,iy>>BinShift);
	  if (++ix==frame.width()) { ix=0; iy++; }
	  w++;
	}
      }
      
      advance += frame.data_size();

      //  zero out the information overlay
      for(unsigned ip=0; ip<4; ip++)
	image.content(0,ip,0);
      
      _sem.give();

      image.time(now);
      return advance;
    }
    int  update_fex  (InDatagramIterator& iter,
		      const ClockTime& now) {
      return _fex->update(iter, now);
    }
  private:
    Semaphore&     _sem;
    MonEntryImage* _image;
    CamFex*        _fex;
  };

  class ConfigAction : public Action, XtcIterator {
  public:
    ConfigAction(MonServerManager& monsrv) : 
      _monsrv(monsrv), 
      _iter(sizeof(ZcpDatagramIterator),1) 
    {}
    ~ConfigAction() {}
  public:
    DisplayGroup* group(unsigned phy) { 
      MapType::const_iterator it = _groups.find(phy);
      return (it == _groups.end()) ? 0 : it->second;
    } 
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* in) {
      _monsrv.dontserve();
      InDatagramIterator* in_iter = in->iterator(&_iter);
      iterate(in->datagram().xtc,in_iter);
      delete in_iter;
      _monsrv.serve();
      return in;
    }
    int process(const Xtc& xtc,
		InDatagramIterator* iter) {

      if (xtc.contains.id()==TypeId::Id_Xtc)
	return iterate(xtc,iter);

      int advance = 0;
      switch(xtc.contains.id()) {
      case TypeId::Id_Opal1kConfig: 
	{ const Pds::Opal1k::ConfigV1& cfg = *reinterpret_cast<const Pds::Opal1k::ConfigV1*>(xtc.payload());
	  _groups.insert(ElType(xtc.src.phy(),
				new DisplayGroup(DetInfo::name((DetInfo&)xtc.src),
						 Opal1kConfigType::Column_Pixels,
						 Opal1kConfigType::Row_Pixels,
						 cfg.output_resolution_bits(),
						 _monsrv)));
	  break; }
      case TypeId::Id_TM6740Config:
	{ const Pds::Pulnix::TM6740ConfigV1& cfg = *reinterpret_cast<const Pds::Pulnix::TM6740ConfigV1*>(xtc.payload());
	  _groups.insert(ElType(xtc.src.phy(),
				new DisplayGroup(DetInfo::name((DetInfo&)xtc.src),
						 TM6740ConfigType::Column_Pixels >> cfg.horizontal_binning(),
						 TM6740ConfigType::Row_Pixels >> cfg.vertical_binning(),
						 cfg.output_resolution_bits(),
						 _monsrv)));
	  break; }
      default: break;
      }
      return advance;
    }
    void reset() {
      _monsrv.dontserve();
      _monsrv.cds().reset();
    }
  private:
    MonServerManager& _monsrv;
    GenericPool   _iter;
    typedef std::map <unsigned,DisplayGroup*> MapType;
    typedef std::pair<unsigned,DisplayGroup*> ElType;
    MapType _groups;
  };

  class UnconfigAction : public Action, XtcIterator {
  public:
    UnconfigAction(ConfigAction& config) : _config(config) {}
    ~UnconfigAction() {}
  public:
    Transition* fire(Transition* tr) { _config.reset(); return tr; }
    InDatagram* fire(InDatagram* dg) { return dg; }
    int process(const Xtc&, InDatagramIterator*) { return 0; }
  private:
    ConfigAction& _config;
  };

  class L1Action : public Action, XtcIterator {
  public:
    L1Action(ConfigAction& config) : 
      _config(config), 
      _iter(sizeof(ZcpDatagramIterator),1) 
    {}
    ~L1Action() {}
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* in) {
      _now = in->datagram().seq.clock();
      InDatagramIterator* in_iter = in->iterator(&_iter);
      iterate(in->datagram().xtc,in_iter);
      delete in_iter;
      return in;
    }
  public:
    int process(const Xtc& xtc, 
		InDatagramIterator* iter) {
      if (xtc.contains.id()==TypeId::Id_Xtc)
	return iterate(xtc,iter);

      int advance = 0;
      if (xtc.contains.id() == TypeId::Id_Frame) {
	DisplayGroup* g = _config.group(xtc.src.phy());
	advance += g==0 ? 0 : g->update_frame(*iter,_now);
      }
      else if (xtc.contains.id() == TypeId::Id_TwoDGaussian) {
	DisplayGroup* g = _config.group(xtc.src.phy());
	advance += g==0 ? 0 : g->update_fex(*iter, _now);
      }
      return advance;
    }
  private:
    ConfigAction& _config;
    GenericPool   _iter;
    ClockTime     _now;
  };
};

using namespace Pds;


CamDisplay::CamDisplay(MonServerManager& monsrv) 
{	
  ConfigAction* config = new ConfigAction(monsrv);
  callback(TransitionId::Configure  , config);
  callback(TransitionId::Unconfigure, new UnconfigAction(*config));
  callback(TransitionId::L1Accept   , new L1Action(*config));
}

CamDisplay::~CamDisplay()
{
}
