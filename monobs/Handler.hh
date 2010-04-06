#ifndef PdsCas_Handler_hh
#define PdsCas_Handler_hh

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
  class ClockTime;
};

namespace PdsCas {
  //  class Entry;

  class Handler {
  public:
    Handler(const Pds::Src&     info,
	    Pds::TypeId::Type   data_type,
	    Pds::TypeId::Type   config_type);
    virtual ~Handler();
  public:
    virtual void   _configure(const void* payload, const Pds::ClockTime& t) = 0;
    virtual void   _event    (const void* payload, const Pds::ClockTime& t) = 0;
    virtual void   _damaged  () = 0;
  public:
    //    virtual unsigned     nentries() const = 0;
    //    virtual const Entry* entry   (unsigned) const = 0;
    virtual void         initialize() = 0;
    virtual void         update_pv () = 0;
  public:
    const Pds::Src&     info() const { return _info; }
    const Pds::TypeId::Type&  data_type  () const { return _data_type; }
    const Pds::TypeId::Type&  config_type() const { return _config_type; }
  private:
    Pds::Src             _info;
    Pds::TypeId::Type    _data_type;
    Pds::TypeId::Type    _config_type;
  };
};

#endif
