//
//  Fix misaligned data from BLD cameras
//
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"

#include <string.h>

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static const char* FILLER = "aaaaaaaa";

namespace Pds {
  class EventIter {
  public:
    EventIter(Dgram& dg) : _buff(new char[0x2000000]), _p(_buff) 
    {
      _write(&dg, sizeof(dg)-sizeof(Xtc));
      iterate(&dg.xtc);
    }
    ~EventIter() { delete[] _buff; }
  public:
    int process(Xtc* xtc) {
      if (xtc->contains.id()==TypeId::Id_Xtc) {
        return iterate(xtc);
      }
      else if (xtc->extent&3) {
        char* p = _p;
        _write(xtc, xtc->extent&~3);
        _write(FILLER,4);
        reinterpret_cast<Xtc*>(p)->extent = _p-p;
        return -(xtc->extent&3);
      }
      else
        _write(xtc, xtc->extent);
      return 0;
    }
    int iterate(Xtc* root) {
      int result = 0;

      if (root->damage.value() & ( 1 << Damage::IncompleteContribution)) {
        _write(root, root->extent);
        return result;
      }

      char* p = _p;
      _write(root, sizeof(Xtc));
      
      Xtc* xtc     = (Xtc*)root->payload();
      int remaining = root->sizeofPayload();
      
      while(remaining > 0)
        {
          if(xtc->extent==0) break; // try to skip corrupt event
          int correction = process(xtc);
          remaining -= xtc->sizeofPayload() + sizeof(Xtc);
          xtc        = (Xtc*)((char*)xtc->next()+correction);
          result    += correction;
        }
      
      unsigned extent = _p - p;
      if (extent&3) {
        int n = 4-(extent&3);
        _write(FILLER, n);
        extent += n;
      }
      
      Xtc* x = reinterpret_cast<Xtc*>(p);
      x->extent = extent;
      
      return result;
    }
    const Dgram& out() const { return *reinterpret_cast<const Dgram*>(_buff); }
  private:
    void _write(const void* p, unsigned sz) { memcpy(_p, p, sz); _p += sz; }
  private:
    char* _buff;
    char* _p;
  };
};

using namespace Pds;

void usage(char* progname) {
  fprintf(stderr,
          "Usage: %s -i <filename> [-o <filename>] [-n events] [-h]\n"
          "       -i <filename>  : input xtc file\n"
          "       -o <filename>  : output xtc file\n"
          "       -n <events>    : number to process\n"
          "       -h             : help\n",
          progname);
}

int main(int argc, char* argv[]) {
  int c;
  char* inxtcname=0;
  char* outxtcname=0;
  int parseErr = 0;
  unsigned nevents = -1;

  while ((c = getopt(argc, argv, "hn:i:o:")) != -1) {
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
    default:
      parseErr++;
    }
  }
  
  if (!inxtcname || parseErr) {
    usage(argv[0]);
    exit(2);
  }

  int ifd = open(inxtcname, O_RDONLY | O_LARGEFILE);
  if (ifd < 0) {
    perror("Unable to open input file\n");
    exit(2);
  }

  FILE* ofd = fopen(outxtcname,"wx");
  if (ofd == 0) {
    perror("Unable to open output file\n");
    exit(2);
  }
  
  const unsigned MAX_DG_SIZE = 0x2000000;
  XtcFileIterator iter(ifd,MAX_DG_SIZE);
  Dgram* dg;

  while ((dg = iter.next())) {

    Dgram& ddg = *reinterpret_cast<Dgram*>(dg);

    printf("%s transition: time 0x%x/0x%x, payloadSize %d\n",
           TransitionId::name(dg->seq.service()),
           dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(), dg->xtc.sizeofPayload());

    if (dg->seq.service()!=TransitionId::L1Accept) {
      fwrite(&ddg, sizeof(ddg)+ddg.xtc.sizeofPayload(),1,ofd);
    }
    else {
      EventIter it(ddg);
      const Dgram& odg = it.out();
      fwrite(&odg, sizeof(odg)+odg.xtc.sizeofPayload(),1,ofd);
      
      if (--nevents == 0)
        break;
    }
  }
  
  close (ifd);
  fclose(ofd);
  return 0;
}

