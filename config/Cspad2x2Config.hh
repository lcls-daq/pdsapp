#ifndef Pds_Cspad2x2Config_hh
#define Pds_Cspad2x2Config_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class Cspad2x2ConfigTable;

  class Cspad2x2Config : public Serializer {
  public:
    Cspad2x2Config();
    ~Cspad2x2Config() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    Cspad2x2ConfigTable* _table;
  };

};

#endif
