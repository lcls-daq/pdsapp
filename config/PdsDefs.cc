#include "pdsapp/config/PdsDefs.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/EvrConfigType.hh"

#include <sstream>
using std::istringstream;
using std::ostringstream;

using namespace Pds_ConfigDb;


const Pds::TypeId* PdsDefs::typeId(ConfigType id)
{
  Pds::TypeId* type(0);
  switch(id) {
  case Evr     : type = &_evrConfigType; break;
    //  case Acq     : type = &_acqConfigType; break;
  case Opal1k  : type = &_opal1kConfigType; break;
  case FrameFex: type = &_frameFexConfigType; break;
  default: break;
  }
  return type;
}

const Pds::TypeId* PdsDefs::typeId(const UTypeName& name)
{
#define test(type) { if (name==Pds::TypeId::name(type.id())) return &type; }
  test(_evrConfigType);
  test(_opal1kConfigType);
  test(_frameFexConfigType);
#undef test
  return 0;
}

const Pds::TypeId* PdsDefs::typeId(const QTypeName& name)
{
#define test(type) { if (name==PdsDefs::qtypeName(type)) return &type; }
  test(_evrConfigType);
  test(_opal1kConfigType);
  test(_frameFexConfigType);
#undef test
  return 0;
}

UTypeName PdsDefs::utypeName(ConfigType type)
{
  return PdsDefs::utypeName(*PdsDefs::typeId(type));
}

UTypeName PdsDefs::utypeName(const Pds::TypeId& type)
{
  ostringstream o;
  o << Pds::TypeId::name(type.id());
  return UTypeName(o.str());
}

QTypeName PdsDefs::qtypeName(const Pds::TypeId& type)
{
  ostringstream o;
  o << Pds::TypeId::name(type.id()) << "_v" << type.version();
  return QTypeName(o.str());
}

QTypeName PdsDefs::qtypeName(const UTypeName& utype)
{
  return PdsDefs::qtypeName(*PdsDefs::typeId(utype));
}


