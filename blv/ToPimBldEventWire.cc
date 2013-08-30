#include "pdsapp/blv/ToPimBldEventWire.hh"

#include "pds/xtc/InDatagram.hh"
#include "pdsdata/psddl/camera.ddl.h"

//#define DBUG

using namespace Pds;

static const unsigned extent         = 2*sizeof(Xtc) +
  sizeof(TM6740ConfigType) + sizeof(Xtc) +
  sizeof(PimImageConfigType) + sizeof(Xtc) + 
  sizeof(Camera::FrameV1) + 
  2*Pulnix::TM6740ConfigV2::Row_Pixels*Pulnix::TM6740ConfigV2::Column_Pixels;

ToPimBldEventWire::ToPimBldEventWire(Outlet&        outlet, 
                                     int            interface, 
                                     int            write_fd,
                                     const BldInfo& bld,
                                     unsigned       wait_us) :
  ToBldEventWire(outlet, interface, write_fd, bld, wait_us, extent),
  _camConfig(0,0,0,0,false,
             TM6740ConfigType::Eight_bit,
             TM6740ConfigType::x1,
             TM6740ConfigType::x1,
             TM6740ConfigType::Linear)
{
}

ToPimBldEventWire::~ToPimBldEventWire() {}

void ToPimBldEventWire::_handle_config(const Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_TM6740Config:
    _camConfig = *reinterpret_cast<const Pulnix::TM6740ConfigV2*>(xtc->payload());
    break;
  case TypeId::Id_PimImageConfig:
    _pimConfig = *reinterpret_cast<const Lusi::PimImageConfigV1*>(xtc->payload());
    break;
  default:
    break;
  }
}

void ToPimBldEventWire::_attach_config(InDatagram* dg)
{
  {
    Xtc* nxtc = new ((char*)dg->xtc.alloc(sizeof(_camConfig)+sizeof(Xtc))) Xtc(_tm6740ConfigType,_bld);
    memcpy(nxtc->alloc(sizeof(_camConfig)),&_camConfig,sizeof(_camConfig));
  }
  {
    Xtc* nxtc = new ((char*)dg->xtc.alloc(sizeof(_pimConfig)+sizeof(Xtc))) Xtc(_pimImageConfigType,_bld);
    memcpy(nxtc->alloc(sizeof(_pimConfig)),&_pimConfig,sizeof(_pimConfig));
  }
}
