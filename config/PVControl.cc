#include "pdsapp/config/PVControl.hh"

#include "pdsdata/psddl/control.ddl.h"

#include <new>
#include <float.h>
#include <string.h>
#include <stdio.h>

using namespace Pds_ConfigDb;

PVControl::PVControl() :
  _name        ("PV Name", "", Pds::ControlData::PVControl::NameSize),
  _value       ("PV Value", 0, -DBL_MAX, DBL_MAX)
{
}
    
void PVControl::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_name);
  pList.insert(&_value);
}

bool PVControl::pull(const Pds::ControlData::PVControl& tc) {
  // construct the full name from the array base and index
  if (tc.array())
    snprintf(_name.value, Pds::ControlData::PVControl::NameSize,
             "%s[%d]", tc.name(), tc.index());
  else
    strncpy(_name.value, tc.name(), Pds::ControlData::PVControl::NameSize);
  _value.value = tc.value();
  return true;
}

int PVControl::push(void* to) {
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
