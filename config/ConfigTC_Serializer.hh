#ifndef Pds_CfgSerializer_hh
#define Pds_CfgSerializer_hh

#include "ConfigTC_Parameters.hh"

namespace ConfigGui {

  class Serializer {
  public:
    Serializer(const char* l) : label(l) {}
    virtual ~Serializer() {}
  public:
    virtual bool          readParameters (void* from) = 0;
    virtual int           writeParameters(void* to) = 0;
  public:
    const char*                label;
    Pds::LinkedList<Parameter> pList;
  };

};

#endif
