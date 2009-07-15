#ifndef Pds_Opal1kConfig_hh
#define Pds_Opal1kConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class Opal1kConfig : public Serializer {
  public:
    Opal1kConfig();
    ~Opal1kConfig() {}
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
