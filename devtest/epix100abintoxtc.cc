#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/xtc/XtcType.hh"
#include "pds/utility/Transition.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixDataType.hh"

#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <list>
#include <queue>

using namespace Pds;

//static ProcInfo _proc(Level::Segment,1,0x7f000001);
static DetInfo  _src (0,DetInfo::NoDetector,0,DetInfo::Epix100a,99);
static char* _buffer = new char[0x1000];
unsigned pulseId=0;

namespace Pds {
class TimeStampApp {
public:
  static void stamp(Transition& tr) { 
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    ClockTime clocktime(tp.tv_sec, tp.tv_nsec);
    TimeStamp timestamp(0, pulseId++, 0);
    tr._sequence = Sequence(Sequence::Event, tr.id(), clocktime, timestamp);
  }
};
};

void _write(FILE* f, TransitionId::Value id, const Xtc* ixtc=0)
{
  Transition tr(id,Env(0));
  Pds::TimeStampApp::stamp(tr);
  Dgram* dg = (Dgram*)_buffer;
  new ((char*)&dg->seq) Sequence(tr.sequence());
  new ((char*)&dg->env) Env(tr.env());
  new ((char*)&dg->xtc) Xtc(_xtcType,Src(Level::Event));
  if (ixtc) {
    Xtc* xtc = new((char*)dg->xtc.alloc(ixtc->extent)) Xtc(*ixtc);
    xtc->alloc(ixtc->sizeofPayload());
    fwrite(dg, sizeof(Dgram)+dg->xtc.sizeofPayload()-ixtc->sizeofPayload(),1,f);
    fwrite(ixtc->payload(),ixtc->sizeofPayload(),1,f);
  }
  else {
    fwrite(dg, sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  }
}

int main(int argc, char* argv[]) {
  // Exactly one of these values must be supplied
  std::list<std::string> inFiles;
  std::string outFile("out.xtc");
  unsigned nevents=-1;

  int c;
  while ((c = getopt(argc, argv, "i:o:n:vh?")) != -1) {
    switch (c) {
    case 'i':
      inFiles.push_back(std::string(optarg));
      break;
    case 'o':
      outFile = std::string(optarg); 
      break;
    case 'n':
      nevents = strtoul(optarg,0,0);
      break;
    case 'v':
      //        verbose = true;
      break;
    case 'h':
    case '?':
      //        usage(argv[0]);
      exit(0);
    default:
      fprintf(stderr, "Unrecognized option -%c!\n", c);
      //usage(argv[0]);
      exit(0);
    }
  }

  const unsigned AsicsPerColumn=2, AsicsPerRow=2;
  const unsigned Rows=354, Columns=384, Excluded=2;
  //  const unsigned asicMask=0x1;
  const unsigned asicMask=0xf;
  
  const unsigned Asics = AsicsPerRow*AsicsPerColumn;
  Epix100aConfigType* cfgt = new Epix100aConfigType(AsicsPerRow,
                                                    AsicsPerColumn,
                                                    Rows, Columns, Excluded);

  /*
#define FILL_ROW(v) {						\
    uint16_t* p = const_cast<uint16_t*>(&array[i][j][0]);	\
    for(unsigned k=0; k<array.shape()[2]; k++)			\
      p[k] = v; }						\

  ndarray<const uint16_t,2> array = cfgt->asicPixelConfigArray();
  for(unsigned i=0; i<array.shape()[0]; i++) {
    unsigned j=0;
    do { FILL_ROW(2) } while(++j<1);
    do { FILL_ROW(0) } while(++j<24);
    do { FILL_ROW(4) } while(++j<48);
    do { FILL_ROW(0) } while(++j<51);
  }
  */

  const unsigned CfgSize = cfgt->_sizeof() + sizeof(Xtc);

  char* _cfgpayload = new char[CfgSize];
  Xtc* _cfgtc = new(_cfgpayload) Xtc(_epix100aConfigType,_src);
  
  Epix::Asic100aConfigV1 asics[Asics];
    
  Epix100aConfigType* cfg = 
    new (_cfgtc->next()) Epix100aConfigType( 0, 0, 0, 0, 0, // version
                                             0, 0, 0,
                                             0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, // asicAcq
                                             0, 0, 0, 0, 0, // asicGRControl
                                             0, 0, 0, 0, 0, // asicR0Control
                                             0, 0, 0, 0, 0, // asicR0ToAsicAcq
                                             1, 0, 0, 0, 0, // adcClkHalfT
                                             0, 0, 0, 0, 0, // digitalCardId0
                                             0, 0, 0, 0, 2, // digitalCardId0
                                             AsicsPerRow, AsicsPerColumn, 
                                             Rows, Rows, Columns, 0, 1, 
                                             0x200000, // 200MHz
                                             asicMask, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // scope
                                             asics, 0, 0 );
  _cfgtc->alloc(cfg->_sizeof());

  delete cfgt;

  const size_t sz = Epix100aDataType::_sizeof(*cfg);
  printf("data type sz [%d]: asics per row [%d]: asics per col [%d]: rows [%d]: cols [%d]\n",
         sz, cfg->numberOfAsicsPerRow(), cfg->numberOfAsicsPerColumn(),
         cfg->numberOfRowsPerAsic(), cfg->numberOfPixelsPerAsicRow());
  printf("calib rows [%d]: env rows [%d]\n",cfg->numberOfCalibrationRows(),cfg->numberOfEnvironmentalRows());

  unsigned evtsz = sz + sizeof(Xtc);
  unsigned evtst = (evtsz+3)&~3;
  char* _evtpayload = new char[evtst];
  char* _evtraw     = new char[evtst];
  Xtc* _evttc = new (_evtpayload) Xtc(_epix100aDataType,_src);
  Epix100aDataType* q = new (_evttc->alloc(sz)) Epix100aDataType;
  Epix100aDataType* e = reinterpret_cast<Epix100aDataType*>(_evtraw);

  FILE* f = fopen(outFile.c_str(),"w");
  _write(f, TransitionId::Configure, _cfgtc);
  _write(f, TransitionId::BeginRun);
  _write(f, TransitionId::BeginCalibCycle);
  _write(f, TransitionId::Enable);
  for(std::list<std::string>::iterator it=inFiles.begin(); it!=inFiles.end(); it++) {
    FILE* g = fopen(it->c_str(),"r");
    if (g) {
      uint32_t esiz;
      fread(&esiz, sizeof(esiz), 1, g);
      printf("File %s: %zd [%zd] bytes per event\n",it->c_str(),esiz*sizeof(uint32_t),sz);
      while(!feof(g) && nevents) {
        fread(_evtraw, esiz, sizeof(uint32_t), g);

        //  header
        memcpy(q, e, sizeof(*e));

        //  frame data
        unsigned nrows = cfg->numberOfRows()/2;
        ndarray<const uint16_t,2> iframe = e->frame(*cfg);
        ndarray<const uint16_t,2> oframe = q->frame(*cfg);
        for(unsigned i=0; i<nrows; i++) {
          memcpy(const_cast<uint16_t*>(&oframe[nrows+i+0][0]), &iframe[2*i+0][0], cfg->numberOfColumns()*sizeof(uint16_t));
          memcpy(const_cast<uint16_t*>(&oframe[nrows-i-1][0]), &iframe[2*i+1][0], cfg->numberOfColumns()*sizeof(uint16_t));
        }

        //  after frame data
        unsigned tsz = reinterpret_cast<const uint8_t*>(e) + e->_sizeof(*cfg) - reinterpret_cast<const uint8_t*>(iframe.end());
        memcpy(const_cast<uint16_t*>(oframe.end()), iframe.end(), tsz);
        
        _write(f, TransitionId::L1Accept, _evttc);
        fread(&esiz, sizeof(esiz), 1, g);
        nevents--;
      }
      fclose(g);
    }
  }
  _write(f, TransitionId::Disable);
  _write(f, TransitionId::EndCalibCycle);
  _write(f, TransitionId::EndRun);
  fclose(f);
  return 0;
}
