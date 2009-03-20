#ifndef Pds_Serializer_hh
#define Pds_Serializer_hh

#include "pdsapp/config/Parameters.hh"

namespace Pds_ConfigDb {

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
