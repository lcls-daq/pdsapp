#ifndef Pds_EvsConfig_hh
#define Pds_EvsConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class EvsConfig : public Serializer {
  public:
    EvsConfig();
    ~EvsConfig();
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
