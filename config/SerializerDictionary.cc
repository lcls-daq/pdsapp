#include "pdsapp/config/SerializerDictionary.hh"

#include "pdsapp/config/Serializer.hh"

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
