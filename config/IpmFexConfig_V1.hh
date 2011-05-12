#ifndef Pds_IpmFexConfig_V1_hh
#define Pds_IpmFexConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class IpmFexTable;

  class IpmFexConfig_V1 : public Serializer {
  public:
    IpmFexConfig_V1();
    ~IpmFexConfig_V1() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    IpmFexTable* _table;
  };

};

#endif
