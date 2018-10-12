#include "pdsapp/config/pnCCDConfig.hh"

#include "pdsapp/config/Parameters.hh"
//#include "pds/config/pnCCDConfigType.hh"
#include "pdsdata/psddl/pnccd.ddl.h"

#include <new>

typedef Pds::PNCCD::ConfigV1 pnCCDConfigType;

namespace Pds_ConfigDb {

  class pnCCDConfig::Private_Data {
  public:
    Private_Data() :
      _numLinks         ("NumLinks"   , 4, 1, 4),
      _payloadSize      ("LinkPayload", 524304, 0, 0x7fffffff)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_numLinks);
      pList.insert(&_payloadSize);
    }

    int pull(void* from) {
      pnCCDConfigType& tc = *reinterpret_cast<pnCCDConfigType*>(from);
      _numLinks   .value = tc.numLinks();
      _payloadSize.value = tc.payloadSizePerLink();
      return tc._sizeof();
    }

    int push(void* to) {
      pnCCDConfigType& tc = *new(to) pnCCDConfigType(_numLinks.value,_payloadSize.value);
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(pnCCDConfigType);
    }

  public:
    NumericInt<unsigned>    _numLinks;
    NumericInt<unsigned>    _payloadSize;
  };
};


using namespace Pds_ConfigDb;

pnCCDConfig::pnCCDConfig() : 
  Serializer("pnCCD_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  pnCCDConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  pnCCDConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  pnCCDConfig::dataSize() const {
  return _private_data->dataSize();
}



