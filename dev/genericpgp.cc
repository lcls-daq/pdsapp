#include "pds/service/CmdLineTools.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/genericpgp/Manager.hh"
#include "pds/genericpgp/Server.hh"

#include "pdsdata/psddl/alias.ddl.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <new>

namespace Pds {
  //
  //  This class creates the server when the streams are connected.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(GenericPgp::Server* server,
	      const char*         unique_id,
	      unsigned            evr_module,
	      unsigned            evr_channel);

    virtual ~MySegWire() {}

    void connect( InletWire& wire,
		  StreamParams::StreamType s,
		  int interface );

    const std::list<Src>& sources() const { return _sources; }
    const std::list<Alias::SrcAlias>* pAliases() const 
    { return _aliases.size() ? &_aliases : 0; }

    unsigned max_event_size () const { return 4*1024*1024; }
    unsigned max_event_depth() const { return _max_event_depth; }
    void max_event_depth(unsigned d) { _max_event_depth = d; }
  
    bool     is_triggered() const { return true; }
    unsigned module      () const { return _evr_module; }
    unsigned channel     () const { return _evr_channel; }

  private:
    GenericPgp::Server* _server;
    std::list<Src>      _sources;
    std::list<Alias::SrcAlias> _aliases;
    unsigned            _max_event_depth;
    unsigned            _evr_module;
    unsigned            _evr_channel;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
  public:
    Seg( Task* task,
	 unsigned platform,
	 SegWireSettings& settings,
	 Arp* arp,
	 GenericPgp::Server* server,
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
    GenericPgp::Server* _server;
    unsigned     _pgpcard;
    bool         _failed;
  };
};

Pds::MySegWire::MySegWire( GenericPgp::Server* server,
			   const char*         unique_id,
			   unsigned            evr_module,
			   unsigned            evr_channel) :
  _server         (server), 
  _max_event_depth(128),
  _evr_module     (evr_module),
  _evr_channel    (evr_channel)
{ 
   _sources.push_back(server->client());

   if (unique_id)
     _aliases.push_back(Pds::Alias::SrcAlias(server->client(), unique_id));
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _server->fd() );
   wire.add_input( _server );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::GenericPgp::Server* server = Pds::GenericPgp::Server::instance();
  psignal( signal, "Signal received by application");
  if (server != 0) {
    server->disable();
    if (myWire != 0) {
      myWire->remove_input(server);
    }
    //    if (server != 0) server->dumpFrontEnd(); else printf("\tsigHandler found nil server 2!\n");
    if (server != 0) server->die(); else printf("\tsigHandler found nil server 3!\n");
  } else {
    printf("sigHandler found nil server 1!\n");
  }

  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}

Pds::Seg::Seg( Task* task,
               unsigned platform,
               SegWireSettings& settings,
               Arp* arp,
               GenericPgp::Server* server,
               unsigned pgpcard  )
   : _task(task),
     _platform(platform),
     _server(server),
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
   GenericPgp::Manager& mgr = * new GenericPgp::Manager( _server, _pgpcard );
   mgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: %s [-h] [-d <detector>] [-i <deviceID>] [-e <numb>] [-R <bool>] [-r <runTimeConfigName>] [-D <debug>] [-P <pgpcardNumb> -p <platform,evrmod,evrchan> -u <alias>\n"
      "    -h        Show usage\n"
      "    -p        Set platform id, evr module, and channel [required]\n"
      "    -i        Set device info           [required]\n"
      "                integer/integer/integer/integer or string/integer/string/integer\n"
      "                    (e.g. XppEndStation/0/Epix/1 or 22/0/29/1)\n"
      "    -i        Set device id             [Default: 0]\n"
      "    -P        Set pgpcard index number  [Default: 0]\n"
      "    -u <name> Set alias name\n"
      "    -e <N>    Set the maximum event depth, default is 128\n"
      "    -R <B>    Set flag to reset on every config or just the first if false\n"
      "    -r        set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n"
      "    -D        Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label enable and disable\n"
      "                bit 08          turn on printing of FE Internal status on\n"
      "                "
      "              NB, if you can't remember the detector names\n"
      "              just make up something and it'll list them\n", s
  );
}

int main( int argc, char** argv )
{
  bool                parseValid=true;
  DetInfo             info;
  unsigned            platform            = 0;
  unsigned            module = 0, channel = 0;
  bool                platformEntered     = false;
  const char*         unique_id           = 0;
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 128;
  unsigned            resetOnEverConfig   = 0;
  char                runTimeConfigname[256] = {""};
  ::signal( SIGINT,  sigHandler );
  ::signal( SIGSEGV, sigHandler );
  ::signal( SIGFPE,  sigHandler );
  ::signal( SIGTERM, sigHandler );
  ::signal( SIGQUIT, sigHandler );

   extern char* optarg;
   int c;
   while( ( c = getopt( argc, argv, "hi:p:m:e:R:r:D:P:u:" ) ) != EOF ) {
     switch(c) {
         case 'p':
	   parseValid &= CmdLineTools::parseUInt(optarg,platform,module,channel)==3;
           platformEntered = true;
           break;
         case 'u':
	   if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
	     printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
	   } else {
	     unique_id = optarg;
	   }
	   break;
         case 'i':
	   parseValid &= CmdLineTools::parseDetInfo(optarg,info);
           break;
         case 'm':
           parseValid &= CmdLineTools::parseUInt(optarg,mask);
           break;
         case 'P':
           parseValid &= CmdLineTools::parseUInt(optarg,pgpcard);
           break;
         case 'e':
           parseValid &= CmdLineTools::parseUInt(optarg,eventDepth);
           printf("Using event depth of  %u\n", eventDepth);
           break;
         case 'R':
           parseValid &= CmdLineTools::parseUInt(optarg,resetOnEverConfig);
           break;
         case 'r':
           strcpy(runTimeConfigname, optarg);
           break;
         case 'D':
           parseValid &= CmdLineTools::parseUInt(optarg,debug);
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

   if( !parseValid ) {
     printf("Argument parsing failed\n");
     printUsage(argv[0]);
     return 0;
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

   printf("making GenericPgpServer\n");
   GenericPgp::Server* server = new GenericPgp::Server(info, mask);
   printf("Will reset on %s configuration\n", resetOnEverConfig ? "every" : "only the first");
   server->resetOnEveryConfig(resetOnEverConfig);
   server->runTimeConfigName(runTimeConfigname);
   printf("setting server debug level\n");
   server->debug(debug);

   printf("MySetWire settings\n");
   MySegWire settings(server, unique_id, module, channel);
   settings.max_event_depth(eventDepth);

   printf("making Seg\n");
   Seg* seg = new Seg( task,
                       platform,
                       settings,
                       0,
                       server,
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
     printf("entering task main loop, \n\tDetector: %s\n\tPlatform: %u\n",
	    DetInfo::name(info), platform);
     task->mainLoop();
     printf("exiting task main loop\n");
   }
   return 0;
}
