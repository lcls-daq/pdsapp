#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/phasics/PhasicsManager.hh"
#include "pds/phasics/PhasicsServer.hh"
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
   MySegWire(PhasicsServer* phasicsServer);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

 private:
   PhasicsServer* _phasicsServer;
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
        PhasicsServer* phasicsServer);

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
   PhasicsServer* _phasicsServer;
   bool         _failed;
};


Pds::MySegWire::MySegWire( PhasicsServer* phasicsServer )
   : _phasicsServer(phasicsServer)
{ 
   _sources.push_back(phasicsServer->client());
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _phasicsServer->fd() );
   wire.add_input( _phasicsServer );
   myWire = &wire;
}

Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               PhasicsServer* phasicsServer)
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _phasicsServer(phasicsServer),
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
   PhasicsManager& phasicsMgr = * new PhasicsManager( _phasicsServer );
   phasicsMgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: %s [-h] [-d <detector>] [-i <deviceID>] [-D <debug>] -p <platform>\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: XcsEndstation]\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -f      Set server to drop 1st img[Default 1]\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offset and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label PhasicsL1Action::fire\n"
      "                bit 05          label PhasicsServer enable and disable\n"
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
      , s
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::XppEndstation;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  bool                platformEntered     = false;
  unsigned            debug               = 0;
  bool                dropFirst           = true;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:f:D:" ) ) != EOF ) {
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
         case 'f':
           dropFirst = 0 != strtoul(optarg, NULL, 0);
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
   PhasicsServer* phasicsServer;
   printf("making client\n");
   CfgClientNfs* cfgService;

   printf("making detinfo\n");
   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    DetInfo::Phasics,
                    deviceId );
   printf("making cfgService\n");
   cfgService = new CfgClientNfs(detInfo);
   printf("making PhasicsServer\n");
   phasicsServer = new PhasicsServer(detInfo);
   printf("setting PhasicsServer debug level 0x%x\n", debug);
   phasicsServer->debug(debug);
   printf("Setting dropFirst flag to %s\n", dropFirst ? "true" : "false");
   phasicsServer->dropTheFirst(dropFirst);

   printf("MySegWire settings\n");
   MySegWire settings(phasicsServer);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       phasicsServer );

   printf("making SegmentLevel\n");
   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   printf("attaching seglevel\n");
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering phasics task main loop, \n\tDetector: %s\n\tDeviceId: %d\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), deviceId, platform);
     task->mainLoop();
     printf("exiting phasics task main loop\n");
   }
   return 0;
}
