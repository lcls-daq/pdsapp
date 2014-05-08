#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <string>
#include <vector>
#include <sstream>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>

using std::string;

namespace PdsUser {

  class EvrDatagram {
  public:
    enum {NumFiducialBits = 17};
    enum {MaxFiducials = (1<<17)-32};
    enum {ErrFiducial = (1<<17)-1};
  public:
    //  360 Hz globally distributed counter
    unsigned fiducials() const { return high&0x1ffff; }
    unsigned ticks    () const { return low &0xffffff; }
    unsigned vector   () const { return (high &0xfffe0000) >> 17; }
    unsigned control  () const { return (low &0xff000000)  >> 24; }
    unsigned numcmds  () const { return ncmds; }
    bool     is_event () const { return ((low>>24)&0xf) == 0xc; }
  public:
    uint32_t    nanoseconds;  // event time
    uint32_t    seconds;      // event time
    uint32_t    low;
    uint32_t    high;
    uint32_t    env;
    uint32_t    evr;    // events since configure
    uint32_t    ncmds;
    //  'ncmds' bytes follow
    enum { MaxCmds = 0x100 };
  };
};

static unsigned parse_ip(const char* ipString) {
  unsigned ip = 0;
  in_addr inp;
  if (inet_aton(ipString, &inp)) {
    ip = ntohl(inp.s_addr);
  }
  return ip;
}

static unsigned parse_interface(const char* interfaceString) {
  unsigned interface = parse_ip(interfaceString);
  if (interface == 0) {
    int so = socket(AF_INET, SOCK_DGRAM, 0);
    if (so < 0) {
      perror("Failed to open socket\n");
      return 0;
    }
    ifreq ifr;
    strcpy(ifr.ifr_name, interfaceString);
    int rv = ioctl(so, SIOCGIFADDR, (char*)&ifr);
    close(so);
    if (rv != 0) {
      printf("Cannot get IP address for network interface %s.\n",interfaceString);
      return 0;
    }
    interface = ntohl( *(unsigned*)&(ifr.ifr_addr.sa_data[2]) );
  }
  return interface;
}


/*
 * Test function
 */

static void showUsage(const char* s)
{
  printf( "Usage:  %s  -a <Multicast Address> -p <Port> -i <Interface Name/IP> -d <debug type>\n",s);
  printf( "  Debug type:  0: No Debug, 1: Debug duplcated clocktime\n");
}

int main(int argc, char** argv)
{
  unsigned      mcast_addr      = 0;
  unsigned      mcast_interface = 0;
  unsigned      mcast_port      = 0;
  int           debugType       = 0;

  char opt;
  while ( (opt = getopt(argc, argv, "a:i:p:d:h"))!=-1 ) {
    switch(opt) {
    case 'a':
      mcast_addr = ntohl(inet_addr(optarg));
      break;
    case 'i':
      mcast_interface = parse_interface(optarg);
      break;
    case 'p':
      mcast_port = strtoul(optarg, NULL, 0);
      break;
    case 'd':
      debugType  = strtol(optarg, NULL, 0);
      break;
    case 'h':
      showUsage(argv[0]);
      return 0;
    }
  }

#if 0
  if (mcast_addr==0 ||
      mcast_port==0 ||
      mcast_interface==0) {
    showUsage(argv[0]);
    return -1;
  }
#endif

  //
  //  Open UDP socket to receive multicasts
  //
  int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
  { int y=1;
    if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&y, sizeof(y)) == -1) {
      perror("set broadcast error");
      return -1; }
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1) {
      perror("set reuseaddr error");
      return -1; }

    sockaddr_in sa;
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(mcast_addr);
    sa.sin_port        = htons(mcast_port);
    memset(sa.sin_zero,0,sizeof(sa.sin_zero));
    if (::bind(fd, (sockaddr*)&sa, sizeof(sa))) {
      perror("bind error");
      return -1;
    }

    //
    //  Register for the multicast
    //
    struct ip_mreq ipMreq;
    bzero ((char*)&ipMreq, sizeof(ipMreq));
    ipMreq.imr_multiaddr.s_addr = htonl(mcast_addr);
    ipMreq.imr_interface.s_addr = htonl(mcast_interface);
    int error_join = setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                                 (char*)&ipMreq, sizeof(ipMreq));
    if (error_join==-1) {
      perror("failed to join mcast group");
      return -1;
    }
  }

  //
  //  Receive the data via ::poll()
  //
  const int len = sizeof(PdsUser::EvrDatagram)+PdsUser::EvrDatagram::MaxCmds;
  char buff[len];
  sockaddr_in sa;
  socklen_t sa_len(sizeof(sa));

  pollfd pfd[1];
  pfd[0].fd = fd;
  pfd[0].events = POLLIN | POLLERR;
  pfd[0].revents = 0;
  int nfds = 1;

  unsigned prevEvr     = 0;
  unsigned prevSeconds = 0;
  unsigned prevNanoSec = 0;
  unsigned prevFid     = 0;
  unsigned prevTicks   = 0;
  unsigned prevVector  = 0;
  unsigned prevCntl    = 0;
  unsigned prevDupFid  = 0;
  unsigned prevDupEvr  = 0;
  unsigned prevDupEvrDiff = 0;
  unsigned numDup      = 0;

  if (debugType == 0)
    printf("%6.6s  %6.6s  %6.6s  %9.9s.%9.9s   %6s   %3s %3s  %-9s\n",
    "count","fiduc","ticks","seconds","nseconds","vector","ctl","cmd","is_event");
  else if (debugType == 1) {
  }

  while(1) {
    if (::poll(pfd, nfds, 1000) > 0) {
      if (pfd[0].revents & (POLLIN|POLLERR)) {
        while (recvfrom(pfd[0].fd, buff, len, MSG_DONTWAIT, (sockaddr*)&sa, &sa_len)>0) {
          const PdsUser::EvrDatagram& dg = *reinterpret_cast<const PdsUser::EvrDatagram*>(buff);

          if (debugType == 0)
            printf("%06x  %06x  %06x  %09d.%09d  %06x  %3x %3d   %c\n",
              dg.evr, dg.fiducials(), dg.ticks(), dg.seconds, dg.nanoseconds, dg.vector(), dg.control(), dg.numcmds(), dg.is_event()?'*':' ');
          else if (debugType == 1) {
            if (dg.seconds == prevSeconds && dg.nanoseconds == prevNanoSec){
              ++numDup;

              if (prevEvr - prevDupEvr != prevDupEvrDiff)
                printf("                                         \nDuplicated clocktime [%d]:\n"
                  "    Diff from prev dup  evr %02d (prev %02d)  fiducial %02x\n"
                  "    Prev evr %06x clocktime %09d.%09d fid %06x/%06x vector %06x ctl %02x\n"
                  "    Cur  evr %06x clocktime %09d.%09d fid %06x/%06x vector %06x ctl %02x\n",
                  numDup,
                  prevEvr - prevDupEvr, prevDupEvrDiff, prevFid - prevDupFid,
                  prevEvr, prevSeconds, prevNanoSec, prevFid, prevTicks, prevVector, prevCntl,
                  dg.evr, dg.seconds, dg.nanoseconds, dg.fiducials(), dg.ticks(), dg.vector(), dg.control() );
              else
                printf("Duplicated clocktime [%d]\r", numDup);

              prevDupEvrDiff = prevEvr - prevDupEvr;
              prevDupFid     = prevFid;
              prevDupEvr     = prevEvr;
            }

            prevEvr     = dg.evr;
            prevSeconds = dg.seconds;
            prevNanoSec = dg.nanoseconds;
            prevFid     = dg.fiducials();
            prevTicks   = dg.ticks();
            prevVector  = dg.vector();
            prevCntl    = dg.control();
          }

        }
        pfd[0].revents = 0;
      }
    }
  }
}
