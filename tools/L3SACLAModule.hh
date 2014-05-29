#ifndef L3SACLAModule_hh
#define L3SACLAModule_hh

/**
 *  A sample module that filters based upon the EVR information 
 *  in the event.
 */

#include "pdsdata/app/L3FilterModule.hh"
#include "pdsdata/psddl/evr.ddl.h"
#include "pdsdata/psddl/usdusb.ddl.h"

namespace Pds {
  class L3SACLAModule : public L3FilterModule {
  public:
    L3SACLAModule();
    ~L3SACLAModule() {}
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
