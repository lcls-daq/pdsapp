#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/SxrSpectrum.hh"

using namespace PdsCas;

int main(int argc, char* argv[])
{
  ShmClient client(argc,argv);
  client.insert(new SxrSpectrum("sxr:spec"));
  fprintf(stderr, "client returned: %d\n", client.start());
}
