#ifndef Pds_UxiConfig_V1_hh
#define Pds_UxiConfig_V1_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

class UxiConfig_V1 : public Serializer {
public:
  UxiConfig_V1(bool expert_mode=false);
  ~UxiConfig_V1() {}
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
