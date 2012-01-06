#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/CspadMon.hh"
#include "pdsdata/xtc/DetInfo.hh"

using namespace PdsCas;

void usage(const char* p)
{
  printf("Usage: %s -d <detector id> %s\n",
	 p, ShmClient::options());
}

int main(int argc, char* argv[])
{
  ShmClient client;

  unsigned det = 0;

  int c;
  char opts[128];
  sprintf(opts,"?hd:%s",client.opts());
  while ((c = getopt(argc, argv, opts)) != -1) {
    switch (c) {
    case '?':
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'd':
      det = atoi(optarg);
      break;
    default:
      if (!client.arg(c,optarg)) {
	usage(argv[0]);
	exit(1);
      }
      break;
    }
  }
  
  if (det==0 || !client.valid()) {
    usage(argv[0]);
    exit(1);
  }

  CspadMon::monitor(client, "userpv", 
                    Pds::DetInfo(0, 
                                 Pds::DetInfo::Detector(det),0,
                                 Pds::DetInfo::Cspad,0));
  
  fprintf(stderr, "client returned: %d\n", client.start());
}
