#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "pds/service/NetServer.hh"
#include "pds/service/Client.hh"
#include "pds/utility/Mtu.hh"

#include "pdsdata/xtc/Dgram.hh"

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -i <input interface> -o <output interface> -a <address> -p <port>>\n",p);
}

static unsigned parse_network(const char* arg)
{
  unsigned interface = 0;
  if (arg[0]<'0' || arg[0]>'9') {
    int skt = socket(AF_INET, SOCK_DGRAM, 0);
    if (skt<0) {
      perror("Failed to open socket\n");
      exit(1);
    }
    ifreq ifr;
    strcpy( ifr.ifr_name, optarg);
    if (ioctl( skt, SIOCGIFADDR, (char*)&ifr)==0)
      interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
    else {
      printf("Cannot get IP address for network interface %s.\n",optarg);
    }
    printf("Using interface %s (%d.%d.%d.%d)\n",
           arg,
           (interface>>24)&0xff,
           (interface>>16)&0xff,
           (interface>> 8)&0xff,
           (interface>> 0)&0xff);
    close(skt);
  }
  else {
    in_addr inp;
    if (inet_aton(arg, &inp))
      interface = ntohl(inp.s_addr);
  }
  return interface;
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned input(0);
  unsigned output(0);
  unsigned mcaddr = 0xefff1800;
  unsigned short port = 10148;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:o:a:p:")) != EOF ) {
    switch(c) {
    case 'i':
      input = parse_network(optarg);
      break;
    case 'o':
      output = parse_network(optarg);
      break;
    case 'a':
      mcaddr = parse_network(optarg);
      break;
    case 'p':
      port = strtoul(optarg,NULL,0);
      break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  if (input==0 || output==0) {
    usage(argv[0]);
    exit(1);
  }

  const int PktSize = Mtu::Size + sizeof(Xtc);

  Ins dst((int)mcaddr,port);
  Ins idst((int)output);
  Client cli(sizeof(Dgram),
             PktSize,
             dst,
             idst,
             32);

  Ins src((int)mcaddr,port);
  Ins isrc((int)input);
  NetServer srv(0, src, sizeof(Dgram), PktSize);
  srv.join(src, isrc);

  char* buff = new char[PktSize];

  while(1) {
    int len = srv.fetch(buff,0);
    
    int result = cli.send(const_cast<char*>(srv.datagram()),
                          buff,
                          len);
  }
  
  return 0;
}
