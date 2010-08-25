#ifndef Pds_CspadConfig_hh
#define Pds_CspadConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class CspadConfigTable;

  class CspadConfig : public Serializer {
  public:
    CspadConfig();
    ~CspadConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    CspadConfigTable* _table;
  };

};

#endif
