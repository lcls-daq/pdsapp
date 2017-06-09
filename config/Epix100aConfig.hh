#ifndef Pds_Epix100aConfig_hh
#define Pds_Epix100aConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class Epix100aConfig : public Serializer {
  public:
    Epix100aConfig(bool expert=false);
    ~Epix100aConfig();
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    class PrivateData;
    PrivateData* _private;
  };

};

#endif
