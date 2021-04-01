#include "pdsapp/config/ControlConfigType_V3.hh"

using Pds::ControlData::PVControl;
using Pds::ControlData::PVMonitor;
using Pds::ControlData::PVLabel;

static bool compare_control(const PVControl& a, const PVControl& b) { return strcmp(a.name(),b.name())<=0; }
static bool compare_monitor(const PVMonitor& a, const PVMonitor& b) { return strcmp(a.name(),b.name())<=0; }
static bool compare_label  (const PVLabel  & a, const PVLabel  & b) { return strcmp(a.name(),b.name())<=0; }

static PVControl* list_to_array(const std::list<PVControl>&);
static PVMonitor* list_to_array(const std::list<PVMonitor>&);
static PVLabel  * list_to_array(const std::list<PVLabel  >&);

ControlConfigType* Pds_ConfigDb::ControlConfig_V3::_new(void* p)
{
  return new(p) ControlConfigType(0,0,0,1,Pds::ClockTime(0,0),0,0,0,0,0,0);
}

ControlConfigType* Pds_ConfigDb::ControlConfig_V3::_new(void* p, 
                                            const std::list<PVControl>& controls,
                                            const std::list<PVMonitor>& monitors,
                                            const std::list<PVLabel>&   labels,
                                            const Pds::ClockTime& ctime)
{
  PVControl* c = list_to_array(controls);
  PVMonitor* m = list_to_array(monitors);
  PVLabel  * l = list_to_array(labels);
  ControlConfigType* r = new(p) ControlConfigType(0, 0, 1, 0, ctime,
                                                  controls.size(),
                                                  monitors.size(),
                                                  labels  .size(),
                                                  c, m, l);

  if (c) delete[] c;
  if (m) delete[] m;
  if (l) delete[] l;
  
  return r;
}

ControlConfigType* Pds_ConfigDb::ControlConfig_V3::_new(void* p, 
                                            const std::list<PVControl>& controls,
                                            const std::list<PVMonitor>& monitors,
                                            const std::list<PVLabel>&   labels,
                                            unsigned events)
{
  PVControl* c = list_to_array(controls);
  PVMonitor* m = list_to_array(monitors);
  PVLabel  * l = list_to_array(labels);
  ControlConfigType* r = new(p) ControlConfigType(events, 0, 0, 1, Pds::ClockTime(0,0),
                                                  controls.size(),
                                                  monitors.size(),
                                                  labels  .size(),
                                                  c, m, l);

  if (c) delete[] c;
  if (m) delete[] m;
  if (l) delete[] l;
  
  return r;
}

ControlConfigType* Pds_ConfigDb::ControlConfig_V3::_new(void* p, 
                                            const std::list<PVControl>& controls,
                                            const std::list<PVMonitor>& monitors,
                                            const std::list<PVLabel>&   labels,
                                            L3TEvents events)
{
  PVControl* c = list_to_array(controls);
  PVMonitor* m = list_to_array(monitors);
  PVLabel  * l = list_to_array(labels);
  ControlConfigType* r = new(p) ControlConfigType(events, 1, 0, 0, Pds::ClockTime(0,0),
                                                  controls.size(),
                                                  monitors.size(),
                                                  labels  .size(),
                                                  c, m, l);

  if (c) delete[] c;
  if (m) delete[] m;
  if (l) delete[] l;
  
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

PVLabel  * list_to_array(const std::list<PVLabel  >& pvcs)
{
  PVLabel  * c = 0;
  if (pvcs.size()!=0) {
    std::list<PVLabel  > sorted_pvcs (pvcs); sorted_pvcs.sort(compare_label);
    c = new PVLabel  [pvcs.size()];
    unsigned i=0;
    for(std::list<PVLabel  >::const_iterator iter = sorted_pvcs.begin();
        iter != sorted_pvcs.end(); ++iter)
      c[i++] = *iter;
  }
  return c;
}
