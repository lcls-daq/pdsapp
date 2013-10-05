#ifndef L3TestModule_hh
#define L3TestModule_hh

/**
 *  A sample module that filters based upon the EVR information 
 *  in the event.
 */

#include "pdsdata/app/L3FilterModule.hh"
#include "pdsdata/psddl/evr.ddl.h"

namespace Pds {
  class L3TestModule : public L3FilterModule {
  public:
    L3TestModule();
    ~L3TestModule() {}
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
  private:
    const EvrData::DataV3* _evr;
  };
};

#endif
