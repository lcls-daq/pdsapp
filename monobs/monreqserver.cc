#include "pdsdata/app/XtcMonitorServer.hh"
#include "pdsapp/tools/MonReqServer.hh"
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

#include "pdsapp/tools/udp_info.hh"
#include "pdsapp/tools/event_tracking.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Ins.hh"
#include <vector>

namespace Pds {
  class MyXtcMonitorServer : public XtcMonitorServer {
  public:
    MyXtcMonitorServer(const char* tag,
		       unsigned sizeofBuffers, 
		       unsigned numberofEvBuffers, 
		       unsigned numberofEvQueues, const char* intf) : 
      XtcMonitorServer(tag,
		       sizeofBuffers,
		       numberofEvBuffers,
		       numberofEvQueues) 

    {
      _init();
    }
    ~MyXtcMonitorServer() {}
  public:

    void run(int node) { 

  //udp multicast setup//
 int iip = -1;
  {
    CollectionManager m(Level::Observer, 0, sizeof(Node), 250, 0);
    m.start();
    if (m.connect()) {
      iip = Route::interface();
      printf("Found source on interface %08x\n",iip);
    }
    else
      printf("Failed to find source\n");
    m.cancel();
  }

in_addr inaddr;
inaddr.s_addr = htonl(iip);

        int udp_socket_info;
        //struct sockaddr_in; 
        //struct udp_server;

    	udp_socket_info = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket_info == -1) {
        puts("Could not create udp socket");
        }
	puts("Udp socket created");

 Sockaddr sv(StreamPorts::monRequest(0));
        //udp_server.sin_addr.s_addr = inet_addr("225.0.0.37"); 
        //udp_server.sin_port = htons(1100);
	//udp_server.sin_family = AF_INET;

	//int port = 1100;
	printf("ip: %i, port: %i, node: %i\n", iip, sv.get().portId(), node);

UdpInfo c(iip, sv.get().portId(), node);
//sendto(udp_socket_info, &c, sizeof(c), 0, (struct sockaddr *)&udp_server, sizeof(udp_server));
sendto(udp_socket_info, &c, sizeof(c), 0, sv.name(), sv.sizeofName());

 //tcp socket setup//

/////////////////////////////////
/// POLL FUNCTION //////////////
///////////////////////////////

        struct sockaddr_in listen_client;
	struct sockaddr_in listen_server;
	int size_listen = sizeof(sockaddr_in);

	listen_socket = socket(AF_INET,SOCK_STREAM,0);
        if (listen_socket == -1) {
        perror("Could not create socket");
        }
	printf("tcp socket created\n");


	listen_server.sin_addr.s_addr = INADDR_ANY;
        listen_server.sin_family = AF_INET;
        listen_server.sin_port = htons(1100);

	int y=1;
        if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1) {
	perror("set reuseaddr");
        }
	
	if (bind(listen_socket, (struct sockaddr *)&listen_server, sizeof(listen_server)) < 0) {
	perror("tcp bind error: \n");
	}
	printf("tcp bind successful\n");

        listen(listen_socket, 5);

	{ pollfd l;
	l.fd = listen_socket;
	l.events = POLLIN|POLLERR;
	pfd.push_back(l); }

	while (poll(&pfd[0], pfd.size(), -1) > 0) { //loop with no timeout
		if(pfd[0].revents&POLLIN) { //add new connections to array
		int i;
		if(( i = accept(pfd[0].fd,(struct sockaddr *)&listen_client, (socklen_t*)&size_listen)) == -1){
			perror("tcp accept failed: \n");
			}
			printf("tcp accept\n");
		
		
		if (pfd.size() == 1) {
		track.push_back(new event_tracking(i, initial_requests));
		}
		if (pfd.size() != 1) {
		int no_requests_yet = 0;
		track.push_back(new event_tracking(i, no_requests_yet));
		}

                pollfd k;
		k.fd = i;
		k.events = POLLIN|POLLERR;
		pfd.push_back(k); 
		}
	        
		for(unsigned j=1; j<pfd.size(); j++) {
			if(pfd[j].revents&POLLIN) {  //if there is new data from existing connection, recv it 
			 
        
         		char* p = track[j-1]->receive_datagram();

			if (p == 0) {
		        delete track[j-1];
			track.erase(track.begin()+j-1);

			pfd.erase(pfd.begin()+j); 
			}


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

	if( listen_socket == 0) {
	initial_requests = initial_requests +1;
	}

	int pend = 0;
	int smallest_pending= 1000000;
	int location= 0;

	if (listen_socket != 0) {

		for (unsigned j=0; j<track.size(); j++) {
		pend = track[j]->number_pending();
		printf("PEND %i\n", pend);

	                if( pend < smallest_pending) {
			smallest_pending = pend;
			location = j;
			}		
		}

		printf("largest number pending: %i, where to send request %i\n", smallest_pending, location); 
		track[location]->send_request();
	}

    }


  private:
   std::vector<event_tracking*> track; 
   int listen_socket;	
   int initial_requests;
   std::vector<pollfd> pfd;

    
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

  apps->run(node); //pass variable (node) 
  
  return 0;
}

