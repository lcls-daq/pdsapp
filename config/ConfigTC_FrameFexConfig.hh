#ifndef Pds_FrameFexConfig_hh
#define Pds_FrameFexConfig_hh

#include "pdsapp/config/ConfigTC_Serializer.hh"

namespace ConfigGui {

  class FrameFexConfig : public Serializer {
  public:
    FrameFexConfig();
    ~FrameFexConfig() {}
  public:
    bool readParameters (void* from);
    int  writeParameters(void* to);
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
