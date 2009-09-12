#include "pdsapp/config/ControlConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/ControlConfigType.hh"
#include "pdsdata/control/PVControl.hh"
#include "pdsdata/control/PVMonitor.hh"

#include <new>
#include <float.h>

namespace Pds_ConfigDb {
  
  class PVControl {
  public:
    PVControl() :
      _name        ("PV Name", "", Pds::ControlData::PVControl::NameSize),
      _value       ("PV Value", 0, -DBL_MAX, DBL_MAX)
    {
    }
    
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_name);
      pList.insert(&_value);
    }

    bool pull(void* from) {
      Pds::ControlData::PVControl& tc = *new(from) Pds::ControlData::PVControl;
      // construct the full name from the array base and index
      if (tc.array())
	snprintf(_name.value, Pds::ControlData::PVControl::NameSize,
		 "%s[%d]", tc.name(), tc.index());
      else
	strncpy(_name.value, tc.name(), Pds::ControlData::PVControl::NameSize);
      _value.value = tc.value();
      return true;
    }

    int push(void* to) {
      // extract the array base and index
      char name[Pds::ControlData::PVControl::NameSize];
      int  index=0;
      strcpy(name, _name.value);
      strtok(name,"[");
      char* sindex = strtok(NULL,"]");
      if (sindex)
	sscanf(sindex,"%d",&index);

      Pds::ControlData::PVControl& tc = *new(to) Pds::ControlData::PVControl(name,
									     index,
									     _value.value);
      return sizeof(tc);
    }
  private:
    TextParameter        _name;
    NumericFloat<double> _value;
  };

  class PVMonitor {
  public:
    PVMonitor() :
      _name        ("PV Name", "", Pds::ControlData::PVControl::NameSize),
      _loValue     ("LoRange", 0, -DBL_MAX, DBL_MAX),
      _hiValue     ("HiRange", 0, -DBL_MAX, DBL_MAX)
    {
    }
    
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_name);
      pList.insert(&_loValue);
      pList.insert(&_hiValue);
    }

    bool pull(void* from) {
      Pds::ControlData::PVMonitor& tc = *new(from) Pds::ControlData::PVMonitor;
      // construct the full name from the array base and index
      if (tc.array())
	snprintf(_name.value, Pds::ControlData::PVMonitor::NameSize,
		 "%s[%d]", tc.name(), tc.index());
      else
	strncpy(_name.value, tc.name(), Pds::ControlData::PVMonitor::NameSize);
      _loValue.value = tc.loValue();
      _hiValue.value = tc.hiValue();
      return true;
    }

    int push(void* to) {
      // extract the array base and index
      char name[Pds::ControlData::PVMonitor::NameSize];
      int  index=0;
      strcpy(name, _name.value);
      strtok(name,"[");
      char* sindex = strtok(NULL,"]");
      if (sindex)
	sscanf(sindex,"%d",&index);

      Pds::ControlData::PVMonitor& tc = *new(to) Pds::ControlData::PVMonitor(name,
									     index,
									     _loValue.value,
									     _hiValue.value);
      return sizeof(tc);
    }
  private:
    TextParameter        _name;
    NumericFloat<double> _loValue;
    NumericFloat<double> _hiValue;
  };

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
      const ControlConfigType& tc = *new(from) ControlConfigType;
      _control      .value = tc.uses_duration() ? Duration : tc.uses_events() ? Events : System;
      _duration_sec .value = tc.duration().seconds();
      _duration_nsec.value = tc.duration().nanoseconds();
      _events       .value = tc.events();
      _npvcs        .value = tc.npvControls();
      for(unsigned k=0; k<tc.npvControls(); k++)
	_pvcs[k].pull(const_cast<Pds::ControlData::PVControl*>(&tc.pvControl(k)));
      _npvms        .value = tc.npvMonitors();
      for(unsigned k=0; k<tc.npvMonitors(); k++)
	_pvms[k].pull(const_cast<Pds::ControlData::PVMonitor*>(&tc.pvMonitor(k)));

      return tc.size();
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
      case Duration: tc = new(to) ControlConfigType(pvcs, pvms, 
						    ClockTime(_duration_sec.value,
							      _duration_nsec.value)); break;
      case Events  : tc = new(to) ControlConfigType(pvcs, pvms,
						    _events.value); break;
      case System  :
      default      : tc = new(to) ControlConfigType(pvcs, pvms); break;
      }
      return tc->size();
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


using namespace Pds_ConfigDb;

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
