#include "pdsapp/dev/CmdLineTools.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using namespace Pds;

bool CmdLineTools::parseDetInfo(const char* args, DetInfo& info)
{
  DetInfo::Detector det(DetInfo::NumDetector);
  DetInfo::Device   dev(DetInfo::NumDevice);
  unsigned detid(0), devid(0);

  printf("Parsing %s\n",args);

  char* p;
  det    = (DetInfo::Detector)strtoul(args, &p, 0);
  if (p != args) {
    detid  = strtoul(p+1 , &p, 0);
    dev    = DetInfo::Opal1000;
    devid  = strtoul(p+1 , &p, 0);
  }
  else {
    int n = (p=strchr(args,'/')) - args;
    det = DetInfo::NumDetector;
    for(int i=0; i<DetInfo::NumDetector; i++)
      if (strncasecmp(args,DetInfo::name((DetInfo::Detector)i),n)==0) {
        det = (DetInfo::Detector)i;
        break;
      }
    if (det == DetInfo::NumDetector)
      return false;

    detid  = strtoul(p+1 , &p, 0);

    args = p+1;
    n = (p=strchr(args,'/')) - args;
    for(int i=0; i<DetInfo::NumDevice; i++)
      if (strncasecmp(args,DetInfo::name((DetInfo::Device)i),n)==0) {
        dev = (DetInfo::Device)i;
        break;
      }
    if (dev == DetInfo::NumDevice)
      return false;

    devid  = strtoul(p+1 , &p, 0);
  }

  info = DetInfo(0, det, detid, dev, devid);
  printf("Sourcing %s\n",DetInfo::name(info));
  return true;
}
