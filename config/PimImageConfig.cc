#include "PimImageConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/PimImageConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {
  
  class PimImageConfig::Private_Data {
  public:
    Private_Data() :
      _xscale  ("Image X Scale (mm/pixel) ", 10, 0, 1e8),
      _yscale  ("Image Y Scale (mm/pixel) ", 10, 0, 1e8) {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_xscale);
      pList.insert(&_yscale);
    }

    int pull(void* from) { // pull "from xtc"
      PimImageConfigType& pimConf = *new(from) PimImageConfigType;
      _xscale.value   = pimConf.xscale();
      _yscale.value   = pimConf.yscale();
      return sizeof(PimImageConfigType);
    }

    int push(void* to) {
      *new(to) PimImageConfigType(_xscale.value,_yscale.value);
      return sizeof(PimImageConfigType);
    }

    int dataSize() const { return sizeof(PimImageConfigType); }

  public:
    NumericFloat<float> _xscale;
    NumericFloat<float> _yscale;
  };
};

PimImageConfig::PimImageConfig() : 
  Serializer("Ipm_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int PimImageConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  PimImageConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  PimImageConfig::dataSize() const {
  return _private_data->dataSize();
}


