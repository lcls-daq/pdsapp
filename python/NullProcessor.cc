//
//  Replaces segment level for pyrogue -> AMI
//
#include "pdsapp/python/NullProcessor.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/psddl/opal1k.ddl.h"
#include "pdsdata/xtc/TransitionId.hh"

typedef Pds::Camera::FrameFexConfigV1 FFConfig;
typedef Pds::Opal1k::ConfigV1         CamConfig;

using namespace Pds;

NullProcessor::NullProcessor(unsigned rows, 
                             unsigned columns, 
                             unsigned bitdepth, 
                             unsigned offset) :
      _rows(rows), _columns(columns), _bitdepth(bitdepth), _offset(offset),
      _srcInfo(0,DetInfo::CxiEndstation,0,DetInfo::Opal1000,0)
{
}

NullProcessor::~NullProcessor() 
{
}

Dgram* NullProcessor::configure(Dgram*         dg) {
  insert(dg, TransitionId::Configure);
  Xtc* seg = &dg->xtc;
  
  { unsigned sizeofPayload = sizeof(CamConfig);
    Xtc* src = new((char*)seg->alloc(sizeofPayload+sizeof(Xtc))) 
      Xtc(TypeId(TypeId::Type(CamConfig::TypeId),CamConfig::Version), _srcInfo);
    *new((char*)src->alloc(sizeofPayload))   
      CamConfig(0, 0, 
                CamConfig::Twelve_bit, 
                CamConfig::x1, 
                CamConfig::None, 
                0, 0, 0, 0, 0, 0); }
  { unsigned sizeofPayload = sizeof(FFConfig);
    Xtc* src = new((char*)seg->alloc(sizeofPayload+sizeof(Xtc))) 
      Xtc(TypeId(TypeId::Id_FrameFexConfig,1), _srcInfo);
    *new((char*)src->alloc(sizeofPayload)) 
      FFConfig(FFConfig::RegionOfInterest, 1, FFConfig::NoProcessing,
               Pds::Camera::FrameCoord(0,0), 
               Pds::Camera::FrameCoord(_columns,_rows),
               0, 0, 0); }
  return dg;
}

Dgram* NullProcessor::event(Dgram*      dg,
                            const char* source) {
  insert(dg, TransitionId::L1Accept);
  Xtc* seg = &dg->xtc;

  unsigned sizeofPayload = _rows*_columns*2 + 16;
  Xtc* src = new((char*)seg->alloc(sizeofPayload+sizeof(Xtc))) 
    Xtc(TypeId(TypeId::Id_Frame,1), _srcInfo);
  uint32_t* p  = (uint32_t*)new((char*)src->alloc(sizeofPayload)) 
    char[sizeofPayload];
  //  Prepend FrameV1 header
  p[0] = _columns;
  p[1] = _rows;
  p[2] = _bitdepth;
  p[3] = 0; // offset
  const uint32_t mask = (1<<_bitdepth)-1;
  const unsigned npixels = _rows*_columns;
  uint16_t* q = reinterpret_cast<uint16_t*>(p+4);
  const uint16_t* r = reinterpret_cast<const uint16_t*>(source+_offset);
  for(unsigned i=0; i<npixels; i++)
    q[i] = r[i]&mask;
  return dg;
}
