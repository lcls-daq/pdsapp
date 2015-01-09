#ifndef Pds_QuartzConfig_V1_hh
#define Pds_QuartzConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace V1 {
    class QuartzConfig : public Serializer {
    public:
      QuartzConfig();
      ~QuartzConfig() {}
    public:
      int  readParameters (void* from);
      int  writeParameters(void* to);
      int  dataSize() const;
    private:
      class Private_Data;
      Private_Data* _private_data;
    };
  };
};

#endif
