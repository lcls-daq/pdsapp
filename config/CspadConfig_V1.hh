#ifndef Pds_CspadConfig_V1_hh
#define Pds_CspadConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class CspadConfigTable_V1;

  class CspadConfig_V1 : public Serializer {
  public:
    CspadConfig_V1();
    ~CspadConfig_V1() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    CspadConfigTable_V1* _table;
  };

};

#endif
