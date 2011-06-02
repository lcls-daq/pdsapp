#ifndef Pds_XampsConfig_hh
#define Pds_XampsConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class XampsConfig : public Serializer {
  public:
    XampsConfig();
    ~XampsConfig() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
