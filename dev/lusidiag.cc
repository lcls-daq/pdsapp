#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/Task.hh"
#include "pds/ipimb/IpimbManager.hh"
#include "pds/ipimb/IpimbServer.hh"
#include "pds/ipimb/LusiDiagFex.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


namespace Pds {

  //
  //  This class creates the server when the streams are connected.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(IpimbServer** ipimbServer, int nServers) : _ipimbServer(ipimbServer), _nServers(nServers) { 
      for (int i=0; i< _nServers; i++) {
        _sources.push_back(ipimbServer[i]->client()); 
      }
    }
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
                  StreamParams::StreamType s,
                  int interface) {
      for (int i=0; i< _nServers; i++) {
        printf("Adding input of server %d, fd %d\n", i, _ipimbServer[i]->fd());
        wire.add_input(_ipimbServer[i]);
      }
    }
    const std::list<Src>& sources() const { return _sources; }
  private:
    IpimbServer** _ipimbServer;
    std::list<Src> _sources;
    const int _nServers;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
  public:
    Seg(Task*                 task,
        unsigned              platform,
        CfgClientNfs**        cfgService,
        SegWireSettings&      settings,
        Arp*                  arp,
        IpimbServer**         ipimbServer,
        int nServers,
        char* portName[16]) :
      _task(task),
      _platform(platform),
      _cfg   (cfgService),
      _ipimbServer(ipimbServer),
      _nServers(nServers)
    {
      for (int i=0; i<16; i++) {
	_portName[i] = portName[i];
      }
    }

    virtual ~Seg()
    {
      _task->destroy();
    }
    
  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("Seg connected to platform 0x%x\n", 
             _platform);
      
      Stream* frmk = streams.stream(StreamParams::FrameWork);
      IpimbManager& ipimbMgr = *new IpimbManager(_ipimbServer, _nServers, _cfg, _portName, *new LusiDiagFex);
      ipimbMgr.appliance().connect(frmk->inlet());
    }
    void failed(Reason reason)
    {
      static const char* reasonname[] = { "platform unavailable", 
                                          "crates unavailable", 
                                          "fcpm unavailable" };
      printf("Seg: unable to allocate crates on platform 0x%x : %s\n", 
             _platform, reasonname[reason]);
      delete this;
    }
    void dissolved(const Node& who)
    {
      const unsigned userlen = 12;
      char username[userlen];
      Node::user_name(who.uid(),username,userlen);
      
      const unsigned iplen = 64;
      char ipname[iplen];
      Node::ip_name(who.ip(),ipname, iplen);
      
      printf("Seg: platform 0x%x dissolved by user %s, pid %d, on node %s", 
             who.platform(), username, who.pid(), ipname);
      
      delete this;
    }
    
  private:
    Task*         _task;
    unsigned      _platform;
    CfgClientNfs** _cfg;
    IpimbServer**  _ipimbServer;
    const int _nServers;
    char* _portName[16];
  };
}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned detid = -1UL;
  unsigned cpuid = -1UL;
  unsigned platform = 0;
  unsigned nboards = 1;
  int baselineSubtraction = 1;
  FILE *fp = NULL;
  Arp* arp = 0;

  extern char* optarg;
  int s;
  while ( (s=getopt( argc, argv, "a:i:c:p:n:b:f:C")) != EOF ) {
    switch(s) {
      case 'a':
      arp = new Arp(optarg);
      break;
    case 'i':
      detid  = strtoul(optarg, NULL, 0);
      break;
    case 'c':
      cpuid  = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 'n':
      nboards = strtoul(optarg, NULL, 0);
      break;
    case 'b':
      baselineSubtraction = strtoul(optarg, NULL, 0);
      break;
    case 'f':
      fp = fopen(optarg,"r");
      if (fp) {
	printf("Have opened configuration file %s\n", optarg);
      } else {
	char message[128];
	sprintf(message, "failed to open ipimb config file %s\n", optarg); 
	printf("%s %s\n",argv[0], message);
	return 0;
      }
      break;
    }
  }

  int detector, detectorId, deviceId;
  int polarity;
  char port[16]; // long enough for "/dev/ttyPSmn\n"
  int portInfo[16][3]; // make this a struct array
  char* portName[16];
  for (int i=0; i<16; i++) {
    portName[i] = new char[16];
  }
  int polarities[16];
  if (fp) {
    char* tmp = NULL;
    size_t sz = 0;
    nboards = 0;
    while (getline(&tmp, &sz, fp)>0) {
      if (tmp[0] != '#') {
	sscanf(tmp,"%d %d %d %s,%d",&detector, &detectorId, &deviceId, port, &polarity);
	portInfo[nboards][0] = detector;
	portInfo[nboards][1] = detectorId;
	portInfo[nboards][2] = deviceId;
	strcpy(portName[nboards], port);
	polarities[nboards] = polarity;
	nboards++;
      }
    }
    printf("Have found %d uncommented lines in config file, will use that many boards\n", nboards);
    rewind(fp);
  }

  if ((!platform) || ((detid == -1UL) && !fp)) {
    printf("Platform and detid or DetInfo file required\n");
    printf("Usage: %s -i <detid> | -f <fileName> -p <platform> [-a <arp process id>]\n", argv[0]);
    return 0;
  }

  // launch the SegmentLevel
  if (arp) {
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create odfArp : %s", 
              strerror(arp->error()));
      printf("%s %s\n",argv[0], message);
      delete arp;
      return 0;
    }
  }

  Node node(Level::Source,platform);

  Task* task = new Task(Task::MakeThisATask);
  
  const unsigned nServers = nboards;
  IpimbServer* ipimbServer[nServers];
  CfgClientNfs* cfgService[nServers];
  
  for (unsigned i=0; i<nServers; i++) {
    if (!fp) {
      if (i==0) 
        printf("No port config file specified, connect densely\n");
      DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detid, cpuid, DetInfo::Ipimb, i);
      cfgService[i] = new CfgClientNfs(detInfo);
      ipimbServer[i] = new IpimbServer(detInfo, baselineSubtraction, polarities[i]);
      portName[i][0] = '\0';
    } else {
      detector = portInfo[i][0]; 
      detectorId = portInfo[i][1];
      deviceId = portInfo[i][2];
      DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detector, detectorId, DetInfo::Ipimb, deviceId);
      cfgService[i] = new CfgClientNfs(detInfo);
      ipimbServer[i] = new IpimbServer(detInfo, baselineSubtraction, 0);
    }
  }
  if (fp) {
    for (unsigned i=0; i<nServers; i++) {
      printf("Using config file info: detector %d, detector id %d, device id %d, port %s\n", 
             portInfo[i][0], portInfo[i][1], portInfo[i][2], portName[i]);
    }
    fclose(fp);
  }

  MySegWire settings(ipimbServer, nServers);
  Seg* seg = new Seg(task, platform, cfgService, settings, arp, ipimbServer, nServers, portName);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, arp);
  seglevel->attach();

  task->mainLoop();


  if (arp) delete arp;
  return 0;
}
