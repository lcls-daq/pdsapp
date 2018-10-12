#include "pdsapp/config/QuadAdcConfig.hh"
#include "pds/config/QuadAdcConfigType.hh"
#include "pdsapp/config/QuadAdcChannelMask.hh"

#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/quadadc.ddl.h"

#include <stdio.h>

#include <new>

static const double delay_max = double((1<<20)-1)/119e-3; // nanoseconds

namespace Pds_ConfigDb {

  class QuadAdcConfig::Private_Data {
  public:
    Private_Data() :
      _channelMask      ("Channel Mask [hex]",1),
      _numChan          (_channelMask),

      _Delay_NS         ("Delay_NS (ns) "   ,   0, 0, unsigned(delay_max)),
      _NSamples         ("NSamples"   ,   32000, 0, 32000),
      _Evt_Code         ("Evt_Code"   ,   40, 0, 255)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_channelMask);
      pList.insert(&_Delay_NS);
      pList.insert(&_NSamples);
      pList.insert(&_Evt_Code);
   
    }

    int pull(void* from) {
      QuadAdcConfigType& tc = *reinterpret_cast<QuadAdcConfigType*>(from);
     
      _channelMask.value = tc.chanMask();
     
      _Delay_NS.value = tc.delayTime();
      _NSamples.value = tc.nbrSamples();
      _Evt_Code.value = tc.evtCode();
      
      _channelMask.setinterleave(tc.interleaveMode());

      if (!_channelMask.interleave())
        _channelMask.setsamplerate(tc.sampleRate());
        
      return tc._sizeof();
    }

    int push(void* to) {

      double sr = _channelMask.samplerate();

      printf("SR: %f\n", sr);

      QuadAdcConfigType& tc = *new(to) QuadAdcConfigType(_channelMask.value,
							 _Delay_NS.value,
							 _channelMask.interleave(),
							 _NSamples.value,
							 _Evt_Code.value,
							 //_channelMask.samplerate()
							 sr);
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(QuadAdcConfigType);
    }

  public:
    QuadAdcChannelMask            _channelMask;
    BitCount                      _numChan;

    NumericFloat<double>          _Delay_NS;
    NumericInt<unsigned short>    _NSamples;
    NumericInt<unsigned short>    _Evt_Code;

  };
};


using namespace Pds_ConfigDb;

QuadAdcConfig::QuadAdcConfig() : 
  Serializer("QuadAdc_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  QuadAdcConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  QuadAdcConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  QuadAdcConfig::dataSize() const {
  return _private_data->dataSize();
}



