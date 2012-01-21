#include "pdsdata/xtc/DetInfo.hh"

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
#include "pdsdata/cspad2x2/ElementV1.hh"

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
   MySegWire(Cspad2x2Server* cspad2x2Server);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

 private:
   Cspad2x2Server* _cspad2x2Server;
   std::list<Src> _sources;
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
        unsigned pgpcard );

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
   unsigned      _pgpcard;
   bool          _failed;
};


Pds::MySegWire::MySegWire( Cspad2x2Server* cspad2x2Server )
   : _cspad2x2Server(cspad2x2Server)
{ 
   _sources.push_back(cspad2x2Server->client());
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
  Pds::Cspad2x2Server* server = Pds::Cspad2x2Server::instance();
  psignal( signal, "Signal received by Cspad2x2Server");
  if (server != 0) {
    printf("server still exists\n");
//    if (myWire != 0) {
//      myWire->remove_input(server);
//    }
//    printf("myWire removed input\nserver ");
    server->ignoreFetch(true);
    server->disable();
    printf("disabled\n");
    server->dumpFrontEnd();
    server->die();
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
               unsigned pgpcard )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _cspad2x2Server(cspad2x2Server),
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
   Cspad2x2Manager& cspad2x2Mgr = * new Cspad2x2Manager( _cspad2x2Server, _pgpcard );
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
  printf( "Usage: cspad2x2 [-h] [-d <detector>] [-i <deviceID>] [-m <configMask>] [-D <debug>] [-P <pgpcardNumb> [-r <runTimeConfigName>] -p <platform>\n"
      "    -h      Show usage\n"
      "    -d      Set detector type by name [Default: XppGon]\n"
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -m      Set config mask           [Default: 0]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "    -D      Set debug value           [Default: 0]\n"
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
      "    -r      set run time config file name\n"
      "    -p      Set platform id           [required]\n"
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::XppGon;
  DetInfo::Device     device              = DetInfo::Cspad2x2;
  TypeId::Type        type                = TypeId::Id_Cspad2x2Element;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  ::signal( SIGINT, sigHandler );
  char                runTimeConfigname[256] = {""};
  bool                platformMissing     = true;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:m:D:xP:r:" ) ) != EOF ) {
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
           platformMissing = false;
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
         case 'D':
           debug = strtoul(optarg, NULL, 0);
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
      printUsage(argv[0]);
      return 0;
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

   MySegWire settings(cspad2x2Server);

   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       cspad2x2Server,
                       pgpcard);

   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering cspad2x2 task main loop, \n\tDetector: %s\n\tDevice: %s\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), DetInfo::name(device), platform);
     task->mainLoop();
     printf("exiting cspad2x2 task main loop\n");
   }
   return 0;
}
