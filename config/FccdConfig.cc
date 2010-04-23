#include "pdsapp/config/FccdConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/FccdConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  class FccdConfig::Private_Data {
  public:
    Private_Data() :
      _u16OutputMode  ("Output Mode",  4,    0,    4)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_u16OutputMode);
    }

    int pull(void* from) {
      FccdConfigType& tc = *new(from) FccdConfigType;
      _u16OutputMode.value = tc.outputMode();
      return tc.size();
    }

    int push(void* to) {
      FccdConfigType& tc = *new(to) FccdConfigType(
        _u16OutputMode.value
      );
      return tc.size();
    }

    int dataSize() const {
      return sizeof(FccdConfigType);
    }

  public:
    NumericInt<uint16_t>    _u16OutputMode;
  };
};

using namespace Pds_ConfigDb;

FccdConfig::FccdConfig() : 
  Serializer("fccd_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  FccdConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  FccdConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  FccdConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

