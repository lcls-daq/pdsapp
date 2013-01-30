#ifndef Pds_SerializerDictionary_hh
#define Pds_SerializerDictionary_hh

#include "pdsdata/xtc/TypeId.hh"

namespace Pds_ConfigDb {

  class Serializer;

  class SerializerDictionary {
  public:
    SerializerDictionary();
    virtual ~SerializerDictionary();
  public:
    virtual Serializer* lookup(const Pds::TypeId& type);
  };

};

#endif
