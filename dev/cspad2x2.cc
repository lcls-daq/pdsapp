#include "pdsdata/xtc/DetInfo.hh"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/cspad2x2/Cspad2x2Manager.hh"
#include "pds/cspad2x2/Cspad2x2Server.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/psddl/cspad2x2.ddl.h"

#include "pds/client/FrameCompApp.hh"

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
   MySegWire(Cspad2x2Server* cspad2x2Server,
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
   Cspad2x2Server* _cspad2x2Server;
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
        Cspad2x2Server* cspad2x2Server,
        bool compress = false );

   virtual ~Seg();
   bool didYouFail() { return _failed; }
    
 private:
   // Implements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task*         _task;
   unsigned      _platform;
   CfgClientNfs* _cfg;
   Cspad2x2Server*  _cspad2x2Server;
   bool          _compress;
   bool          _failed;
};


Pds::MySegWire::MySegWire( Cspad2x2Server* cspad2x2Server, unsigned module, unsigned channel, const char *aliasName )
   : _cspad2x2Server(cspad2x2Server), _module(module), _channel(channel), _max_event_depth(256)
{ 
   _sources.push_back(cspad2x2Server->client());
   if (aliasName) {
     SrcAlias tmpAlias(cspad2x2Server->client(), aliasName);
     _aliases.push_back(tmpAlias);
   }
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _cspad2x2Server->fd() );
   wire.add_input( _cspad2x2Server );
   myWire = &wire;
}

void sigHandler( int signal ) {
  psignal( signal, "Signal received by Cspad2x2 segment level");
  Pds::Cspad2x2Server* server = Pds::Cspad2x2Server::instance();
  if (server != 0) {
    server->disable();
    if (myWire != 0) {
      myWire->remove_input(server);
    } else printf("\tmyWire is gone!\n");
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
               Cspad2x2Server* cspad2x2Server,
               bool compress )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _cspad2x2Server(cspad2x2Server),
     _compress(compress),
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
   Cspad2x2Manager& cspad2x2Mgr = * new Cspad2x2Manager( _cspad2x2Server);
   if (_compress) (new FrameCompApp(0x80000))->connect( frmk->inlet() );
   cspad2x2Mgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: cspad2x2 [-h] [-d <detector>] [-i <deviceID>] [-m <configMask>] [-e <numb>] [-C <nevents>] [-u <alias>] [-D <debug>] [-P <pgpcardNumb>] [-r <runTimeConfigName>] [-R <runTriggerFactor>] [-G] -p <platform>,<mod>,<chan>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id, EVR module, EVR channel [required]\n"
      "    -d      Set detector type by name                [Default: XppGon]\n"
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
      "    -i      Set device id                            [Default: 0]\n"
      "    -m      Set config mask                          [Default: 0]\n"
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
      "    -C <N>  Compress and copy every Nth event\n"
      "    -u      Set device alias                         [Default: none]\n"
      "    -D      Set debug value                          [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label Cspad2x2L1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label Cspad2x2Server enable and disable\n"
      "                bit 06          print out (acqcount - frameNumber) for every L1Accept\n"
      "                bit 08          turn on printing of FE concentrator status on\n"
      "                bit 09          turn on printing of FE quad status\n"
      "                bit 10          print out time dumping front end took\n"
      "                bit 11          print out the front end stat in end calib instead of unconfig\n"
      "                bit 16          suppress the checking of off by one\n"
      "    -R      set run trigger rate  (120, 60, 30Hz ...\n"
      "    -r      set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n"
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::XppGon;
  DetInfo::Device     device              = DetInfo::Cspad2x2;
  TypeId::Type        type                = TypeId::Id_Cspad2x2Element;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  unsigned            module              = 0;
  unsigned            channel             = 0;
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 256;
  unsigned            runTriggerFactor    = 1;
  ::signal( SIGINT, sigHandler );
  char                runTimeConfigname[256] = {""};
  char                g3[16]              = {""};
  bool                platformMissing     = true;
  bool                compressFlag        = false;
  bool                bUsage              = false;
  bool                useAesDriver        = false;
  unsigned            uu1;

   extern char* optarg;
   char* uniqueid = (char *)NULL;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:m:C:e:D:xP:u:r:R:G" ) ) != EOF ) {
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
             bUsage = true;
           } else {
             platformMissing = false;
           }
           break;
         case 'i':
           if (!CmdLineTools::parseInt(optarg,deviceId)) {
             printf("%s: option `-i' parsing error\n", argv[0]);
             bUsage = true;
           }
           break;
         case 'm':
           if (!CmdLineTools::parseUInt(optarg,mask)) {
             printf("%s: option `-m' parsing error\n", argv[0]);
             bUsage = true;
           }
           break;
         case 'P':
           if (!CmdLineTools::parseUInt(optarg,pgpcard)) {
             printf("%s: option `-P' parsing error\n", argv[0]);
             bUsage = true;
           } else {
             printf("Cspad2x2 using pgpcard 0x%x\n", pgpcard);
           }
           break;
         case 'u':
           if (!CmdLineTools::parseSrcAlias(optarg)) {
             printf("%s: option `-u' parsing error\n", argv[0]);
             bUsage = true;
           } else {
             uniqueid = optarg;
           }
           break;
         case 'e':
           if (!CmdLineTools::parseUInt(optarg,eventDepth)) {
             printf("%s: option `-e' parsing error\n", argv[0]);
             bUsage = true;
           } else {
             printf("Cspad2x2 using event depth of  %u\n", eventDepth);
           }
           break;
         case 'C':
           compressFlag = 1;
           if (!CmdLineTools::parseUInt(optarg,uu1)) {
             printf("%s: option `-C' parsing error\n", argv[0]);
             bUsage = true;
           } else {
             FrameCompApp::setCopyPresample(uu1);
           }
           break;
         case 'D':
           if (!CmdLineTools::parseUInt(optarg,debug)) {
             printf("%s: option `-D' parsing error\n", argv[0]);
             bUsage = true;
           } else {
             printf("Cspad2x2 using debug value of 0x%x\n", debug);
           }
           break;
         case 'R':
           if ((!CmdLineTools::parseUInt(optarg,uu1)) || (uu1 < 1) || (uu1 > 120)) {
             printf("%s: option `-R' parsing error\n", argv[0]);
             bUsage = true;
           } else {
             runTriggerFactor = 120 / uu1;
             printf("Cspad2x2 using run trigger rate of %u Hz\n", 120 / runTriggerFactor);
           }
           break;
         case 'G':
           strcpy(g3, "G3");
           break;
         case 'r':
           strcpy(runTimeConfigname, optarg);
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

   if( platformMissing ) {
      printf( "Error: Platform required\n" );
      bUsage = true;
   }

   if (optind < argc) {
      printf( "Error: invalid argument -- %s\n", argv[optind] );
      bUsage = true;
   }

   if (bUsage) {
      printUsage(argv[0]);
      return 1;
   }

   Node node( Level::Source, platform );

   Task* task = new Task( Task::MakeThisATask );
  
   Cspad2x2Server* cspad2x2Server;
   CfgClientNfs* cfgService;

   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    device,
                    deviceId );

   TypeId typeId( type, Pds::CsPad2x2::ElementV1::Version );

   cfgService = new CfgClientNfs(detInfo);
   cspad2x2Server = new Cspad2x2Server(detInfo, typeId, mask);
   cspad2x2Server->debug(debug);
   cspad2x2Server->runTimeConfigName(runTimeConfigname);
   cspad2x2Server->runTrigFactor(runTriggerFactor);

   bool G3Flag = strlen(g3) != 0;
   unsigned card = pgpcard & 0xf;
   unsigned ports = (pgpcard >> 4) & 0xf;
   char devName[128];
   printf("%s pgpcard 0x%x, ports %d\n", argv[0], pgpcard, ports);
   char err[128];
   if ((ports == 0) && !G3Flag) {
     ports = 15;
   }

   // Check what driver the card is using
   sprintf(devName, "/dev/pgpcard%s_%u_%u", g3, card, ports);
   if ( access( devName, F_OK ) != -1 ) {
     useAesDriver = false;
     if (debug & 1) printf("%s found %s - using legacy pgpcard driver\n", argv[0], devName);
   } else {
     sprintf(devName, "/dev/pgpcard_%u", card);
     if ( access( devName, F_OK ) != -1 ) {
       useAesDriver = true;
       if (debug & 1) printf("%s found %s - using aes pgpcard driver\n", argv[0], devName);
     } else {
       fprintf(stderr, "%s unable to deterime pgpcard driver type", argv[0]);
       return 1;
     }
   }

   int fd = open( devName,  O_RDWR | O_NONBLOCK );
   if (debug & 1) printf("%s using %s\n", argv[0], devName);
   if (fd < 0) {
     sprintf(err, "%s opening %s failed", argv[0], devName);
     perror(err);
     return 1;
   }

   unsigned limit =  4;
   unsigned offset = 0;

   if ( !G3Flag && !useAesDriver ) { // G2 or lower
     while ((((ports>>offset) & 1) == 0) && (offset < limit)) {
       offset += 1;
     }
   } else {  // G3 card
     offset = ports -1;
   }

   printf("%s pgpcard opened as fd %d, offset %d, ports %d\n", argv[0], fd, offset, ports);

   Pds::Pgp::Pgp::portOffset(offset);
   cspad2x2Server->setCspad2x2(fd, useAesDriver);

   MySegWire settings(cspad2x2Server, module, channel, uniqueid);
   settings.max_event_depth(eventDepth);

   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       cspad2x2Server,
                       compressFlag);

   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering %s task main loop, \n\tDetector: %s\n\tDevice: %s\n\tPlatform: %u\n",
         argv[0], DetInfo::name((DetInfo::Detector)detector), DetInfo::name(device), platform);
     task->mainLoop();
     printf("exiting %s task main loop\n", argv[0]);
   }
   return 0;
}
