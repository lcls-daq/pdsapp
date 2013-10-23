#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/CspadMon.hh"
#include "pdsapp/monobs/PrincetonMon.hh"
#include "pdsapp/dev/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

using namespace PdsCas;

void usage(const char* p)
{
  printf("Usage: %s -v -f <config file> %s\n",
	 p, ShmClient::options());
}

int main(int argc, char* argv[])
{
  ShmClient client;

  const char* filename = 0;

  int c;
  char opts[128];
  sprintf(opts,"?hvf:%s",client.opts());
  while ((c = getopt(argc, argv, opts)) != -1) {
    switch (c) {
    case 'v':
      CspadMon::verbose();
      break;
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

  DetInfo info;
  //float  chbase[4];

  size_t line_sz = 256;
  char*  line = (char *)malloc(line_sz);

  while(getline(&line, &line_sz, f) != -1) {
    if (line[0]!='#') {
      char* args = strtok(line,"\t ");
      if (!CmdLineTools::parseDetInfo(args,info))
        return -1;

      char* pvbase = strtok(NULL,"\t\n ");
      if (!pvbase) {
        printf("No PV field for %s\n",args);
        return -1;
      }

      switch(info.device()) {
      case DetInfo::Cspad:
      case DetInfo::Cspad2x2:
        CspadMon::monitor(client, 
                          pvbase,
                          info);
        break;
      case DetInfo::Princeton:
        PrincetonMon::monitor(client,
                              pvbase,
                              info);
        break;
      default:
        fprintf(stderr,"Error in lookup of detector %s\n",DetInfo::name(info));
        break;
      }
    }
    line_sz = 256;
  }

  if (line) {
    free(line);
  }


  fprintf(stderr, "client returned: %d\n", client.start());
}
