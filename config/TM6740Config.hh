#ifndef Pds_TM6740Config_hh
#define Pds_TM6740Config_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class TM6740Config : public Serializer {
  public:
    TM6740Config();
    ~TM6740Config() {}
  public:
    bool readParameters (void* from);
    int  writeParameters(void* to);
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
