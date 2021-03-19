#include "pdsdata/xtc/DetInfo.hh"

#include "pdsapp/tools/EventOptions.hh"
#include "pdsapp/tools/CountAction.hh"
#include "pdsapp/tools/StatsTree.hh"
#include "pds/client/Decoder.hh"

#include "pds/service/CmdLineTools.hh"
#include "pds/management/SegmentLevel.hh"
#include "pds/management/EventAppCallback.hh"
#include "pds/management/StdSegWire.hh"
#include "pds/service/Task.hh"
#include "pds/quadadc/Server.hh"
#include "pds/quadadc/Manager.hh"
#include "hsd/Module.hh"
//#include "pds/quadadc/StatsTimer.hh"
//#include "pds/quadadc/EpicsCfg.hh"
#include "pds/config/CfgClientNfs.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <poll.h>

//#include "cadef.h"

extern int optind;

using namespace Pds;
using Pds::QuadAdc::Manager;
using Pds::QuadAdc::Server;
using Pds::HSD::Module;

static void usage(const char *p)
{
  printf("Usage: %s [options]\n",p);
  printf("Options:\n"
         "\t-i <detInfo>\n"
         "\t-p <platform>\n"
         "\t-r <evrId>\n"
         "\t-u <alias>\n"
         "\t-t <testPattern>\n"
         "\t-A (anyMatch)\n");
}

int main(int argc, char** argv) {
  
  char qadc='a';
  int c;
  bool lUsage = false;
  //  bool lSetupClkSynth = false;
  //  bool lReset = false;
  //  bool lResetRx = false;
  //  bool lPolarity = false;
  //  bool lMonitor = false;
  //  bool lRing0 = false;
  //  bool lRing1 = false;
  //  bool lDumpAlign = false;
  //  int  regTest=-1, regValu=-1;
  char* evrid =0;
  //  const char* prefix = "qadc";
  int testpattern = -1;
  bool calib = false;
  bool anyMatch = false;
  unsigned fmc = 0;

  const uint32_t NO_PLATFORM = uint32_t(-1UL);
  uint32_t  platform  = NO_PLATFORM;

  extern char* optarg;
  Pds::DetInfo info(0,DetInfo::NoDetector,0,DetInfo::NoDevice,0);

  char* uniqueid = (char *)NULL;

  while ( (c=getopt( argc, argv, "i:p:P:r:u:def:mht:c:A")) != EOF) {
    switch(c) {
    case 'i':
      if (!CmdLineTools::parseDetInfo(optarg,info)) {
        printf("%s: option `-i' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'p':
      if (!CmdLineTools::parseUInt(optarg,platform)) {
        printf("%s: option `-p' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'P':
      //      prefix = optarg;
      break;
    case 'r':
      evrid = optarg;
      if (strlen(evrid) != 1) {
        printf("%s: option `-r' parsing error\n", argv[0]);
        lUsage = true;
      }
      break;
    case 'u':
      if (strlen(optarg) > SrcAlias::AliasNameMax-1) {
        printf("Device alias '%s' exceeds %d chars, ignored\n", optarg, SrcAlias::AliasNameMax-1);
      } else {
        uniqueid = optarg;
      }
      break;
      //    case 'm': lMonitor = true; break;
    case 'h': lUsage = true; break;
    case 't':
      testpattern=strtoul(optarg, NULL, 0);
      break;
    case 'c': calib = true; break;
    case 'A': anyMatch = true; break;
    case '?':
    default:
      lUsage = true;
    }
  }

  if (lUsage) {
    usage(argv[0]);
    return 0;
  }
 
 char devname[16];
  sprintf(devname,"/dev/qadc%c",qadc);
  int fd = open(devname, O_RDWR);
  if (fd<0) {
    perror("Open device failed");
    return -1;
  }

  //  SEVCHK ( ca_context_create(ca_enable_preemptive_callback ), "epicsArch calling ca_context_create" );

  Pds::HSD::Module* p = Pds::HSD::Module::create(fd, fmc, LCLS);

  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //p->version();

  if(calib){
    
    int off0=0, off1=0, off2=0, off3=0, off4=0, off5=0, off6=0, off7=0; 
    FILE *cals;
    if ((cals = fopen("cals.txt","r")) == NULL){
      printf("Error! opening file");
    }
    
    fscanf(cals, "%i %i %i %i %i %i %i %i\n", &off0, &off1, &off2, &off3, &off4, &off5, &off6, &off7);
    fclose(cals);
    
    printf("OFFSET0, OFFSET1, OFFSET2, OFFSET3, OFFSET4, OFFSET5, OFFSET6, OFFSET7: %i %i %i %i %i %i %i %i\n", off0, off1, off2, off3, off4, off5, off6, off7);
    
    p->set_offset(0, off0);
    p->set_offset(1, off1);
    p->set_offset(2, off2);
    p->set_offset(3, off3);
    p->set_offset(4, off4);
    p->set_offset(5, off5);
    p->set_offset(6, off6);
    p->set_offset(7, off7);
    
    int gain0=0, gain1=0, gain2=0, gain3=0;
    FILE *gaincals;
    if ((gaincals = fopen("gaincals.txt","r")) == NULL){
      printf("Error! opening file");
    }
	  
    fscanf(gaincals, "%i %i %i %i\n", &gain0, &gain1, &gain2, &gain3);
    fclose(gaincals);
    
    printf("GAIN0, GAIN1, GAIN2, GAIN3: %i %i %i %i\n", gain0, gain1, gain2, gain3);
    
    p->set_gain(0, gain0);
    p->set_gain(1, gain1);
    p->set_gain(2, gain2);
    p->set_gain(3, gain3);
    
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////

  //  printf("Axi Version [%p]: BuildStamp[%p]: %s\n", 
  //     &(p->version), &(p->version.BuildStamp[0]), p->version.buildStamp().c_str());

  // p->i2c_sw_control.select(I2cSwitch::PrimaryFmc); 
  // p->i2c_sw_control.dump();
      
  static const DetInfo  src (info);

      // Pds::DetInfo src(p->base.partitionAddr,
      //	   info.detector(),info.detId(),
      //	   info.device  (),info.devId());

  printf("Using %s\n",Pds::DetInfo::name(src)); 

  //  CfgClientNfs* cfg = new CfgClientNfs(src);

  Pds::QuadAdc::Server*  server  = anyMatch ? 
    static_cast<Pds::QuadAdc::Server*>(new Pds::QuadAdc::AnyMatch (src, fd)) :
    static_cast<Pds::QuadAdc::Server*>(new Pds::QuadAdc::SeqMatch( src, fd));

  Pds::QuadAdc::Manager* manager = new Pds::QuadAdc::Manager(*p,
							     *server,
							     *new Pds::CfgClientNfs(src),
							     testpattern);

  Task* task = new Task(Task::MakeThisATask);
  std::list<Appliance*> apps;
 
  // max event size is 32k samples * 8 ch * 2B/sample
  StdSegWire settings(*server, uniqueid,32*1024*16, 256, false, 0, 0, true);
  EventAppCallback* seg = new EventAppCallback(task, platform, *manager);

  SegmentLevel* seglevel = new SegmentLevel(platform, settings, *seg, 0);
  seglevel->attach();

  task->mainLoop();
  return 0;
}
