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
                              DetInfo::CxiEndstation,0,
                              DetInfo::Epix,0);

static double ranuni() { return double(random())/double(RAND_MAX); }

static char* cfg_buff = 0;

class EpixConfig : public Xtc {
public:
  EpixConfig() :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    Xtc* xtc = new(this) Xtc(_cfgType,_src);

    const unsigned AsicsPerRow    = 2;
    const unsigned AsicsPerColumn = 2;
    const unsigned Asics = AsicsPerRow*AsicsPerColumn;
    Epix::AsicConfigV1 asics[Asics];
    const unsigned Columns = 4*96;
    const unsigned Rows    = 4*88;
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
                                        0, 0, 0, 0, 0, // digitalCardId0
                                        AsicsPerRow, AsicsPerColumn, 
                                        Rows, Columns, 
                                        0x200000, // 200MHz
                                        asicMask,
                                        asics, testarray, maskarray );

    if (cfg_buff) delete[] cfg_buff;
    cfg_buff = new char[cfg->_sizeof()];
    memcpy(cfg_buff,cfg,cfg->_sizeof());

    xtc ->alloc(cfg->_sizeof());
    this->alloc(cfg->_sizeof());
  }
};

class EpixData : public Xtc {
public:
  EpixData() :
    Xtc(_xtcType,Src(Level::Segment)) 
  {
    const EpixConfigType& cfg = *reinterpret_cast<const EpixConfigType*>(cfg_buff);

    Xtc* xtc = new(this) Xtc(_evtType,_src);
    Pds::Epix::ElementV1* elem = new(xtc->next()) Pds::Epix::ElementV1;
    xtc->alloc(elem->_sizeof(cfg));

    ndarray<const uint16_t,2> cf = elem->frame(cfg);
    ndarray<uint16_t,2> frame = make_ndarray(const_cast<uint16_t*>(cf.data()),
                                             cf.shape()[0], cf.shape()[1]);

    for(unsigned i=0; i<frame.shape()[0]; i++)
      for(unsigned j=0; j<frame.shape()[1]; j++)
        frame[i][j] = uint16_t(ranuni()*double(1<<14));

    alloc(elem->_sizeof(cfg));
  }
};

static Datagram* createTransition(Pool& pool, TransitionId::Value id)
{
  Transition tr(id, Env(0));
  Datagram* dg = new(&pool) Datagram(tr, _xtcType, Src(Level::Event));
  return dg;
}

int main(int argc,char **argv)
{
  unsigned nevent=5;
  const char* fname = "e0-r0000-s00-c00.xtc";
  
  int c;
  while ( (c=getopt( argc, argv, "f:n:h")) != EOF ) {
    switch(c) {
    case 'f':
      fname = optarg;
      break;
    case 'n':
      nevent = strtoul(optarg,NULL,0);
      break;
    case 'h':
    default:
      printf("Usage: %s -n <nevents> -f <filename>\n",argv[0]);
      return 1;
    }
  }
      
  FILE* f = fopen(fname,"w");
  
  GenericPool* pool = new GenericPool(1<<24,1);

  Datagram* dg;

  dg = createTransition(*pool,TransitionId::Configure);
  dg->xtc.extent += (new (&dg->xtc) EpixConfig   )->extent;
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginRun);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::BeginCalibCycle);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::Enable);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  while (nevent--) {
    dg = createTransition(*pool,TransitionId::L1Accept);
    dg->xtc.extent += (new (&dg->xtc) EpixData)->extent;
    fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
    delete dg;
  }

  dg = createTransition(*pool,TransitionId::Disable);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndCalibCycle);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::EndRun);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  dg = createTransition(*pool,TransitionId::Unconfigure);
  fwrite(dg,sizeof(Dgram)+dg->xtc.sizeofPayload(),1,f);
  delete dg;

  fclose(f);

  return(0);
}
