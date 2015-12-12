#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/epix10k/Epix10kManager.hh"
#include "pds/epix10k/Epix10kServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <new>

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
  MySegWire(Epix10kServer* epix10kServer,
            unsigned channel,
            unsigned module,
            const char* id);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }
   const std::list<SrcAlias>* pAliases() const
   { return (_aliases.size() > 0) ? &_aliases : NULL; }

   bool     is_triggered() const { return true; }
   unsigned module      () const { return _module; }
   unsigned channel     () const { return _channel; }

   unsigned max_event_size () const { return 4*1024*1024; }
   unsigned max_event_depth() const { return _max_event_depth; }
   void max_event_depth(unsigned d) { _max_event_depth = d; }
 private:
   Epix10kServer* _epix10kServer;
   std::list<Src> _sources;
   std::list<SrcAlias> _aliases;
   unsigned _module;
   unsigned _channel;
   unsigned _max_event_depth;
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
        Epix10kServer* epix10kServer,
        unsigned pgpcard );

   virtual ~Seg();
   bool didYouFail() { return _failed; }
    
 private:
   // Implements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task* _task;
   unsigned _platform;
   CfgClientNfs* _cfg;
   Epix10kServer* _epix10kServer;
   unsigned     _pgpcard;
   bool         _failed;
};


Pds::MySegWire::MySegWire( Epix10kServer* epix10kServer,
                           unsigned        module,
                           unsigned        channel,
                           const char*     aliasName )
   : _epix10kServer(epix10kServer), 
     _module         (module),
     _channel        (channel),
     _max_event_depth(128)
{ 
   _sources.push_back(epix10kServer->client());
   if (aliasName) {
     SrcAlias tmpAlias(epix10kServer->client(), aliasName);
     _aliases.push_back(tmpAlias);
   }
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _epix10kServer->fd() );
   wire.add_input( _epix10kServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::Epix10kServer* server = Pds::Epix10kServer::instance();
  psignal( signal, "Signal received by Epix10kApplication");
  if (server != 0) {
    server->disable();
    if (myWire != 0) {
      myWire->remove_input(server);
    }
    if (server != 0) server->dumpFrontEnd();
    if (server != 0) server->die();
  } else {
    printf("sigHandler found nil server 1!\n");
  }
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}

Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               Epix10kServer* epix10kServer,
               unsigned pgpcard  )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _epix10kServer(epix10kServer),
     _pgpcard(pgpcard),
     _failed(false)
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
   Epix10kManager& epixMgr = * new Epix10kManager( _epix10kServer, _pgpcard );
   epixMgr.appliance().connect( frmk->inlet() );
}

void Pds::Seg::failed( Reason reason )
{
   static const char* reasonname[] = { "platform unavailable", 
                                       "crates unavailable", 
                                       "fcpm unavailable" };
   printf( "Seg: unable to allocate crates on platform 0x%x : %s\n", 
           _platform, reasonname[reason]);
   _failed = true;
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

void printUsage(char* s) {
  printf( "Usage: epix10k [-h] [-d <detector>] [-i <deviceID>] [-e <numb>] [-R <bool>] [-m <bool>] [-r <runTimeConfigName>] [-D <debug>] [-P <pgpcardNumb> -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: XcsEndstation]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "    -e <N>  Set the maximum event depth, default is 128\n"
      "    -R <B>  Set flag to reset on every config or just the first if false\n"
      "    -m <B>  Set flag to maintain or not maintain lost run triggers (turn off for slow running\n"
      "    -r      set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label Epix10kL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label Epix10kServer enable and disable\n"
      "                bit 08          turn on printing of FE Internal status on\n"
      "                "
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::XppEndstation;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  bool                platformEntered     = false;
  unsigned            module              = 0;
  unsigned            channel             = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 128;
  unsigned            resetOnEverConfig   = 0;
  bool                maintainRunTrig     = false;
  char                runTimeConfigname[256] = {""};
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

   extern char* optarg;
   char* uniqueid = (char *)NULL;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:m:e:R:r:D:P:u:" ) ) != EOF ) {
     bool     found;
     unsigned index;
     switch(c) {
         case 'd':
           found = false;
           for (index=0; !found && index<DetInfo::NumDetector; index++) {
             if (!strcmp(optarg, DetInfo::name((DetInfo::Detector)index))) {
               detector = (DetInfo::Detector)index;
               found = true;
             }
           }
           if (!found) {
             printf("Bad Detector name: %s\n  Detector Names are:\n", optarg);
             for (index=0; index<DetInfo::NumDetector; index++) {
               printf("\t%s\n", DetInfo::name((DetInfo::Detector)index));
             }
             printUsage(argv[0]);
             return 0;
           }
           break;
         case 'p':
           if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3)
             printf("%s: option `-p' parsing error\n", argv[0]);
           else
             platformEntered = true;
           break;
         case 'i':
           deviceId = strtoul(optarg, NULL, 0);
            break;
         case 'P':
           pgpcard = strtoul(optarg, NULL, 0);
           break;
         case 'e':
           eventDepth = strtoul(optarg, NULL, 0);
           printf("Epix10k using event depth of  %u\n", eventDepth);
           break;
         case 'R':
           resetOnEverConfig = strtoul(optarg, NULL, 0);
           break;
         case 'r':
           strcpy(runTimeConfigname, optarg);
           break;
         case 'm':
           maintainRunTrig = strtoul(optarg, NULL, 0);
           printf("Setting maintain run trigger to %s\n", maintainRunTrig ? "true" : "false");
           break;
         case 'D':
           debug = strtoul(optarg, NULL, 0);
           break;
         case 'h':
           printUsage(argv[0]);
           return 0;
           break;
          case 'u':
           if (!CmdLineTools::parseSrcAlias(optarg)) {
             printf("%s: option `-u' parsing error\n", argv[0]);
           } else {
             uniqueid = optarg;
           }
           break;
        default:
           printf("Error: Option could not be parsed!\n");
           printUsage(argv[0]);
           return 0;
           break;
      }
   }

   if( !platformEntered ) {
      printf( "Error: Platform required\n" );
      printUsage(argv[0]);
      return 0;
   }

   printf("Node node\n");
   Node node( Level::Source, platform );

   printf("making task\n");
   Task* task = new Task( Task::MakeThisATask );
   Epix10kServer* epix10kServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::Epix10k,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making Epix10kServer\n");
   epix10kServer = new Epix10kServer(detInfo, 0);
   printf("Epix10k will reset on %s configuration\n", resetOnEverConfig ? "every" : "only the first");
   epix10kServer->resetOnEveryConfig(resetOnEverConfig);
   epix10kServer->runTimeConfigName(runTimeConfigname);
   epix10kServer->maintainLostRunTrigger(maintainRunTrig);
   printf("setting Epix10kServer debug level\n");
   epix10kServer->debug(debug);

   printf("MySetWire settings\n");
   MySegWire settings(epix10kServer, module, channel, uniqueid);
   settings.max_event_depth(eventDepth);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       epix10kServer,
                       pgpcard );

   printf("making SegmentLevel\n");
   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   printf("attaching seglevel\n");
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering epix10k task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting epix10k task main loop\n");
   }
   return 0;
}
