#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/SxrSpectrum.hh"

using namespace PdsCas;

int main(int argc, char* argv[])
{
  ShmClient client(argc,argv);

  char* endPtr;
  const char* pvName  = argv[argc-2];
  unsigned    detinfo = strtoul(argv[argc-1],&endPtr,0); 
  client.insert(new SxrSpectrum(pvName,detinfo));
  fprintf(stderr, "client returned: %d\n", client.start());
}
