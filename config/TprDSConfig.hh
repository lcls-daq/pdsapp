#ifndef Pds_TprDSConfig_hh
#define Pds_TprDSConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class TprDSConfig : public Serializer {
  public:
    TprDSConfig();
    ~TprDSConfig();
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
    bool validate();
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
