#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/epix100a/Epix100aManager.hh"
#include "pds/epix100a/Epix100aServer.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/pgp/Pgp.hh"

#include <strings.h>
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
   MySegWire(Epix100aServer* epix100aServer);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

   unsigned max_event_size () const { return 4*1024*1024; }
   unsigned max_event_depth() const { return _max_event_depth; }
   void max_event_depth(unsigned d) { _max_event_depth = d; }
 private:
   Epix100aServer* _epix100aServer;
   std::list<Src> _sources;
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
        Epix100aServer* epix100aServer,
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
   Epix100aServer* _epix100aServer;
   unsigned     _pgpcard;
   bool         _failed;
};


Pds::MySegWire::MySegWire( Epix100aServer* epix100aServer )
   : _epix100aServer(epix100aServer), _max_event_depth(128)
{ 
   _sources.push_back(epix100aServer->client());
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _epix100aServer->fd() );
   wire.add_input( _epix100aServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::Epix100aServer* server = Pds::Epix100aServer::instance();
  psignal( signal, "Signal received by Epix100aApplication");
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
               Epix100aServer* epix100aServer,
               unsigned pgpcard  )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _epix100aServer(epix100aServer),
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
   Epix100aManager& epixMgr = * new Epix100aManager( _epix100aServer, _pgpcard );
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
  printf( "Usage: epix100a [-h] [-d <detector>] [-i <deviceID>] [-e <numb>] [-R <bool>] [-m <bool>] [-r <runTimeConfigName>] [-D <debug>] [-P <pgpcardNumb>] [-G] -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: XcsEndstation]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "    -G      Use if pgpcard is a G3 card\n"
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
      "                bit 03          label Epix100aL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label Epix100aServer enable and disable\n"
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
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 128;
  unsigned            resetOnEverConfig   = 0;
  bool                maintainRunTrig     = false;
  char                runTimeConfigname[256] = {""};
  char                g3[16]              = {""};
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:m:e:R:r:D:P:G" ) ) != EOF ) {
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
           platform = strtoul(optarg, NULL, 0);
           platformEntered = true;
           break;
         case 'i':
           deviceId = strtoul(optarg, NULL, 0);
            break;
         case 'P':
           pgpcard = strtoul(optarg, NULL, 0);
           break;
         case 'G':
           strcpy(g3, "G3");
           break;
         case 'e':
           eventDepth = strtoul(optarg, NULL, 0);
           printf("Epix100a using event depth of  %u\n", eventDepth);
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
   Epix100aServer* epix100aServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::Epix100a,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making Epix100aServer\n");
   epix100aServer = new Epix100aServer(detInfo, 0);
   printf("Epix100a will reset on %s configuration\n", resetOnEverConfig ? "every" : "only the first");
   epix100aServer->resetOnEveryConfig(resetOnEverConfig);
   epix100aServer->runTimeConfigName(runTimeConfigname);
   epix100aServer->maintainLostRunTrigger(maintainRunTrig);
   printf("setting Epix100aServer debug level\n");
   epix100aServer->debug(debug);

   printf("MySetWire settings\n");
   MySegWire settings(epix100aServer);
   settings.max_event_depth(eventDepth);

   unsigned ports = (pgpcard >> 4) & 0xf;
   char devName[128];
   char err[128];
   if (ports == 0) {
     ports = 15;
     sprintf(devName, "/dev/pgpcard%s%u", g3, pgpcard);
   } else {
     sprintf(devName, "/dev/pgpcard%s_%u_%u", g3, pgpcard & 0xf, ports);
   }

   int epix100a = open( devName,  O_RDWR );
   if (debug & 1) printf("%s using %s\n", argv[0], devName);
   if (epix100a < 0) {
     sprintf(err, "%s opening %s failed", argv[0], devName);
     perror(err);
     return 1;
   }

   bool G3Flag = strlen(g3) != 0;

   unsigned limit = G3Flag ? 8 : 4;

   unsigned offset = 0;
   while ((((ports>>offset) & 1) == 0) && (offset < limit)) {
     offset += 1;
   }

   if (offset >= limit) {
     printf("%s illegal port mask!! 0x%x\n", argv[0], ports);
     return 1;
   }

   Pds::Pgp::Pgp::portOffset(offset);
   epix100aServer->setEpix100a(epix100a);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       epix100aServer,
                       epix100a );

   printf("making SegmentLevel\n");
   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   printf("attaching seglevel\n");
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering %s task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         argv[0], DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting %s task main loop\n", argv[0]);
   }
   return 0;
}
