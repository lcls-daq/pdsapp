#ifndef Pds_DiodeFexConfig_hh
#define Pds_DiodeFexConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class DiodeFexTable;

  class DiodeFexConfig : public Serializer {
  public:
    DiodeFexConfig();
    ~DiodeFexConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    DiodeFexTable* _table;
  };

};

#endif
