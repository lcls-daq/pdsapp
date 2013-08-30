//
//  padmonservertest
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

#include "pds/config/CsPadConfigType.hh"
#include "pdsdata/psddl/cspad.ddl.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const unsigned Columns = 185;
static const unsigned Rows    = 194;

//
//  usage: padmonservertest -p <shared memory tag> -m <readout mask>
//
//  The shared memory tag is a common name used by both applications to
//  identify the shared memory segment and message queues.
//
//  The readout mask is a bit mask of 2x1's to sparsify the readout.
//  The logic below will create a configuration that describes the 
//  minimum readout necessary to acquire the identified 2x1's, and the
//  underlying classes will further sparsify the data to the exact list 
//  of 2x1's.
//
int main(int argc, char** argv) {

  const char* partition = 0;
  unsigned rmask = 0xffffffff;

  int c;
  while ((c = getopt(argc, argv, "p:m:")) != -1) {
    switch (c) {
    case 'p':
      partition = optarg;
      break;
    case 'm':
      rmask = strtoul(optarg,NULL,0);
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
  //  Calculate the readout masks to acquire the requested 2x1's
  //
  unsigned nq = 0;
  unsigned qmask = 0;
  for(unsigned i=0; i<4; i++)
    if (rmask&(0xff<<(8*i))) {
      qmask |= (1<<i);
      nq++;
    }
  
  unsigned amask, ns;
  if (rmask&0xfcfcfcfc) {
    amask = 0xf;
    ns    = 8;
  }
  else {
    amask = 1;
    ns    = 2;
  }

  //
  //  Calculate the quadrant/link payload size
  //
  const unsigned payloadsize = 
    sizeof(Pds::CsPad::ElementV1) + 4 + 
    Columns*Rows*sizeof(uint16_t) * 2 * ns;  // cspad140k is 1/4 of this

  //
  //  A buffer for generating the event data
  //
  uint32_t* buff = new uint32_t[payloadsize*nq/sizeof(uint32_t)];

  //
  //  The shared memory server
  //
  Pds::PadMonServer srv(Pds::PadMonServer::CsPad,
                        partition);

  //
  //  The DAQ configuration object
  //
  Pds::CsPad::ProtectionSystemThreshold pt[4];
  Pds::CsPad::ConfigV3QuadReg quads[4];
  CsPadConfigType cfg(0, 0, 0,     // conc version, rundelay, eventcode
                      pt, 0,       // protection thresholds, enable
                      0, 0,        // inactiverunmode, activerunmode,
                      0, 0,        // internaltriggerdelay, testdataindex
                      payloadsize, // payload per quad (necessary)
                      0, 0,        // bad asic masks
                      amask,   // asic mask (necessary)
                      qmask,   // quad mask (necessary)
                      rmask,   // roi mask  (necessary)
                      quads
                      );

  //
  //  Serve the configuration to shared memory
  //
  srv.configure(cfg);

  const unsigned Scan = 2*Rows*8*4;

  //
  //  Generate a cycle of events with some test pattern data
  //
  unsigned ievent = 0;
  while(1) {
    //
    //  simulate some data
    //
    ievent++;
    uint32_t* hdr = buff;
    unsigned p=0;  // a reference for simulated data pattern
    for(unsigned i=0; i<4; i++) {
      if (!(qmask&(1<<i))) 
        continue;

      // eight header words
      hdr[1] = i<<24;  // quad id (necessary)
      uint16_t* pixels = reinterpret_cast<uint16_t*>(&hdr[8]);

      for(unsigned s=0; s<ns; s++, p+=2*Rows)
        for(unsigned j=0; j<Columns; j++)
          for(unsigned k=0; k<2*Rows; k++)
            *pixels++ = (ievent-p-k+Scan)%(Scan);
      hdr = reinterpret_cast<uint32_t*>(pixels);
      *hdr++ = 0;  // pgp status word
    }
    
    //
    //  Serve the event to shared memory
    //
    srv.event(*reinterpret_cast<Pds::CsPad::ElementV1*>(buff));

    timespec tv;
    tv.tv_sec = 0;
    tv.tv_nsec = 10000000;
    nanosleep(&tv,0);
  }

  delete[] buff;

  return 0;
}

