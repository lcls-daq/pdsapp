#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/BldIpm.hh"
#include "pdsapp/monobs/XppIpm.hh"
#include "pdsapp/monobs/XppPim.hh"
#include "pdsapp/monobs/CspadMon.hh"
#include "pdsapp/monobs/EpicsToEpics.hh"
#include "pdsapp/monobs/Encoder.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace Pds;
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

  char*  pvbase = new char[64];
  //float  chbase[4];

  size_t line_sz = 256;
  char*  line = (char *)malloc(line_sz);

  while(getline(&line, &line_sz, f) != -1) {
    if (line[0]!='#') {
      char* args = strtok(line,"\t ");
      if (args==NULL || args[0]=='\n')
        break;

      DetInfo info(args);
      BldInfo binfo(args);

      char* pvbase = strtok(NULL,"\t ");
      if (!pvbase) {
        printf("No PV field for %s\n",args);
        return -1;
      }

      if (info.detector()!=DetInfo::NumDetector) {
        switch(info.device()) {
        case DetInfo::Cspad:
        case DetInfo::Cspad2x2:
          CspadMon::monitor(client,
                            pvbase,
                            info);
          break;
        case DetInfo::Ipimb:
          { switch(info.detector()) {
            case Pds::DetInfo::XppMonPim:
            case Pds::DetInfo::XppSb3Pim:
            case Pds::DetInfo::XppSb4Pim:
              client.insert(new XppPim(pvbase,info.detector()));
              //    break;
            case Pds::DetInfo::XppSb1Ipm:
            case Pds::DetInfo::XppSb2Ipm:
            case Pds::DetInfo::XppSb3Ipm:
              client.insert(new XppIpm(pvbase,info.detector()));
              break;
            default:
              fprintf(stderr,"Error in lookup of ipimb %s\n",args);
              break;
            } break;
          default:
            fprintf(stderr,"Error in lookup of detector name %s\n",args);
            break;
          }
        case DetInfo::Opal1000:
          { char* pvIn = strtok(NULL,"\t ");
            client.insert(new EpicsToEpics(info, pvIn, pvbase));
            break; }
        }
      }
      else if (binfo.type()!=BldInfo::NumberOf) {
        switch(binfo.type()) {
        case BldInfo::Nh2Sb1Ipm01:
          client.insert(new BldIpm(pvbase,binfo.type()));
          break;
        case BldInfo::Nh2Sb1Ipm02:
          client.insert(new BldIpm(pvbase,binfo.type()));
          break;
        default:
          fprintf(stderr,"Error in lookup of bld type %s\n",args);
          break;
        }
      }
      else {
        fprintf(stderr,"Error in lookup of name %s\n",args);
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
