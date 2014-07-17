#ifndef L3SACLACOMPOUNDModule_hh
#define L3SACLACOMPOUNDModule_hh

/**
 *  A sample module that filters based upon the EVR information 
 *  in the event.
 */

#include "pdsdata/app/L3FilterModule.hh"
#include "pdsdata/psddl/evr.ddl.h"
#include "pdsdata/psddl/usdusb.ddl.h"

namespace Pds {
  class L3SACLACompoundModule : public L3FilterModule {
  public:
    L3SACLACompoundModule();
    ~L3SACLACompoundModule() {}
  public:
    void configure(const Pds::DetInfo&   src,
                   const Pds::TypeId&    type,
                   void*                 payload) {}
    void configure(const Pds::BldInfo&   src,
                   const Pds::TypeId&    type,
                   void*                 payload) {}
    void configure(const Pds::ProcInfo&  src,
                   const Pds::TypeId&    type,
                   void*                 payload) {}
    void event    (const Pds::DetInfo&   src,
                   const Pds::TypeId&    type,
                   void*                 payload);
    void event    (const Pds::BldInfo&   src,
                   const Pds::TypeId&    type,
                   void*                 payload) {}
    void event    (const Pds::ProcInfo&  src,
                   const Pds::TypeId&    type,
                   void*                 payload) {}
  public:
    std::string name() const;
    std::string configuration() const;
    bool complete ();
    bool accept ();
    void pre_event();
  private:
    uint8_t        _din;
    bool           _dataSeen;
  };
};

#endif
