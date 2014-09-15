#ifndef Pds_GenericEpixConfig_hh
#define Pds_GenericEpixConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class GenericEpixConfig : public Serializer {
  public:
    GenericEpixConfig(unsigned key);
    ~GenericEpixConfig();
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
