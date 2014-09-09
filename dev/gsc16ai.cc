#include "pdsdata/xtc/DetInfo.hh"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/gsc16ai/Gsc16aiManager.hh"
#include "pds/gsc16ai/Gsc16aiServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <climits>

extern int optind;

static void printUsage(const char* p)
{
  printf("Usage: %s -i <detid> -p <platform>,<mod>,<chan> [OPTIONS]\n",p);
  printf("\n"
         "Options:\n"
         "    -i <detid>                  detector ID (e.g. 22 for XppEndstation)\n"
         "    -p <platform>,<mod>,<chan>  platform number, EVR module, EVR channel\n"
         "    -u <alias>                  set device alias\n"
         "    -h                          print this message and exit\n");
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
   MySegWire(Gsc16aiServer* gsc16aiServer,
             unsigned       module,
             unsigned       channel,
             const char*    aliasName);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

   const std::list<SrcAlias>* pAliases() const
   {
      return (_aliases.size() > 0) ? &_aliases : NULL;
   }
   bool     is_triggered() const { return true; }
   unsigned module      () const { return _module; }
   unsigned channel     () const { return _channel; }

 private:
   Gsc16aiServer* _gsc16aiServer;
   std::list<Src> _sources;
   std::list<SrcAlias>  _aliases;
   unsigned       _module;
   unsigned       _channel;
};

//
//  Implements the callbacks for attaching/dissolving.
//  Appliances can be added to the stream here.
//
class Pds::Seg
   : public EventCallback
{
 public:
   Seg( Task* task,
        unsigned platform,
        CfgClientNfs* cfgService,
        SegWireSettings& settings,
        Arp* arp,
        Gsc16aiServer* gsc16aiServer );

   virtual ~Seg();
    
 private:
   // Implements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task* _task;
   unsigned _platform;
   CfgClientNfs* _cfg;
   Gsc16aiServer* _gsc16aiServer;
};


Pds::MySegWire::MySegWire( Gsc16aiServer* gsc16aiServer,
                           unsigned module,
                           unsigned channel,
                           const char* aliasName) :
     _gsc16aiServer(gsc16aiServer),
     _module   (module),
     _channel  (channel)
{ 
   _sources.push_back(gsc16aiServer->client()); 
   if (aliasName) {
      SrcAlias tmpAlias(gsc16aiServer->client(), aliasName);
      _aliases.push_back(tmpAlias);
   }
}

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _gsc16aiServer->fd() );
   wire.add_input( _gsc16aiServer );
}


Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               Gsc16aiServer* gsc16aiServer )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _gsc16aiServer(gsc16aiServer)
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
   Gsc16aiManager& gsc16aiMgr = * new Gsc16aiManager( _gsc16aiServer,
                                                      _cfg );
   gsc16aiMgr.appliance().connect( frmk->inlet() );
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

int main( int argc, char** argv )
{
   unsigned detid = UINT_MAX;
   unsigned platform = UINT_MAX;
   unsigned module = 0;
   unsigned channel = 0;
   Arp* arp = 0;
   bool helpFlag = false;
   char* uniqueid = (char *)NULL;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "a:i:p:u:h" ) ) != EOF ) {
      switch(c) {
         case 'a':
            arp = new Arp(optarg);
            break;
         case 'i':
            if (!CmdLineTools::parseUInt(optarg,detid)) {
              printf("%s: option `-i' parsing error\n", argv[0]);
              helpFlag = true;
            }
            break;
         case 'p':
            if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3) {
              printf("%s: option `-p' parsing error\n", argv[0]);
              helpFlag = true;
            }
            break;
        case 'u':
            if (!CmdLineTools::parseSrcAlias(optarg)) {
              printf("%s: option `-u' parsing error\n", argv[0]);
              helpFlag = true;
            } else {
              uniqueid = optarg;
            }
            break;
         case 'h':
            printUsage(argv[0]);
            return 0;
         case '?':
         default:
            helpFlag = true;
            break;
      }
   }

  if (platform == UINT_MAX) {
    printf("%s: platform is required\n", argv[0]);
    helpFlag = true;
  }

  if (detid == UINT_MAX) {
    printf("%s: detid is required\n", argv[0]);
    helpFlag = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    helpFlag = true;
  }

  if (helpFlag) {
    printUsage(argv[0]);
    return 1;
  }

   if( arp )
   {
      if( arp->error() )
      {
         printf( "%s failed to create odfArp : %s\n",
                 argv[0], strerror( arp->error() ) );

         delete arp;
         return 0;
      }
   }

   Node node( Level::Source, platform );

   Task* task = new Task( Task::MakeThisATask );
  
   Gsc16aiServer* gsc16aiServer;
   CfgClientNfs* cfgService;

   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detid,
                    0,
                    DetInfo::Gsc16ai,
                    0 );
   cfgService = new CfgClientNfs(detInfo);
   gsc16aiServer = new Gsc16aiServer(detInfo);

   MySegWire settings(gsc16aiServer, module, channel, uniqueid);

   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       arp,
                       gsc16aiServer );

   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              arp );
   (void) seglevel->attach();

   printf("entering gsc16ai task main loop\n");
   task->mainLoop();
   printf("exiting gsc16ai task main loop\n");

   if (arp) delete arp;

   return 0;
}
