#ifndef Pds_Epix10kaConfig_V1_hh
#define Pds_Epix10kaConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace V1 {
    class Epix10kaConfig : public Serializer {
    public:
      Epix10kaConfig(bool expert=false);
      ~Epix10kaConfig();
    public:
      int  readParameters (void* from);
      int  writeParameters(void* to);
      int  dataSize       () const;
    private:
      class PrivateData;
      PrivateData* _private;
    };
  };
};

#endif
