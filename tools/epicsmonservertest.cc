//
//  epicsmonservertest
//
//    A simulation of EpicsArch that
//    pushes data into shared memory for the DAQ monitoring
//    applications.
//
#include "EpicsMonServer.hh"

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//
//  usage: epicsmonservertest -p <shared memory tag>
//
//  The shared memory tag is a common name used by both applications to
//  identify the shared memory segment and message queues.
//
//
int main(int argc, char** argv) {

  int nevents=-1;
  const char* partition = 0;
  double rate = 100;
  
  int c;
  while ((c = getopt(argc, argv, "p:r:n:h")) != -1) {
    switch (c) {
    case 'p':
      partition = optarg;
      break;
    case 'r':
      rate = strtod(optarg,NULL);
      break;
    case 'n':
      nevents = atoi(optarg); 
      break;
    default:
      partition = 0;
      break;
    }
  }

  if (!partition) {
    printf("Usage: %s -p <shared memory tag>\n",argv[0]);
    exit(1);
  }

  //
  //  The shared memory server
  //
  Pds::EpicsMonServer srv(partition);

  std::vector<std::string> names;
  std::vector<double     > values;

  while(1) {

    for(int i=2; i<=32; i++) {
      names .resize(i);
      values.resize(i);

      char buff[32];
      for(int j=0; j<i; j++) {
        sprintf(buff,"Pv_%d",j);
        names[j] = std::string(buff);
      }

      srv.configure(names);

      //
      //  Generate a cycle of events with some test pattern data
      //
      for(unsigned ievent = 0; ievent<1000; ievent++, nevents--) {
        //
        //  simulate some data
        //
        ievent++;
        for(int j=0; j<i; j++)
          values[j] = double(j*1000+ievent);

        //
        //  Serve the event to shared memory
        //
        srv.event(values);

        if (nevents==0) {
          srv.unconfigure();
          exit(0);
        }

        timespec tv;
        tv.tv_sec  = int(1./rate);
        tv.tv_nsec = int(1.e9*remainder(1./rate,1.));
        nanosleep(&tv,0);
      }
      
      srv.unconfigure();
    }
  }

  return 0;
}

