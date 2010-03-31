#ifndef Pds_princetonConfig_hh
#define Pds_princetonConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class princetonConfig : public Serializer {
  public:
    princetonConfig();
    ~princetonConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
