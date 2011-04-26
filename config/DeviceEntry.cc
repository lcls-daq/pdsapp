#include "pdsapp/config/DeviceEntry.hh"

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

using namespace Pds_ConfigDb;


DeviceEntry::DeviceEntry(unsigned id) :
  Pds::Src(Pds::Level::Source)
{
  _phy = id; 
}

DeviceEntry::DeviceEntry(const Pds::Src& id) :
  Pds::Src(id)
{
}

DeviceEntry::DeviceEntry(const string& id) 
{
  char sep;
  istringstream i(id);
  i >> std::hex >> _log >> sep >> _phy;
}

string DeviceEntry::id() const
{
  ostringstream o;
  o << std::hex << setfill('0') << setw(8) << _log << '.' << setw(8) << _phy;
  return o.str();
}

string DeviceEntry::path() const
{
  ostringstream o;
  if (level()!=Pds::Level::Source)
    o << std::hex << level();
  else
    o << std::hex << setfill('0') << setw(8) << _phy;
  return o.str();
}

