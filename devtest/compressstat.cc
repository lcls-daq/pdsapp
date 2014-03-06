#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/ana/XtcRun.hh"
#include "pdsdata/compress/CompressedXtc.hh"
#include <boost/shared_ptr.hpp>

#include <list>
#include <string>

#include <glob.h>
#include <math.h>
#include <string.h>

using namespace Pds;

static bool debug = false;

class XtcStats {
public:
  XtcStats(const DetInfo& src) :
    _src(src),
    _ncmp(0),
    _szcmp(0),
    _szcmpsq(0),
    _nucmp(0),
    _szucmp(0),
    _ndmg(0),
    _same(0),
    _diff(0) 
  { 
    if (debug) printf("XtcStats %08x.%08x %s\n",
                      src.log(),src.phy(),DetInfo::name(src));
  }
public:
  void dump() const {
    printf("%20.20s", DetInfo::name(_src));
    printf("%12d",_ncmp);
    printf("%12d",_nucmp);
    printf("%12d",_ndmg);
    if (_ncmp)  printf("%12d",unsigned(_szcmp/double(_ncmp)));
    else        printf("%12.12s","-");
    if (_nucmp) printf("%12d",unsigned(_szucmp/double(_nucmp)));
    else        printf("%12.12s","-");
    if (_ncmp && _nucmp) {
      double m = _szcmp/double(_ncmp);
      double s = sqrt( _szcmpsq/double(_ncmp) - m*m );
      s /= _szucmp/double(_nucmp);
      printf("%12.8f",_szcmp*double(_nucmp)/(_szucmp*double(_ncmp)));
      printf("%12.8f",s);
      printf("%12d",_same);
      printf("%12d",_diff);
      printf("%12d",_nucmp-_same-_diff);
    }
    else {
      printf("%12.12s","-");
      printf("%12.12s","-");
      printf("%12.12s","-");
      printf("%12.12s","-");
      printf("%12.12s","-");
    }
    printf("\n");
  }
  static void dumpHeader() {
    printf("%20.20s","Detector");
    printf("%12.12s","# Cmp");
    printf("%12.12s","# Ucmp");
    printf("%12.12s","# Dmg");
    printf("%12.12s","Avg Cmp Sz");
    printf("%12.12s","Avg Ucmp Sz");
    printf("%12.12s","Ratio");
    printf("%12.12s","Ratio RMS");
    printf("%12.12s","# Same");
    printf("%12.12s","# Diff");
    printf("%12.12s","# Busy");
    printf("\n");
  }
  void pre_event() { _pcmp = _pucmp = 0; }
  void post_event() {
    if (_pcmp && _pucmp) {
      boost::shared_ptr<Xtc> pxtc = Pds::CompressedXtc::uncompress(*_pcmp);
      if (pxtc.get()) {
        if (memcmp(_pucmp,pxtc.get(),_pucmp->extent)==0) _same++;
        else _diff++;
      }
    }
  }
  void event(const Xtc& xtc) {
    if (xtc.src.phy() == _src.phy()) {
      if (xtc.damage.value()) {
        _ndmg++;
      }
      else if (xtc.contains.compressed()) {
        _ncmp++;
        double dsz = double(xtc.sizeofPayload());
        _szcmp   += dsz;
        _szcmpsq += dsz*dsz;
        _pcmp = &xtc;
      }
      else {
        _nucmp++;
        _szucmp += double(xtc.sizeofPayload());
        _pucmp = &xtc;
      }
    }
  }
private:
  DetInfo  _src;
  unsigned _ncmp;
  double   _szcmp;
  double   _szcmpsq;
  unsigned _nucmp;
  double   _szucmp;
  unsigned _ndmg;
  unsigned _same;
  unsigned _diff;
  const Xtc* _pcmp;
  const Xtc* _pucmp;
};

class CfgIter : public XtcIterator {
public:
  CfgIter(std::list<XtcStats*>& stats) : _stats(stats) {}
public:
  int process(Xtc* xtc) {
    if (xtc->contains.id() == TypeId::Id_Xtc)
      iterate(xtc);
    else if (xtc->src.level()==Pds::Level::Source) {
      _stats.push_back(new XtcStats(static_cast<DetInfo&>(xtc->src)));
      return 0;
    }
    return 1;
  }
private:
  std::list<XtcStats*>& _stats;
};

class L1Iter : public XtcIterator {
public:
  L1Iter(std::list<XtcStats*>& stats) : _stats(stats) {}
public:
  int process(Xtc* xtc) {
    if (xtc->contains.id() == TypeId::Id_Xtc)
      iterate(xtc);
    else {
      for(std::list<XtcStats*>::iterator it=_stats.begin();
          it!=_stats.end(); it++) 
        (*it)->event(*xtc);
    }
    return 1;
  }
private:
  std::list<XtcStats*>& _stats;
};

static const unsigned MAX_DG_SIZE = 0x3000000;


void usage(char* progname) {
  fprintf(stderr,
          "Usage: %s -p <path> -r <run>\n",
          progname);
}

int main(int argc, char* argv[]) {
  int c;
  const char* path = 0;
  unsigned run = 0;
  unsigned parseErr = 0;
  unsigned nupdate = 100;

  while ((c = getopt(argc, argv, "dhn:p:r:")) != -1) {
    switch (c) {
    case 'd': debug = true; break;
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'n':
      nupdate = atoi(optarg);
      break;
    case 'r':
      run = atoi(optarg);
      break;
    case 'p':
      path = optarg;
      break;
    default:
      parseErr++;
    }
  }
  
  if (!(parseErr==0 && run && path)) {
    usage(argv[0]);
    exit(2);
  }

  Pds::Ana::XtcRun files;
  files.live_read(true);

  char buff[256];
  sprintf(buff,"%s/*-r%04d-s*.xtc",path,run);
  glob_t g;
  glob(buff, 0, 0, &g);

  if (debug)
    printf("glob %s found %zu files\n",buff,g.gl_pathc);

  for(unsigned i=0; i<g.gl_pathc; i++) {
    char* p = g.gl_pathv[i];
    if (debug) printf("adding file %s\n",p);
    if (i==0) files.reset   (p);
    else      files.add_file(p);
  }

  files.init();
  std::list<XtcStats*> stats;

  Dgram* dg = NULL;
  int slice = -1;
  int64_t offset = -1;
  unsigned events = 0;

  while(1) {
    Pds::Ana::Result result = files.next(dg, &slice, &offset);
    if (debug) {
      printf("next result %d  service %s\n",
             int(result), TransitionId::name(dg->seq.service()));
    }
    if (result == Pds::Ana::OK) {
      switch (dg->seq.service()) {
      case Pds::TransitionId::Configure:
        { CfgIter iter(stats); 
          iter.iterate(&dg->xtc); } 
        break;
      case Pds::TransitionId::L1Accept:
        { L1Iter  iter(stats); 
          for(std::list<XtcStats*>::iterator it=stats.begin();
              it!=stats.end(); it++)
            (*it)->pre_event();
          iter.iterate(&dg->xtc); 
          for(std::list<XtcStats*>::iterator it=stats.begin();
              it!=stats.end(); it++)
            (*it)->post_event();
          if ((++events % nupdate) == 0) {
            XtcStats::dumpHeader();
            for(std::list<XtcStats*>::iterator it=stats.begin();
                it!=stats.end(); it++)
              (*it)->dump();
          }
        } 
        break;
      default: break;
      }
    }
    else break;
  }

  return 0;
}

