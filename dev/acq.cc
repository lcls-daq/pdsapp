#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/psddl/alias.ddl.h"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventCallback.hh"
#include "pds/collection/Arp.hh"

#include "pds/management/SegStreams.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/Semaphore.hh"
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
#include <string.h>

#include <list>
#include <climits>

static char acqFlag;

using Pds::Alias::SrcAlias;

namespace Pds {

  //
  //  This class creates the server when the streams are connected.
  //  Real implementations will have something like this.
  //
  class MySegWire : public SegWireSettings {
  public:
    MySegWire(std::list<AcqServer*>& servers,
              unsigned               module,
              unsigned               channel,
              const char*            aliasName) :
      _servers(servers),
      _module (module),
      _channel(channel)

    {
      char tmpBuffer[SrcAlias::AliasNameMax];
      for(std::list<AcqServer*>::iterator it=servers.begin(); it!=servers.end(); it++) {
        _sources.push_back((*it)->client());
        if (aliasName) {
          if (servers.size() == 1) {
            SrcAlias tmpAlias((*it)->client(), aliasName);
            _aliases.push_back(tmpAlias);
          } else {
            snprintf(tmpBuffer, SrcAlias::AliasNameMax, "%s_b%d",
                     aliasName, reinterpret_cast<const DetInfo&>((*it)->client()).devId());
            SrcAlias tmpAlias((*it)->client(), tmpBuffer);
            _aliases.push_back(tmpAlias);
          }
        }
      }
    }
    virtual ~MySegWire() {}
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface) {
      for(std::list<AcqServer*>::iterator it=_servers.begin(); it!=_servers.end(); it++)
	wire.add_input(*it);
    }
    const std::list<Src>& sources() const { return _sources; }
    const std::list<SrcAlias>* pAliases() const
    {
      return (_aliases.size() > 0) ? &_aliases : NULL;
    }
    bool     is_triggered() const { return true; }
    unsigned module      () const { return _module; }
    unsigned channel     () const { return _channel; }

  private:
    std::list<AcqServer*>& _servers;
    std::list<Src>         _sources;
    std::list<SrcAlias>    _aliases;
    unsigned       _module;
    unsigned       _channel;
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

static void calibrate_module(ViSession id,
                             uint32_t nbrConverters,
                             uint32_t calChannelMask) 
{
  ViStatus status;
  if (nbrConverters==0 && calChannelMask==0)
    status = Acqrs_calibrate(id);
  else {
    // configure channels
    printf("Configuring channelmask 0x%x  nbrConverters %d\n",
           calChannelMask, nbrConverters);
    AcqrsD1_configChannelCombination(id, nbrConverters, calChannelMask);
    status = Acqrs_calibrateEx(id,1,0,0);
  }

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

static long temperature(ViSession id, int module) {
  long degC = 999;    // unknown temp
  char tempstring[32];
  sprintf(tempstring,"Temperature %d",module);
  ViStatus status = AcqrsD1_getInstrumentInfo(id, tempstring, &degC);
  if(status != VI_SUCCESS) {
    degC = 999;
    char message[256];
    AcqrsD1_errorMessage(id,status,message);
    printf("Acqiris id %u module %d temperature reading error: %s\n", (unsigned)id, module, message);
  }
  return degC;
}

static void calibrate(AcqFinder& acqFinder,
                      unsigned   nbrConverters,
                      unsigned   calChannelMask, 
                      char *     pvPrefix)
{
  printf("Calibrating %ld D1 instruments\n",acqFinder.numD1Instruments());
  ViSession instrumentId;
  unsigned nbrModulesInInstrument;
  for(int i=0; i<acqFinder.numD1Instruments();i++) {
    if (pvPrefix) {
      instrumentId = acqFinder.D1Id(i);
      ViStatus status = AcqrsD1_getInstrumentInfo(instrumentId,"NbrModulesInInstrument",
                                &nbrModulesInInstrument);
      if (status != VI_SUCCESS) {
        char message[256];
        AcqrsD1_errorMessage(instrumentId,status,message);
        printf("%s: Acqiris NbrModulesInInstrument error: %s\n", __PRETTY_FUNCTION__,  message);
      } else {
        for (int module = 1; module <= (int)nbrModulesInInstrument; module++) {
          printf("*** Acqiris Temperature %d: %ld C ***\n", module, temperature(instrumentId, module));
        }
      }
    }
    calibrate_module(acqFinder.D1Id(i),nbrConverters,calChannelMask);
  }

  printf("Calibrating %ld T3 instruments\n",acqFinder.numT3Instruments());
  for(int i=0; i<acqFinder.numT3Instruments();i++)
    calibrate_module(acqFinder.T3Id(i),0,0);
}

static void usage(char *p)
{
  printf("Usage: %s -i <detid> -p <platform>,<mod>,<chan> [OPTIONS]\n\n", p);
  printf("       %s -C [OPTIONS]\n", p);
  printf("\n"
         "Options:\n"
         "    -i <detid>                  detector ID (e.g. 12 for SxrEndstation)\n"
         "    -p <platform>,<mod>,<chan>  platform number, EVR module, EVR channel\n"
         "    -d <devid>                  device ID \n"
         "    -t                          multi-instrument (look for more than one module, ADC or TDC, in crate)\n"
         "    -P <prefix>[,<period>]      temperature monitoring: PV prefix (req'd) and period in sec (default %d)\n"
         "    -z                          temperature monitoring: do not pause during data collection\n"
         "    -u <alias>                  set device alias\n"
         "    -v                          be verbose\n"
         "    -C                          calibrate\n"
         "    -c <nConverters>            number of converters (used by calibrate function only)\n"
         "    -m <mask>                   calibration channel mask (used by calibrate function only)\n"
         "\n"
         "     * Warning! Setting 'z' flag may negatively affect data quality due to crosstalk\n",
         AcqManager::AcqTemperaturePeriod);
}


int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  unsigned detid = UINT_MAX;
  unsigned devid = 0;
  unsigned platform = UINT_MAX;
  unsigned module = 0;
  unsigned channel = 0;
  bool multi_instruments_only = true;
  char* pvPrefix = (char *)NULL;
  char* pComma = (char *)NULL;
  unsigned pvPeriod = AcqManager::AcqTemperaturePeriod;
  bool lcalibrate = false;
  unsigned nbrConverters=0;
  unsigned calChannelMask=0;
  char* uniqueid = (char *)NULL;
  bool helpFlag = false;
  bool verboseFlag = false;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "i:d:p:tP:zCc:m:u:hv")) != EOF ) {
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseUInt(optarg,detid)) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'd':
      if (!CmdLineTools::parseUInt(optarg,devid)) {
        printf("%s: option `-d' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'p':
      if (CmdLineTools::parseUInt(optarg,platform,module,channel) != 3) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 't':
      multi_instruments_only = false;
      break;
    case 'v':
      verboseFlag = true;
      acqFlag |= AcqManager::AcqFlagVerbose;
      break;
    case 'P':
        pComma = strchr(optarg, ',');
        if (pComma) {
          *pComma++ = '\0';
          if (!CmdLineTools::parseUInt(pComma,pvPeriod)) {
            printf("%s: option `-P' parsing error\n", argv[0]);
            helpFlag = true;
          }
        }
        if (strlen(optarg) > 20) {
          printf("PV prefix '%s' exceeds 20 chars, ignored\n", optarg);
        } else {
          pvPrefix = optarg;
        }
      break;
    case 'z':
      acqFlag |= AcqManager::AcqFlagZealous;
      printf("%s: 'z' flag is set. Temperature monitoring will not pause during data collection.\n", argv[0]);
      printf("Warning!  Crosstalk may affect data quality.\n");
      break;
    case 'u':
      if (!CmdLineTools::parseSrcAlias(optarg)) {
        printf("%s: option `-u' parsing error\n", argv[0]);
        helpFlag = true;
      } else {
        uniqueid = optarg;
      }
      break;
    case 'C':
      lcalibrate = true;
      break;
    case 'c':
      if (!CmdLineTools::parseUInt(optarg,nbrConverters)) {
        printf("%s: option `-c' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'm':
      if (!CmdLineTools::parseUInt(optarg,calChannelMask)) {
        printf("%s: option `-m' parsing error\n", argv[0]);
        helpFlag = true;
      }
      break;
    case 'h':
      usage(argv[0]);
      return 0;
    case '?':
    default:
      helpFlag = true;
      break;
    }
  }

  if (!pvPrefix) {
    // no temperature monitoring, so ignore the 'z' flag
    acqFlag &= ~AcqManager::AcqFlagZealous;
  }

  if (!lcalibrate && (detid == UINT_MAX)) {
    printf("%s: detid is required\n", argv[0]);
    helpFlag = true;
  }

  if (!lcalibrate && (platform == UINT_MAX)) {
    printf("%s: platform is required\n", argv[0]);
    helpFlag = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n",argv[0], argv[optind]);
    helpFlag = true;
  }

  if (helpFlag) {
    usage(argv[0]);
    return 1;
  }

  if ((pvPrefix) && verboseFlag) {
    printf("%s: PV prefix: %s  period: %u\n", argv[0], pvPrefix, pvPeriod);
  }

  AcqFinder acqFinder(multi_instruments_only ? 
		      AcqFinder::MultiInstrumentsOnly :
		      AcqFinder::All);

  if (lcalibrate) {
    calibrate(acqFinder,nbrConverters,calChannelMask,pvPrefix);
    return 0;
  }

  Node node(Level::Source,platform);

  std::list<AcqServer*>    servers;
  std::list<AcqD1Manager*> D1Managers;
  std::list<AcqT3Manager*> T3Managers;

  Semaphore sem(Semaphore::FULL);
  for(int i=0; i<acqFinder.numD1Instruments();i++) {
    DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detid, 0, DetInfo::Acqiris, i+devid);
    AcqServer* srv = new AcqServer(detInfo,_acqDataType);
    servers   .push_back(srv);
    D1Managers.push_back(new AcqD1Manager(acqFinder.D1Id(i),*srv,*new CfgClientNfs(detInfo),sem,pvPrefix,pvPeriod,&acqFlag));
  }
  for(int i=0; i<acqFinder.numT3Instruments();i++) {
    DetInfo detInfo(node.pid(), (Pds::DetInfo::Detector)detid, 0, DetInfo::AcqTDC, i+devid);
    AcqServer* srv = new AcqServer(detInfo,_acqTdcDataType);
    servers   .push_back(srv);
    T3Managers.push_back(new AcqT3Manager(acqFinder.T3Id(i),*srv,*new CfgClientNfs(detInfo),sem));
  }
  
  Task* task = new Task(Task::MakeThisATask);
  MySegWire settings(servers, module, channel, uniqueid);
  Seg* seg = new Seg(task, platform, settings, 0, D1Managers, T3Managers);
  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();
  return 0;
}
