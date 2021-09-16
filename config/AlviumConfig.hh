#ifndef AlviumConfig_hh
#define AlviumConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class AlviumConfig : public Serializer {
  public:
    AlviumConfig(bool expert_mode=false);
    ~AlviumConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
    bool validate();
    bool fixedSize();

  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
