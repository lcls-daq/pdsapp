#ifndef Pds_EpixConfig_hh
#define Pds_EpixConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class EpixConfigP;

  class EpixConfig : public Serializer {
  public:
    EpixConfig();
    ~EpixConfig();
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    EpixConfigP* _private;
  };

};

#endif
