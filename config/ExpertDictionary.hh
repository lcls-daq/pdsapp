#ifndef Pds_ExpertDictionary_hh
#define Pds_ExpertDictionary_hh

#include "pdsapp/config/SerializerDictionary.hh"

namespace Pds_ConfigDb {

  class ExpertDictionary : public SerializerDictionary {
  public:
    ExpertDictionary();
    ~ExpertDictionary();
  public:
    virtual Serializer* lookup(const Pds::TypeId& type);
  };

};

#endif
