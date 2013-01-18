#ifndef Pds_Cspad2x2Config_V1_hh
#define Pds_Cspad2x2Config_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class Cspad2x2ConfigTable_V1;

  class Cspad2x2Config_V1 : public Serializer {
  public:
    Cspad2x2Config_V1();
    ~Cspad2x2Config_V1() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    Cspad2x2ConfigTable_V1* _table;
  };

};

#endif
