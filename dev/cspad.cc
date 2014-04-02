#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/Task.hh"
#include "pds/cspad/CspadManager.hh"
#include "pds/cspad/CspadServer.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/psddl/cspad.ddl.h"

#include "pds/client/FrameCompApp.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <new>
#include <dlfcn.h>

namespace Pds
{
   class MySegWire;
   class Seg;
}

typedef std::list<Pds::Appliance*> AppList;

//
//  This class creates the server when the streams are connected.
//
class Pds::MySegWire
   : public SegWireSettings
{
  public:
   MySegWire(CspadServer* cspadServer, const char *aliasName);
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
   CspadServer* _cspadServer;
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
        CspadServer* cspadServer,
        unsigned pgpcard,
	const AppList& user_apps,
        bool compress = false,
        unsigned nthreads = 0);

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
   CspadServer*  _cspadServer;
   unsigned      _pgpcard;
   AppList       _user_apps;
   bool          _compress;
   unsigned       _nthreads;
   bool          _failed;
};


Pds::MySegWire::MySegWire( CspadServer* cspadServer, const char *aliasName )
   : _cspadServer(cspadServer), _max_event_depth(64)
{ 
   _sources.push_back(cspadServer->client());
   if (aliasName) {
     SrcAlias tmpAlias(cspadServer->client(), aliasName);
     _aliases.push_back(tmpAlias);
   }
}

static Pds::InletWire* myWire = 0;

void Pds::MySegWire::connect( InletWire& wire,
                              StreamParams::StreamType s,
                              int interface )
{
   printf("Adding input of server, fd %d\n", _cspadServer->fd() );
   wire.add_input( _cspadServer );
   myWire = &wire;
}

void sigHandler( int signal ) {
  Pds::CspadServer* server = Pds::CspadServer::instance();
  psignal( signal, "Signal received by Cspad segment level");
  if (server != 0) {
    server->disable();
    if (myWire != 0) {
      myWire->remove_input(server);
    } else printf("\tmyWire is gone!\n");
    if (server != 0) server->ignoreFetch(true); else printf("\tServer is gone!\n");
    if (server != 0) server->dumpFrontEnd(); else printf("\tServer is gone!\n");
    if (server != 0) server->die(); else printf("\tServer is gone!\n");
  } else printf("\tServer is gone! Did nothing\n");
  printf("Signal handler pulling the plug\n");
  ::exit(signal);
}

Pds::Seg::Seg( Task* task,
               unsigned platform,
               CfgClientNfs* cfgService,
               SegWireSettings& settings,
               Arp* arp,
               CspadServer* cspadServer,
               unsigned pgpcard,
	       const AppList& user_apps,
               bool compress,
               unsigned nthreads)
   : _task(task),
     _platform(platform),
     _cfg   (cfgService),
     _cspadServer(cspadServer),
     _pgpcard(pgpcard),
     _user_apps(user_apps),
     _compress(compress),
     _nthreads(nthreads),
     _failed(false)
{}

Pds::Seg::~Seg()
{
  for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
    delete (*it);
  _task->destroy();
}
    
void Pds::Seg::attached( SetOfStreams& streams )
{
   printf("Seg connected to platform 0x%x\n", 
          _platform);
      
   Stream* frmk = streams.stream(StreamParams::FrameWork);
   //   CspadManager& cspadMgr = * new CspadManager( _cspadServer, _pgpcard, _compress );
   //   if (_compress) cspadMgr.appProcessor().connect( frmk->inlet() );
   CspadManager& cspadMgr = * new CspadManager( _cspadServer, _pgpcard, false );
   if (_compress) 
     (new FrameCompApp(0x600000,_nthreads))->connect( frmk->inlet() );

   for(AppList::iterator it=_user_apps.begin(); it!=_user_apps.end(); it++)
     (*it)->connect(frmk->inlet());

   cspadMgr.appliance().connect( frmk->inlet() );
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
  printf( "Usage: %s [-h] -p <platform> [-d <detector>] [-i <deviceID>] [-m <configMask>] [-e <numb>] [-C <compressFlag>] [-u <alias>] [-D <debug>] [-P <pgpcardNumb> [-L <plugin>] [-r <runTimeConfigName>]\n"
      "    -h      Show usage\n"
      "    -p      Set platform id           [required]\n"
      "    -d      Set detector type by name [Default: XppGon]\n"
      "            NB, if you can't remember the detector names\n"
      "            just make up something and it'll list them\n"
      "    -i      Set device id             [Default: 0]\n"
      "    -m      Set config mask           [Default: 0]\n"
      "    -P      Set pgpcard index number  [Default: 0]\n"
      "                The format of the index number is a one byte number with the bottom nybble being\n"
      "                the index of the card and the top nybble being a port mask where one bit is for\n"
      "                each port, but a value of zero maps to 15 for compatiblity with unmodified\n"
      "                applications that use the whole card\n"
      "    -e <N>  Set the maximum event depth, default is 64\n"
      "    -C <N> or \"<N>,<T>\"  Compress and copy every Nth event (and use <T> threads)\n"
      "    -u      Set device alias          [Default: none]\n"
      "    -D      Set debug value           [Default: 0]\n"
      "                bit 00          label every fetch\n"
      "                bit 01          label more, offest and count calls\n"
      "                bit 02          fill in fetch details\n"
      "                bit 03          label CspadL1Action::fire\n"
      "                bit 04          print out FE config read only registers after config or record\n"
      "                bit 05          label CspadServer enable and disable\n"
      "                bit 06          print out (acqcount - frameNumber) for every L1Accept\n"
      "                bit 08          turn on printing of FE concentrator status on\n"
      "                bit 09          turn on printing of FE quad status\n"
      "                bit 10          print out time dumping front end took\n"
      "                bit 11          print out the front end stat in end calib instead of unconfig\n"
      "                bit 12          print out compression status info\n"
      "    -L <p>  Load the appliance plugin library from path <p>\n"
      "    -r      set run time config file name\n"
      "                The format of the file consists of lines: 'Dest Addr Data'\n"
      "                where Addr and Data are 32 bit unsigned integers, but the Dest is a\n"
      "                four bit field where the bottom two bits are VC and The top two are Lane\n",
      s
  );
}

int main( int argc, char** argv )
{
  DetInfo::Detector   detector            = DetInfo::XppGon;
  DetInfo::Device     device              = DetInfo::Cspad;
  TypeId::Type        type                = TypeId::Id_CspadElement;
  int                 deviceId            = 0;
  unsigned            platform            = 0;
  unsigned            mask                = 0;
  unsigned            pgpcard             = 0;
  unsigned            debug               = 0;
  unsigned            eventDepth          = 64;
  ::signal( SIGINT, sigHandler );
  char                runTimeConfigname[256] = {""};
  bool                platformMissing     = true;
  bool                compressFlag        = false;
  unsigned            compressThreads     = 0;
  AppList user_apps;

   extern char* optarg;
   char* endPtr;
   char* uniqueid = (char *)NULL;
   int c;
   while( ( c = getopt( argc, argv, "hd:i:p:m:e:C:D:xL:P:r:u:" ) ) != EOF ) {
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
           printf("Cspad using pgpcard 0x%x\n", pgpcard);
           break;
         case 'e':
           eventDepth = strtoul(optarg, NULL, 0);
           printf("Cspad using event depth of  %u\n", eventDepth);
           break;
         case 'C':
           compressFlag = 1;
	   FrameCompApp::setCopyPresample(strtoul(optarg, &endPtr, 0));
           if (*endPtr)
             compressThreads = strtoul(endPtr+1,NULL,0);
           break;
         case 'D':
           debug = strtoul(optarg, NULL, 0);
           printf("Cspad using debug value of 0x%x\n", debug);
           break;
         case 'L':
	   { for(const char* p = strtok(optarg,","); p!=NULL; p=strtok(NULL,",")) {
	       printf("dlopen %s\n",p);

	       void* handle = dlopen(p, RTLD_LAZY);
	       if (!handle) {
		 printf("dlopen failed : %s\n",dlerror());
		 break;
	       }

	       // reset errors
	       const char* dlsym_error;
	       dlerror();

	       // load the symbols
	       create_app* c_user = (create_app*) dlsym(handle, "create");
	       if ((dlsym_error = dlerror())) {
		 fprintf(stderr,"Cannot load symbol create: %s\n",dlsym_error);
		 break;
	       }
	       user_apps.push_back( c_user() );
	     }
	     break;
	   }
         case 'r':
           strcpy(runTimeConfigname, optarg);
           break;
         case 'h':
           printUsage(argv[0]);
           return 0;
           break;
         case 'u':
           if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
             printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
           } else {
             uniqueid = optarg;
           }
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
  
   CspadServer* cspadServer;
   CfgClientNfs* cfgService;

   printf("Compression is %s.\n", compressFlag ? "enabled" : "disabled");

   DetInfo detInfo( node.pid(),
                    (Pds::DetInfo::Detector) detector,
                    0,
                    device,
                    deviceId );

   TypeId typeId( type, Pds::CsPad::DataV1::Version );

   cfgService = new CfgClientNfs(detInfo);
   cspadServer = new CspadServer(detInfo, typeId, mask);
   cspadServer->debug(debug);
   cspadServer->runTimeConfigName(runTimeConfigname);

   MySegWire settings(cspadServer, uniqueid);
   settings.max_event_depth(eventDepth);

   Seg* seg = new Seg( task,
                       platform,
                       cfgService,
                       settings,
                       0,
                       cspadServer,
                       pgpcard,
		       user_apps,
                       compressFlag,
                       compressThreads);

   SegmentLevel* seglevel = new SegmentLevel( platform,
                                              settings,
                                              *seg,
                                              0 );
   seglevel->attach();

   if (seg->didYouFail()) printf("So, goodbye cruel world!\n ");
   else  {
     printf("entering cspad task main loop, \n\tDetector: %s\n\tDevice: %s\n\tPlatform: %u\n",
         DetInfo::name((DetInfo::Detector)detector), DetInfo::name(device), platform);
     task->mainLoop();
     printf("exiting cspad task main loop\n");
   }
   return 0;
}
