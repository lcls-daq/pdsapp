#include "pdsapp/config/Epix10kConfig.hh"
#include "pdsapp/config/Epix10kConfigP.hh"

using namespace Pds_ConfigDb;

Epix10kConfig::Epix10kConfig() :
  Serializer("Epix10k_Config"),
  _private(new Epix10kConfigP)
{
  name("EPIX10k Configuration");
  pList.insert(_private);
}

Epix10kConfig::~Epix10kConfig()
{
  delete _private;
}

int Epix10kConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int Epix10kConfig::writeParameters(void* to) {
  return _private->push(to);
}

int Epix10kConfig::dataSize() const {
  return _private->dataSize();
}


#include "Parameters.icc"
