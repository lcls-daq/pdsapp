#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/encoder/EncoderManager.hh"
#include "pds/encoder/EncoderServer.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

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
   MySegWire(EncoderServer** encoderServer, int nServers);
   virtual ~MySegWire() {}

   void connect( InletWire& wire,
                 StreamParams::StreamType s,
                 int interface );

   const std::list<Src>& sources() const { return _sources; }

 private:
   EncoderServer** _encoderServer;
   std::list<Src> _sources;
   const int _nServers;
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
        CfgClientNfs** cfgService,
        SegWireSettings& settings,
        Arp* arp,
        EncoderServer** encoderServer,
        int nServers );
   virtual ~Seg();
    
 private:
   // Implements EventCallback
   void attached( SetOfStreams& streams );
   void failed( Reason reason );
   void dissolved( const Node& who );

   Task* _task;
   unsigned _platform;
   CfgClientNfs** _cfg;
   EncoderServer** _encoderServer;
   const int _nServers;
};


Pds::MySegWire::MySegWire( EncoderServer** encoderServer,
                           int nServers)
   : _encoderServer(encoderServer),
     _nServers(nServers)
{ 
   for( int i = 0; i < _nServers; i++ )
   {
      _sources.push_back(encoderServer[i]->client()); 
   }
}

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   for( int i = 0; i < _nServers; i++ )
   {
      printf("Adding input of server %d, fd %d\n",
             i, _encoderServer[i]->fd() );
      wire.add_input( _encoderServer[i] );
   }
}


Pds::Seg::Seg( Task* task,
          unsigned platform,
	  CfgClientNfs** cfgService,
          SegWireSettings& settings,
          Arp* arp,
          EncoderServer** encoderServer,
	  int nServers )
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _encoderServer(encoderServer),
     _nServers(nServers)
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
   EncoderManager& encoderMgr = * new EncoderManager( _encoderServer,
                                                      _nServers,
                                                      _cfg );
   encoderMgr.appliance().connect( frmk->inlet() );
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
   unsigned platform = 0;
   unsigned nboards = 1;
   Arp* arp = 0;

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "a:i:p:n:C" ) ) != EOF ) {
      switch(c) {
         case 'a':
            arp = new Arp(optarg);
            break;
         case 'i':
            detid  = strtoul(optarg, NULL, 0);
            break;
         case 'p':
            platform = strtoul(optarg, NULL, 0);
            break;
         case 'n':
            nboards = strtoul(optarg, NULL, 0);
            break;
      }
   }

   if( !platform || ( detid == -1UL ) ) {
      printf( "Error: Platform and detid required\n" );
      printf( "Usage: %s -i <detid> -p <platform> [-a <arp process id>]\n",
              argv[0] );
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
  
   const unsigned nServers = nboards;
   EncoderServer* encoderServer[nServers];
   CfgClientNfs* cfgService[nServers];

   for (unsigned i = 0; i < nServers; i++)
   {
      DetInfo detInfo( node.pid(),
                       (Pds::DetInfo::Detector) detid,
                       0,
                       DetInfo::Encoder,
                       i );
      cfgService[i] = new CfgClientNfs(detInfo);
      encoderServer[i] = new EncoderServer(detInfo);
   }

   MySegWire settings(encoderServer, nServers);

   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       arp,
                       encoderServer,
                       nServers );

   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              arp );
   seglevel->attach();

   printf("entering encoder task main loop\n");
   task->mainLoop();
   printf("exiting encoder task main loop\n");

   if (arp) delete arp;

   return 0;
}
