#include "PVMonitor.hh"

#include "pdsapp/control/PVRunnable.hh"
#include "pdsapp/control/EpicsCA.hh"

#include "pdsdata/control/PVMonitor.hh"

#include "db_access.h"

#define handle_type(ctype, stype, dtype) case ctype: \
  { struct stype* pval = (struct stype*)dbr; \
    dtype* data = &pval->value; \
    for(int k=0; k<nelem; k++) value[k] = *data++; }

typedef Pds::ControlData::PVMonitor PvType;

namespace Pds {

  class MonitorCA : public EpicsCA {
  public:
    MonitorCA(PVMonitor& monitor,
	      const std::list<PvType>& channels) :
      EpicsCA(channels.front().name(),true),
      _monitor(monitor), _channels(channels), 
      _connected(false), _runnable(false) 
    {
    }
    virtual ~MonitorCA() {}
  public:
    void connected   (bool c)
    {
      _connected = c;
      if (_runnable) {
	_runnable=false;
	_monitor.channel_changed();
      }
    }
    void getData (const void* dbr) 
    {
      int nelem = _channel.nelements();
      double* value = new double[nelem];
      switch(_channel.type()) {
	handle_type(DBR_TIME_SHORT , dbr_time_short , dbr_short_t ) break;
	handle_type(DBR_TIME_FLOAT , dbr_time_float , dbr_float_t ) break;
	handle_type(DBR_TIME_ENUM  , dbr_time_enum  , dbr_enum_t  ) break;
	handle_type(DBR_TIME_LONG  , dbr_time_long  , dbr_long_t  ) break;
	handle_type(DBR_TIME_DOUBLE, dbr_time_double, dbr_double_t) break;
      default: printf("Unknown type %d\n", int(_channel.type())); break;
      }
      bool runnable = true;
      for(std::list<PvType>::const_iterator iter = _channels.begin();
	  iter != _channels.end(); iter++) {
	runnable &= (value[iter->index()] >= iter->loValue() &&
		     value[iter->index()] <= iter->hiValue());
      }
      if (_runnable != runnable) {
	_runnable=runnable;
	_monitor.channel_changed();
      }
      delete[] value;
    }
    void* putData() { return 0; }
    void putStatus(bool) {}
  public:
    bool runnable() const { return _runnable; }
  private:
    PVMonitor& _monitor;
    std::list<PvType> _channels;
    bool _connected;
    bool _runnable;
  };

};

using namespace Pds;

PVMonitor::PVMonitor (PVRunnable& report) :
  _report  (report),
  _runnable(false)
{
}

PVMonitor::~PVMonitor()
{
}

bool PVMonitor::runnable() const { return _runnable; }

void PVMonitor::configure(const ControlConfigType& tc)
{
  if (tc.npvMonitors()==0) {
    if (!_runnable) 
      _report.runnable_change(_runnable=true);
  }
  else {
    if (_runnable)
      _report.runnable_change(_runnable=false);

    std::list<PvType> array;
    array.push_back(tc.pvMonitor(0));
    for(unsigned k=1; k<tc.npvMonitors(); k++) {
      const PvType& pv = tc.pvMonitor(k);
      if (strcmp(pv.name(),array.front().name())) {
	_channels.push_back(new MonitorCA(*this,array));
	array.clear();
      }
      array.push_back(pv);
    }
    _channels.push_back(new MonitorCA(*this,array));
  }
}

void PVMonitor::unconfigure()
{
  for(std::list<MonitorCA*>::iterator iter = _channels.begin();
      iter != _channels.end(); iter++)
    delete *iter;
  _channels.clear();
}

void PVMonitor::channel_changed()
{
  bool runnable = true;
  for(std::list<MonitorCA*>::iterator iter = _channels.begin();
      iter != _channels.end(); iter++)
    runnable &= (*iter)->runnable();
  if (runnable != _runnable) 
    _report.runnable_change(_runnable = runnable);
}
