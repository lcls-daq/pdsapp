#ifndef Pds_CspadConfig_V2_hh
#define Pds_CspadConfig_V2_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class CspadConfigTable_V2;

  class CspadConfig_V2 : public Serializer {
  public:
    CspadConfig_V2();
    ~CspadConfig_V2() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    CspadConfigTable_V2* _table;
  };

};

#endif
