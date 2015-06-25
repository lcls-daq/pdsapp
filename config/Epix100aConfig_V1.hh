#ifndef Pds_Epix100aConfig_V1_hh
#define Pds_Epix100aConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  namespace V1 {
    class Epix100aConfig : public Pds_ConfigDb::Serializer {
    public:
      Epix100aConfig();
      ~Epix100aConfig();
    public:
      int  readParameters (void* from);
      int  writeParameters(void* to);
      int  dataSize       () const;
    private:
      class PrivateData;
      PrivateData* _private;
    };
  }; // namespace V1
};  // namespace Pds_ConfigDb

#endif
