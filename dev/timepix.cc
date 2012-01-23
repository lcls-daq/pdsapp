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

#include "mpxfilelogger.h"

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

// static int    iSignalCaught   = 0;
static Task*  readTask = NULL;
static Task*  decodeTask = NULL;
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
  char *threshFileName = NULL;
  char *logpath = NULL;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "a:i:p:m:vd:T:o:" ) ) != EOF ) {
      switch(c) {
         case 'a':
            arp = new Arp(optarg);
            break;
         case 'i':
            detid  = strtoul(optarg, NULL, 0);
            break;
         case 'm':
            moduleId  = strtoul(optarg, NULL, 0);
            break;
         case 'd':
            debug  = strtoul(optarg, NULL, 0);
            break;
         case 'v':
            ++verbosity;
            break;
         case 'p':
            platform = strtoul(optarg, NULL, 0);
            break;
         case 'T':
            threshFileName = optarg;
            break;
         case 'o':
            logpath = optarg;
            break;
      }
   }

   if( (platform==-1UL) || ( detid == -1UL ) ) {
      printf( "Error: Platform and detid required\n" );
      printf( "Usage: %s -i <detid> -p <platform> [-a <arp process id>] [-m <module id>] [-v]\n"
              "                   [-d <debug flags>] [-T <threshold file>] [-o <logpath>]\n", argv[0] );
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

   if( arp )
   {
      if( arp->error() )
      {
         printf( "%s failed to create odfArp : %s\n",
                 argv[0], strerror( arp->error() ) );

         delete arp;
         return 0;
      }
   }

   Node node( Level::Source, platform );

   Task* task = new Task( Task::MakeThisATask );
  
   CfgClientNfs* cfgService;

   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detid,
                    0,
                    DetInfo::Timepix,
                    0 );

  cfgService = new CfgClientNfs(detInfo);

  timepixServer = new TimepixServer(detInfo, moduleId, verbosity, debug, threshFileName);
  readTask = timepixServer->readTask();
  decodeTask = timepixServer->decodeTask();

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

   if (arp) delete arp;

   return 0;
}
