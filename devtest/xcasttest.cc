#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <time.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <vector>

//#define USE_RAW

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
  printf("Using interface %s (%d.%d.%d.%d)\n",
         interfaceString,
         (interface>>24)&0xff,
         (interface>>16)&0xff,
         (interface>> 8)&0xff,
         (interface>> 0)&0xff);
  return interface;
}


int main(int argc, char **argv) 
{
  unsigned interface = 0x7f000001;
  unsigned port  = 0;
  ssize_t  sz    = 0x2000;
  bool lreceiver = false;
  std::vector<unsigned> uaddr;

  for(int i=0; i<argc; i++) {
    if (strcmp(argv[i],"-a")==0) {
      for(char* arg = strtok(argv[++i],","); arg!=NULL; arg=strtok(NULL,","))
	uaddr.push_back(parse_ip(arg));
    }
    else if (strcmp(argv[i],"-i")==0) {
      interface = parse_interface(argv[++i]);
    }
    else if (strcmp(argv[i],"-p")==0) {
      port = strtoul(argv[++i],NULL,0);
    }
    else if (strcmp(argv[i],"-s")==0) {
      sz   = strtoul(argv[++i],NULL,0);
    }
    else if (strcmp(argv[i],"-r")==0)
      lreceiver = true;
  }

  std::vector<sockaddr_in> address(uaddr.size());
  for(unsigned i=0; i<uaddr.size(); i++) {
    sockaddr_in sa;
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = htonl(uaddr[i]);
    sa.sin_port        = htons(port);
    memset(sa.sin_zero,0,sizeof(sa.sin_zero));
    address[i] = sa;
    printf("dst[%d] : %x.%d\n",i,uaddr[i],port);
  }

#ifdef USE_RAW
  int fd;
  if (lreceiver) {
    if ((fd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      perror("socket");
      return -1;
    }
  }
  else {
    if ((fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0) {
      perror("socket");
      return -1;
    }
    int y=1;
    if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (char*)&y, sizeof(y)) == -1) {
      perror("set ip_hdrincl");
      return -1;
    }
  }
#else
  int fd;
  if ((fd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
#endif
  
  unsigned skb_size = 0x100000;

  int y=1;
  if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char*)&y, sizeof(y)) == -1) {
    perror("set broadcast");
    return -1;
  }
      
  if (lreceiver) {
    if (::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, 
                     (char*)&skb_size, sizeof(skb_size)) < 0) {
      perror("so_rcvbuf");
      return -1;
    }

    if (::bind(fd, (sockaddr*)&address[0], sizeof(sockaddr_in)) < 0) {
      perror("bind");
      return -1;
    }
    
    if ((uaddr[0]>>28) == 0xe) {

      if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1) {
	perror("set reuseaddr");
	return -1;
      }

      struct ip_mreq ipMreq;
      bzero ((char*)&ipMreq, sizeof(ipMreq));
      ipMreq.imr_multiaddr.s_addr = htonl(uaddr[0]);
      ipMreq.imr_interface.s_addr = htonl(interface);
      int error_join = setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                                   (char*)&ipMreq, sizeof(ipMreq));
      if (error_join==-1) {
        perror("mcast_join");
        return -1;
      }
      else
	printf("Joined %x on if %x\n", uaddr[0], interface);
    }
  }
  else {
    if (::setsockopt(fd, SOL_SOCKET, SO_SNDBUF, 
                     (char*)&skb_size, sizeof(skb_size)) < 0) {
      perror("so_sndbuf");
      return -1;
    }

#ifdef USE_RAW
#else
    { sockaddr_in sa;
      sa.sin_family      = AF_INET;
      sa.sin_addr.s_addr = htonl(interface|0xff);
      sa.sin_port        = htons(port);
      memset(sa.sin_zero,0,sizeof(sa.sin_zero));
      printf("binding to %x.%d\n",ntohl(sa.sin_addr.s_addr),ntohs(sa.sin_port));
      if (::bind(fd, (sockaddr*)&sa, sizeof(sockaddr_in)) < 0) {
        perror("bind");
        return -1;
      }
      sockaddr_in name;
      socklen_t name_len=sizeof(name);
      if (::getsockname(fd,(sockaddr*)&name,&name_len) == -1) {
        perror("getsockname");
        return -1;
      }
      printf("bound to %x.%d\n",ntohl(name.sin_addr.s_addr),ntohs(name.sin_port));
    }
#endif    

    if ((uaddr[0]>>28) == 0xe) {
      in_addr addr;
      addr.s_addr = htonl(interface);
      if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char*)&addr,
                     sizeof(in_addr)) < 0) {
        perror("set ip_mc_if");
        return -1;
      }
    }
  }

  uint64_t nbytes = 0, tbytes = 0;
  char* buff = new char[sz];
  iphdr* ip = (iphdr*)buff;
  ip->ihl = 5;
  ip->version = 4;
  ip->tos = 0;
  ip->tot_len = sz + sizeof(udphdr) + sizeof(iphdr);
  ip->id = 0;
  ip->frag_off = 0;
  ip->ttl = 1;
  ip->protocol = IPPROTO_UDP;
  ip->check = 0;
  ip->saddr = htonl(interface|0xff);

  udphdr* udp = (udphdr*)(ip+1);
  udp->source = 0;
  udp->dest   = htons(port);
  udp->len    = htons(sz+sizeof(udphdr));
  udp->check  = 0;

  timespec tv_begin;
  clock_gettime(CLOCK_REALTIME,&tv_begin);
  double t = 0;

  unsigned iaddr=0;

  while(1) {

    ssize_t bytes;
    if (lreceiver) 
      bytes = ::recv(fd, buff, sz, 0);
    else {
      const sockaddr_in& sa = address[iaddr];
#ifdef USE_RAW
      ip->id++;
      ip->daddr = sa.sin_addr.s_addr;
      bytes = ::sendto(fd, buff, sz+sizeof(iphdr), 0, (const sockaddr*)&sa, sizeof(sockaddr_in));
#else
      bytes = ::sendto(fd, buff, sz, 0, (const sockaddr*)&sa, sizeof(sockaddr_in));
#endif
      iaddr = (iaddr+1)%address.size();
    }

    if (bytes < 0) {
      perror("recv/send");
      continue;
    }
    tbytes += bytes;

    timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    double dt = double(tv.tv_sec - tv_begin.tv_sec) +
      1.e-9*(double(tv.tv_nsec)-double(tv_begin.tv_nsec));
    if (dt > 1) {
      t += dt;
      nbytes += tbytes;
      printf("\t%uMB/s\t%uMB/s\t%dMB\t%d.%09d\n",
	     unsigned(double(nbytes)/ t*1.e-6),
	     unsigned(double(tbytes)/dt*1.e-6),
             unsigned(double(tbytes)*1.e-6),
             tv.tv_sec, tv.tv_nsec);
                      
      tv_begin = tv;
      tbytes = 0;
    }
  }

  return 0;
}
