//
//  simapp.cc - a small program to read an XTC file and simulate the reception of
//              data for the analysis server Appliance (TestApp).
//
#include "pdsapp/tools/TestApp.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/collection/Route.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <new>
#include <list>
#include <fcntl.h>
#include <netinet/in.h>

using namespace Pds;

//
//  create a class to iterate through the configure transition and compile a list of detectors
//
class DetInfoFinder : public XtcIterator {
public:
  DetInfoFinder() {}
public:
  const std::list<DetInfo>& detectors() const { return _detInfoList; }
public:
  int process(Xtc* xtc) {
    if (xtc->src.level()==Level::Source) {
      _detInfoList.push_back(static_cast<DetInfo&>(xtc->src));
      return 0;
    }
    else if (xtc->contains.id()==TypeId::Id_Xtc) {
      iterate(xtc);
    }
    return 1;
  }
private:
  std::list<DetInfo> _detInfoList;
};


//
//  create a class to iterate through the datagram and extract one detector's contribution
//
class DetXtcIterator : public XtcIterator {
public:
  DetXtcIterator(DetInfo info) : _info(info), _buff(new char[32]) {}
  ~DetXtcIterator() { delete[] _buff; }
public:
  void find(Dgram* dg) {
    delete[] _buff;
    _dg = dg;
    _buff = new char[sizeof(*dg)];
    memcpy(_buff,dg,sizeof(*dg));
    new (_buff+sizeof(*dg)-sizeof(Xtc)) Xtc(dg->xtc.contains,dg->xtc.src);
    XtcIterator::iterate(&dg->xtc);  // start iterating
  }
  int process(Xtc* xtc) {
    if (xtc->src.level()==Level::Source &&
        static_cast<DetInfo&>(xtc->src)==_info) {
      delete[] _buff;
      _buff = new char[sizeof(*_dg)+xtc->extent];
      memcpy(_buff, _dg, sizeof(*_dg));
      memcpy(_buff+sizeof(*_dg)-sizeof(*xtc), xtc, xtc->extent);
      return 0;  // stop iterating
    }
    else if (xtc->contains.id()==TypeId::Id_Xtc) {
      iterate(xtc);
    }
    return 1;  // continue iterating
  }
public:
  InDatagram* datagram() const { 
    //    const uint32_t* p = reinterpret_cast<const uint32_t*>(_buff);
    //    printf("%u.%09d %08x.%08x %08x : %08x %08x.%08x %08x %x\n",
    //           p[1],p[0],p[3],p[2],p[4],p[5],p[6],p[7],p[8],p[9]);
    return reinterpret_cast<InDatagram*>(_buff); 
  }
private:
  DetInfo   _info;  // the detector we search for
  Dgram*    _dg;    // the input datagram
  char*     _buff;  // the output datagram
};


void printUsage(char* s) {
  printf( "Usage: %s [-h] -f <fname>\n"
          "    -h          Show usage\n"
          "    -f          Input Xtc file\n",
          s
          );
}

int main(int argc, char** argv) {

  const char* fname = 0;  // XTC input file name

  extern char* optarg;
  extern int optind;
  char* endPtr;
  char* uniqueid = (char *)NULL;
  int c;
  while ( (c=getopt( argc, argv, "hf:")) != EOF ) {
    switch(c) {
    case 'f':
      fname = optarg;
      break;
    case 'h':
      printUsage(argv[0]);
      return 0;
    case '?':
    default:
      printUsage(argv[0]);
      return 1;
    }
  }

  if (fname==0) {
    printf("%s: -f <fname> required\n",argv[0]);
    printUsage(argv[0]);
    return 1;
  }

  int fd = open(fname, O_RDONLY | O_LARGEFILE);
  if (fd<0) {
    perror("Error opening xtc file");
    return -1;
  }

  //
  //  Lookup the FEZ subnet interface
  //    (available from Route::interface() afterwards)
  //
  { CollectionManager m(Level::Observer, 0, 32, 250, 0);
    m.start();
    if (m.connect()) {
      struct in_addr a; a.s_addr = htonl(Route::interface());
      printf("FEZ interface at %s\n",inet_ntoa(a));
    }
    else {
      printf("Unable to connect to FEZ\n");
      return -1;
    }
    m.cancel();
  }
    
  //
  //  Instanciate the test appliance
  //
  TestApp* app = new TestApp;

  //
  //  Instanciate an iterator to make a list of all detectors
  //
  DetInfoFinder detIter;
  {
    //
    //  Instanciate an iterator to process the file
    //
    XtcFileIterator fileIter(fd,0x4000000);

    //
    //  Get the list of detectors.  We will simulate one of them.
    //
    Dgram* dg = fileIter.next();
    if (dg==0) {
      printf("Error loading configure datagram\n");
      return -1;
    }
    
    detIter.iterate(&dg->xtc);
    if (detIter.detectors().empty()) {
      printf("No detectors found in xtc file\n");
      return 1;
    }
  }
  close(fd);

  while(1) {

    //  List the detectors by name
    unsigned i=0;
    for(std::list<DetInfo>::const_iterator it=detIter.detectors().begin(); 
        it!=detIter.detectors().end(); it++,i++) {
      printf("[%u] %s\n", i, DetInfo::name(*it));
    }
    printf("Choose a detector [0-%u] (%u to End): ",i-1,i);

    //  Get the choice
    scanf("%u",&i);

    if (i==detIter.detectors().size())
      return 1;

    //  Find it in the list
    DetInfo info;
    for(std::list<DetInfo>::const_iterator it=detIter.detectors().begin(); 
        it!=detIter.detectors().end(); it++,i--) {
      if (i==0) {
        info = *it;
        break;
      }
    }
    
    //  Instanciate the iterator to find this detector's data in the xtc file
    DetXtcIterator iter(info);

    int fd = open(fname, O_RDONLY | O_LARGEFILE);
    XtcFileIterator fileIter(fd,0x4000000);
    
    //  Find the detector in the configure transition
    iter.find(fileIter.next());

    //  Call the test appliance with the LCLS-II datagram
    app->events(iter.datagram());

    printf("Hit <Enter> to begin run");
    getchar();
    getchar();

    //
    //  Do the same for all the remaining transitions/events in the file
    //
    Dgram* dg;
    while( (dg=fileIter.next()) ) {
      
      iter.find(dg);
      app->events(iter.datagram());

    }
    close(fd);
  }

  return 0;
}
