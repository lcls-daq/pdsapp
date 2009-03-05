#ifndef Pds_CfgOpal1kConfig_hh
#define Pds_CfgOpal1kConfig_hh

#include "pdsapp/config/CfgSerializer.hh"

namespace ConfigGui {

  class Opal1kConfig : public Serializer {
  public:
    Opal1kConfig();
    ~Opal1kConfig() {}
  public:
    bool readParameters (void* from);
    int  writeParameters(void* to);
  };

};
