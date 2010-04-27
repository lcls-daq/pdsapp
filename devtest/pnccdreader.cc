
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/pnCCD/ConfigV1.hh"
#include "pdsdata/pnCCD/FrameV1.hh"
#include "pdsapp/tools/PnccdFrameDetail.hh"

static PNCCD::ConfigV1 cfg;

static unsigned short expectedPattern[] = {0xfedc,0xba98,0x7654,0};
static unsigned cpoframes=0;
static unsigned cpoframeerrors=0;
static unsigned cponprint=0;

#define MAXPRINT 20

class myLevelIter : public XtcIterator {
public:
  enum {Stop, Continue};
  myLevelIter(Xtc* xtc, unsigned depth) : XtcIterator(xtc), _depth(depth) {}

  void patternCheck(unsigned link, const PNCCD::FrameV1* f) {
    PNCCD::Line* line = (PNCCD::Line*)(const_cast<uint16_t*>(f->data()));
    expectedPattern[3]=0;
    for (unsigned j=0;j<PNCCD::Image::NumLines;j++) {
      for (unsigned k=0;k<PNCCD::Camex::NumChan;k++) {
        for (unsigned l=0;l<PNCCD::Line::NumCamex;l++) {
          if (line->cmx[l].data[k]!=expectedPattern[l]) {
            if (cponprint++<MAXPRINT) {
              printf("%d/%d/%d/%d: expectedPattern 0x%x found 0x%x\n",
                     link,j,k,l,expectedPattern[l],line->cmx[l].data[k]);
            }
            cpoframeerrors++;
          }
        }
        expectedPattern[3]++;
      }
      line++;
    }
  }

  void process(const DetInfo& d, const PNCCD::FrameV1* f) {
    cpoframes++;
    for (unsigned i=0;i<cfg.numLinks();i++) {
//       printf("*** Processing pnCCD frame number %x segment %d time 0x%x/0x%x\n",f->frameNumber(),i,f->timeStampHi(),f->timeStampLo());
//       const uint16_t* data = f->data();
//       unsigned last  = f->sizeofData(cfg); 
//       printf("First data words: 0x%4.4x 0x%4.4x\n",data[0],data[1]);
//       printf("Last  data words: 0x%4.4x 0x%4.4x\n",data[last-2],data[last-1]);
      patternCheck(i,f);
      f = f->next(cfg);
    }
  }

  void process(const DetInfo&, const PNCCD::ConfigV1& config) {
    cfg = config;
    printf("*** Processing pnCCD config.  Number of Links: %d, PayloadSize per Link: %d\n",
           cfg.numLinks(),cfg.payloadSizePerLink());
  }

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
      myLevelIter iter(xtc,_depth+1);
      iter.iterate();
      break;
    }
    case (TypeId::Id_pnCCDframe) :
      process(info, (const PNCCD::FrameV1*)(xtc->payload()));
      break;
    case (TypeId::Id_pnCCDconfig) :
      process(info, *(const PNCCD::ConfigV1*)(xtc->payload()));
      break;
    default :
      break;
    }
    return Continue;
  }
private:
  unsigned _depth;
};

void usage(char* progname) {
  fprintf(stderr,"Usage: %s -f <filename> [-h]\n", progname);
}

int main(int argc, char* argv[]) {
  int c;
  char* xtcname=0;
  int parseErr = 0;

  while ((c = getopt(argc, argv, "f:")) != -1) {
    switch (c) {
    case 'f':
      xtcname = optarg;
      break;
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

  XtcFileIterator iter(fd,0x900000);
  Dgram* dg;
  while ((dg = iter.next())) {
    myLevelIter iter(&(dg->xtc),0);
    iter.iterate();
    if (cpoframes && (cpoframes%100==0)) printf("Frames analyzed: %d, errors: %d\n",cpoframes, cpoframeerrors);
  }
  printf("Frames analyzed: %d, errors: %d\n",cpoframes, cpoframeerrors);

  close(fd);
  return 0;
}
