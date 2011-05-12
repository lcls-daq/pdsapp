#ifndef Pds_DiodeFexConfig_V1_hh
#define Pds_DiodeFexConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class DiodeFexTable;

  class DiodeFexConfig_V1 : public Serializer {
  public:
    DiodeFexConfig_V1();
    ~DiodeFexConfig_V1() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    DiodeFexTable* _table;
  };

};

#endif
