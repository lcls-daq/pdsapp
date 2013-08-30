#include "pdsapp/config/OceanOpticsConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/OceanOpticsConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  class OceanOpticsConfig::Private_Data {
  public:
    Private_Data() :
      // Note: Here the min exposure time need to set 9.99e-6 to allow user to input 1e-5, due to floating points imprecision
      _f32ExposureTime      ("Exposure time (sec)", 1e-3, 9.99e-6, 65)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_f32ExposureTime);
    }

    int pull(void* from) {
      OceanOpticsConfigType& tc = *reinterpret_cast<OceanOpticsConfigType*>(from);
      _f32ExposureTime      .value = tc.exposureTime();
      return tc._sizeof();
    }

    int push(void* to) {
      double dummy[] = {0,0,0,0,0,0,0,0};
      OceanOpticsConfigType tc = *new(to) 
        OceanOpticsConfigType (_f32ExposureTime.value, dummy, dummy, dummy[0]);
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(OceanOpticsConfigType);
    }

  public:
    NumericFloat<float>     _f32ExposureTime;    
  };
};

using namespace Pds_ConfigDb;

OceanOpticsConfig::OceanOpticsConfig() : 
  Serializer("OceanOptics_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  OceanOpticsConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  OceanOpticsConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  OceanOpticsConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

