#ifndef Pds_IpmFexConfig_hh
#define Pds_IpmFexConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class IpmFexTable;

  class IpmFexConfig : public Serializer {
  public:
    IpmFexConfig();
    ~IpmFexConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    IpmFexTable* _table;
  };

};

#endif
