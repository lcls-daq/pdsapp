#include "pdsapp/config/PVMonitor.hh"

#include <string.h>
#include <new>
#include <float.h>
#include <stdio.h>

using namespace Pds_ConfigDb;

PVMonitor::PVMonitor() :
  _name        ("PV Name", "", PVMonitorType::NameSize),
  _loValue     ("LoRange", 0, -DBL_MAX, DBL_MAX),
  _hiValue     ("HiRange", 0, -DBL_MAX, DBL_MAX)
{
}
    
void PVMonitor::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_name);
  pList.insert(&_loValue);
  pList.insert(&_hiValue);
}

bool PVMonitor::pull(const PVMonitorType& tc) {
  // construct the full name from the array base and index
  if (tc.array())
    snprintf(_name.value, PVMonitorType::NameSize,
             "%s[%d]", tc.name(), tc.index());
  else
    strncpy(_name.value, tc.name(), PVMonitorType::NameSize);
  _loValue.value = tc.loValue();
  _hiValue.value = tc.hiValue();
  return true;
}

int PVMonitor::push(void* to) {
  // extract the array base and index
  char name[PVMonitorType::NameSize];
  int  index=0;
  strcpy(name, _name.value);
  strtok(name,"[");
  char* sindex = strtok(NULL,"]");
  if (sindex)
    sscanf(sindex,"%d",&index);

  PVMonitorType& tc = *new(to) PVMonitorType(name,
                                                                         index,
                                                                         _loValue.value,
                                                                         _hiValue.value);
  return sizeof(tc);
}
