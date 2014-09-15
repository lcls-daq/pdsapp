#include "pdsapp/config/EpixConfig.hh"
#include "pdsapp/config/EpixConfigP.hh"

using namespace Pds_ConfigDb;

EpixConfig::EpixConfig() :
  Serializer("Epix_Config"),
  _private(new EpixConfigP)
{
  name("EPIX Configuration");
  pList.insert(_private);
}

EpixConfig::~EpixConfig()
{
  delete _private;
}

int EpixConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int EpixConfig::writeParameters(void* to) {
  return _private->push(to);
}

int EpixConfig::dataSize() const {
  return _private->dataSize();
}
