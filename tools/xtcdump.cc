
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <new>

#include "pds/client/Browser.hh"
#include "pds/client/Decoder.hh"
#include "pds/utility/Inlet.hh"
#include "pds/utility/Outlet.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/CDatagramIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"

namespace Pds {
  class XtcFileIteratorC {
  public:
    XtcFileIteratorC(int fd, size_t maxDgramSize) :
      _fd(fd), _maxDgramSize(maxDgramSize), _buf(maxDgramSize,1) {}
    ~XtcFileIteratorC() {}
  public:
    CDatagram* next() {
      CDatagram* _cdg = new (&_buf)CDatagram(Dgram());
      Datagram& dg = _cdg->dg();
      if (::read(_fd, &dg, sizeof(dg))==0) return 0;
      size_t payloadSize = dg.xtc.sizeofPayload();
      if ((payloadSize+sizeof(*_cdg))>_maxDgramSize) {
	printf("Datagram size %zu larger than maximum: %zu\n", payloadSize+sizeof(*_cdg), _maxDgramSize);
	return 0;
      }
      ssize_t sz = ::read(_fd, dg.xtc.payload(), payloadSize);
      if (sz != payloadSize) {
	printf("XtcFileIterator::next read incomplete payload %d/%d\n",
	       sz,payloadSize);
      }
      
      return sz!=payloadSize ? 0: _cdg;
    }
  private:
    int         _fd;
    size_t      _maxDgramSize;
    GenericPool _buf;
  };
}

static void dump(const Pds::Datagram* dg)
{
  char buff[128];
  time_t t = dg->seq.clock().seconds();
  strftime(buff,128,"%H:%M:%S",localtime(&t));
  printf("%s %08x/%08x %s extent 0x%x damage %x\n",
         buff,
         dg->seq.stamp().fiducials(),dg->seq.stamp().vector(),
         Pds::TransitionId::name(dg->seq.service()),
         dg->xtc.extent, dg->xtc.damage.value());
}

using namespace Pds;

void usage(char* progname) {
  fprintf(stderr,"Usage: %s -f <filename> [-t l1Timestamp] [-T trTimestamp] [-n ndump] [-s nskip] [-l bytes] [-H] [-h]\n", progname);
  fprintf(stderr,"  timestamp format is XXXXXXXX/XXXXXXXX (hi/lo)\n");
}

static ClockTime parseClock(const char* ts)
{
  unsigned hi=0, lo=0;
  if (ts) {
    char* endPtr;
    hi = strtoul(ts      , &endPtr, 16);
    lo = strtoul(endPtr+1, &endPtr, 16);
    printf("parseClock found %08x/%08x\n",hi,lo);
  }
  return ClockTime(hi,lo);
}

int main(int argc, char* argv[]) {
  int c;
  const char* xtcname=0;
  const char* l1timestamp=0;
  const char* trtimestamp=0;
  unsigned ndump=-1;
  unsigned nskip =0;
  unsigned bytes=80;
  bool lHeader=false;
  int parseErr = 0;

  while ((c = getopt(argc, argv, "hHf:t:T:n:s:l:")) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      exit(0);
    case 'H': lHeader=true; break;
    case 'f': xtcname     = optarg; break;
    case 't': l1timestamp = optarg; break;
    case 'T': trtimestamp = optarg; break;
    case 'n': ndump = strtoul(optarg,NULL,0); break;
    case 's': nskip = strtoul(optarg,NULL,0); break;
    case 'l': bytes = strtoul(optarg,NULL,0); break;
    default:
      parseErr++;
    }
  }
  
  if (!xtcname) {
    usage(argv[0]);
    exit(2);
  }

  int fd = open(xtcname, O_RDONLY | O_LARGEFILE);
  if (fd < 0) {
    perror("Unable to open file %s\n");
    exit(2);
  }

  Inlet*   inlet   = new Inlet;
  Outlet*  outlet  = new Outlet;
  new OpenOutlet(*outlet);
  outlet->connect(inlet);
  (new Decoder(Level::Reporter))->connect(inlet);
  Browser::setDumpLength(bytes);

  XtcFileIteratorC iter(fd,0x900000);
  CDatagram* cdg;
  ClockTime l1clk = parseClock(l1timestamp);
  ClockTime trclk = parseClock(trtimestamp);
  while ((cdg = iter.next())) {
    const Datagram& dg = cdg->datagram();
    if (dg.seq.service()==TransitionId::L1Accept) {
      if (l1timestamp && l1clk == dg.seq.clock())
	l1timestamp = 0;
    }
    else {
      if (trtimestamp && trclk == dg.seq.clock())
	trtimestamp = 0;
    }
    if (l1timestamp || trtimestamp) { delete cdg; continue; }
    if (nskip) { nskip--; delete cdg; continue; }
    if (!ndump--) break;

    if (lHeader) {
      dump(&dg);
      delete cdg;
    }
    else
      inlet->post(cdg);
  }
  ::close(fd);
  return 0;
}
