#ifndef Pds_PimaxConfig_hh
#define Pds_PimaxConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

class PimaxConfig : public Serializer {
public:
  PimaxConfig();
  ~PimaxConfig() {}
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
