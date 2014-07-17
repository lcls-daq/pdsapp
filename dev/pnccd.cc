#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/pnccd/pnCCDManager.hh"
#include "pds/pnccd/pnCCDServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string>
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
   MySegWire(pnCCDServer* pnccdServer, const char *aliasName);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

   const std::list<SrcAlias>* pAliases() const
   {
      return (_aliases.size() > 0) ? &_aliases : NULL;
   }

   unsigned max_event_size () const { return 8*1024*1024; }
   unsigned max_event_depth() const { return _max_event_depth; }
   void max_event_depth(unsigned d) { _max_event_depth = d; }
 private:
   pnCCDServer* _pnccdServer;
   std::list<Src> _sources;
   std::list<SrcAlias> _aliases;
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
        pnCCDServer* pnccdServer,
        unsigned pgpcard,
        std::string sConfigFile);

   virtual ~Seg();
   bool didYouFail() { return _failed; }
    
 private:
   // pnCCDlements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task* _task;
   unsigned _platform;
   CfgClientNfs* _cfg;
   pnCCDServer* _pnccdServer;
   unsigned     _pgpcard;
   std::string  _sConfigFile;
   bool         _failed;
};


Pds::MySegWire::MySegWire( pnCCDServer* pnccdServer, const char *aliasName )
   : _pnccdServer(pnccdServer), _max_event_depth(64)
{ 
   _sources.push_back(pnccdServer->client());
   if (aliasName) {
      SrcAlias tmpAlias(pnccdServer->client(), aliasName);
      _aliases.push_back(tmpAlias);
   }
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _pnccdServer->fd() );
   wire.add_input( _pnccdServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::pnCCDServer* server = Pds::pnCCDServer::instance();
  psignal( signal, "Signal received by pnCCDServer");
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
               pnCCDServer* pnccdServer,
               unsigned pgpcard,
               std::string sConfigFile )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _pnccdServer(pnccdServer),
     _pgpcard(pgpcard),
     _sConfigFile(sConfigFile),
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
   pnCCDManager& pnccdMgr = * new pnCCDManager( _pnccdServer, _pgpcard, _sConfigFile );
   pnccdMgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: pnccd [-h] -p <platform> -f <cnfgFileName> -P <pgpcardNumb> [-d <detector>] [-i <deviceID>] [-e <numb>] [-D <debug>]\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -f      Set the config file name  [required]\n"
      "    -P      Set pgpcard index number  [required]\n"
      "    -d      Set detector type by name [Default: XcsEndstation]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -e <N>  Set the maximum event depth, default is 64\n"
      "    -u      Set device alias          [Default: none]\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offset and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label pnCCDL1Action::fire\n"
      "                bit 04          print out pnCCDcard state after config or record\n"
      "                bit 05          label pnCCDServer enable and disable\n"
      "                "
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it will list them\n"
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::XppEndstation;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  bool                platformEntered     = false;
  bool                pgpcardEntered      = false;
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 64;
  char*               uniqueid            = (char *)NULL;
  std::string         sConfigFile;
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:u:m:e:D:P:f:" ) ) != EOF ) {
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
        case 'u':
            if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
              printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
            } else {
              uniqueid = optarg;
            }
            break;
         case 'f':
           sConfigFile = optarg;
           break;
         case 'i':
           deviceId = strtoul(optarg, NULL, 0);
            break;
         case 'P':
           pgpcard = strtoul(optarg, NULL, 0);
           pgpcardEntered = true;
           break;
         case 'e':
           eventDepth = strtoul(optarg, NULL, 0);
           printf("pnCCD using event depth of  %u\n", eventDepth);
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

   if ( sConfigFile.empty() )
   {
     printf( "Error: pnCCD Config File is required\n" );
     printUsage(argv[0]);
     return 0;
   }

   if ( !pgpcardEntered )
   {
     printf("Error: pgpcard parameter required\n");
     printUsage(argv[0]);
     return 0;
   }
   printf("Node node\n");
   Node node( Level::Source, platform );

   printf("making task\n");
   Task* task = new Task( Task::MakeThisATask );
   pnCCDServer* pnccdServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::pnCCD,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making pnCCDServer\n");
   pnccdServer = new pnCCDServer(detInfo, mask);
   printf("setting pnCCDServer debug level\n");
   pnccdServer->debug(debug);

   printf("MySegWire settings\n");
   MySegWire settings(pnccdServer, uniqueid);
   settings.max_event_depth(eventDepth);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       pnccdServer,
                       pgpcard,
                       sConfigFile );

   printf("making SegmentLevel\n");
   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   printf("attaching seglevel\n");
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering pnccd task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting pnccd task main loop\n");
   }
   return 0;
}
