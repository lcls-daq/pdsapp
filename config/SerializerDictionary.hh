#ifndef Pds_SerializerDictionary_hh
#define Pds_SerializerDictionary_hh

#include "pdsdata/xtc/TypeId.hh"

#include <list>
using std::list;

namespace Pds_ConfigDb {

  class Serializer;

  class SerializerDEntry {
  public:
    SerializerDEntry(Pds::TypeId::Type, Serializer*);
    ~SerializerDEntry();
  public:
    bool operator==(const SerializerDEntry&) const;
    Pds::TypeId::Type type;
    Serializer*       serializer;
  };

  class SerializerDictionary {
  public:
    SerializerDictionary();
    ~SerializerDictionary();
  public:
    Serializer* lookup(Pds::TypeId::Type type);
    void enroll(Pds::TypeId::Type, Serializer*);
  private:
    list<SerializerDEntry> _list;
  };

};

#endif
