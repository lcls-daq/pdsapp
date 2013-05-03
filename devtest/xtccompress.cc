//
//  Unofficial example of XTC compression
//
#include "pds/client/FrameCompApp.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Semaphore.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"

#include <string.h>

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <map>
using std::map;

namespace Pds {
  class MyRecorder : public Appliance {
  public:
    MyRecorder(FILE* f,Semaphore& sem) : _f(f), _sem(sem) {}
    ~MyRecorder() {}
  public:
    Transition* transitions(Transition* tr) { return 0; }
    InDatagram* events     (InDatagram* in) {
      const Dgram* odg = reinterpret_cast<const Dgram*>(&in->datagram());
      fwrite(odg, sizeof(*odg) + odg->xtc.sizeofPayload(), 1, _f);
      fflush(_f);
      _sem.give();
      return 0;
    }
  private:
    FILE* _f;
    Semaphore& _sem;
  };
};

using namespace Pds;

void usage(char* progname) {
  fprintf(stderr,
          "Usage: %s -i <filename> [-o <filename>] [-n events] [-x] [-h] [-1] [-2]\n"
          "       -i <filename>  : input xtc file\n"
          "       -o <filename>  : output xtc file\n"
          "       -n <events>    : number to process\n"
          "       -O             : use OMP\n"
          "       -V             : verbose\n",
          progname);
}

int main(int argc, char* argv[]) {
  int c;
  char* inxtcname=0;
  char* outxtcname=0;
  int parseErr = 0;
  unsigned nevents = -1;

  while ((c = getopt(argc, argv, "hOVn:i:o:")) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'n':
      nevents = atoi(optarg);
      break;
    case 'i':
      inxtcname = optarg;
      break;
    case 'o':
      outxtcname = optarg;
      break;
    case 'O':
      FrameCompApp::useOMP(true);
      break;
    case 'V':
      FrameCompApp::setVerbose(true);
      break;
    default:
      parseErr++;
    }
  }
  
  if (!inxtcname) {
    usage(argv[0]);
    exit(2);
  }

  int ifd = open(inxtcname, O_RDONLY | O_LARGEFILE);
  if (ifd < 0) {
    perror("Unable to open input file\n");
    exit(2);
  }

  FILE* ofd = fopen(outxtcname,"wx");
  if (ofd == 0) {
    perror("Unable to open output file\n");
    exit(2);
  }
  
  const unsigned MAX_DG_SIZE = 0x2000000;
  XtcFileIterator iter(ifd,MAX_DG_SIZE);
  Dgram* dg;

  GenericPool pool(MAX_DG_SIZE,1);

  unsigned long long total_payload=0, total_comp=0;

  Semaphore sem(Semaphore::EMPTY);

  FrameCompApp* app = new FrameCompApp(0x1000000);
  (new MyRecorder(ofd,sem))->connect(app);

  while ((dg = iter.next())) {

    Datagram& ddg = *reinterpret_cast<Datagram*>(dg);
    CDatagram* odg = new(&pool) CDatagram(ddg,ddg.xtc);

    app->events(odg);
    sem.take();

    printf("%s transition: time 0x%x/0x%x, payloadSize %d (%d)\n",TransitionId::name(dg->seq.service()),
           dg->seq.stamp().fiducials(),dg->seq.stamp().ticks(), dg->xtc.sizeofPayload(), odg->xtc.sizeofPayload());

    total_payload += dg ->xtc.sizeofPayload();
    total_comp    += odg->xtc.sizeofPayload();

    if (dg->seq.isEvent())
      if (--nevents == 0)
        break;

    delete odg;
  }
  
  printf("total payload %lld  comp %lld  %f%%\n",
         total_payload, total_comp, 100*double(total_comp)/double(total_payload));

  close (ifd);
  fclose(ofd);
  return 0;
}

