#ifndef Pds_TriggerConfigP_hh
#define Pds_TriggerConfigP_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {
  class TriggerConfigP : public Serializer {
  public:
    TriggerConfigP();
    ~TriggerConfigP();
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
