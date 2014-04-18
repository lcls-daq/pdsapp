// $Id$

#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/rayonix/RayonixManager.hh"
#include "pds/rayonix/RayonixServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

static void usage(const char *p)
{
  printf("Usage: %s -i <detid> -p <platform> [-u <alias>] [-v]\n", p);
}

static void help()
{
  printf("Options:\n"
         "  -i <detid>            detector ID        [required]\n"
         "  -p <platform>         platform number    [required]\n"
         "  -u <alias>            set device alias\n"
         "  -v                    increase verbosity (may be repeated)\n");
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
    MySegWire(RayonixServer* rayonixServer, const char *aliasName);
    virtual ~MySegWire() {}

    void connect( InletWire& wire,
                  StreamParams::StreamType s,
                  int interface );

    const std::list<Src>& sources() const { return _sources; }

    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }

  private:
    RayonixServer* _rayonixServer;
    std::list<Src> _sources;
    std::list<SrcAlias> _aliases;
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
         RayonixServer* rayonixServer );

    virtual ~Seg();

  private:
    // Implements EventCallback
    void attached( SetOfStreams& streams );
    void failed( Reason reason );
    void dissolved( const Node& who );

    Task* _task;
    unsigned _platform;
    CfgClientNfs* _cfg;
    RayonixServer* _rayonixServer;
};


Pds::MySegWire::MySegWire( RayonixServer* rayonixServer, const char *aliasName)
  : _rayonixServer(rayonixServer)
{
  _sources.push_back(rayonixServer->client());
  if (aliasName) {
    SrcAlias tmpAlias(rayonixServer->client(), aliasName);
    _aliases.push_back(tmpAlias);
  }
}

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
  printf("Adding input of server, fd %d\n", _rayonixServer->fd() );
  wire.add_input( _rayonixServer );
}


Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               RayonixServer* rayonixServer )
  : _task(task),
    _platform(platform),
    _cfg   (cfgService),
    _rayonixServer(rayonixServer)
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
  RayonixManager& rayonixMgr = * new RayonixManager( _rayonixServer,
                                                      _cfg );
  rayonixMgr.appliance().connect( frmk->inlet() );
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

static RayonixServer* rayonixServer;

int main( int argc, char** argv )
{
  unsigned detid = -1UL;
  unsigned platform = -1UL;
  Arp* arp = 0;
  unsigned verbosity = 0;
  bool helpFlag = false;
  char *endPtr;
  char* uniqueid = (char *)NULL;

  extern char* optarg;
  int c;
  while( ( c = getopt( argc, argv, "i:p:vhu:" ) ) != EOF ) {
    switch(c) {
      case 'i':
        errno = 0;
        endPtr = NULL;
        detid  = strtoul(optarg, &endPtr, 0);
        if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
          printf("Error: failed to parse detector ID\n");
          usage(argv[0]);
          return -1;
        }
        break;
      case 'u':
        if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
          printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
        } else {
          uniqueid = optarg;
        }
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
      }
   }

  if (helpFlag) {
    usage(argv[0]);
    help();
    return 0;
  } else if ((platform == -1UL) || (detid == -1UL)) {
    printf("Error: Platform and detid required\n");
    usage(argv[0]);
    return 0;
  }

  Node node( Level::Source, platform );

  Task* task = new Task( Task::MakeThisATask );

  CfgClientNfs* cfgService;

  DetInfo detInfo( node.pid(),
                   (Pds::DetInfo::Detector) detid,
                   0,
                   DetInfo::Rayonix,
                   0 );

  cfgService = new CfgClientNfs(detInfo);

  rayonixServer = new RayonixServer(detInfo, verbosity);

  MySegWire settings(rayonixServer, uniqueid);

  Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       arp,
                       rayonixServer );

  SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              arp );

  if (seglevel->attach()) {
    printf("entering rayonix task main loop\n");
    task->mainLoop();
    printf("exiting rayonix task main loop\n");
  }

   return 0;
}
