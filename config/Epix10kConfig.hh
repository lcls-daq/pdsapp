#ifndef Pds_Epix10kConfig_hh
#define Pds_Epix10kConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class Epix10kConfig : public Serializer {
  public:
    Epix10kConfig();
    ~Epix10kConfig();
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
