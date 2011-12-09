#include "AcqConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/AcqChannelMask.hh"
#include "pds/config/AcqConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {

  enum AcqVFullScale {V0_1,V0_2,V0_5,V1,V2,V5};
  static const double fullscale_value[] = {0.1,0.2,0.5,1.0,2.0,5.0};
  static const char*  fullscale_range[] = {"0.1","0.2","0.5","1","2","5",NULL};

  static const char* coupling_range[] = {"GND","DC","AC","DC50ohm","AC50ohm",NULL};
  static const char* bandwidth_range[]= {"None","25MHz","700MHz","200MHz","20MHz","35MHz",NULL};

  class AcqVert {
  public:
    AcqVert() :
      _fullScale("Full Scale (Volts)",V2, fullscale_range),
      _offset("Offset (Volts)",0.0,-2.0,2.0),
      _coupling("Coupling", Pds::Acqiris::VertV1::DC50ohm, coupling_range),
      _bandwidth("Bandwidth", Pds::Acqiris::VertV1::None, bandwidth_range)
    {}
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_fullScale);
      pList.insert(&_offset);
      pList.insert(&_coupling);
      pList.insert(&_bandwidth);
    }
    bool pull(void* from) {
      Pds::Acqiris::VertV1& tc = *new(from) Pds::Acqiris::VertV1;
      unsigned i=0;
      while(tc.fullScale()!=fullscale_value[i] && ++i<V5) ;
      _fullScale.value = AcqVFullScale(i);
      _offset.value = tc.offset();
      _coupling.value = (Pds::Acqiris::VertV1::Coupling)(tc.coupling());
      _bandwidth.value = (Pds::Acqiris::VertV1::Bandwidth)tc.bandwidth();
      return true;
    }

    int push(void* to) {
      Pds::Acqiris::VertV1& tc = *new(to) Pds::Acqiris::VertV1(fullscale_value[_fullScale.value],
                                                               _offset.value,
                                                               _coupling.value,
                                                               _bandwidth.value);
      return sizeof(tc);
    }
  private:
    Enumerated<AcqVFullScale> _fullScale;
    NumericFloat<double> _offset;
    Enumerated<Pds::Acqiris::VertV1::Coupling>  _coupling;
    Enumerated<Pds::Acqiris::VertV1::Bandwidth> _bandwidth;
  };

  static const char* trigcoupling_range[] = {"DC","AC","HFreject",NULL};
  static const char* trigslope_range[] = {"Positive","Negative","OutOfWindow","IntoWindow",
                                          "HFDivide","SpikeStretcher",NULL};

  class AcqTrig {
  public:
    AcqTrig() :
      _coupling("Trig Coupling", Pds::Acqiris::TrigV1::DC, trigcoupling_range),
      _input("Trig Input",-1,-10,10),
      _slope("Trig Slope", Pds::Acqiris::TrigV1::Positive, trigslope_range),
      _level("Trig Level (V)",0.0,-5.0,5.0)
    {}
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_coupling);
      pList.insert(&_input);
      pList.insert(&_slope);
      pList.insert(&_level);
    }
    bool pull(void* from) {
      Pds::Acqiris::TrigV1& tc = *new(from) Pds::Acqiris::TrigV1;
      _coupling.value = (Pds::Acqiris::TrigV1::Coupling)(tc.coupling());
      _input.value = tc.input();
      _slope.value = (Pds::Acqiris::TrigV1::Slope)(tc.slope());
      _level.value = tc.level();
      return true;
    }

    int push(void* to) {
      Pds::Acqiris::TrigV1& tc = *new(to) Pds::Acqiris::TrigV1(_coupling.value,
                                                               _input.value,
                                                               _slope.value,
                                                               _level.value);
      return sizeof(tc);
    }
    enum Source {Internal=1,External=-1};
  private:
    Enumerated<Pds::Acqiris::TrigV1::Coupling> _coupling;
    NumericInt<int32_t> _input;
    Enumerated<Pds::Acqiris::TrigV1::Slope> _slope;
    NumericFloat<double> _level;
  };

  class AcqHoriz {
  public:

    AcqHoriz() :
      _sampInterval("Sample Interval [sec]",1.e-6,0.0,1.0),
      _delayTime("Delay Time [sec]",0.0,-1.0,1.0),
      _nbrSamples("Samples",1000,1,1000000000),
      _nbrSegments("Segments",1,1,1000000000)
    {}
   
    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_sampInterval);
      pList.insert(&_delayTime);
      pList.insert(&_nbrSamples);
      pList.insert(&_nbrSegments);
    }

    bool pull(void* from) {
      Pds::Acqiris::HorizV1& tc = *new(from) Pds::Acqiris::HorizV1;
      _sampInterval.value = tc.sampInterval();
      _delayTime.value = tc.delayTime();
      _nbrSamples.value = tc.nbrSamples();
      _nbrSegments.value = tc.nbrSegments();
      return true;
    }

    int push(void* to) {
      Pds::Acqiris::HorizV1& tc = *new(to) Pds::Acqiris::HorizV1(_sampInterval.value,
                                                                 _delayTime.value,
                                                                 _nbrSamples.value,
                                                                 _nbrSegments.value);
      return sizeof(tc);
    }
  private:
    NumericFloat<double> _sampInterval;
    NumericFloat<double> _delayTime;
    NumericInt<uint32_t> _nbrSamples;
    NumericInt<uint32_t> _nbrSegments;
  };

  class AcqConfig::Private_Data {
  public:
    Private_Data() :
      _nbrConvertersPerChannel("Number of Converters Per Channel",1,1,4),
      //      _channelMask("Channel Mask [hex]",1,1,0xfffff,Hex),
      _channelMask("Channel Mask [hex]",1),
      _nbrBanks("Number of Banks",1,1,1),
      _numChan(_channelMask),
      _vertSet("Vert Config", _vertArgs, _numChan)
    {
      for(unsigned k=0; k<Pds::Acqiris::ConfigV1::MaxChan; k++)
	_vert[k].insert(_vertArgs[k]);
    }

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_nbrConvertersPerChannel);
      pList.insert(&_channelMask);
      pList.insert(&_nbrBanks);
      _trig.insert(pList);
      _horiz.insert(pList);
      pList.insert(&_vertSet);
    }

    int pull(void* from) { // pull "from xtc"
      AcqConfigType& acqconf = *new(from) AcqConfigType;
      _nbrConvertersPerChannel.value = acqconf.nbrConvertersPerChannel();
      _channelMask.value = acqconf.channelMask();
      _nbrBanks.value = acqconf.nbrBanks();
      _trig.pull(&acqconf.trig());
      _horiz.pull(&(acqconf.horiz()));
      for(unsigned k=0; k<_numChan.count(); k++)
        _vert[k].pull(&(acqconf.vert(k)));

      return sizeof(AcqConfigType);
    }

    int push(void* to) { // push "to xtc"
      Pds::Acqiris::TrigV1* t = new Pds::Acqiris::TrigV1;
      Pds::Acqiris::HorizV1* h = new Pds::Acqiris::HorizV1;
      Pds::Acqiris::VertV1* v = new Pds::Acqiris::VertV1[_numChan.count()];
      _trig.push(t);
      _horiz.push(h);
      for(unsigned k=0; k<_numChan.count(); k++)
	_vert[k].push(&v[k]);
      *new(to) AcqConfigType(_nbrConvertersPerChannel.value,
                             _channelMask.value,
                             _nbrBanks.value,
                             *t,*h,v);
      delete t;
      delete h;
      delete[] v;
      return sizeof(AcqConfigType);
    }

    int dataSize() const { return sizeof(AcqConfigType); }
  public:
    NumericInt<uint32_t> _nbrConvertersPerChannel;
    //    NumericInt<uint32_t> _channelMask;
    AcqChannelMask _channelMask;
    NumericInt<uint32_t> _nbrBanks;
    BitCount _numChan;
    AcqTrig  _trig;
    AcqHoriz _horiz;
    AcqVert  _vert[Pds::Acqiris::ConfigV1::MaxChan];
    Pds::LinkedList<Parameter> _vertArgs[Pds::Acqiris::ConfigV1::MaxChan];
    ParameterSet _vertSet;
  };
};

AcqConfig::AcqConfig() : 
  Serializer("Acq_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int AcqConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  AcqConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  AcqConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<Pds::Acqiris::VertV1::Coupling>;
template class Enumerated<Pds::Acqiris::VertV1::Bandwidth>;
template class Enumerated<Pds::Acqiris::TrigV1::Coupling>;
template class Enumerated<Pds::Acqiris::TrigV1::Slope>;
