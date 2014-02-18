//
//  Unofficial example of XTC compression
//
#include "pds/client/FrameCompApp.hh"
#include "pds/management/VmonServerAppliance.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/utility/Inlet.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Semaphore.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/compress/CompressedXtc.hh"
#include <boost/shared_ptr.hpp>

#include <string.h>

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <map>
using std::map;

static const unsigned MAX_DG_SIZE = 0x3000000;

static int verbose=0;

namespace Pds {
  class MyRecorder : public Appliance {
  public:
    MyRecorder(FILE* f) : _f(f) {}
    ~MyRecorder() {}
  public:
    Transition* transitions(Transition* tr) { return 0; }
    InDatagram* events     (InDatagram* in) {
      const Dgram* odg = reinterpret_cast<const Dgram*>(&in->datagram());
      fwrite(odg, sizeof(*odg) + odg->xtc.sizeofPayload(), 1, _f);
      fflush(_f);
      return in;
    }
  private:
    FILE* _f;
  };
  
  class myIter {
  public:
    enum Status {Stop, Continue};
    myIter() : _aligned(true), _pwrite(0)
    {
    }
    ~myIter() 
    {
    }  
  private:
    //
    //  Iterate through the Xtc and compress, decompress, copy into new Xtc
    //
    void iterate(Xtc* root) {
      if (root->damage.value() & ( 1 << Damage::IncompleteContribution)) {
        return _write(root,root->extent);
      }
    
      int remaining = root->sizeofPayload();
      Xtc* xtc     = (Xtc*)root->payload();

      uint32_t* pwrite = _pwrite;
      _write(root, sizeof(Xtc));
    
      while(remaining > 0)
        {
          unsigned extent = xtc->extent;
          if(extent==0) {
            printf("Breaking on zero extent\n");
            abort();
            break; // try to skip corrupt event
          }
          process(xtc);
          remaining -= extent;
          xtc        = (Xtc*)((char*)xtc+extent);
        }

      reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
    }
  
    void process(Xtc* xtc) {

      //
      //  We're only interested in compressing/decompressing
      //
      switch (xtc->contains.id()) {
      case (TypeId::Id_Xtc):
        iterate(xtc);
        return;
      default:
        break;
      }

      if (xtc->contains.compressed()) {

        boost::shared_ptr<Xtc> pxtc = Pds::CompressedXtc::uncompress(*xtc);
        if (pxtc.get()) {
          _write(pxtc.get(), pxtc->extent);
          return;
        }
        else {
          printf("extract failed on xtc dmg %08x src %08x:%08x contains %08x extent %08x\n",
                 xtc->damage.value(), xtc->src.log(), xtc->src.phy(), xtc->contains.value(), xtc->extent);
        }
      }

      _write(xtc,xtc->extent);
    }

  private:
    //
    //  Xtc headers are 32b aligned.  
    //  Compressed data is not.
    //  Enforce alignment during Xtc construction.
    //
    char* _new(ssize_t sz)
    {
      uint32_t* p = _pwrite;
      _pwrite += sz>>2;
      return (char*)p;
    }

    void _write(const void* p, ssize_t sz) 
    {
      if (!_aligned)
        perror("Writing 32b data alignment not guaranteed\n");

      const uint32_t* pread = (uint32_t*)p;
      if (_pwrite!=pread) {
        const uint32_t* const end = pread+(sz>>2);
        while(pread < end)
          *_pwrite++ = *pread++;
      }
      else
        _pwrite += sz>>2;
    }
    void _uwrite(const void* p, ssize_t sz) 
    {
      if (_aligned)
        perror("Writing 8b data when 32b alignment required\n");

      const uint8_t* pread = (uint8_t*)p;
      if (_upwrite!=pread) {
        const uint8_t* const end = pread+sz;
        while(pread < end)
          *_upwrite++ = *pread++;
      }
      else
        _upwrite += sz;
    }
    void _align_unlock()
    {
      _aligned = false;
      _upwrite = (uint8_t*)_pwrite;
    }
    void _align_lock()
    {
      _pwrite += (_upwrite - (uint8_t*)_pwrite +3)>>2;
      _aligned = true;
    }
  
  public:
    void iterate(const Dgram* dg, char* pwrite) 
    {
      _pwrite = reinterpret_cast<uint32_t*>(pwrite);
      _write(dg, sizeof(*dg)-sizeof(Xtc));
      iterate(const_cast<Xtc*>(&(dg->xtc)));
    }

  private:
    bool                                      _extract;
    bool                                      _aligned;
    uint32_t*                                 _pwrite;
    uint8_t*                                  _upwrite;
  };

  class MyInfoIterator : public Pds::XtcIterator {
  public:
    MyInfoIterator(const DetInfo& info,
                   CDatagram&     dg) :
      _info(info), _dg(dg), _found(false) {}
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==TypeId::Id_Xtc)
        iterate(xtc);
      else if (static_cast<const DetInfo&>(xtc->src)==_info) {
        if (verbose>1)
          printf("%s found at %p\n",DetInfo::name(_info),xtc);
        _found=true;
        return 0;
      }

      if (_found) {
        if (verbose>1)
          printf("Building new dg from %p\n",xtc);
        _dg.xtc.extent=0;
        memcpy(_dg.xtc.alloc(xtc->extent),xtc,xtc->extent);
        return 0;
      }

      return 1;
    }
  private:
    const DetInfo& _info;
    CDatagram&     _dg;
    bool           _found;
  };

  class MyValidator : public Appliance {
  public:
    MyValidator(Dgram*& dg) : _dg(dg), _dbuff(new char[MAX_DG_SIZE]) {}
    ~MyValidator() { delete[] _dbuff; }
  public:
    Transition* transitions(Transition* tr) { return 0; }
    InDatagram* events     (InDatagram* in) {

      const Dgram* dg = reinterpret_cast<const Dgram*>(&in->datagram());
      
      myIter iter;
      iter.iterate(dg,_dbuff);
      if (memcmp(_dg,_dbuff,sizeof(*_dg)+_dg->xtc.sizeofPayload())) {
        printf("  memcmp failed\n"); 

        dg = reinterpret_cast<const Dgram*>(_dg);
        printf("[%p]  in : %s transition: time 0x%x/0x%x, payloadSize %d\n",
               dg, TransitionId::name(dg->seq.service()),
               dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(), dg->xtc.sizeofPayload());

        dg = reinterpret_cast<const Dgram*>(_dbuff);
        printf("[%p]  out: %s transition: time 0x%x/0x%x, payloadSize %d\n",
               dg, TransitionId::name(dg->seq.service()),
               dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(), dg->xtc.sizeofPayload());
        
      }

      return in;
    }
  private:
    Dgram*&    _dg;
    char*      _dbuff;
  };

  class MyOutlet : public Appliance {
  public:
    MyOutlet(Semaphore& sem) : _sem(sem) {}
  public:
    Transition* transitions(Transition* in) { return 0; }
    InDatagram* events     (InDatagram* in) {
      _sem.give();
      return in;
    }
  private:
    Semaphore& _sem;
  };
};

using namespace Pds;

void usage(char* progname) {
  fprintf(stderr,
          "Usage: %s -i <filename> [-o <filename>] [-n events] [-x] [-h] [-1] [-2]\n"
          "       -i <filename>  : input xtc file\n"
          "       -o <filename>  : output xtc file\n"
          "       -n <events>    : number to process\n"
          "       -d <DetInfo>   : camera name (default: all)\n"
          "       -O             : use OMP\n"
          "       -v             : validate\n"
          "       -s             : serve vmon\n"
          "       -V             : verbose\n",
          progname);
}

int main(int argc, char* argv[]) {
  int c;
  char* inxtcname=0;
  char* outxtcname=0;
  bool  validate=false;
  bool  vmon=false;
  int parseErr = 0;
  unsigned nevents = -1;
  Pds::DetInfo info("");

  while ((c = getopt(argc, argv, "hOVn:i:o:vd:s")) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'n':
      nevents = atoi(optarg);
      break;
    case 'i':
      inxtcname = optarg;
      break;
    case 'o':
      outxtcname = optarg;
      break;
    case 'O':
      FrameCompApp::useOMP(true);
      break;
    case 'd':
      info = DetInfo(optarg);
      if (info.detector()==DetInfo::NumDetector) {
        printf("Parsing %s failed\n",optarg);
        return -1;
      }
      break;
    case 's':
      vmon = true;
      break;
    case 'v':
      validate = true;
      break;
    case 'V':
      verbose++;
      FrameCompApp::setVerbose(true);
      break;
    default:
      parseErr++;
    }
  }
  
  if (!inxtcname) {
    usage(argv[0]);
    exit(2);
  }

  int ifd = open(inxtcname, O_RDONLY | O_LARGEFILE);
  if (ifd < 0) {
    perror("Unable to open input file\n");
    exit(2);
  }

  FILE* ofd = 0;
  if (outxtcname) {
    ofd = fopen(outxtcname,"wx");
    if (ofd == 0) {
      perror("Unable to open output file\n");
      exit(2);
    }
  }
  
  XtcFileIterator iter(ifd,MAX_DG_SIZE);
  Dgram* dg;

  GenericPool pool(MAX_DG_SIZE,2);

  unsigned long long total_payload=0, total_comp=0;

  Semaphore sem(Semaphore::EMPTY);

  Inlet* inlet = new Inlet;

  Outlet* outlet = new Outlet;
  new OpenOutlet(*outlet);
  outlet->connect(inlet);

  (new MyOutlet(sem))->connect(inlet);

  if (ofd)
    (new MyRecorder(ofd))->connect(inlet);

  if (validate)
    (new MyValidator(dg))->connect(inlet);

  (new FrameCompApp(MAX_DG_SIZE))->connect(inlet);

  if (vmon) {
    if (info.detector()==DetInfo::NumDetector) {
      printf("Cannot serve vmon for more than one detector.  Use -d <DetInfo> option.\n");
      return -1;
    }
    (new VmonServerAppliance(ProcInfo(Level::Segment,0,0)))->connect(inlet);
  }

  while ((dg = iter.next())) {

    Datagram& ddg = *reinterpret_cast<Datagram*>(dg);
    CDatagram* odg = new(&pool) CDatagram(ddg,ddg.xtc);

    //
    //  Only post the segment of interest
    //
    if (info.detector()<DetInfo::NumDetector) {
      MyInfoIterator it(info, *odg);
      it.iterate(&odg->datagram().xtc);
    }

    struct timespec tv_begin, tv_end;
    clock_gettime(CLOCK_REALTIME,&tv_begin);

    inlet->post(odg);
    sem.take();

    clock_gettime(CLOCK_REALTIME,&tv_end);

    if (verbose>0 || dg->seq.service()!=TransitionId::L1Accept)
      printf("%s transition: time 0x%x/0x%x, payloadSize %d (%d) [%f]\n",
             TransitionId::name(dg->seq.service()),
             dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(), dg->xtc.sizeofPayload(), odg->xtc.sizeofPayload(),
             double(tv_end.tv_sec-tv_begin.tv_sec)+1.e-9*double(tv_end.tv_nsec-tv_begin.tv_nsec));

    total_payload += dg ->xtc.sizeofPayload();
    total_comp    += odg->xtc.sizeofPayload();

    delete odg;

    if (dg->seq.isEvent())
      if (--nevents == 0)
        break;

  }
  
  printf("total payload %lld  comp %lld  %f%%\n",
         total_payload, total_comp, 100*double(total_comp)/double(total_payload));

  delete inlet;

  close (ifd);
  if (ofd) fclose(ofd);
  return 0;
}

