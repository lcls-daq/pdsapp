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

#include <vector>

#include <time.h>
#include <math.h>

namespace Pds {

  enum { BinShift=1 };
  enum { FexMeanShift =3 };
  enum { FexWidthShift=2 };

  class CamFex {
  public:
    CamFex(const char* name, MonCds& cds, unsigned width, unsigned height, unsigned depth) :
      _cds(cds)
    {
      char name_buffer[64];
      sprintf(name_buffer,"%s Fex",name);
      _group = new MonGroup(name_buffer);
      _cds.add(_group);

      MonGroup& group = *_group;
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
      _cds.remove(_group);
      delete _group;
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
    MonCds&       _cds;
    MonGroup*     _group;
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
		 MonCds&  cds) :
      _cds(cds)
    {
      char name_buffer[64];
      sprintf(name_buffer,"%s Image",name);
      _group = new MonGroup(name_buffer);
      _cds.add(_group);
      MonDescImage desc("Image",width>>BinShift,height>>BinShift,1<<BinShift,1<<BinShift);
      _group->add(_image = new MonEntryImage(desc));
      _fex = new CamFex(name,cds,width,height,depth);
    }
    ~DisplayGroup() { 
      delete _fex; 
      _cds.remove(_group);
      delete _group;
    }
  public:
    int  update_frame(InDatagramIterator& iter,
		      const ClockTime& now) {
      Semaphore& sem = _cds.payload_sem();
      sem.take();

      //  copy the frame header
      FrameType frame;
      int advance = iter.copy(&frame, sizeof(FrameType));

      unsigned remaining = frame.data_size();
      unsigned short offset = frame.offset();
      iovec iov;
      unsigned ix=0,iy=0;
      MonEntryImage& image = *_image;

      const int mask = (1<<BinShift)-1;

      //  Ignoring the possibility of fragmenting on an odd-byte
      while(remaining) {
	int len = iter.read(&iov,1,remaining);
	remaining -= len;
	const unsigned short* w = (const unsigned short*)iov.iov_base;
	const unsigned short* end = w + (len>>1);
	while(w < end) {
	  if ((ix&mask)==0 && (iy&mask)==0)
	    image.content(0,ix>>BinShift,iy>>BinShift);
	  if (*w > offset)
	    image.addcontent(*w-offset,ix>>BinShift,iy>>BinShift);
	  if (++ix==frame.width()) { ix=0; iy++; }
	  w++;
	}
      }
      
      advance += frame.data_size();

      //  zero out the information overlay
      for(unsigned ip=0; ip<4; ip++)
	image.content(0,ip,0);
      
      sem.give();

      image.time(now);
      return advance;
    }
    int  update_fex  (InDatagramIterator& iter,
		      const ClockTime& now) {
      return _fex->update(iter, now);
    }
    MonGroup* group() { return _group; }
  private:
    MonCds&        _cds;
    MonGroup*      _group;
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
      for(unsigned i=0; i<_src.size(); i++)
	if (_src[i]==phy)
	  return _groups[i];
      return 0;
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

      // already booked?
      for(unsigned i=0; i<_src.size(); i++)
	if (_src[i]==xtc.src.phy())
	  return 0;

      int advance = 0;
      switch(xtc.contains.id()) {
      case TypeId::Id_Opal1kConfig: 
	{ const Pds::Opal1k::ConfigV1& cfg = *reinterpret_cast<const Pds::Opal1k::ConfigV1*>(xtc.payload());
	  _src   .push_back(xtc.src.phy());
	  _groups.push_back(new DisplayGroup(DetInfo::name((DetInfo&)xtc.src),
					     Opal1kConfigType::Column_Pixels,
					     Opal1kConfigType::Row_Pixels,
					     cfg.output_resolution_bits(),
					     _monsrv.cds()));
	  printf("Created group %d @ xtc %p\n", _groups.size(), &xtc);
	  break; }
      case TypeId::Id_TM6740Config:
	{ const TM6740ConfigType& cfg = *reinterpret_cast<const TM6740ConfigType*>(xtc.payload());
	  _src   .push_back(xtc.src.phy());
	  _groups.push_back(new DisplayGroup(DetInfo::name((DetInfo&)xtc.src),
					     TM6740ConfigType::Column_Pixels >> cfg.horizontal_binning(),
					     TM6740ConfigType::Row_Pixels >> cfg.vertical_binning(),
					     cfg.output_resolution_bits(),
					     _monsrv.cds()));
	  break; }
      default: break;
      }
      return advance;
    }
    void reset() {
      _monsrv.dontserve();
      for(unsigned i=0; i<_groups.size(); i++)
	delete _groups[i];
      _src   .clear();
      _groups.clear();
      _monsrv.serve();
    }
  private:
    MonServerManager& _monsrv;
    GenericPool   _iter;
    std::vector<unsigned>      _src;
    std::vector<DisplayGroup*> _groups;
  };

  class UnconfigAction : public Action {
  public:
    UnconfigAction(ConfigAction& config) : _config(config) {}
    ~UnconfigAction() {}
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* dg) { _config.reset(); return dg; }
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
