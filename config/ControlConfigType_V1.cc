#include "pdsapp/config/ControlConfigType_V1.hh"

using Pds::ControlData::PVControl;
using Pds::ControlData::PVMonitor;

static bool compare_control(const PVControl& a, const PVControl& b) { return strcmp(a.name(),b.name()); }
static bool compare_monitor(const PVMonitor& a, const PVMonitor& b) { return strcmp(a.name(),b.name()); }

static PVControl* list_to_array(const std::list<PVControl>&);
static PVMonitor* list_to_array(const std::list<PVMonitor>&);

ControlConfigType* Pds_ConfigDb::ControlConfig_V1::_new(void* p, 
                                                        const std::list<PVControl>& controls,
                                                        const std::list<PVMonitor>& monitors,
                                                        const Pds::ClockTime& ctime)
{
  PVControl* c = list_to_array(controls);
  PVMonitor* m = list_to_array(monitors);
  ControlConfigType* r = new(p) ControlConfigType(0, 0, 1, ctime,
                                                  controls.size(),
                                                  monitors.size(),
                                                  c, m);

  if (c) delete[] c;
  if (m) delete[] m;
  
  return r;
}

ControlConfigType* Pds_ConfigDb::ControlConfig_V1::_new(void* p, 
                                                        const std::list<PVControl>& controls,
                                                        const std::list<PVMonitor>& monitors,
                                                        unsigned events)
{
  PVControl* c = list_to_array(controls);
  PVMonitor* m = list_to_array(monitors);
  ControlConfigType* r = new(p) ControlConfigType(1, events, 0, Pds::ClockTime(0,0),
                                                  controls.size(),
                                                  monitors.size(),
                                                  c, m);

  if (c) delete[] c;
  if (m) delete[] m;
  
  return r;
}

PVControl* list_to_array(const std::list<PVControl>& pvcs)
{
  PVControl* c = 0;
  if (pvcs.size()!=0) {
    std::list<PVControl> sorted_pvcs (pvcs); sorted_pvcs.sort(compare_control);
    c = new PVControl[pvcs.size()];
    unsigned i=0;
    for(std::list<PVControl>::const_iterator iter = sorted_pvcs.begin();
        iter != sorted_pvcs.end(); ++iter)
      c[i++] = *iter;
  }
  return c;
}

PVMonitor* list_to_array(const std::list<PVMonitor>& pvcs)
{
  PVMonitor* c = 0;
  if (pvcs.size()!=0) {
    std::list<PVMonitor> sorted_pvcs (pvcs); sorted_pvcs.sort(compare_monitor);
    c = new PVMonitor[pvcs.size()];
    unsigned i=0;
    for(std::list<PVMonitor>::const_iterator iter = sorted_pvcs.begin();
        iter != sorted_pvcs.end(); ++iter)
      c[i++] = *iter;
  }
  return c;
}

