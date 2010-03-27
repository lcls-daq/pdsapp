#include "pdsapp/config/SerializerDictionary.hh"

#include "pdsapp/config/Serializer.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/EvrConfig.hh"
#include "pdsapp/config/EvrConfig_V1.hh"
#include "pdsapp/config/AcqConfig.hh"
#include "pdsapp/config/IpimbConfig.hh"
#include "pdsapp/config/Opal1kConfig.hh"
#include "pdsapp/config/TM6740Config.hh"
#include "pdsapp/config/pnCCDConfig.hh"
#include "pdsapp/config/FrameFexConfig.hh"
#include "pdsapp/config/ControlConfig.hh"

#include "pds/config/EvrConfigType.hh"
#include "pds/config/AcqConfigType.hh"
#include "pds/config/IpimbConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/ControlConfigType.hh"

using namespace Pds_ConfigDb;

SerializerDEntry::SerializerDEntry(const Pds::TypeId& t,
				   Serializer* s) :
  type(t), serializer(s) {}

SerializerDEntry::~SerializerDEntry()
{
}

bool SerializerDEntry::operator==(const SerializerDEntry& s) const
{
  return type.value()==s.type.value();
}


SerializerDictionary::SerializerDictionary()
{
  enroll(_acqConfigType     ,new AcqConfig);
  enroll(_evrConfigType     ,new EvrConfig);
  enroll(_ipimbConfigType     ,new IpimbConfig);
  enroll(_opal1kConfigType  ,new Opal1kConfig);
  enroll(_tm6740ConfigType  ,new TM6740Config);
  enroll(_pnCCDConfigType   ,new pnCCDConfig);
  enroll(_frameFexConfigType,new FrameFexConfig);
  enroll(_controlConfigType ,new ControlConfig);
  //  retired
  enroll(Pds::TypeId(Pds::TypeId::Id_EvrConfig,1), new EvrConfig_V1);
}

SerializerDictionary::~SerializerDictionary()
{
  for(list<SerializerDEntry>::iterator iter=_list.begin();
      iter!=_list.end(); iter++)
    delete iter->serializer;
}

void SerializerDictionary::enroll(const Pds::TypeId& type,
				  Serializer* s)
{
  SerializerDEntry entry(type,s);
  _list.remove(entry);
  _list.push_back(entry);
}

Serializer* SerializerDictionary::lookup(const Pds::TypeId& type)
{
  for(list<SerializerDEntry>::iterator iter=_list.begin();
      iter!=_list.end(); iter++)
    if (iter->type.value()==type.value())
      return iter->serializer;
  return 0;
}
