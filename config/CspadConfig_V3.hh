#ifndef Pds_CspadConfig_V3_hh
#define Pds_CspadConfig_V3_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class CspadConfigTable_V3;

  class CspadConfig_V3 : public Serializer {
  public:
    CspadConfig_V3();
    ~CspadConfig_V3() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    CspadConfigTable_V3* _table;
  };

};

#endif
