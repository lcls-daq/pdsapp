#include "pdsapp/blv/ToOpalBldEventWire.hh"

#include "pds/xtc/InDatagram.hh"
#include "pdsdata/psddl/camera.ddl.h"

//#define DBUG

using namespace Pds;

static const unsigned extent         = 2*sizeof(Xtc) +
  sizeof(Opal1kConfigType) + sizeof(Xtc) +
  sizeof(Camera::FrameV1) + 
  2*Opal1kConfigType::Row_Pixels*Opal1kConfigType::Column_Pixels;

ToOpalBldEventWire::ToOpalBldEventWire(Outlet&        outlet, 
                                       int            interface, 
                                       int            write_fd,
                                       const BldInfo& bld,
                                       unsigned       wait_us) :
  ToBldEventWire(outlet, interface, write_fd, bld, wait_us, extent),
  _camConfig(0,0,
             Opal1kConfigType::Twelve_bit,
             Opal1kConfigType::x1,
             Opal1kConfigType::None,
             false, false, false, 0, 0, 0)
{
}

ToOpalBldEventWire::~ToOpalBldEventWire() {}

void ToOpalBldEventWire::_handle_config(const Xtc* xtc)
{
  switch(xtc->contains.id()) {
  case TypeId::Id_Opal1kConfig:
    _camConfig = *reinterpret_cast<const Opal1kConfigType*>(xtc->payload());
    break;
  default:
    break;
  }
}

void ToOpalBldEventWire::_attach_config(InDatagram* dg)
{
  Xtc* nxtc = new ((char*)dg->xtc.alloc(sizeof(_camConfig)+sizeof(Xtc))) Xtc(_opal1kConfigType,_bld);
  memcpy(nxtc->alloc(sizeof(_camConfig)),&_camConfig,sizeof(_camConfig));
}
