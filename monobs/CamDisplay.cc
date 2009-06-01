#include "CamDisplay.hh"

#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonEntryImage.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescImage.hh"
#include "pds/mon/MonDescTH1F.hh"

#include "pds/service/Semaphore.hh"

#include "pds/camera/FrameType.hh"
#include "pds/camera/TwoDGaussianType.hh"

#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"

#include "pdsdata/xtc/Xtc.hh"

#include <time.h>
#include <math.h>

namespace Pds {
  class MonFex {
  public:
    MonFex(MonGroup&, unsigned width, unsigned height, unsigned depth);
    
    int update(InDatagramIterator& iter,
	       const ClockTime& now);
  private:
    MonEntryTH1F* integral;
    MonEntryTH1F* logintegral;
    MonEntryTH1F* meanx;
    MonEntryTH1F* meany;
    MonEntryTH1F* major;
    MonEntryTH1F* minor;
    MonEntryTH1F* tilt;
  };

};

using namespace Pds;

enum { BinShift=1 };

CamDisplay::CamDisplay(const char* name,
		       unsigned detectorId,
		       unsigned width,
		       unsigned height,
		       unsigned depth,
		       MonServerManager& monsrv) :
  _detectorId(detectorId),
  _iter      (sizeof(ZcpDatagramIterator),1),
  _monsrv    (monsrv)
{	
  char name_buffer[64];
  sprintf(name_buffer,"%s Image",name);
  { MonGroup* group = new MonGroup(name_buffer);
    monsrv.cds().add(group);
    MonDescImage desc("Image",width>>BinShift,height>>BinShift,1<<BinShift,1<<BinShift);
    group->add(_image = new MonEntryImage(desc));
  }
  sprintf(name_buffer,"%s Fex",name);
  { MonGroup* group = new MonGroup(name_buffer);
    monsrv.cds().add(group);
    _fex = new MonFex(*group,width,height,depth);
  }
}

CamDisplay::~CamDisplay()
{
}

static int updateImage(MonEntryImage& image, 
		       InDatagramIterator* iter,
		       const ClockTime& now)
{
  //  copy the frame header
  FrameType frame;
  int advance = iter->copy(&frame, sizeof(FrameType));
	
  unsigned remaining = frame.data_size();
  unsigned short offset = frame.offset();
  iovec iov;
  unsigned ix=0,iy=0;

  //  Ignoring the possibility of fragmenting on an odd-byte
  while(remaining) {
    int len = iter->read(&iov,1,remaining);
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

  //  zero out the information overlay
  for(unsigned ip=0; ip<4; ip++)
    image.content(0,ip,0);

  image.time(now);
  return advance;
}

int CamDisplay::process(const Xtc& xtc,
			InDatagramIterator* iter)
{
//   printf("xtc dmg %x  src %x/%x  ctns %x\n",
// 	 xtc.damage.value(),
// 	 xtc.src.phy(), xtc.src.log(),
// 	 xtc.contains.id());

  if (xtc.damage.value()!=0)
    return 0;

  if (xtc.contains.id()==TypeId::Id_Xtc)
    return iterate(xtc,iter);

  int advance = 0;
  if (xtc.src.phy() == _detectorId) {
    if (xtc.contains.id() == TypeId::Id_Frame) {
      Pds::Semaphore& sem = _monsrv.cds().payload_sem();
      sem.take();  // make image update atomic
      advance += updateImage(*_image, iter, _now);
      sem.give();  // make image update atomic
    }
    else if (xtc.contains.id() == TypeId::Id_TwoDGaussian) {
      advance += _fex->update(*iter, _now);
    }
  }
  return advance;
}

InDatagram* CamDisplay::events     (InDatagram* in)
{
  if (in->datagram().seq.service() == TransitionId::L1Accept) {
    _now = in->datagram().seq.clock();
    InDatagramIterator* in_iter = in->iterator(&_iter);
    iterate(in->datagram().xtc,in_iter);
    delete in_iter;
  }
  return in; 
}


enum { FexMeanShift =3 };
enum { FexWidthShift=2 };

MonFex::MonFex(MonGroup& group, unsigned width, unsigned height, unsigned depth)
{
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


int MonFex::update(InDatagramIterator& iter,
		   const ClockTime& now)
{
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

