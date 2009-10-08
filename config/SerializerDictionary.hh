#ifndef Pds_SerializerDictionary_hh
#define Pds_SerializerDictionary_hh

#include "pdsdata/xtc/TypeId.hh"

#include <list>
using std::list;

namespace Pds_ConfigDb {

  class Serializer;

  class SerializerDEntry {
  public:
    SerializerDEntry(const Pds::TypeId&, Serializer*);
    ~SerializerDEntry();
  public:
    bool operator==(const SerializerDEntry&) const;
    Pds::TypeId       type;
    Serializer*       serializer;
  };

  class SerializerDictionary {
  public:
    SerializerDictionary();
    ~SerializerDictionary();
  public:
    Serializer* lookup(const Pds::TypeId& type);
    void enroll(const Pds::TypeId&, Serializer*);
  private:
    list<SerializerDEntry> _list;
  };

};

#endif
