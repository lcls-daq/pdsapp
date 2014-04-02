#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <new>
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/utility/Transition.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"

using namespace Pds;

typedef Pds::Epix::ConfigV1 EpixConfigType;

static const TypeId  _cfgType(TypeId::Id_EpixConfig,1);
static const TypeId  _evtType(TypeId::Id_EpixElement,1);
static const DetInfo _src    (0,
                              DetInfo::AmoEndstation,0,
                              DetInfo::Epix,0);

static char* cfg_buff = 0;
static char* evt_buff = 0;

class EpixConfig : public Xtc {
public:
  EpixConfig(unsigned nrows, unsigned ncols, 
	     unsigned nxrows) :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    Xtc* xtc = new(this) Xtc(_cfgType,_src);

    const unsigned AsicsPerRow    = 2;
    const unsigned AsicsPerColumn = 2;
    const unsigned Asics = AsicsPerRow*AsicsPerColumn;
    Epix::AsicConfigV1 asics[Asics];
    const unsigned Columns = ncols;
    const unsigned Rows    = nrows;
    uint32_t* testarray = new uint32_t[Asics*Rows*(Columns+31)/32];
    memset(testarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
    uint32_t* maskarray = new uint32_t[Asics*Rows*(Columns+31)/32];
    memset(maskarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
    unsigned asicMask = 0xf;

    EpixConfigType* cfg = 
      new (xtc->next()) EpixConfigType( 0, 1, 0, 1, 0, // version
                                        0, 0, 0, 0, 0, // asicAcq
                                        0, 0, 0, 0, 0, // asicGRControl
                                        0, 0, 0, 0, 0, // asicR0Control
                                        0, 0, 0, 0, 0, // asicR0ToAsicAcq
                                        1, 0, 0, 0, 0, // adcClkHalfT
                                        0, 0, 0, 0, nxrows, // digitalCardId0
                                        AsicsPerRow, AsicsPerColumn, 
                                        Rows, Columns, 
                                        0x200000, // 200MHz
                                        asicMask,
                                        asics, testarray, maskarray );

    if (cfg_buff) delete[] cfg_buff;
    cfg_buff = new char[cfg->_sizeof()];
    memcpy(cfg_buff,cfg,cfg->_sizeof());

    if (evt_buff) delete[] evt_buff;
    evt_buff = new char[Pds::Epix::ElementV1::_sizeof(*cfg)];

    xtc ->alloc(cfg->_sizeof());
    this->alloc(cfg->_sizeof());
  }
};

class EpixData : public Xtc {
public:
  EpixData(FILE* inf) :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    const EpixConfigType& cfg = *reinterpret_cast<const EpixConfigType*>(cfg_buff);

    Xtc* xtc = new(this) Xtc(_evtType,_src);
    size_t sz = Pds::Epix::ElementV1::_sizeof(cfg);
    fread(evt_buff, sz, 1, inf);

    Pds::Epix::ElementV1* o =  reinterpret_cast<Pds::Epix::ElementV1*>(xtc->alloc(sz));
    Pds::Epix::ElementV1& e = *reinterpret_cast<Pds::Epix::ElementV1*>(evt_buff);
    memcpy(o, &e, sizeof(e));

    //  frame data
    unsigned nrows = cfg.numberOfRows()/2;
    ndarray<const uint16_t,2> iframe = e. frame(cfg);
    ndarray<const uint16_t,2> oframe = o->frame(cfg);
    for(unsigned i=0; i<nrows; i++) {
      memcpy(const_cast<uint16_t*>(&oframe[nrows+i+0][0]), &iframe[2*i+0][0], cfg.numberOfColumns()*sizeof(uint16_t));
      memcpy(const_cast<uint16_t*>(&oframe[nrows-i-1][0]), &iframe[2*i+1][0], cfg.numberOfColumns()*sizeof(uint16_t));
    }

    //  after frame data
    unsigned tsz = reinterpret_cast<const uint8_t*>(&e) + e._sizeof(cfg) - reinterpret_cast<const uint8_t*>(iframe.end());
    memcpy(const_cast<uint16_t*>(oframe.end()), iframe.end(), tsz);
  
    alloc(sz);
  }
};

static Datagram* createTransition(Pool& pool, TransitionId::Value id)
{
  static unsigned evrcnt = 0;
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  ClockTime clocktime(tp.tv_sec, tp.tv_nsec);
  unsigned pulseId = -1;
  unsigned vector = 0;
  if (id==TransitionId::L1Accept) {
    vector = evrcnt++;
  }
  TimeStamp timestamp(0, pulseId, vector);
  Sequence seq(Sequence::Event, id, clocktime, timestamp);
  Transition tr(id, Transition::Record, seq, Env(0));
  Datagram* dg = new(&pool) Datagram(tr, _xtcType, Src(Level::Event));
  return dg;
}

int main(int argc,char **argv)
{
  const char* ifname = 0;
  const char* ofname = "e0-r0000-s00-c00.xtc";
  unsigned nevent = -1;
  unsigned nxrows = 0;
  unsigned nrows = 50;
  unsigned ncols = 192;

  int c;
  while ( (c=getopt( argc, argv, "i:o:n:x:r:c:h")) != EOF ) {
    switch(c) {
    case 'i':
      ifname = optarg;
      break;
    case 'o':
      ofname = optarg;
      break;
    case 'n':
      nevent = strtoul(optarg,NULL,0);
      break;
    case 'x':
      nxrows = strtoul(optarg,NULL,0);
      break;
    case 'r':
      nrows = strtoul(optarg,NULL,0);
      break;
    case 'c':
      ncols = strtoul(optarg,NULL,0);
      break;
    case 'h':
    default:
      printf("Usage: %s -i <filename> [-o <filename>] [-n <nevents]\n",argv[0]);
      return 1;
    }
  }
      
  FILE* inf  = fopen(ifname,"r");
  FILE* outf = fopen(ofname,"w");
  
  GenericPool* pool = new GenericPool(1<<24,1);

  Datagram* dg;

  dg = createTransition(*pool,TransitionId::Configure);
  dg->xtc.extent += (new (&dg->xtc) EpixConfig(nrows,ncols,nxrows)   )->extent;
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginRun);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginCalibCycle);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  dg = createTransition(*pool,TransitionId::Enable);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  while (nevent--) {
    dg = createTransition(*pool,TransitionId::L1Accept);
    dg->xtc.extent += (new (&dg->xtc) EpixData(inf))->extent;
    fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
    delete dg;
  }

  dg = createTransition(*pool,TransitionId::Disable);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndCalibCycle);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndRun);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,outf);
  delete dg;

  fclose(outf);

  return(0);
}
