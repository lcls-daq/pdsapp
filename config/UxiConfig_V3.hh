#ifndef Pds_UxiConfig_V3_hh
#define Pds_UxiConfig_V3_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

class UxiConfig_V3 : public Serializer {
public:
  UxiConfig_V3(bool expert_mode=false);
  ~UxiConfig_V3() {}
public:
  int  readParameters (void* from);
  int  writeParameters(void* to);
  int  dataSize() const;
  bool validate();
private:
  class Private_Data;
  Private_Data* _private_data;
};

};


#endif
