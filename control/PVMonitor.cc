#include "pdsapp/control/PVMonitor.hh"
#include "pdsapp/control/PVRunnable.hh"

#include "pds/epicstools/EpicsCA.hh"
#include "pds/epicstools/PVMonitorCb.hh"

#include "pdsdata/control/PVMonitor.hh"

#include "db_access.h"

#include <stdio.h>
#include <string.h>

#define handle_type(ctype, stype, dtype) case ctype: \
  { struct stype* pval = (struct stype*)dbr; \
    dtype* data = &pval->value; \
    for(int k=0; k<nelem; k++) value[k] = *data++; }

typedef Pds::ControlData::PVMonitor PvType;

namespace Pds {

  class MonitorCA : public Pds_Epics::EpicsCA,
                    public Pds_Epics::PVMonitorCb {
  public:
    MonitorCA(PVMonitor& monitor,
	      const std::list<PvType>& channels) :
      Pds_Epics::EpicsCA(channels.front().name(),this),
      _monitor (monitor ), 
      _channels(channels), 
      _runnable(false   ) 
    {
    }
    virtual ~MonitorCA() {}
  public:
    void updated()
    {
      double* value = (double*)data();
      bool runnable = true;
      for(std::list<PvType>::const_iterator iter = _channels.begin();
	  iter != _channels.end(); iter++) {
	int idx = iter->index();
	if (idx <0) idx=0;
	printf("monitor[%d] %g < %g < %g\n", 
	       iter->index  (),
	       iter->loValue(),
	       value[idx],
	       iter->hiValue());
	runnable &= (value[idx] >= iter->loValue() &&
		     value[idx] <= iter->hiValue());
      }
      if (_runnable != runnable) {
	_runnable=runnable;
	_monitor.channel_changed();
      }
    }
  public:
    bool runnable() const { return _runnable; }
  private:
    PVMonitor& _monitor;
    std::list<PvType> _channels;
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

  if (!_runnable)
    _report.runnable_change(_runnable=true);
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
