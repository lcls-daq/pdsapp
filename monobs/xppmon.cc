#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/BldIpm.hh"
#include "pdsapp/monobs/XppIpm.hh"
#include "pdsapp/monobs/XppPim.hh"
#include "pdsapp/monobs/CspadMon.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

using namespace PdsCas;

void usage(const char* p)
{
  printf("Usage: %s -f <config file> %s\n",
	 p, ShmClient::options());
}

int main(int argc, char* argv[])
{
  ShmClient client;

  const char* filename = 0;

  int c;
  char opts[128];
  sprintf(opts,"?hf:%s",client.opts());
  while ((c = getopt(argc, argv, opts)) != -1) {
    switch (c) {
    case '?':
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'f':
      filename = optarg;
      break;
    default:
      if (!client.arg(c,optarg)) {
	usage(argv[0]);
	exit(1);
      }
      break;
    }
  }
  
  if (filename==0 || !client.valid()) {
    usage(argv[0]);
    exit(1);
  }
  
  FILE* f = fopen(filename,"r");
  if (!f) {
    char buff[128];
    sprintf(buff,"Error opening file %s",filename);
    perror(buff);
  }

  int    detid;
  char*  pvbase = new char[64];
  //float  chbase[4];

  size_t line_sz = 256;
  char*  line = (char *)malloc(line_sz);

  while(getline(&line, &line_sz, f) != -1) {
    if (line[0]!='#') {
      if (sscanf(line,"%d\t%s",
		 &detid,pvbase) < 2) {
	fprintf(stderr,"Error scanning line: %s\n",line);
      }
      else {
	switch(detid) {
        case -Pds::BldInfo::Nh2Sb1Ipm01:
          client.insert(new BldIpm(pvbase,-detid));
          break;
	case Pds::DetInfo::XppMonPim:
	case Pds::DetInfo::XppSb3Pim:
	case Pds::DetInfo::XppSb4Pim:
	  client.insert(new XppPim(pvbase,detid));
	  //	  break;
	case Pds::DetInfo::XppSb1Ipm:
	case Pds::DetInfo::XppSb2Ipm:
	case Pds::DetInfo::XppSb3Ipm:
	  client.insert(new XppIpm(pvbase,detid));
	  break;
        case Pds::DetInfo::XppGon:
          CspadMon::monitor(client, 
                            Pds::DetInfo(0, Pds::DetInfo::Detector(detid), 0, 
                                         Pds::DetInfo::Cspad,0));
          break;
	default:
	  fprintf(stderr,"Error in lookup of detector index %d\n",detid);
	  break;
	}
      }
    }
    line_sz = 256;
  }

  delete[] pvbase;
  if (line) {
    free(line);
  }


  fprintf(stderr, "client returned: %d\n", client.start());
}
