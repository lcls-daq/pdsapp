#ifndef PdsCas_EpicsToEpics_hh
#define PdsCas_EpicsToEpics_hh

#include "pdsapp/monobs/Handler.hh"

#include <string>

namespace Pds { class DetInfo; }

namespace Pds_Epics { class PVWriter; }

namespace PdsCas {
  class EpicsToEpics : public Handler {
  public:
    EpicsToEpics(const Pds::DetInfo& info, const char* pvin, const char* pvout);
    ~EpicsToEpics();
  public:
    virtual void   _configure(const void* payload, const Pds::ClockTime& t);
    virtual void   _event    (const void* payload, const Pds::ClockTime& t);
    virtual void   _damaged  ();
  public:
    virtual void    initialize();
    virtual void    update_pv ();
  private:
    std::string _pvIn;
    std::string _pvOut;
    int         _index;
    bool _initialized;
    Pds_Epics::PVWriter*  _valu_writer;
  };
};

#endif
