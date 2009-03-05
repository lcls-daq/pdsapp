#ifndef Pds_CfgSerializer_hh
#define Pds_CfgSerializer_hh

#include "pds/service/LinkedList.hh"

namespace ConfigGui {

  class Parameter;

  class Serializer {
  public:
    Serializer() {}
    virtual ~Serializer() {}
  public:
    virtual bool          readParameters (void* from) = 0;
    virtual int           writeParameters(void* to) = 0;
    Pds::LinkedList<Parameter> parameterList;
  };

};

#endif
