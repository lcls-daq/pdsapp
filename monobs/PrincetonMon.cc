#include "pdsapp/monobs/PrincetonMon.hh"
#include "pdsapp/monobs/ShmClient.hh"
#include "pdsapp/monobs/Handler.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/psddl/princeton.ddl.h"
#include "pds/epicstools/PVWriter.hh"

#include <stdio.h>
#include <math.h>
#include <string.h>

using namespace Pds;
using Pds_Epics::PVWriter;

namespace PdsCas {
  class PrincetonTHandler : public Handler {
  public:
    PrincetonTHandler(const char* pvbase, const DetInfo& info) :
      Handler(info, Pds::TypeId::Id_PrincetonInfo, Pds::TypeId::Id_PrincetonConfig),
      _initialized(false)
    {
      strncpy(_pvName,pvbase,PVNAMELEN);
    }
    ~PrincetonTHandler()
    {
      if (_initialized) {
        delete _valu_writer;
      }
    }
  public:
    void   _configure(const void* payload, const Pds::ClockTime& t) {}
    void   _event    (const void* payload, const Pds::ClockTime& t)
    {
      if (!_initialized) return;

      const Pds::Princeton::InfoV1& info = *reinterpret_cast<const Pds::Princeton::InfoV1*>(payload);
      *reinterpret_cast<double*>(_valu_writer->data()) = info.temperature();
    }
    void   _damaged  () {}
  public:
    void    initialize()
    {
      printf("Initializing PrincetonT %s\n",_pvName);
      _valu_writer = new PVWriter(_pvName);
      _initialized = true;
    }
    void    update_pv ()
    {
      _valu_writer->put();
    }
  private:
    enum { PVNAMELEN=32 };
    char _pvName[PVNAMELEN];
    bool _initialized;
    PVWriter* _valu_writer;
  };
};

void PdsCas::PrincetonMon::monitor(ShmClient&     client,
                                   const char*    pvbase,
                                   const DetInfo& det)
{
  client.insert(new PrincetonTHandler(pvbase,det));
}
