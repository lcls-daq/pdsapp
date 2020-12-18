#ifndef iStarConfig_hh
#define iStarConfig_hh

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb {

  class iStarConfig : public Serializer {
  public:
    iStarConfig(bool expert_mode=false);
    ~iStarConfig() {}

    int  readParameters (void* from);
    int  writeParameters(void* to);
    int  dataSize       () const;
    bool validate();

  private:
    class Private_Data;
    Private_Data* _private_data;
  };

};

#endif
