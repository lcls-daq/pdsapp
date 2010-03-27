#ifndef Pds_IpimbConfig_hh
#define Pds_IpimbConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class IpimbConfig : public Serializer {
  public:
    IpimbConfig();
    ~IpimbConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
