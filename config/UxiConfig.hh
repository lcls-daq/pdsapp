#ifndef Pds_UxiConfig_hh
#define Pds_UxiConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

class UxiConfig : public Serializer {
public:
  UxiConfig(bool expert_mode=false);
  ~UxiConfig() {}
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
