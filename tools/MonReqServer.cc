#include "pdsapp/tools/MonReqServer.hh"
#include <stdio.h>
#include <stdlib.h>

#include "udp_info.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Ins.hh"

using namespace Pds;


MonReqServer::MonReqServer(unsigned int& k) :
  _task(new Task(TaskObject("monlisten")))
{
  _task->call(this);
  id=k;
}

MonReqServer::~MonReqServer()
{
  _task->destroy();
}


Transition* MonReqServer::transitions(Transition* tr)
{
  printf("MonReqServer transition %s\n",TransitionId::name(tr->id()));
  return tr;
}

void MonReqServer::routine()
{

//udp multicast setup//

    int udp_socket_info;
    //struct sockaddr_in udp_server;
    struct sockaddr addr;
    //struct ip_mreq mreq;
	socklen_t fromlen;
	fromlen = sizeof addr;

        udp_socket_info = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket_info == -1) {
        perror("Could not create udp socket");
	    }
	puts("UDP SOCKET CREATED");

 Sockaddr usv(StreamPorts::monRequest(0).portId());
	//memset((char*)&udp_server,0,sizeof(udp_server));
        //udp_server.sin_family=AF_INET;
        //udp_server.sin_port = htons( 1100 ); 
	//udp_server.sin_addr.s_addr = INADDR_ANY; 
     
        //if (bind(udp_socket_info,(struct sockaddr *)&udp_server, sizeof(udp_server)) < 0) {
	if (bind(udp_socket_info,(struct sockaddr *)&usv, sizeof(usv)) < 0) {
	  perror("udp bind error");
	  exit (1);
     	  }
	puts("udp bind");
 
 Sockaddr sv(StreamPorts::monRequest(0));
     //mreq.imr_multiaddr.s_addr=inet_addr("225.0.0.37"); 
     //mreq.imr_interface.s_addr=htonl(INADDR_ANY); 

     //if (setsockopt(udp_socket_info, IPPROTO_IP,IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
	if (setsockopt(udp_socket_info, IPPROTO_IP,IP_ADD_MEMBERSHIP, sv.name(), sv.sizeofName()) < 0) {

	  perror("setsockopt");
	  exit (1);
          }

int node, ip, port;

	while(1){
			UdpInfo c(ip, port, node);
			if (recvfrom(udp_socket_info, &c, sizeof(c), 0, &addr, &fromlen) < 0) {
			perror("Recvfrom error:");
			}
		node = c.node_num;
		printf("Node vs 1<<id: %i, %i\n", node, (1<<id));
		
		in_addr inaddr;
		inaddr.s_addr = htonl(c.ip_add);
     		tcp_ip = inet_ntoa(inaddr);
                port = c.port_num;
		printf( "ip: %s, port: %i, node: %i\n", tcp_ip, port, node);

		if(node&(1<<id)) break; 
		}



	printf("Node vs 1<<id: %i, %i\n", node, (1<<id));
		
        struct sockaddr_in tcp_server;
        tcp_socket_info = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_socket_info == -1) {
        printf("Could not create socket");
        }
        printf("TCP socket created\n");

	tcp_port = (char*)port;
        unsigned long test_port = strtoul(tcp_port, NULL, 0);

        printf("tcp_ip: %s tcp_port: %li \n", tcp_ip, test_port);

        tcp_server.sin_addr.s_addr = inet_addr(tcp_ip);
        tcp_server.sin_family = AF_INET;
        tcp_server.sin_port = htons(test_port);


        if (::connect(tcp_socket_info, (struct sockaddr *)&tcp_server, sizeof(tcp_server)) < 0) {
        perror("TCP Connection error");
        }
        puts("TCP Connected");

  	number_of_requests_fullfilled = 0;
  	number_of_requests_received = 0;

    	///receiving message asking for event
    	while (1) {  
      	  if(recvfrom(tcp_socket_info, (char *)&number_of_requests_received , sizeof(number_of_requests_received), 0, &addr, &fromlen) > 0) {
      	  printf("NUMBER OF REQUESTS RECEIVED: %i\n", number_of_requests_received);
		}
   	}
	

}  

InDatagram* MonReqServer::events     (InDatagram* dg) 
{

if( tcp_socket_info != 0) {

         //if we have any requests
         if (dg->seq.service()==TransitionId::L1Accept) { 

		 if(number_of_requests_fullfilled < number_of_requests_received) {


            	 // send an event
            	 int ca = send(tcp_socket_info, &dg->datagram(), sizeof(Dgram)+ dg->datagram().xtc.sizeofPayload(), 0 );
            	 printf("Datagram that is sending: %i\n", ca);

		 //checking if sending correctly
		 printf("sizeofPayload: [%d]\n", dg->datagram().xtc.sizeofPayload());
             
		number_of_requests_fullfilled = number_of_requests_fullfilled +1;
		printf("REQUESTS FULLFILLED: %i\n", number_of_requests_fullfilled);
		}
	}
        else {
         int ca = send(tcp_socket_info, &dg->datagram(), sizeof(Dgram)+ dg->datagram().xtc.sizeofPayload(), 0 );
             printf("%i\n", ca);
    	}
         
}


 return dg;
}



