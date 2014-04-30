#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/xtc/XtcType.hh"
#include "pds/utility/Transition.hh"

#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <list>
#include <queue>

using namespace Pds;

//static ProcInfo _proc(Level::Segment,1,0x7f000001);
static DetInfo  _src (0,DetInfo::NoDetector,0,DetInfo::Epix,0);
static char* _buffer = new char[0x1000];

void _write(FILE* f, TransitionId::Value id, const Xtc* ixtc=0)
{
  Transition tr(id,Env(0));
  Dgram* dg = (Dgram*)_buffer;
  new ((char*)&dg->seq) Sequence(tr.sequence());
  new ((char*)&dg->env) Env(tr.env());
  new ((char*)&dg->xtc) Xtc(_xtcType,Src(Level::Event));
  if (ixtc) {
    Xtc* xtc = new((char*)dg->xtc.alloc(ixtc->extent)) Xtc(*ixtc);
    xtc->alloc(ixtc->sizeofPayload());
    fwrite(dg, sizeof(Dgram)+dg->xtc.sizeofPayload()-ixtc->sizeofPayload(),1,f);
    //    printf("wrote 0x%x bytes\n",sizeof(Dgram)+dg->xtc.sizeofPayload()-ixtc->sizeofPayload());
    fwrite(ixtc->payload(),ixtc->sizeofPayload(),1,f);
    //    printf("wrote 0x%x bytes\n",ixtc->sizeofPayload());
  }
  else {
    fwrite(dg, sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
    //    printf("wrote 0x%x bytes\n",sizeof(Dgram)+dg->xtc.sizeofPayload());
  }
}

static TypeId _EpixConfigType(TypeId::Id_EpixConfig,1);
typedef Epix::ConfigV1 EpixConfigType;

static TypeId _EpixDataType  (TypeId::Id_EpixElement,1);
typedef Epix::ElementV1 EpixDataType;

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
  const unsigned Rows=98, Columns=96*4;

  const unsigned Asics = AsicsPerRow*AsicsPerColumn;
  const unsigned CfgSize = sizeof(Epix::ConfigV1)
    +Asics*Epix::AsicConfigV1::_sizeof()          // AsicsConfigV1
    +2*Asics*Rows*((Columns+31)/32)*sizeof(uint32_t) // PixelArrays [Test,Mask]
    +sizeof(Xtc);

  char* _cfgpayload = new char[CfgSize];
  Xtc* _cfgtc = new(_cfgpayload) Xtc(_EpixConfigType,_src);
  
  Epix::AsicConfigV1 asics[Asics];
  uint32_t* testarray = new uint32_t[Asics*Rows*(Columns+31)/32];
  memset(testarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
  uint32_t* maskarray = new uint32_t[Asics*Rows*(Columns+31)/32];
  memset(maskarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
  unsigned asicMask = 0xf;
    
  EpixConfigType* cfg = 
    new (_cfgtc->next()) EpixConfigType( 0, 1, 0, 1, 0, // version
                                         0, 0, 0, 0, 0, // asicAcq
                                         0, 0, 0, 0, 0, // asicGRControl
                                         0, 0, 0, 0, 0, // asicR0Control
                                         0, 0, 0, 0, 0, // asicR0ToAsicAcq
                                         1, 0, 0, 0, 0, // adcClkHalfT
                                         0, 0, 0, 0, 2, // digitalCardId0
                                         AsicsPerRow, AsicsPerColumn, 
                                         Rows, Columns, 
                                         0x200000, // 200MHz
                                         asicMask,
                                         asics, testarray, maskarray );
  _cfgtc->alloc(cfg->_sizeof());

  const size_t sz = EpixDataType::_sizeof(*cfg);
  unsigned evtsz = sz + sizeof(Xtc);
  unsigned evtst = (evtsz+3)&~3;
  char* _evtpayload = new char[evtst];
  char* _evtraw     = new char[evtst];
  Xtc* _evttc = new (_evtpayload) Xtc(_EpixDataType,_src);
  EpixDataType* q = new (_evttc->alloc(sz)) EpixDataType;
  EpixDataType* e = reinterpret_cast<EpixDataType*>(_evtraw);

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
      printf("File %s: %d [%d] bytes per event\n",it->c_str(),esiz*sizeof(uint32_t),sz);
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
