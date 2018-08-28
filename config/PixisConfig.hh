#ifndef Pds_PixisConfig_hh
#define Pds_PixisConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

class PixisConfig : public Serializer {
public:
  PixisConfig(bool expert_mode=false);
  ~PixisConfig() {}
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
