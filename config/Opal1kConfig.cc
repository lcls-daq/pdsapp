#include "pdsapp/config/Opal1kConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/Opal1kConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  enum OpalLUT { None };

  static const char* OpalLUTNames[] = { "None",
					NULL };
  static const char* depth_range[] = { "8 Bit", "10 Bit", "12 Bit", NULL };
  static const char* binning_range[] = { "x1", "x2", "x4", NULL };
  static const char* mirroring_range[] = { "None", "HFlip", "VFlip", "HVFlip", NULL };

  class Opal1kConfig::Private_Data {
  public:
    Private_Data() :
      _black_level      ("Black Level",   0, 0, 0xfff),
      _gain             ("Gain (x100)"   , 100, 100, 3200),
      _depth            ("Depth"      , Opal1kConfigType::Twelve_bit, depth_range),
      _binning          ("Binning", Opal1kConfigType::x1, binning_range),
      _mirroring        ("Mirroring", Opal1kConfigType::None, mirroring_range),
      _vertical_remap   ("Vertical Remap", Enums::True, Enums::Bool_Names),
      _defect_pixel_corr("Defect Pixel Correction", Enums::True, Enums::Bool_Names),
      _output_lut       ("Output Lookup Table", None, OpalLUTNames)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_black_level);
      pList.insert(&_gain);
      pList.insert(&_depth);
      pList.insert(&_binning);
      pList.insert(&_mirroring);
      pList.insert(&_vertical_remap);
      pList.insert(&_defect_pixel_corr);
      pList.insert(&_output_lut);
    }

    int pull(void* from) {
      Opal1kConfigType& tc = *reinterpret_cast<Opal1kConfigType*>(from);
      _black_level.value = tc.black_level();
      _gain       .value = tc.gain_percent();
      _depth      .value = tc.output_resolution();
      _binning    .value = tc.vertical_binning();
      _mirroring  .value = tc.output_mirroring();
      _vertical_remap.value = tc.vertical_remapping() ? Enums::True : Enums::False;
      _defect_pixel_corr.value = tc.defect_pixel_correction_enabled() ? Enums::True : Enums::False;
      _output_lut.value = None;
      return tc._sizeof();
    }

    int push(void* to) {
      Opal1kConfigType& tc = *new(to) Opal1kConfigType(_black_level.value,
                                                       _gain.value,
                                                       _depth.value,
                                                       _binning.value,
                                                       _mirroring.value,
                                                       _vertical_remap.value==Enums::True,
                                                       _defect_pixel_corr.value==Enums::True,
                                                       false, 0, 0, 0);
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(Opal1kConfigType);
    }

  public:
    NumericInt<unsigned short>    _black_level;
    NumericInt<unsigned short>    _gain;
    Enumerated<Opal1kConfigType::Depth>     _depth;
    Enumerated<Opal1kConfigType::Binning>   _binning;
    Enumerated<Opal1kConfigType::Mirroring> _mirroring;
    Enumerated<Enums::Bool>            _vertical_remap;
    Enumerated<Enums::Bool>            _defect_pixel_corr;
    Enumerated<OpalLUT>           _output_lut;
  };
};


using namespace Pds_ConfigDb;

Opal1kConfig::Opal1kConfig() : 
  Serializer("Opal1k_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  Opal1kConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  Opal1kConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  Opal1kConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<Opal1kConfigType::Depth>;
template class Enumerated<Opal1kConfigType::Binning>;
template class Enumerated<Opal1kConfigType::Mirroring>;
template class Enumerated<OpalLUT>;
