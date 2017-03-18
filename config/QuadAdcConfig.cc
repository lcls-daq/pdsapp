#include "pdsapp/config/QuadAdcConfig.hh"
#include "pds/config/QuadAdcConfigType.hh"
#include "pdsapp/config/QuadAdcChannelMask.hh"

#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "/reg/neh/home4/tookey/myrelease/pdsdata/psddl/quadadc.ddl.h"

#include <new>

namespace Pds_ConfigDb {

  enum sample_rate_enum {opt1, opt2, opt3, opt4, opt5, opt6, opt7, opt8, opt9, opt10, opt11};

  class QuadAdcConfig::Private_Data {
  public:
    Private_Data() :
      _channelMask      ("Channel Mask [hex]",1),
      _numChan          (_channelMask),

      _Delay_NS         ("Delay_NS (ns) "   ,   0, 0, 100),
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

      if (_channelMask.interleave()) {
	switch(int(tc.sampleRate())){
	case 1250000000 :
	  _channelMask.setsamplerate(opt1);
	  break;
	case 625000000 :
	  _channelMask.setsamplerate(opt2);
	  break; 
	case 250000000 :
	  _channelMask.setsamplerate(opt3);
	  break;
	case 125000000 :
	  _channelMask.setsamplerate(opt4);
	  break;
	case 62500000 :
	  _channelMask.setsamplerate(opt5);
	  break;
	case 41700000 :
	  _channelMask.setsamplerate(opt6);
	  break;
	case 31300000 :
	  _channelMask.setsamplerate(opt7);
	  break;
	case 25000000 :
	  _channelMask.setsamplerate(opt8);
	  break;
	case 20800000 :
	  _channelMask.setsamplerate(opt9);
	  break;
	case 17900000 :
	  _channelMask.setsamplerate(opt10);
	  break;
	case 15600000 :
	  _channelMask.setsamplerate(opt11);
	  break; 
	default:
	  _channelMask.setsamplerate(opt1);
	  break; 
	}
      }
        
      return tc._sizeof();
    }

    int push(void* to) {

      int sr = 0;
      printf("samplerate() : %i\n", _channelMask.samplerate());

      if(_channelMask.interleave()) {
	sr = 5000000; //5MHz always 
      }

      else {
	switch(_channelMask.samplerate()) {
	case 0 :
	  sr = 1250000000;
	  break;
	case 1 :
	  sr = 625000000;
	  break;
	case 2 :
	  sr = 250000000;
	  break;
	case 3 :
	  sr = 125000000;
	  break;
	case 4 :
	  sr = 62500000;
	  break;
	case 5 :
	  sr = 41700000;
	  break;
	case 6 :
	  sr = 31300000;
	  break;
	case 7 :
	  sr = 25000000;
	  break;
	case 8 :
	  sr = 20800000;
	  break;
	case 9 :
	  sr = 17900000;
	  break;
	case 10 :
	  sr = 15600000;
	  break;
	  default :
	  sr = 1250000000;
	  break;
	}
      }

      printf("SR: %i\n", sr);

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

#include "Parameters.icc"

