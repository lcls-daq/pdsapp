#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/SxrSpectrum.hh"

using namespace PdsCas;

void usage(const char* p)
{
  printf("Usage: %s -n <PV (SXR:EXS)> -d <DetInfo (0x0b000300)> %s\n",
	 p, ShmClient::options());
}

int main(int argc, char* argv[])
{
  ShmClient client(argc,argv);

  const char* pvName = 0;
  unsigned detinfo   = -1UL;

  int c;
  char* endPtr;
  while ((c = getopt(argc, argv, "?hi:n:d:")) != -1) {
    switch (c) {
    case '?':
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'n':
      pvName = optarg;
      break;
    case 'd':
      detinfo = strtoul(optarg,&endPtr,0);
      break;
    default:
      break;
    };
  }
  
  if (pvName==0 || detinfo==-1UL || !client.valid()) {
    usage(argv[0]);
    exit(1);
  }
  
  client.insert(new SxrSpectrum(pvName,detinfo));
  fprintf(stderr, "client returned: %d\n", client.start());
}
