// $Id$

#include "pdsapp/dev/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/udpcam/UdpCamManager.hh"
#include "pds/udpcam/UdpCamServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#ifndef UDPCAM_DEFAULT_DATA_PORT
#define UDPCAM_DEFAULT_DATA_PORT 30060
#endif

static void usage(const char *p)
{
  printf("Usage: %s -i <device info> -p <platform> [-P <port>] [-v] [-h] [-d <debug flags>] [-a affinity cpu]\n", p);
}

static void help()
{
  printf("Options:\n"
         "  -i              Device info           [required]\n"
         "                    integer/integer/integer/integer or string/integer/string/integer\n"
         "                    (e.g. XcsEndStation/0/Fccd960/0 or 25/0/33/0)\n"
         "  -p <platform>   Platform number       [required]\n"
         "  -D <port>       Data port             (default: %d)\n"
         "  -v              Increase verbosity    (may be repeated)\n"
         "  -d              Debug flags\n"
         "  -a              Affinity CPU Id\n"
         "  -h              Help: print this message and exit\n"
         "Debug flags:\n"
         "  0x0010          Ignore frame count errors\n"
         "  0x0020          Ignore packet count errors\n", UDPCAM_DEFAULT_DATA_PORT);
}

namespace Pds
{
  class MySegWire;
  class Seg;
}

//
//  This class creates the server when the streams are connected.
//
class Pds::MySegWire
   : public SegWireSettings
{
  public:
    MySegWire(UdpCamServer* udpCamServer);
    virtual ~MySegWire() {}

    void connect( InletWire& wire,
                  StreamParams::StreamType s,
                  int interface );

    const std::list<Src>& sources() const { return _sources; }

  private:
    UdpCamServer* _udpCamServer;
    std::list<Src> _sources;
};

//
//  Implements the callbacks for attaching/dissolving.
//  Appliances can be added to the stream here.
//
class Pds::Seg : public EventCallback
{
  public:
    Seg( Task* task,
         unsigned platform,
         CfgClientNfs* cfgService,
         SegWireSettings& settings,
         Arp* arp,
         UdpCamServer* udpCamServer );

    virtual ~Seg();

  private:
    // Implements EventCallback
    void attached( SetOfStreams& streams );
    void failed( Reason reason );
    void dissolved( const Node& who );

    Task* _task;
    unsigned _platform;
    CfgClientNfs* _cfg;
    UdpCamServer* _udpCamServer;
};


Pds::MySegWire::MySegWire( UdpCamServer* udpCamServer )
  : _udpCamServer(udpCamServer)
{
  _sources.push_back(udpCamServer->client());
}

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
  printf("Adding input of server, fd %d\n", _udpCamServer->fd() );
  wire.add_input( _udpCamServer );
}


Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               UdpCamServer* udpCamServer )
  : _task(task),
    _platform(platform),
    _cfg   (cfgService),
    _udpCamServer(udpCamServer)
{}

Pds::Seg::~Seg()
{
  _task->destroy();
}

void Pds::Seg::attached( SetOfStreams& streams )
{
  printf("Seg connected to platform 0x%x\n",
         _platform);

  Stream* frmk = streams.stream(StreamParams::FrameWork);
  UdpCamManager& udpCamMgr = * new UdpCamManager( _udpCamServer,
                                                      _cfg );
  udpCamMgr.appliance().connect( frmk->inlet() );
}

void Pds::Seg::failed( Reason reason )
{
  static const char* reasonname[] = { "platform unavailable",
                                      "crates unavailable",
                                      "fcpm unavailable" };
  printf( "Seg: unable to allocate crates on platform 0x%x : %s\n",
          _platform, reasonname[reason]);
  delete this;
}

void Pds::Seg::dissolved( const Node& who )
{
  const unsigned userlen = 12;
  char username[userlen];
  Node::user_name( who.uid(),
                   username,userlen );

  const unsigned iplen = 64;
  char ipname[iplen];
  Node::ip_name( who.ip(),
                 ipname, iplen );

  printf( "Seg: platform 0x%x dissolved by user %s, pid %d, on node %s",
          who.platform(), username, who.pid(), ipname);

  delete this;
}

using namespace Pds;

static UdpCamServer* udpCamServer;

int main( int argc, char** argv )
{
  DetInfo info;
  bool infoInitialized = false;
  unsigned platform = -1UL;
  unsigned dataPort = UDPCAM_DEFAULT_DATA_PORT;
  unsigned debugFlags = 0;
  Arp* arp = 0;
  unsigned verbosity = 0;
  bool helpFlag = false;
  int cpu0 = -1;
  char *endPtr;

  extern char* optarg;
  int c;
  while( ( c = getopt( argc, argv, "i:D:vhp:u:d:a:" ) ) != EOF ) {
    switch(c) {
      case 'i':
        if (!CmdLineTools::parseDetInfo(optarg,info)) {
          usage(argv[0]);
          return -1;
        }
        infoInitialized = true;
        break;
      case 'u':
        printf("Warning: -u flag ignored\n");
        break;
      case 'h':
        helpFlag = true;
        break;
      case 'v':
        ++verbosity;
        break;
      case 'p':
        errno = 0;
        endPtr = NULL;
        platform = strtoul(optarg, &endPtr, 0);
        if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
          printf("Error: failed to parse platform number\n");
          usage(argv[0]);
          return -1;
        }
        break;
      case 'a':
        errno = 0;
        endPtr = NULL;
        cpu0 = strtoul(optarg, &endPtr, 0);
        if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
          printf("Error: failed to parse affinity cpu\n");
          usage(argv[0]);
          return -1;
        }
        break;
      case 'D':
        errno = 0;
        endPtr = NULL;
        dataPort = strtoul(optarg, &endPtr, 0);
        if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
          printf("Error: failed to parse data port number\n");
          usage(argv[0]);
          return -1;
        }
        break;
      case 'd':
        errno = 0;
        endPtr = NULL;
        debugFlags = strtoul(optarg, &endPtr, 0);
        if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
          printf("Error: failed to parse debug flags\n");
          usage(argv[0]);
          return -1;
        }
        break;
      }
   }

  if (helpFlag) {
    usage(argv[0]);
    help();
    return 0;
  } else if ((platform == -1UL) || (!infoInitialized)) {
    printf("Error: Platform and device info required\n");
    usage(argv[0]);
    return 0;
  }

  Node node( Level::Source, platform );

  Task* task = new Task( Task::MakeThisATask );

  CfgClientNfs* cfgService;

  cfgService = new CfgClientNfs(info);

  udpCamServer = new UdpCamServer(info, verbosity, dataPort, debugFlags, cpu0);

  MySegWire settings(udpCamServer);

  Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       arp,
                       udpCamServer );

  SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              arp );

  if (seglevel->attach()) {
    printf("entering udpCam task main loop\n");
    task->mainLoop();
    printf("exiting udpCam task main loop\n");
  }

   return 0;
}
