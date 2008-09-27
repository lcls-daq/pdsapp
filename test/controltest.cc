#include "pds/collection/CollectionManager.hh"
#include "pds/collection/Transition.hh"

#include "pds/service/Semaphore.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/management/EbJoin.hh"
#include "pds/management/ControlCallback.hh"
#include "pds/management/ControlLevel.hh"
#include "pds/client/Decoder.hh"

#include <time.h> // Required for timespec struct and nanosleep()
#include <stdlib.h> // Required for timespec struct and nanosleep()
#include <string.h>
#include <unistd.h>
#include <errno.h>

namespace Pds {

  class MyCallback : public ControlCallback {
  public:
    void allocated(SetOfStreams& streams) {
      //  By default, there are no external clients for this stream
      Stream& frmk = *streams.stream(StreamParams::FrameWork);
      frmk.outlet()->sink(Sequence::Event, (1<<Sequence::NumberOfServices)-1);
      (new Decoder(Level::Control))->connect(frmk.inlet());

      printf("Partition allocated\n");
    }
    void failed(Reason reason) {
      printf("Partition failed to allocate: reason %d\n", reason);
    }
    void dissolved(const Node& node) {
      printf("Partition dissolved by uid %d pid %d ip %x\n",
	     node.uid(), node.pid(), node.ip());
    }
  };

}

using namespace Pds;



int main(int argc, char** argv)
{
  unsigned partition = 0;
  unsigned bldList[32];
  unsigned nbld = 0;

  int c;
  while ((c = getopt(argc, argv, "p:b:")) != -1) {
    char* endPtr;
    switch (c) {
    case 'b':
      bldList[nbld++] = strtoul(optarg, &endPtr, 0);
      break;
    case 'p':
      partition = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) partition = 0;
      break;
    }
  }
  if (!partition) {
    printf("usage: %s -p <partition> [-b <bld>]\n", argv[0]);
    return 0;
  }

  MyCallback callback;
  ControlLevel control(partition & 0xff,
		       callback,
		       0);
  while(nbld--) {
    unsigned id = bldList[nbld];
    control.add_bld(id);
  }

  //  control.dotimeout(ConnectTimeOut);
  control.connect();

  {
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    {
      unsigned  sec  = tp.tv_sec;
      unsigned  nsec = tp.tv_nsec&~0x7FFFFF;
      unsigned  pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
      printf("control joined at %08x/%08x pulseId %08x\n",sec,nsec,pulseId);
    }

    fprintf(stdout, "Commands: EOF=quit\n");
    while (true) {
      const int maxlen=128;
      char line[maxlen];
      char* result = fgets(line, maxlen, stdin);
      if (!result) {
        fprintf(stdout, "\nExiting\n");
        break;
      } else {
	clock_gettime(CLOCK_REALTIME, &tp);
	unsigned  sec  = tp.tv_sec;
	unsigned  nsec = tp.tv_nsec&~0x7FFFFF;
	unsigned  pulseId = (tp.tv_nsec >> 23) | (tp.tv_sec << 9);
	ClockTime clockTime(sec,nsec);
	
        int len = strlen(result)-1;
        if (len <= 0) continue;
        while (*result && *result != '\n') {
          char cmd = *result++;
          switch (cmd) {
          case 'm':
            {
              Transition tr(Transition::Map,
			    Transition::Execute,
			    Sequence(Sequence::Event,
				     (Sequence::Service)Transition::Map,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
            break;
          case 'M':
            {
              Transition tr(Transition::Map,
			    Transition::Record,
			    Sequence(Sequence::Event,
				     (Sequence::Service)Transition::Map,
				     clockTime, 0, pulseId),
			    0 );
              control.mcast(tr);
            }
            break;
	  case 's':
	    {
	      //  Set the L1 time with granularity of ~8.39ms (2**23 ns) / 119Hz
	      Transition tr(Transition::L1Accept,
			    Transition::Record,
			    Sequence(Sequence::Event,
				     (Sequence::Service)Transition::L1Accept,
				     clockTime, 0, pulseId),
			    0 );
	      control.mcast(tr);
	      //	      printf("softl1 %x/%x\n",l1key.high(),l1key.low());
	    }
          }
        }
      }  
    } 
    Message msg(Message::Resign);
    control.mcast(msg);
  }

  return 0;
}
