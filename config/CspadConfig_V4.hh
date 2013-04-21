#ifndef Pds_CspadConfig_V4_hh
#define Pds_CspadConfig_V4_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class CspadConfigTable_V4;

  class CspadConfig_V4 : public Serializer {
  public:
    CspadConfig_V4();
    ~CspadConfig_V4() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    CspadConfigTable_V4* _table;
  };

};

#endif
