#ifndef Pds_GenericEpix10kConfig_hh
#define Pds_GenericEpix10kConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class GenericEpix10kConfig : public Serializer {
  public:
    GenericEpix10kConfig(unsigned key);
    ~GenericEpix10kConfig();
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
