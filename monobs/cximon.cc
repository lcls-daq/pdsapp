#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/CspadMon.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

  char*  sid = new char[64];
  int    detid,devid;
  char*  pvbase = new char[64];
  //float  chbase[4];

  size_t line_sz = 256;
  char*  line = (char *)malloc(line_sz);

  while(getline(&line, &line_sz, f) != -1) {
    if (line[0]!='#') {
      if (sscanf(line,"%s\t%s",
		 sid,pvbase) < 2) {
	fprintf(stderr,"Error scanning line: %s\n",line);
      }
      else {
        printf("scanning %s for det/devid\n",sid);

        devid=0;
        sscanf(sid,"%d.%d",&detid,&devid);

        CspadMon::monitor(client, pvbase, detid, devid);
      }
    }
    line_sz = 256;
  }

  delete[] sid;
  delete[] pvbase;
  if (line) {
    free(line);
  }


  fprintf(stderr, "client returned: %d\n", client.start());
}
