#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsapp/devtest/fformat.h"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/Datagram.hh"

void usage(char* progname) {
  fprintf(stderr,"Usage: %s [-h] -n <numbXTCs> -f <filename> -o <outFilename>\n", progname);
}

int main(int argc, char* argv[]) {
  int numb = 40;
  int count = 0;
  int c;
  char* xtcname=0;
  char* outFilename=0;

  while ((c = getopt(argc, argv, "hf:n:o:")) != -1) {
    switch (c) {
    case 'f':
      xtcname = optarg;
      break;
    case 'n':
      sscanf(optarg, "%d", &numb);
      break;
    case 'o':
      outFilename = optarg;
      break;
    case 'h':   // help
    default:
      usage(argv[0]);
      exit(1);
    }
  }
  
  if (!xtcname || !outFilename) {
    usage(argv[0]);
    exit(1);
  }

  int fd = open(xtcname,O_RDONLY | O_LARGEFILE);
  if (fd < 0) {
    char s[120];
    sprintf(s, "Unable to open XTC file %s ", xtcname);
    perror(s);
    exit(2);
  }

  FILE* outFile = fopen(outFilename,"w");
  if (!outFile) {
    char s[120];
    sprintf(s, "Unable to open output file %s ", outFilename);
    perror(s);
    exit(2);
  }

  XtcFileIterator iter(fd,0x900000);
  Dgram* indg;
  while ((indg = iter.next()) && (count < numb)) {
    printf("%s transition: time 0x%x/0x%x, payloadSize %d\n",TransitionId::name(indg->seq.service()),
           indg->seq.stamp().fiducials(),indg->seq.stamp().ticks(),indg->xtc.sizeofPayload());
    fwrite((char*)indg,sizeof(Dgram)+indg->xtc.sizeofPayload(),1,outFile);
    count++;
  }
  close(fd);
  fclose(outFile);
  printf("\nWrote %d Datagrams to %s\n", count, outFilename);
  return 0;
}
