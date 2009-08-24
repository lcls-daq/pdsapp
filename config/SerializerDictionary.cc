#include "pdsapp/config/SerializerDictionary.hh"

#include "pdsapp/config/Serializer.hh"

#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/EvrConfig.hh"
#include "pdsapp/config/AcqConfig.hh"
#include "pdsapp/config/Opal1kConfig.hh"
#include "pdsapp/config/TM6740Config.hh"
#include "pdsapp/config/FrameFexConfig.hh"
#include "pdsapp/config/ControlConfig.hh"

using namespace Pds_ConfigDb;

SerializerDEntry::SerializerDEntry(Pds::TypeId::Type t,
				   Serializer* s) :
  type(t), serializer(s) {}

SerializerDEntry::~SerializerDEntry()
{
}

bool SerializerDEntry::operator==(const SerializerDEntry& s) const
{
  return type==s.type;
}


SerializerDictionary::SerializerDictionary()
{
  enroll(Pds::TypeId::Id_AcqConfig,new AcqConfig);
  enroll(Pds::TypeId::Id_EvrConfig,new EvrConfig);
  enroll(Pds::TypeId::Id_Opal1kConfig,new Opal1kConfig);
  enroll(Pds::TypeId::Id_TM6740Config,new TM6740Config);
  enroll(Pds::TypeId::Id_FrameFexConfig,new FrameFexConfig);
  enroll(Pds::TypeId::Id_ControlConfig ,new ControlConfig);
}

SerializerDictionary::~SerializerDictionary()
{
  for(list<SerializerDEntry>::iterator iter=_list.begin();
      iter!=_list.end(); iter++)
    delete iter->serializer;
}

void SerializerDictionary::enroll(Pds::TypeId::Type type,
				  Serializer* s)
{
  SerializerDEntry entry(type,s);
  _list.remove(entry);
  _list.push_back(entry);
}

Serializer* SerializerDictionary::lookup(Pds::TypeId::Type type)
{
  for(list<SerializerDEntry>::iterator iter=_list.begin();
      iter!=_list.end(); iter++)
    if (iter->type==type)
      return iter->serializer;
  return 0;
}
