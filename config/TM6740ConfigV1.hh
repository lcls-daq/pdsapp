#ifndef Pds_TM6740ConfigV1_hh
#define Pds_TM6740ConfigV1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class TM6740ConfigV1 : public Serializer {
  public:
    TM6740ConfigV1();
    ~TM6740ConfigV1() {}
  public:
    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize() const;
  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
