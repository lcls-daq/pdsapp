#include "pdsapp/config/TM6740Config.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/TM6740ConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  static const char* depth_range[] = { "8 Bit", "10 Bit", NULL };
  static const char* binning_range[] = { "x1", "x2", "x4", NULL };
  static const char* lut_modes[] = { "Gamma(.45)", "Linear", NULL };

  class TM6740Config::Private_Data {
  public:
    Private_Data() :
      _vref             ("Vref"  ,   0, 0, 0x1ff),
      _gain_a           ("Gain_A",   0, 0x42, 0x1e8),
      _gain_b           ("Gain_B",   0, 0x42, 0x1e8),
      _shutter_width    ("Shutter Width", 100, 0, 0xffff),
      _gain_balance     ("Use Gain Balance", Enums::False, Enums::Bool_Names),
      _depth            ("Depth", TM6740ConfigType::Ten_bit, depth_range),
      _hbinning         ("Horizontal Binning", TM6740ConfigType::x1, binning_range),
      _vbinning         ("Vertical Binning", TM6740ConfigType::x1, binning_range),
      _output_lut       ("Lookup Table Mode", TM6740ConfigType::Linear, lut_modes)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_vref);
      pList.insert(&_gain_a);
      pList.insert(&_gain_b);
      pList.insert(&_shutter_width);
      pList.insert(&_gain_balance);
      pList.insert(&_depth);
      pList.insert(&_hbinning);
      pList.insert(&_vbinning);
      pList.insert(&_output_lut);
    }

    bool pull(void* from) {
      TM6740ConfigType& tc = *new(from) TM6740ConfigType;
      _vref         .value = tc.vref  ();
      _gain_a       .value = tc.gain_a();
      _gain_b       .value = tc.gain_b();
      _shutter_width.value = tc.shutter_width();
      _gain_balance .value = tc.gain_balance() ? Enums::True : Enums::False;
      _depth        .value = tc.output_resolution();
      _hbinning     .value = tc.horizontal_binning();
      _vbinning     .value = tc.vertical_binning();
      _output_lut   .value = tc.lookuptable_mode();
      return true;
    }

    int push(void* to) {
      TM6740ConfigType& tc = *new(to) TM6740ConfigType(_vref  .value,
						       _gain_a.value,
						       _gain_b.value,
						       _shutter_width.value,
						       _gain_balance.value==Enums::True,
						       _depth.value,
						       _hbinning.value,
						       _vbinning.value,
						       _output_lut.value);
      return sizeof(tc);
    }
  public:
    NumericInt<unsigned>    _vref  ;
    NumericInt<unsigned>    _gain_a;
    NumericInt<unsigned>    _gain_b;
    NumericInt<unsigned>    _shutter_width;
    Enumerated<Enums::Bool> _gain_balance;
    Enumerated<TM6740ConfigType::Depth>     _depth;
    Enumerated<TM6740ConfigType::Binning>   _hbinning;
    Enumerated<TM6740ConfigType::Binning>   _vbinning;
    Enumerated<TM6740ConfigType::LookupTable> _output_lut;
  };
};


using namespace Pds_ConfigDb;

TM6740Config::TM6740Config() : 
  Serializer("TM6740_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

bool TM6740Config::readParameters (void* from) {
  return _private_data->pull(from);
}

int  TM6740Config::writeParameters(void* to) {
  return _private_data->push(to);
}

#include "Parameters.icc"

template class Enumerated<TM6740ConfigType::Depth>;
template class Enumerated<TM6740ConfigType::Binning>;
template class Enumerated<TM6740ConfigType::LookupTable>;
