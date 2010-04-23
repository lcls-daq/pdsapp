#include "pdsapp/config/PdsDefs.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/FccdConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/ControlConfigType.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/PrincetonConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/EncoderConfigType.hh"

#include <sstream>
using std::istringstream;
using std::ostringstream;

using namespace Pds_ConfigDb;


const Pds::TypeId* PdsDefs::typeId(ConfigType id)
{ 
  Pds::TypeId* type(0);
  switch(id) {
  case Encoder    : type = &_encoderConfigType;   break;
  case Evr        : type = &_evrConfigType;       break;
  case Acq        : type = &_acqConfigType;       break;
  case Opal1k     : type = &_opal1kConfigType;    break;
  case Fccd       : type = &_fccdConfigType;      break;
  case TM6740     : type = &_tm6740ConfigType;    break;
  case FrameFex   : type = &_frameFexConfigType;  break;
  case pnCCD      : type = &_pnCCDConfigType;     break;
  case Princeton  : type = &_princetonConfigType; break;
  case Ipimb      : type = &_ipimbConfigType;     break;
  case RunControl : type = &_controlConfigType;   break;
  default: 
    printf("PdsDefs::typeId id %d not found\n",unsigned(id));
    break;
  }
  return type;
}

const Pds::TypeId* PdsDefs::typeId(const UTypeName& name)
{
#define test(type) { if (name==Pds::TypeId::name(type.id())) return &type; }
  test(_evrConfigType);
  test(_acqConfigType);
  test(_ipimbConfigType);
  test(_encoderConfigType);
  test(_opal1kConfigType);
  test(_fccdConfigType);
  test(_tm6740ConfigType);
  test(_pnCCDConfigType);
  test(_frameFexConfigType);
  test(_controlConfigType);
  test(_princetonConfigType);    
#undef test
  printf("PdsDefs::typeId id %s not found\n",name.data());
  return 0;
}

const Pds::TypeId* PdsDefs::typeId(const QTypeName& name)
{
#define test(type) { if (name==PdsDefs::qtypeName(type)) return &type; }
  test(_evrConfigType);
  test(_acqConfigType);
  test(_ipimbConfigType);
  test(_encoderConfigType);
  test(_opal1kConfigType);
  test(_fccdConfigType);
  test(_tm6740ConfigType);
  test(_pnCCDConfigType);
  test(_frameFexConfigType);
  test(_controlConfigType);
  test(_princetonConfigType);    
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


