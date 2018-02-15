
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/psddl/generic1d.ddl.h"
#include "pdsdata/psddl/quadadc.ddl.h"

using namespace Pds;

class MyAnalysis {
public:
  MyAnalysis() : _adcCfgBuff(0), _genCfgBuff(0), _nevents(0) {}
  ~MyAnalysis() 
  {
    if (_adcCfgBuff) 
      delete[] _adcCfgBuff;
    if (_genCfgBuff)
      delete[] _genCfgBuff;
  }
public:
  void config(const Pds::QuadAdc::ConfigV1& cfg)
  {
    //  Make a copy
    _adcCfgBuff = new char[cfg._sizeof()];
    memcpy(_adcCfgBuff, &cfg, cfg._sizeof());
    //  Point to the copy
    _adcCfg = reinterpret_cast<const Pds::QuadAdc::ConfigV1*>(_adcCfgBuff);
  }
  void config(const Pds::Generic1D::ConfigV0& cfg)
  {
    //  Make a copy
    _genCfgBuff = new char[cfg._sizeof()];
    memcpy(_genCfgBuff, &cfg, cfg._sizeof());
    //  Point to the copy
    _genCfg = reinterpret_cast<const Pds::Generic1D::ConfigV0*>(_genCfgBuff);
  }
  void event(const Pds::Sequence& seq) 
  {
    _seq = seq;
  }
  void event(const Pds::Generic1D::DataV0& data)
  {
    //
    //  Inspect the data
    //
    printf("-- Event %u [%9u.%09u] --\n", 
           _nevents, 
           _seq.clock().seconds(), 
           _seq.clock().nanoseconds());

    //  Loop over channels
    for(unsigned ch=0; ch<_genCfg->NChannels(); ch++) {
      ndarray<const uint16_t,1> a = data.data_u16(*_genCfg, ch);
      printf("-- Channel %u --\n", ch);
      //  Loop over samples in waveform
      for(unsigned k=0; k<a.shape()[0]; k++)
        printf("%3x%c", a[k], (k%16)==15 ? '\n':' ');
    }      

    _nevents++;
  }
  unsigned nevents() const { return _nevents; }
private:
  char*                           _adcCfgBuff;
  char*                           _genCfgBuff;
  const Pds::QuadAdc::ConfigV1*   _adcCfg;
  const Pds::Generic1D::ConfigV0* _genCfg;
  Pds::Sequence                   _seq;
  unsigned                        _nevents;
};

class myLevelIter : public XtcIterator {
public:
  enum {Stop, Continue};
  myLevelIter(Xtc* xtc, unsigned depth, MyAnalysis& analysis) : 
    XtcIterator(xtc), _depth(depth), _analysis(analysis) {}

  int process(Xtc* xtc) {
    const DetInfo& info = *(DetInfo*)(&xtc->src);
    Level::Type level = xtc->src.level();
    if (level < 0 || level >= Level::NumberOfLevels )
    {
        printf("Unsupported Level %d\n", (int) level);
        return Continue;
    }    
    switch (xtc->contains.id()) {
    case (TypeId::Id_Xtc) : {
      myLevelIter iter(xtc,_depth+1,_analysis);
      iter.iterate();
      break;
    }
    case Pds::QuadAdc::ConfigV1::TypeId:
      _analysis.config(*reinterpret_cast<const Pds::QuadAdc::ConfigV1*>(xtc->payload()));
      break;
    case Pds::Generic1D::ConfigV0::TypeId:
      _analysis.config(*reinterpret_cast<const Pds::Generic1D::ConfigV0*>(xtc->payload()));
      break;
    case Pds::Generic1D::DataV0::TypeId:
      _analysis.event (*reinterpret_cast<const Pds::Generic1D::DataV0*>(xtc->payload()));
      break;
    default :
      break;
    }
    return Continue;
  }
private:
  unsigned    _depth;
  MyAnalysis& _analysis;
};

void usage(char* progname) {
  fprintf(stderr,"Usage: %s -f <filename> [-h]\n", progname);
}

int main(int argc, char* argv[]) {
  int c;
  char* xtcname=0;
  unsigned nevents=10;
  int parseErr = 0;

  while ((c = getopt(argc, argv, "f:n:h")) != -1) {
    switch (c) {
    case 'f':
      xtcname = optarg;
      break;
    case 'n':
      nevents = strtoul(optarg, NULL, 0);
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    default:
      parseErr++;
    }
  }
  
  if (!xtcname) {
    usage(argv[0]);
    exit(2);
  }

  int fd = open(xtcname,O_RDONLY | O_LARGEFILE);
  if (fd < 0) {
    perror("Unable to open file %s\n");
    exit(2);
  }

  MyAnalysis analysis;
  XtcFileIterator iter(fd,0x900000);
  Dgram* dg;
  while ((analysis.nevents()<nevents) &&
         (dg = iter.next())) {
    myLevelIter iter(&(dg->xtc),0,analysis);
    iter.iterate();
  }

  close(fd);
  return 0;
}
