#include "pdsdata/xtc/DetInfo.hh"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/imp/ImpManager.hh"
#include "pds/imp/ImpServer.hh"
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
   MySegWire(ImpServer*   impServer,
             unsigned     module,
             unsigned     channel,
             const char*  aliasName);
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

   unsigned max_event_size () const { return 2*1024*1024; }
   unsigned max_event_depth() const { return _max_event_depth; }
   void max_event_depth(unsigned d) { _max_event_depth = d; }
 private:
   ImpServer* _impServer;
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
        ImpServer* impServer,
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
   ImpServer* _impServer;
   unsigned        _pgpcard;
   bool       _failed;
};


Pds::MySegWire::MySegWire( ImpServer* impServer, unsigned module, unsigned channel, const char *aliasName )
   : _impServer(impServer), _module(module), _channel(channel),_max_event_depth(256)
{ 
   _sources.push_back(impServer->client());
   if (aliasName) {
     SrcAlias tmpAlias(impServer->client(), aliasName);
     _aliases.push_back(tmpAlias);
   }
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _impServer->fd() );
   wire.add_input( _impServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::ImpServer* server = Pds::ImpServer::instance();
  psignal( signal, "Signal received by ImpServer");
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
               ImpServer* impServer,
               unsigned pgpcard  )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _impServer(impServer),
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
   ImpManager& impMgr = * new ImpManager( _impServer, _pgpcard );
   impMgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: imp [-h] [-d <detector>] [-i <deviceID>] [-u <alias>] [-e <numb>] [-D <debug>]  [-G] [-P <pgpcardNumb> -p <platform>,<mod>,<chan>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id, EVR module, EVR channel [required]\n"
      "    -d      Set detector type by name                [Default: XcsEndstation]\n"
      "    -i      Set device id                            [Default: 0]\n"
      "    -u      Set device alias                         [Default: none]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "                The format of the index number is a one byte number with the bottom nybble being\n"
      "                the index of the card and the top nybble being a value that depends on the card\n"
      "                in use.  For the G2 or earlier, it is a port mask where one bit is for\n"
      "                each port, but a value of zero maps to 15 for compatibility with unmodified\n"
      "                applications that use the whole card\n"
      "                For a G3 card, the top nybble is the index of the bottom port in use, with the\n"
      "                index of 1 for the first port, i.e. 1,2,3,4,5,6,7 or 8\n"
      "    -G      Use if pgpcard is a G3 card\n"
      "    -e <N>  Set the maximum event depth, default is 256\n"
      "    -D      Set debug value                          [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label ImpL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label ImpServer enable and disable\n"
//      "                bit 08          turn on printing of FE Internal status on\n"
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
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 256;
  char                g3[16]              = {""};
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

   extern char* optarg;
   char* uniqueid = (char *)NULL;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:u:m:e:D:P:G" ) ) != EOF ) {
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
           if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3) {
             printf("%s: option `-p' parsing error\n", argv[0]);
             printUsage(argv[0]);
             return 1;
           } else {
             platformEntered = true;
           }
           break;
         case 'u':
           if (!CmdLineTools::parseSrcAlias(optarg)) {
             printf("%s: option `-u' parsing error\n", argv[0]);
             printUsage(argv[0]);
             return -1;
           } else {
             uniqueid = optarg;
           }
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
           printf("Imp using event depth of  %u\n", eventDepth);
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
   ImpServer* impServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::Imp,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making ImpServer\n");
   impServer = new ImpServer(detInfo, mask);
   printf("setting ImpServer debug level\n");
   impServer->debug(debug);

   printf("MySetWire settings\n");
   MySegWire settings(impServer, module, channel, uniqueid);
   settings.max_event_depth(eventDepth);

   bool G3Flag = strlen(g3) != 0;
   unsigned ports = (pgpcard >> 4) & 0xf;
   char devName[128];
//   printf("%s pgpcard 0x%x, ports %d\n", argv[0], pgpcard, ports);
   char err[128];
   if ((ports == 0) && !G3Flag) {
     ports = 15;
   }
   sprintf(devName, "/dev/pgpcard%s_%u_%u", g3, pgpcard & 0xf, ports);

   int fd = open( devName,  O_RDWR | O_NONBLOCK );
   if (debug & 1) printf("%s using %s\n", argv[0], devName);
   if (fd < 0) {
     sprintf(err, "%s opening %s failed", argv[0], devName);
     perror(err);
     return 1;
   }

   unsigned limit =  4;
   unsigned offset = 0;

   if ( !G3Flag ) {  // G2 or lower
     while ((((ports>>offset) & 1) == 0) && (offset < limit)) {
       offset += 1;
     }
   } else {  // G3 card
     offset = ports -1;
   }

   printf("%s pgpcard opened as fd %d, offset %d, ports %d\n", argv[0], fd, offset, ports);

   Pds::Pgp::Pgp::portOffset(offset);
   impServer->setImp(fd);



   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       impServer,
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
     printf("entering imp task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting imp task main loop\n");
   }
   return 0;
}
