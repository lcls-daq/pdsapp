//
//  epixmonservertest
//
//    A simulation of pixel array detector readout that
//    pushes data into shared memory for the DAQ monitoring
//    applications.
//
//  An application will need the PadMonServer.hh file and
//  the header files describing the data format.
//
//
#include "PadMonServer.hh"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//
//  Some generic format
//
static const unsigned NSamples = 16;
static const unsigned NPixels  = 96;

class ADCData {
public:
  uint16_t channel[2];
};

class PixelData {
public:
  ADCData sample[NSamples];
};

class EpixData {
public:
  uint32_t header[1];
  PixelData pixel[NPixels];
  uint32_t footer[1];
};
  
//
//  usage: epixmonservertest -p <shared memory tag>
//
//  The shared memory tag is a common name used by both applications to
//  identify the shared memory segment and message queues.
//
int main(int argc, char** argv) {

  const char* partition = 0;
  unsigned    channel   = 0;
  unsigned    step      = 2;
  unsigned    nsamples  = 12;

  int c;
  while ((c = getopt(argc, argv, "p:c:s:n:")) != -1) {
    switch (c) {
    case 'p':
      partition = optarg;
      break;
    case 'c':
      channel   = atoi(optarg);
      break;
    case 's':
      step      = atoi(optarg);
      break;
    case 'n':
      nsamples  = atoi(optarg);
      break;
    default:
      break;
    }
  }

  if (!partition) {
    printf("partition tag must be given\n");
    exit(1);
  }

  //
  //  A buffer for generating the event data
  //
  EpixData* data = new EpixData;
  memset(data,0,sizeof(EpixData));

  //
  //  The shared memory server
  //
  Pds::PadMonServer srv(Pds::PadMonServer::Imp,
                        partition);

  //
  //  Serve the configuration to shared memory
  //
  srv.config_1d(nsamples);

  //
  //  Generate a cycle of events with some test pattern data
  //
  unsigned ievent = 0;
  while(1) {
    //
    //  simulate some data
    //
    ievent++;
    for(unsigned ip=0; ip<NPixels; ip++) {
      for(unsigned is=0; is<NSamples; is++) {
	data->pixel[ip].sample[is].channel[0] += ip*NSamples + is;
      }
    }

    //
    //  Serve the event to shared memory
    //
    srv.event_1d( reinterpret_cast<const uint16_t*>(&data->pixel[0].sample[0]+channel), step);

    timespec tv;
    tv.tv_sec = 0;
    tv.tv_nsec = 10000000;
    nanosleep(&tv,0);
  }

  delete data;

  return 0;
}

