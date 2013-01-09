// timepix.cc
// Author: Chris Ford <caf@slac.stanford.edu>

#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/timepix/TimepixManager.hh"
#include "pds/timepix/TimepixServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "mpxfilelogger.h"

static void usage(const char *p)
{
  printf("Usage: %s -i <detid> -p <platform> [-m <module id>] [-v]\n"
         "                   [-d <debug flags>] [-T <threshold file>] [-o <logpath>]\n"
         "                   [-a <cpu0>,<cpu1>] [-h]\n", p);
}

static void help()
{
  printf("Options:\n"
         "  -i <detid>            detector ID\n"
         "  -p <platform>         platform number\n"
         "  -m <module id>        module ID (default=0)\n"
         "  -v                    increase verbosity (may be repeated)\n"
         "  -d <debug flags>      debug flags (default=0)\n"
         "  -T <threshold file>   binary pixel configuration (bpc) file (default=none)\n"
         "  -o <logpath>          Timepix logging path (default=/tmp)\n"
         "  -a <cpu0>,<cpu1>      affinity cpu IDs (default=none)\n"
         "  -h                    help: print this message and exit\n");
  printf("Debug flags:\n"
         "  0x0010                ignore hardware frame counter\n");
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
   MySegWire(TimepixServer* timepixServer);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

 private:
   TimepixServer* _timepixServer;
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
        TimepixServer* timepixServer );

   virtual ~Seg();
    
 private:
   // Implements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task* _task;
   unsigned _platform;
   CfgClientNfs* _cfg;
   TimepixServer* _timepixServer;
};


Pds::MySegWire::MySegWire( TimepixServer* timepixServer )
   : _timepixServer(timepixServer)
{ 
   _sources.push_back(timepixServer->client()); 
}

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _timepixServer->fd() );
   wire.add_input( _timepixServer );
}


Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               TimepixServer* timepixServer )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _timepixServer(timepixServer)
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
   TimepixManager& timepixMgr = * new TimepixManager( _timepixServer,
                                                      _cfg );
   timepixMgr.appliance().connect( frmk->inlet() );
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

static TimepixServer* timepixServer;

void timepixSignalIntHandler( int iSignalNo )
{
  printf( "\n%s: signal %d received. Stopping all activities\n", __FUNCTION__, iSignalNo );
  // iSignalCaught = 1;

  if (timepixServer) {
    timepixServer->shutdown();
    sleep(1);
  }
}

int main( int argc, char** argv )
{
   unsigned detid = -1UL;
   unsigned platform = -1UL;
   Arp* arp = 0;
  unsigned moduleId = 0;
  unsigned verbosity = 0;
  unsigned debug = 0;
  bool helpFlag = false;
  char *threshFileName = NULL;
  char *imageFileName = NULL;
  int cpu0 = -1;
  int cpu1 = -1;
  char *logpath = NULL;
  char *pComma = NULL;
  char *endPtr;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "i:p:m:vd:T:o:I:a:h" ) ) != EOF ) {
      switch(c) {
         case 'i':
            errno = 0;
            endPtr = NULL;
            detid  = strtoul(optarg, &endPtr, 0);
            if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
              printf("Error: failed to parse detector ID\n");
              usage(argv[0]);
              return -1;
            }
            break;
         case 'm':
            errno = 0;
            endPtr = NULL;
            moduleId  = strtoul(optarg, &endPtr, 0);
            if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
              printf("Error: failed to parse module ID\n");
              usage(argv[0]);
              return -1;
            }
            break;
         case 'd':
            errno = 0;
            endPtr = NULL;
            debug  = strtoul(optarg, &endPtr, 0);
            if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
              printf("Error: failed to parse debug flags\n");
              usage(argv[0]);
              return -1;
            }
            break;
         case 'h':
            helpFlag = true;
            break;
         case 'a':
            // cpu affinity
            errno = 0;
            endPtr = NULL;
            cpu0  = strtol(optarg, &endPtr, 0);
            if (errno || (endPtr == NULL) || (*endPtr != ',')) {
              printf("Error: failed to parse affinity cpu IDs\n");
              usage(argv[0]);
              return -1;
            }
            pComma = strchr(optarg, ',');
            if (pComma) {
              errno = 0;
              endPtr = NULL;
              cpu1  = strtol(pComma+1, &endPtr, 0);
              if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
                printf("Error: failed to parse affinity cpu IDs\n");
                usage(argv[0]);
                return -1;
              }
            }
            break;
         case 'v':
            ++verbosity;
            break;
         case 'p':
            errno = 0;
            endPtr = NULL;
            platform = strtoul(optarg, &endPtr, 0);
            if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
              printf("Error: failed to parse platform number\n");
              usage(argv[0]);
              return -1;
            }
            break;
         case 'T':
            threshFileName = optarg;
            break;
         case 'I':
            // test image file
            imageFileName = optarg;
            break;
         case 'o':
            logpath = optarg;
            break;
      }
   }

  if (helpFlag) {
    usage(argv[0]);
    help();
    return 0;
  } else if ((platform == -1UL) || (detid == -1UL)) {
    printf("Error: Platform and detid required\n");
    usage(argv[0]);
    return 0;
  }

  // default logging dictory for Timepix is /tmp
  if (setenv(MPX_LOG_DIR_ENV_NAME, logpath ? logpath : "/tmp", 0)) {
    perror("setenv");
  }

  // Register signal handler
  struct sigaction sigActionSettings;
  sigemptyset(&sigActionSettings.sa_mask);
  sigActionSettings.sa_handler = timepixSignalIntHandler;
  sigActionSettings.sa_flags   = SA_RESTART;

  if (sigaction(SIGINT, &sigActionSettings, 0) != 0 )
    printf( "main(): Cannot register signal handler for SIGINT\n" );
  if (sigaction(SIGTERM, &sigActionSettings, 0) != 0 )
    printf( "main(): Cannot register signal handler for SIGTERM\n" );

   Node node( Level::Source, platform );

   Task* task = new Task( Task::MakeThisATask );
  
   CfgClientNfs* cfgService;

   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detid,
                    0,
                    DetInfo::Timepix,
                    0 );

  cfgService = new CfgClientNfs(detInfo);

  timepixServer = new TimepixServer(detInfo, moduleId, verbosity, debug, threshFileName, imageFileName, cpu0, cpu1);

  MySegWire settings(timepixServer);

  Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       arp,
                       timepixServer );

  SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              arp );

  if (seglevel->attach()) {
    printf("entering timepix task main loop\n");
    task->mainLoop();
    printf("exiting timepix task main loop\n");
  }

   return 0;
}
