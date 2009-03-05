#ifndef Pds_Opal1kConfig_hh
#define Pds_Opal1kConfig_hh

#include "pdsapp/config/ConfigTC_Serializer.hh"

namespace ConfigGui {

  class Opal1kConfig : public Serializer {
  public:
    Opal1kConfig();
    ~Opal1kConfig() {}
  public:
    bool readParameters (void* from);
    int  writeParameters(void* to);
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
