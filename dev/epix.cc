#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/epix/EpixManager.hh"
#include "pds/epix/EpixServer.hh"
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
   MySegWire(EpixServer* epixServer);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

   unsigned max_event_size () const { return 4*1024*1024; }
   unsigned max_event_depth() const { return _max_event_depth; }
   void max_event_depth(unsigned d) { _max_event_depth = d; }
 private:
   EpixServer* _epixServer;
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
        EpixServer* epixServer,
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
   EpixServer* _epixServer;
   unsigned     _pgpcard;
   bool         _failed;
};


Pds::MySegWire::MySegWire( EpixServer* epixServer )
   : _epixServer(epixServer), _max_event_depth(128)
{ 
   _sources.push_back(epixServer->client());
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _epixServer->fd() );
   wire.add_input( _epixServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::EpixServer* server = Pds::EpixServer::instance();
  psignal( signal, "Signal received by EpixApplication");
  if (server != 0) {
    server->disable();
    if (myWire != 0) {
      myWire->remove_input(server);
    }
    if (server != 0) server->dumpFrontEnd(); else printf("\tsigHandler found nil server 2!\n");
    if (server != 0) server->die(); else printf("\tsigHandler found nil server 3!\n");
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
               EpixServer* epixServer,
               unsigned pgpcard  )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _epixServer(epixServer),
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
   EpixManager& epixMgr = * new EpixManager( _epixServer, _pgpcard );
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
  printf( "Usage: epix [-h] [-d <detector>] [-i <deviceID>] [-e <numb>] [-R <bool>] [-r <runTimeConfigName>] [-D <debug>] [-P <pgpcardNumb> -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: XcsEndstation]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "    -e <N>  Set the maximum event depth, default is 128\n"
      "    -R <B>  Set flag to reset on every config or just the first if false\n"
      "    -r      set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label EpixL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label EpixServer enable and disable\n"
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
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 128;
  unsigned            resetOnEverConfig   = 0;
  char                runTimeConfigname[256] = {""};
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:m:e:R:r:D:P:" ) ) != EOF ) {
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
         case 'm':
           mask = strtoul(optarg, NULL, 0);
           break;
         case 'P':
           pgpcard = strtoul(optarg, NULL, 0);
           break;
         case 'e':
           eventDepth = strtoul(optarg, NULL, 0);
           printf("Epix using event depth of  %u\n", eventDepth);
           break;
         case 'R':
           resetOnEverConfig = strtoul(optarg, NULL, 0);
           break;
         case 'r':
           strcpy(runTimeConfigname, optarg);
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
   EpixServer* epixServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::Epix,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making EpixServer\n");
   epixServer = new EpixServer(detInfo, mask);
   printf("Epix will reset on %s configuration\n", resetOnEverConfig ? "every" : "only the first");
   epixServer->resetOnEveryConfig(resetOnEverConfig);
   epixServer->runTimeConfigName(runTimeConfigname);
   printf("setting EpixServer debug level\n");
   epixServer->debug(debug);

   printf("MySetWire settings\n");
   MySegWire settings(epixServer);
   settings.max_event_depth(eventDepth);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       epixServer,
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
     printf("entering epix task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting epix task main loop\n");
   }
   return 0;
}
