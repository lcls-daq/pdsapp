
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/acqiris/ConfigV1.hh"
#include "pdsdata/types/WaveformV1.hh"

class myLevelIter : public XtcIterator {
public:
  enum {Stop, Continue};
  myLevelIter(Xtc* xtc, unsigned depth) : XtcIterator(xtc), _depth(depth) {}
  void process(const Acqiris::ConfigV1& config) {
    printf("*** Processing Acqiris configuration object, number of samples %u\n",
           config.nbrSamples());
  }
  void process(const WaveformV1&) {
    printf("*** Processing waveform object\n");
  }
  int process(Xtc* xtc) {
    unsigned i=_depth; while (i--) printf("  ");
    Level::Type level = xtc->src.level();
    printf("%s level: ",Level::name(level));
    if (level==Level::Source) {
      DetInfo& info = *(DetInfo*)(&xtc->src);
      printf("%s%d %s%d\n",
             DetInfo::name(info.detector()),info.detId(),
             DetInfo::name(info.device()),info.devId());
    } else {
      ProcInfo& info = *(ProcInfo*)(&xtc->src);
      printf("IpAddress 0x%x ProcessId 0x%x\n",info.ipAddr(),info.processId());
    }
    switch (xtc->contains.id()) {
    case (TypeId::Id_Xtc) : {
      myLevelIter iter(xtc,_depth+1);
      iter.iterate();
      break;
    }
    case (TypeId::Id_Waveform) :
      process(*(const WaveformV1*)(xtc->payload()));
      break;
    case (TypeId::Id_AcqConfig) :
      unsigned version = xtc->contains.version();
      switch (version) {
      case 1:
        process(*(const Acqiris::ConfigV1*)(xtc->payload()));
        break;
      default:
        printf("Unsupported acqiris configuration version %d\n",version);
      break;
      }
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

  while ((c = getopt(argc, argv, "hf:")) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      exit(0);
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

  FILE* file = fopen(xtcname,"r");
  if (!file) {
    perror("Unable to open file %s\n");
    exit(2);
  }

  XtcFileIterator iter(file,0x100000);
  Dgram* dg;
  while (dg = iter.next()) {
    printf("%s transition: time 0x%x/0x%x, payloadSize 0x%x\n",TransitionId::name(dg->seq.service()),
           dg->seq.high(),dg->seq.low(),dg->xtc.sizeofPayload());
    myLevelIter iter(&(dg->xtc),0);
    iter.iterate();
  }

  fclose(file);
  return 0;
}
