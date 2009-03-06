#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/opal1k/ConfigV1.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"

#include <iostream>

using namespace Pds;
using std::cout;
using std::cin;
using std::endl;

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define fetchu(arg,ctyp,hint) { \
  unsigned u; \
  cout << "Enter " << #arg << " [" << hint << "] : "; \
  cout.flush(); \
  cin >> u; \
  config.arg = (ctyp)u; \
}

#define fetch(arg) { \
  cout << "Enter " << #arg << " : "; \
  cout.flush(); \
  cin >> config.arg; \
}

#define fetchp(arg) { \
  cout << "Enter " << #arg << " (x,y) : "; \
  cout.flush(); \
  cin >> config.arg.column >> config.arg.row; \
}

#define store(ctype,cobj) { \
  char fname[32]; \
  int fd; \
  int k=0; \
  do { \
    sprintf(fname,"%08x.%d",TypeId::Id_##ctype,k++); \
    fd = ::open(fname,O_WRONLY | O_CREAT | O_EXCL); \
  } while (fd < 0); \
  cout << "Storing as " << fname << endl; \
  int len = ::write(fd,&cobj,sizeof(config)); \
  cout << "Wrote " << len << " bytes" << endl; \
  ::close(fd); \
}

void createOpalConfig()
{
  Opal1kConfig config;
  fetch(Depth_bits);
  fetch(Gain_percent);
  fetch(BlackLevel_percent);
  fetch(ShutterWidth_us);
  config.nDefectPixels = 0;
  store(Opal1kConfig,config);
}

void createFexConfig()
{
  CameraFexConfig config;
  char line[128];
  int  c=0;
  for(int k=0; k<CameraFexConfig::NumberOf; k++)
    c += sprintf(&line[c],"%s%d=%s",
		 (k==0) ? "" : ", ",
		 k,CameraFexConfig::algorithm_title((CameraFexConfig::Algorithm)k));

  fetchu( algorithm,CameraFexConfig::Algorithm,line );
  fetchp( regionOfInterestStart);
  fetchp( regionOfInterestEnd);
  fetch ( threshold);
  fetch ( nMaskedPixels);
  for(int k=0; k<config.nMaskedPixels; k++) {
    CameraPixelCoord& c = const_cast<CameraPixelCoord&>(config.maskedPixelCoord(k));
    cout << "Masked Pixel " << k << " (x,y) : ";
    cin >> c.column >> c.row;
  }
  store(CameraFexConfig,config);
}
 
int main(int argc, char** argv)
{
  for(int k=1; k<argc; k++) {
    if (strcmp(argv[k],"--opal")==0) createOpalConfig();
    if (strcmp(argv[k],"--fex" )==0) createFexConfig ();
  }
  return 1;
}
