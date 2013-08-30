#include "pdsapp/config/ControlConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/PVControl.hh"
#include "pdsapp/config/PVMonitor.hh"
#include "pdsapp/config/ControlConfigType_V1.hh"

#include "pdsdata/psddl/control.ddl.h"


#include "pdsdata/xtc/ClockTime.hh"

#include <new>
#include <float.h>

namespace Pds_ConfigDb {
  namespace ControlConfig_V1 {

    enum StepControl { System, Duration, Events };
    static const char* step_control[] = { "System",
                                          "Duration",
                                          "Events",
                                          NULL };

    class ControlConfig::Private_Data {
      enum { MaxPVs = 100 };
    public:
      Private_Data() :
        _control      ("Control", System, step_control),
        _duration_sec ("Duration sec" , 0, 0, 1000000),
        _duration_nsec("Duration nsec", 0, 0, 1000000000),
        _events       ("Nevents"      , 0, 0, 100000000),
        _npvcs        ("Number of Control PVs", 0, 0, MaxPVs),
        _pvcSet       ("Control PV"           , _pvcArgs, _npvcs ),
        _npvms        ("Number of Monitor PVs", 0, 0, MaxPVs),
        _pvmSet       ("Monitor PV"           , _pvmArgs, _npvms )
      {
        for(unsigned k=0; k<MaxPVs; k++)
          _pvcs[k].insert(_pvcArgs[k]);
        for(unsigned k=0; k<MaxPVs; k++)
          _pvms[k].insert(_pvmArgs[k]);
      }

      void insert(Pds::LinkedList<Parameter>& pList) {
        pList.insert(&_control);
        pList.insert(&_duration_sec);
        pList.insert(&_duration_nsec);
        pList.insert(&_events);
        pList.insert(&_npvcs);
        pList.insert(&_pvcSet);
        pList.insert(&_npvms);
        pList.insert(&_pvmSet);
      }

      int pull(void* from) {
        const ControlConfigType& tc = *reinterpret_cast<const ControlConfigType*>(from);
        _control      .value = tc.uses_duration() ? Duration : tc.uses_events() ? Events : System;
        _duration_sec .value = tc.duration().seconds();
        _duration_nsec.value = tc.duration().nanoseconds();
        _events       .value = tc.events();
        _npvcs        .value = tc.npvControls();
        for(unsigned k=0; k<tc.npvControls(); k++)
          _pvcs[k].pull(tc.pvControls()[k]);
        _npvms        .value = tc.npvMonitors();
        for(unsigned k=0; k<tc.npvMonitors(); k++)
          _pvms[k].pull(tc.pvMonitors()[k]);

        return tc._sizeof();
      }

      int push(void* to) {
        std::list<Pds::ControlData::PVControl> pvcs;
        for(unsigned k=0; k<_npvcs.value; k++) {
          Pds::ControlData::PVControl pvc;
          _pvcs[k].push(&pvc);
          pvcs.push_back(pvc);
        }

        std::list<Pds::ControlData::PVMonitor> pvms;
        for(unsigned k=0; k<_npvms.value; k++) {
          Pds::ControlData::PVMonitor pvm;
          _pvms[k].push(&pvm);
          pvms.push_back(pvm);
        }

        ControlConfigType* tc;
        switch(_control.value) {
        case Duration: tc = _new(to, pvcs, pvms, 
                                 Pds::ClockTime(_duration_sec.value,
                                                _duration_nsec.value)); break;
        case Events  : tc = _new(to, pvcs, pvms, _events.value); break;
        case System  :
        default      : tc = _new(to, pvcs, pvms, 0); break;
        }
        return tc->_sizeof();
      }

      int dataSize() const {
        return sizeof(ControlConfigType) + 
          _npvcs.value*sizeof(Pds::ControlData::PVControl) +
          _npvms.value*sizeof(Pds::ControlData::PVMonitor);
      }

    private:
      Enumerated<StepControl> _control;
      NumericInt<unsigned>    _duration_sec;
      NumericInt<unsigned>    _duration_nsec;
      NumericInt<unsigned>    _events;
      NumericInt<unsigned>    _npvcs;
      PVControl               _pvcs[MaxPVs];
      Pds::LinkedList<Parameter> _pvcArgs[MaxPVs];
      ParameterSet            _pvcSet;
      NumericInt<unsigned>    _npvms;
      PVMonitor               _pvms[MaxPVs];
      Pds::LinkedList<Parameter> _pvmArgs[MaxPVs];
      ParameterSet            _pvmSet;
    };
  };
};


using namespace Pds_ConfigDb::ControlConfig_V1;

ControlConfig::ControlConfig() : 
  Serializer("Control_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  ControlConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  ControlConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  ControlConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<StepControl>;
