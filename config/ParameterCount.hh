#ifndef Pds_ParameterCount_hh
#define Pds_ParameterCount_hh

namespace Pds_ConfigDb {
  class ParameterSet;
  class ParameterCount {
  public:
    virtual ~ParameterCount() {}
    virtual bool connect(ParameterSet&) = 0;
    virtual unsigned count() = 0;
  };
};

#endif
