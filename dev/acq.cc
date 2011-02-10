#include "pdsdata/xtc/DetInfo.hh"

#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/Task.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/AcqDataType.hh"
#include "pds/acqiris/AcqD1Manager.hh"
#include "pds/acqiris/AcqT3Manager.hh"
#include "pds/acqiris/AcqFinder.hh"
#include "pds/acqiris/AcqServer.hh"
#include "pds/config/CfgClientNfs.hh"
#include "acqiris/aqdrv4/AcqirisImport.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <list>

namespace Pds {

  //
  //  This class creates the server when the streams are connected.
  //  Real implementations will have something like this.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(std::list<AcqServer*>& servers) : _servers(servers) 
    {
      for(std::list<AcqServer*>::iterator it=servers.begin(); it!=servers.end(); it++)
	_sources.push_back((*it)->client()); 
    }
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface) {
      for(std::list<AcqServer*>::iterator it=_servers.begin(); it!=_servers.end(); it++)
	wire.add_input(*it);
    }
    const std::list<Src>& sources() const { return _sources; }
  private:
    std::list<AcqServer*>& _servers;
    std::list<Src>         _sources;
  };

  //
  //  Implements the callbacks for attaching/dissolving.
  //  Appliances can be added to the stream here.
  //
  class Seg : public EventCallback {
  public:
    Seg(Task*                     task,
        unsigned                  platform,
        SegWireSettings&          settings,
        Arp*                      arp,
        std::list<AcqD1Manager*>& D1Managers,
	std::list<AcqT3Manager*>& T3Managers) :
      _task      (task),
      _platform  (platform),
      _d1managers(D1Managers),
      _t3managers(T3Managers)
    {
    }

    virtual ~Seg()
    {
      _task->destroy();
    }
    
  private:
    // Implements EventCallback
    void attached(SetOfStreams& streams)
    {
      printf("Seg connected to platform 0x%x\n",_platform);
      
      Stream* frmk = streams.stream(StreamParams::FrameWork);
      for(std::list<AcqD1Manager*>::iterator it=_d1managers.begin(); it!=_d1managers.end(); it++)
	(*it)->appliance().connect(frmk->inlet());
      for(std::list<AcqT3Manager*>::iterator it=_t3managers.begin(); it!=_t3managers.end(); it++)
	(*it)->appliance().connect(frmk->inlet());
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
    Task*                     _task;
    unsigned                  _platform;
    std::list<AcqD1Manager*>& _d1managers;
    std::list<AcqT3Manager*>& _t3managers;
  };
}

using namespace Pds;

static void calibrate_module(ViSession id) 
{
  ViStatus status = Acqrs_calibrate(id);
  if(status != VI_SUCCESS) {
    char message[256];
    Acqrs_errorMessage(id,status,message,256);
    printf("Acqiris calibration error: %s\n",message);
  } else {
    printf("Acqiris calibration successful.\n");
    status = Acqrs_calSave(id,AcqManager::calibPath(),1);
    if(status != VI_SUCCESS) {
      char message[256];
      Acqrs_errorMessage(id,status,message,256);
      printf("Acqiris calibration save error: %s\n",message);
    } else {
      printf("Calibration saved.\n");
    }
  }
}

static void calibrate() 
{
  AcqFinder acqFinder(AcqFinder::MultiInstrumentsOnly);

  printf("Calibrating %d D1 instruments\n",acqFinder.numD1Instruments());
  for(int i=0; i<acqFinder.numD1Instruments();i++)
    calibrate_module(acqFinder.D1Id(i));

  printf("Calibrating %d T3 instruments\n",acqFinder.numT3Instruments());
  for(int i=0; i<acqFinder.numT3Instruments();i++)
    calibrate_module(acqFinder.T3Id(i));
}

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned detid = -1UL;
  unsigned devid = 0;
  unsigned platform = -1UL;
  bool multi_instruments_only = true;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:d:p:tC")) != EOF ) {
    switch(c) {
    case 'i':
      detid  = strtoul(optarg, NULL, 0);
      break;
    case 'd':
      devid  = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    case 't':
      multi_instruments_only = false;
      break;
    case 'C':
      calibrate();
      return 0;
      break;
    }
  }

  if ((platform == -1UL) || (detid == -1UL)) {
    printf("Platform and detid required\n");
    printf("Usage: %s -i <detid> -p <platform> [-a <arp process id>]\n", argv[0]);
    return 0;
  }

  Node node(Level::Source,platform);

  std::list<AcqServer*>    servers;
  std::list<AcqD1Manager*> D1Managers;
  std::list<AcqT3Manager*> T3Managers;

  AcqFinder acqFinder(multi_instruments_only ? 
		      AcqFinder::MultiInstrumentsOnly :
		      AcqFinder::All);
  for(int i=0; i<acqFinder.numD1Instruments();i++) {
    DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detid, 0, DetInfo::Acqiris, i+devid);
    AcqServer* srv = new AcqServer(detInfo,_acqDataType);
    servers   .push_back(srv);
    D1Managers.push_back(new AcqD1Manager(acqFinder.D1Id(i),*srv,*new CfgClientNfs(detInfo)));
  }
  for(int i=0; i<acqFinder.numT3Instruments();i++) {
    DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detid, 0, DetInfo::AcqTDC, i+devid);
    AcqServer* srv = new AcqServer(detInfo,_acqTdcDataType);
    servers   .push_back(srv);
    T3Managers.push_back(new AcqT3Manager(acqFinder.T3Id(i),*srv,*new CfgClientNfs(detInfo)));
  }
  
  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(servers);
  Seg* seg = new Seg(task, platform, settings, 0, D1Managers, T3Managers);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();
  return 0;
}
