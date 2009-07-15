#include "PVMonitor.hh"

#include "pdsapp/control/EpicsCA.hh"

#include "pdsdata/control/PVMonitor.hh"

#include "alarm.h"

namespace Pds {

  class MonitorCA : public EpicsCA {
  public:
    MonitorCA(PVMonitor& monitor,
	      const Pds::ControlData::PVMonitor& channel) :
      //      EpicsCA(channel.name(), DBR_FLOAT),
      EpicsCA(channel.name()),
      _monitor(monitor), _channel(channel), _okay(false) {}
    virtual ~MonitorCA() {}
  public:
    void dataCallback(const void* dbr) 
    {
      struct dbr_time_string* cdData = (struct dbr_time_string*)dbr;
      bool okay = true;
      switch(cdData->status) {
      case HIHI_ALARM: okay &= ~(_channel.restrictedAlarms()&Pds::ControlData::PVMonitor::HiHi); break;
      case HIGH_ALARM: okay &= ~(_channel.restrictedAlarms()&Pds::ControlData::PVMonitor::High); break;
      case LOLO_ALARM: okay &= ~(_channel.restrictedAlarms()&Pds::ControlData::PVMonitor::LoLo); break;
      case LOW_ALARM : okay &= ~(_channel.restrictedAlarms()&Pds::ControlData::PVMonitor::Low ); break;
      default: break;
      }
      if (_okay != okay) {
	_okay=okay;
	_monitor.channel_changed();
      }
    }
  public:
    const Pds::ControlData::PVMonitor& channel () const { return _channel; }
    bool                               okay    () const { return _okay; }
  private:
    PVMonitor& _monitor;
    Pds::ControlData::PVMonitor _channel;
    bool _okay;
  };

};

using namespace Pds;

PVMonitor::PVMonitor () :
  _state(NotOK)
{
}

PVMonitor::~PVMonitor()
{
  for(std::list<MonitorCA*>::iterator iter = _channels.begin();
      iter != _channels.end(); iter++)
    delete *iter;
}

PVMonitor::State PVMonitor::state() const { return _state; }

void PVMonitor::configure(const ControlConfigType& tc)
{
  for(unsigned k=0; k<tc.npvMonitors(); k++)
    _channels.push_back(new MonitorCA(*this,tc.pvMonitor(k)));
}

void PVMonitor::channel_changed()
{
  bool okay = true;
  for(std::list<MonitorCA*>::iterator iter = _channels.begin();
      iter != _channels.end(); iter++)
    okay &= (*iter)->okay();
  State state = okay ? OK : NotOK;
  if (state != _state) 
    state_changed(_state = state);
}
