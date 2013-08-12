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
  uint32_t header[8];
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
  unsigned    asic_x    = 96;
  unsigned    asic_y    = 96;
  unsigned    nasics_x  = 1;
  unsigned    nasics_y  = 1;
  unsigned    nsamples  = 12;
  bool        lView2D   = true;

  int c;
  while ((c = getopt(argc, argv, "p:x:y:X:Y:s:12")) != -1) {
    switch (c) {
    case 'p':
      partition = optarg;
      break;
    case 'x':
      asic_x    = atoi(optarg);
      break;
    case 'y':
      asic_y    = atoi(optarg);
      break;
    case 'X':
      nasics_x  = atoi(optarg);
      break;
    case 'Y':
      nasics_y  = atoi(optarg);
      break;
    case 's':
      nsamples  = atoi(optarg);
      break;
    case '1':
      lView2D   = false;
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
  const unsigned header_shorts = 16;
  unsigned nasics = (nasics_x*nasics_y+1)&~1;
  uint16_t* data = new uint16_t[asic_x*asic_y*nsamples*nasics+header_shorts];
  memset(data,0,sizeof(EpixData));

  //
  //  The shared memory server
  //
  Pds::PadMonServer srv(lView2D ? Pds::PadMonServer::Epix : Pds::PadMonServer::Imp,
                        partition);

  //
  //  Serve the configuration to shared memory
  //
  if (lView2D)
    srv.config_2d(nasics_x, nasics_y, nsamples, asic_x, asic_y);
  else
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
    uint16_t* p = data+header_shorts;
    for(unsigned iy=0; iy<asic_y; iy++)
      for(unsigned ix=0; ix<asic_x; ix++)
	for(unsigned is=0; is<nsamples; is++) {
	  unsigned asic=0;
	  for(unsigned i=0; i<nasics_y; i++)
	    for(unsigned j=0; j<nasics_x; j++,asic++)
	      p[asic] += (32 + (asic+1)*(iy*asic_x+ix) + is)&0x3fff;
	  p += (asic+1)&~1;
	}
    
    //
    //  Serve the event to shared memory
    //
    if (lView2D)
      srv.event_2d(data);
    else
      srv.event_1d(data, (nasics_x*nasics_y+1)&~1);

    timespec tv;
    tv.tv_sec = 0;
    tv.tv_nsec = 10000000;
    nanosleep(&tv,0);
  }

  delete data;

  return 0;
}

