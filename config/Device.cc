#include "pdsapp/config/Device.hh"

#include "pds/config/PdsDefs.hh"
#include "pdsapp/config/GlobalCfg.hh"

#include <sys/stat.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sstream>
using std::istringstream;
using std::ostringstream;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <iomanip>
using std::setw;
using std::setfill;

//#define DBUG

using namespace Pds_ConfigDb;

const mode_t _fmode = S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP | S_IRWXU;

Device::Device() :
  _name("None")
{
}

Device::Device(const string& name,
	       const Table&  table,
	       const list<DeviceEntry>& src_list) :
  _name (name),
  _table(table),
  _src_list(src_list)
{
}

bool Device::operator==(const Device& d) const
{
  return name() == d.name(); 
}

