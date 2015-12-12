#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/epixSampler/EpixSamplerManager.hh"
#include "pds/epixSampler/EpixSamplerServer.hh"
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
  MySegWire(EpixSamplerServer* epixSamplerServer, unsigned module, unsigned channel, const char* id);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }
   const std::list<SrcAlias>* pAliases() const
   { return (_aliases.size() > 0) ? &_aliases : NULL; }

   bool is_triggered() const { return true; }
   unsigned module      () const { return _module; }
   unsigned channel     () const { return _channel; }

   unsigned max_event_size () const { return 2*1024*1024; }
   unsigned max_event_depth() const { return _max_event_depth; }
   void max_event_depth(unsigned d) { _max_event_depth = d; }
 private:
   EpixSamplerServer* _epixSamplerServer;
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
        EpixSamplerServer* epixSamplerServer,
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
   EpixSamplerServer* _epixSamplerServer;
   unsigned     _pgpcard;
   bool         _failed;
};


Pds::MySegWire::MySegWire( EpixSamplerServer* epixSamplerServer,
                           unsigned        module,
                           unsigned        channel,
                           const char*     aliasName )
   : _epixSamplerServer(epixSamplerServer), 
     _module         (module),
     _channel        (channel),
     _max_event_depth(256)
{ 
   _sources.push_back(epixSamplerServer->client());
   if (aliasName) {
     SrcAlias tmpAlias(epixSamplerServer->client(), aliasName);
     _aliases.push_back(tmpAlias);
   }
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _epixSamplerServer->fd() );
   wire.add_input( _epixSamplerServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::EpixSamplerServer* server = Pds::EpixSamplerServer::instance();
  psignal( signal, "Signal received by EpixSamplerApplication");
  if (server != 0) {
    if (myWire != 0) {
      myWire->remove_input(server);
    }
    if (server != 0) server->disable();
    if (server != 0) server->dumpFrontEnd();
    if (server != 0) server->die();
  }
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}

Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               EpixSamplerServer* epixSamplerServer,
               unsigned pgpcard  )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _epixSamplerServer(epixSamplerServer),
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
   EpixSamplerManager& epixSamplerMgr = * new EpixSamplerManager( _epixSamplerServer, _pgpcard );
   epixSamplerMgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: epixsampler [-h] [-d <detector>] [-i <deviceID>] [-e <numb>] [-D <debug>] [-P <pgpcardNumb> -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: CxiDsu]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "    -e <N>  Set the maximum event depth, default is 256\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offset and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label EpixSamplerL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label EpixSamplerServer enable and disable\n"
      "                bit 08          turn on printing of FE Internal status on\n"
      "                "
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::CxiDsu;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  bool                platformEntered     = false;
  unsigned            module              = 0;
  unsigned            channel             = 0;
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 256;
  ::signal( SIGINT, sigHandler );

   extern char* optarg;
   char* uniqueid = (char *)NULL;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:e:D:P:u:" ) ) != EOF ) {
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
           printf("EpixSampler using event depth of  %u\n", eventDepth);
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
   EpixSamplerServer* epixSamplerServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::EpixSampler,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making EpixSamplerServer\n");
   epixSamplerServer = new EpixSamplerServer(detInfo, mask);
   printf("setting EpixSamplerServer debug level\n");
   epixSamplerServer->debug(debug);

   printf("MySetWire settings\n");
   MySegWire settings(epixSamplerServer, module, channel, uniqueid);
   settings.max_event_depth(eventDepth);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       epixSamplerServer,
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
     printf("entering epixsampler task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting epixsampler task main loop\n");
   }
   return 0;
}
