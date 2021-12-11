#ifndef Pds_TimeTool_V2_hh
#define Pds_TimeTool_V2_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace V2 {
    class TimeToolConfig : public Serializer {
    public:
      TimeToolConfig();
      ~TimeToolConfig() {}
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
};

#endif
