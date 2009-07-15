#include "pds/vmon/VmonReader.hh"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pdsdata/xtc/Src.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonDescEntry.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonUsage.hh"
#include "pds/mon/MonStats1D.hh"
#include "pds/mon/MonStats2D.hh"

#include "pds/vmon/VmonReader.hh"

#include <vector>
using std::vector;

namespace Pds {
  class VmonReaderDump : public VmonReaderCallback {
  public:
    VmonReaderDump(const char* fname) 
    {
      char buff[128];
      sprintf(buff,"%s.out",fname);
      _file = fopen(buff,"w");
    }
    ~VmonReaderDump()
    {
      fclose(_file);
    }
    void process(const ClockTime& time,
		 const Src& src,
		 int signature,
		 const MonStats1D& stats)
    {
      fprintf(_file,"%08x/%08x  %9g %9g %9g\n",
	      time.seconds(),time.nanoseconds(),
	      stats.sum(), stats.sumx(), stats.sumx2());
    }
    void process(const ClockTime& time,
		 const Src& src,
		 int signature,
		 const MonStats2D& stats)
    {
      fprintf(_file,"%08x/%08x  %9g %9g %9g %9g %9g %9g\n",
	      time.seconds(),time.nanoseconds(),
	      stats.sum(), stats.sumx(), stats.sumy(),
	      stats.sumx2(), stats.sumy2(), stats.sumxy());
    }
  private:
    FILE* _file;
  };
};

using namespace Pds;

int main(int argc, char** argv)
{
  const char* filename=0;
  int c;
  while ((c = getopt(argc, argv, "f:")) != -1) {
    switch(c) {
    case 'f':
      filename = optarg;
      break;
    default:
      break;
    }
  }

  if (!filename) {
    printf("Usage: %s -f <filename>\n",argv[0]);
    exit(1);
  }

  VmonReader reader(filename);

  while(1) {
    const vector<Src>& sources = reader.sources();

    printf("Choices (Ctrl<D> to exit):\n");
    int i=0;
    for(vector<Src>::const_iterator it = sources.begin(); it!=sources.end(); it++,i++) {
      const MonCds& cds = *reader.cds(*it);
      for(unsigned g=0; g<cds.ngroups(); g++) {
	const MonGroup& gr = *cds.group(g);
	for(unsigned e=0; e<gr.nentries(); e++) {
	  const MonEntry* en = gr.entry(e);
	  printf("%x)  %x.%x.%s\n", 
		 (i<<24) | (g<<16)|e, 
		 it->log(), it->phy(), en->desc().name());
	}
      }
    }

    const int maxlen=64;
    char line[maxlen];
    char* result = fgets(line, maxlen, stdin);
    if (!result) {
      fprintf(stdout, "\nExiting\n");
      break;
    }

    unsigned choice;
    if (sscanf(result,"%x",&choice)) {
      VmonReaderDump dump(result);
      MonUsage usage;
      unsigned i=0;
      for(vector<Src>::const_iterator it = sources.begin(); it!=sources.end(); it++,i++) {
	if ((choice>>24)==i) {
	  usage.use(choice&0xffffff);
	  reader.use(*it,usage);
	}
      }
      reader.process(dump);
      reader.reset();
    }
  }
}
