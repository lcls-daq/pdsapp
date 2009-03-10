#include "pds/client/XtcIterator.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/camera/Frame.hh"
#include "pds/camera/TwoDMoments.hh"

#include <time.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <math.h>

using namespace Pds;

class CDatagramFileIterator {
public:
  CDatagramFileIterator(FILE* file, size_t maxDgramSize) :
    _file(file), 
    _maxDgramSize(maxDgramSize), 
    _pool(maxDgramSize+sizeof(CDatagram),1),
    _cdg (new(&_pool) CDatagram)
  {}
  ~CDatagramFileIterator() 
  { delete _cdg; }
  CDatagram* next()
  {
    Datagram& dg = _cdg->datagram();
    fread(&dg, sizeof(dg), 1, _file);
    if (feof(_file)) return 0;
    unsigned payloadSize = dg.xtc.sizeofPayload();
    if (payloadSize>_maxDgramSize) {
      printf("Datagram size 0x%x larger than maximum: 0x%x\n",
	     payloadSize,_maxDgramSize);
      return 0;
    }
    fread(dg.xtc.payload(), payloadSize, 1, _file);
    return (feof(_file)) ? 0 : _cdg;
  }
private:
  FILE*       _file;
  unsigned    _maxDgramSize;
  GenericPool _pool;
  CDatagram*  _cdg;
};
  
class CamAnalysis : public XtcIterator {
public:
  CamAnalysis(unsigned detectorId) :
    _detectorId(detectorId),
    _iter      (sizeof(ZcpDatagramIterator),1)
  {	
  }
  ~CamAnalysis()
  {
  }

  int process(const Xtc& xtc,
	      InDatagramIterator* iter)
  {
    if (xtc.contains.id()==TypeId::Id_Xtc)
      return iterate(xtc,iter);

    int advance = 0;
    if (xtc.src.phy() == _detectorId) {
      if (xtc.contains.id() == TypeId::Id_Frame) {
	//  copy the frame header
	Frame frame;
	advance += iter->copy(&frame, sizeof(Frame));

	unsigned short* frame_data = 
	  reinterpret_cast<unsigned short*>(iter->read_contiguous(frame.data_size(),0));
	advance += frame.data_size();

	TwoDMoments moments(frame.width(), frame.height(), 
			    frame.offset(), frame_data);
	printf("integral %016llx\n",moments._n);
	printf("xmoment  %016llx\n",moments._x);
	printf("ymoment  %016llx\n",moments._y);
	printf("xxmoment %016llx\n",moments._xx);
	printf("yymoment %016llx\n",moments._yy);
	printf("xymoment %016llx\n",moments._xy);
      }
    }
    return advance;
  }

  void event(InDatagram* in)
  {
    if (in->datagram().seq.service() == TransitionId::L1Accept) {
      InDatagramIterator* in_iter = in->iterator(&_iter);
      iterate(in->datagram().xtc,in_iter);
      delete in_iter;
    }
  }
private:
  unsigned          _detectorId;
  GenericPool       _iter;
};


int main(int argc, char** argv) {

  DetInfo::Detector det(DetInfo::NoDetector);
  unsigned detid(0), devid(0);
  char* xtcname=0;

  int c;
  while ((c = getopt(argc, argv, "d:f:w:")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'd':
      det    = (DetInfo::Detector)strtoul(optarg, &endPtr, 0);
      detid  = strtoul(endPtr, &endPtr, 0);
      devid  = strtoul(endPtr, &endPtr, 0);
      break;
    case 'f':
      xtcname = optarg;
      break;
    default:
      break;
    }
  }

  if (!xtcname) {
    //    usage(argv[0]);
    exit(2);
  }

  FILE* file = fopen(xtcname,"r");
  if (!file) {
    perror("Unable to open file %s\n");
    exit(2);
  }

  CamAnalysis* analysis = 
    new CamAnalysis(DetInfo(0,
			    det, detid,
			    DetInfo::Opal1000, devid).phy());

  CDatagramFileIterator iter(file,0x1000000);
  CDatagram* cdg;
  while ((cdg = iter.next())) {
    analysis->event(cdg);
  }

  fclose(file);

  delete analysis;
  return 0;
}

