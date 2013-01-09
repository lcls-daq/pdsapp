#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/gsc16ai/Gsc16aiManager.hh"
#include "pds/gsc16ai/Gsc16aiServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

static void usage(const char* p)
{
  printf("Usage: %s -i <detid> -p <platform> [-h]\n",p);
}

static void help()
{
  printf("Options:\n"
         "  -i <detid>            detector ID\n"
         "  -p <platform>         platform number\n"
         "  -h                    help: print this message and exit\n");
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
   MySegWire(Gsc16aiServer* gsc16aiServer);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

 private:
   Gsc16aiServer* _gsc16aiServer;
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
        Gsc16aiServer* gsc16aiServer );

   virtual ~Seg();
    
 private:
   // Implements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task* _task;
   unsigned _platform;
   CfgClientNfs* _cfg;
   Gsc16aiServer* _gsc16aiServer;
};


Pds::MySegWire::MySegWire( Gsc16aiServer* gsc16aiServer )
   : _gsc16aiServer(gsc16aiServer)
{ 
   _sources.push_back(gsc16aiServer->client()); 
}

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _gsc16aiServer->fd() );
   wire.add_input( _gsc16aiServer );
}


Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               Gsc16aiServer* gsc16aiServer )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _gsc16aiServer(gsc16aiServer)
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
   Gsc16aiManager& gsc16aiMgr = * new Gsc16aiManager( _gsc16aiServer,
                                                      _cfg );
   gsc16aiMgr.appliance().connect( frmk->inlet() );
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

int main( int argc, char** argv )
{
   unsigned detid = -1UL;
   unsigned platform = -1UL;
   Arp* arp = 0;
   bool helpFlag = false;
   char *endPtr;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "a:i:p:h" ) ) != EOF ) {
      switch(c) {
         case 'a':
            arp = new Arp(optarg);
            break;
         case 'i':
            errno = 0;
            endPtr = NULL;
            detid = strtoul(optarg, &endPtr, 0);
            if (errno || (endPtr == NULL) || (*endPtr != '\0')) {
              printf("Error: failed to parse detector ID\n");
              usage(argv[0]);
              return -1;
            }
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
         case 'h':
            helpFlag = true;
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
  
   Gsc16aiServer* gsc16aiServer;
   CfgClientNfs* cfgService;

   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detid,
                    0,
                    DetInfo::Gsc16ai,
                    0 );
   cfgService = new CfgClientNfs(detInfo);
   gsc16aiServer = new Gsc16aiServer(detInfo);

   MySegWire settings(gsc16aiServer);

   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       arp,
                       gsc16aiServer );

   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              arp );
   (void) seglevel->attach();

   printf("entering gsc16ai task main loop\n");
   task->mainLoop();
   printf("exiting gsc16ai task main loop\n");

   if (arp) delete arp;

   return 0;
}
