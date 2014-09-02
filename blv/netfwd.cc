#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pds/service/CmdLineTools.hh"
#include "pds/service/NetServer.hh"
#include "pds/service/Client.hh"
#include "pds/utility/Mtu.hh"

#include "pdsdata/xtc/Dgram.hh"

#define NETFWD_DEFAULT_MCADDR   0xefff1800
#define NETFWD_DEFAULT_PORT     10148

extern int optind;

using namespace Pds;

void usage(const char* p) {
  printf("Usage: %s -i <input interface> -o <output interface> [OPTIONS]\n\n",p);
  printf("Options:\n");
  printf("  -a <address>     Address (default=0x%x)\n", NETFWD_DEFAULT_MCADDR);
  printf("  -p <port>        Port (default=%d)\n", NETFWD_DEFAULT_PORT);
  printf("  -h               Help: print this message and exit\n");
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
  unsigned mcaddr = NETFWD_DEFAULT_MCADDR;
  unsigned short port = NETFWD_DEFAULT_PORT;
  unsigned int uu;
  bool lUsage = false;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:o:a:p:h")) != EOF ) {
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
      if (CmdLineTools::parseUInt(optarg, uu)) {
        port = (unsigned short)uu;
      } else {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'h':
      usage(argv[0]);
      exit(0);
    case '?':
    default:
      lUsage = true;
      break;
    }
  }

  if (input==0 || output==0) {
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
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
    
    cli.send(const_cast<char*>(srv.datagram()),
             buff,
             len);
  }
  
  return 0;
}
