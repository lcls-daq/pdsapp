#include "pdsdata/app/XtcMonitorServer.hh"
#include "pdsapp/tools/PnccdShuffle.hh"
#include "pdsapp/tools/CspadShuffle.hh"
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "pds/collection/CollectionManager.hh"
#include "pds/collection/Route.hh"

#include "pds/monreq/ConnectionRequestor.hh"
#include "pds/monreq/ReceivingConnection.hh"
#include "pds/monreq/ServerConnection.hh"

#include "pds/service/Sockaddr.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Task.hh"
#include <vector>

#include "pds/vmon/VmonEb.hh"
#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonDescScalar.hh"

#include "pds/management/Query.hh"
#include "pds/management/SourceLevel.hh"

#include "pds/xtc/CDatagram.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include <time.h>
#include <math.h>

namespace Pds {
  class MyXtcMonitorServer : public XtcMonitorServer,
                             public Routine {
  public:
    MyXtcMonitorServer(const char* tag,
		       unsigned sizeofBuffers, 
		       unsigned numberofEvBuffers, 
		       unsigned numberofEvQueues, const char* intf) : 
      XtcMonitorServer(tag,
		       sizeofBuffers,
		       numberofEvBuffers,
		       numberofEvQueues), 
      _task(new Task(TaskObject("monmcast")))   
    {
      _init();
    }
    ~MyXtcMonitorServer() {}
  public:

    void run(int nodes, unsigned int platform) { 


      //
      //  Find the interface for our private network
      //
      int iip = -1;
      {
        CollectionManager m(Level::Observer, 
                            0, 
                            sizeof(PartitionAllocation), 
                            250, 
                            0);
        m.start();
        if (m.connect()) {
          iip = Route::interface();
          printf("Found source on interface %08x\n",iip);
        }
        else
          printf("Failed to find source\n");
        m.cancel();
      }
   
 Ins vmon(StreamPorts::vmon(platform));

ProcInfo  info(Level::Observer, getpid(), iip);
char buff[256];
      sprintf(buff,"%s[%d.%d.%d.%d : %d]",
              Level::name(info.level()),
              (info.ipAddr()>>24)&0xff,
              (info.ipAddr()>>16)&0xff,
              (info.ipAddr()>> 8)&0xff,
              (info.ipAddr()>> 0)&0xff,
              info.processId());
    VmonServerManager::instance(buff)->listen(info, vmon);
        MonGroup* group = new MonGroup("event_tracking");
  	VmonServerManager::instance()->cds().add(group);
  	
      //
      //  Start a thread to request connections to the servers
      //
      _connReq = Pds::MonReq::ConnectionRequestor(iip, nodes, StreamPorts::monRequest(platform));
      _task->call(this);

      //
      //  Poll for connections and received events (forever)
      //
      while(1) {

	int nfd=1+_receivers.size();
    std::vector<pollfd> pfd(nfd);
    pfd[0].fd=_connReq.socket();
    pfd[0].events = POLLIN|POLLERR;
    for(unsigned i=0; i<_receivers.size(); i++) {
      pfd[i+1].fd = _receivers[i].socket();
      pfd[i+1].events = POLLIN|POLLERR;
    }

        int r;
	r=poll(&pfd[0], pfd.size(), 1000);
	
		if(pfd[0].revents&POLLIN) { //add new connections to array
			int fd=_connReq.receiveConnection();
			if (fd <0) {
			}
			else {
			_receivers.push_back(Pds::MonReq::ReceivingConnection(fd, group));
			while (_initialRequests) { 
			_receivers.back().request();
			--_initialRequests;
			}
			}
		}

		for(unsigned j=1; j<pfd.size(); j++) {
			if(pfd[j].revents&POLLIN) {  //if there is new data from existing connection, recv it 
				char* p= _receivers[j-1].receive();
				if(p == 0) {
					_receivers.erase(_receivers.begin()+j-1); 
					pfd.erase(pfd.begin()+j);
					continue;
				}
				
				Dgram* dg = ((Dgram*)p);

				if( !dg->seq.isEvent() && j != 1 ) {
					 delete[] p;
					}
				else { 
				if (XtcMonitorServer::events((Dgram*)p) == XtcMonitorServer::Deferred) {
				continue;
				} 
				else {
				delete[] p; 
				}
				}
			}

		}
	
      }
    }

    void routine() {
      //
      // Poll for new connections on listener and request new connections
      //

     while(1) {
        _connReq.requestConnections();
        sleep(1);
	}
   }
  private:
    void _copyDatagram(Dgram* dg, char* b) {
    Datagram& dgrm = *reinterpret_cast<Datagram*>(dg);

    PnccdShuffle::shuffle(dgrm);
    CspadShuffle::shuffle(reinterpret_cast<Dgram&>(dgrm));

    //  write the datagram
    memcpy(b, &dgrm, sizeof(Datagram));
    //  write the payload
    memcpy(b+sizeof(Datagram), dgrm.xtc.payload(), dgrm.xtc.sizeofPayload());
	}

    void _deleteDatagram(Dgram* dg) {
	delete[] (char *)dg;
	}
    void _requestDatagram() {
	//printf("_requestDatagram\n");
      if (_receivers.size()==0) {
	_initialRequests++;
      }
      else {
        //
        //  Make a request from one of the servers
        //

        //
        //  Determine "best"
        //
	int smallest_pending=0;
	int location =0;
	int pend=0;

        for(unsigned i=0; i<_receivers.size(); i++) {

		pend = _receivers[i].ratio();

			if (i == 0) {
			smallest_pending = pend;
			}

		if( pend < smallest_pending) {
			smallest_pending = pend;
			location = i;
		}
	}
	_receivers[location].request();

      }
    }


  private:
    Task*                            _task;
    int                              _initialRequests;
    Pds::MonReq::ConnectionRequestor _connReq;
    std::vector<Pds::MonReq::ReceivingConnection>  _receivers; 
  };
};

using namespace Pds;

void usage(char* progname) {
  printf("Usage: %s -p <platform> -P <partition> -i <node mask> -n <numb shm buffers> -s <shm buffer size> [-q <# event queues>] [-t <tag name>] [-d] [-c] [-g <max groups>] [-h]\n", progname);
}

int main(int argc, char** argv) {

  const unsigned NO_PLATFORM = unsigned(-1UL);
  unsigned platform=NO_PLATFORM;
  const char* partition = 0;
  const char* tag = 0;
const char* intf = 0;
  int numberOfBuffers = 0;
  unsigned sizeOfBuffers = 0;
  unsigned nevqueues = 1;
  unsigned node =  0xffff;
  unsigned nodes = 6;
  bool ldist = false;

  int c;
  while ((c = getopt(argc, argv, "I:p:i:g:n:P:s:q:t:dch")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = NO_PLATFORM;
      break;
case 'I':
intf = optarg;
break;
    case 'i':
      node = strtoul(optarg, &endPtr, 0);
      break;
    case 'g':
      nodes = strtoul(optarg, &endPtr, 0);
      break;
    case 'n':
      sscanf(optarg, "%d", &numberOfBuffers);
      break;
    case 'P':
      partition = optarg;
      break;
    case 't':
      tag = optarg;
      break;
    case 'q':
      nevqueues = strtoul(optarg, NULL, 0);
      break;
    case 's':
      sizeOfBuffers = (unsigned) strtoul(optarg, NULL, 0);
      break;
    case 'd':
      ldist = true;
      break;
    case 'h':
      // help
      usage(argv[0]);
      return 0;
      break;
    default:
      printf("Unrecogized parameter\n");
      usage(argv[0]);
      break;
    }
  }

  if (!numberOfBuffers || !sizeOfBuffers || platform == NO_PLATFORM || !partition || node == 0xffff) {
    fprintf(stderr, "Missing parameters!\n");
    usage(argv[0]);
    return 1;
  }

  if (numberOfBuffers<8) numberOfBuffers=8;

  if (!tag) tag=partition;

  printf("\nPartition Tag:%s\n", tag);

  MyXtcMonitorServer* apps = new MyXtcMonitorServer(tag, 
						    sizeOfBuffers, 
						    numberOfBuffers, 
						    nevqueues, intf);
  apps->distribute(ldist);

  apps->run(node, platform); //pass variable node and platform
  
  return 0;
}

