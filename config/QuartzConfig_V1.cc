#include "pdsapp/config/QuartzConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/psddl/quartz.ddl.h"

#include <new>

typedef Pds::Quartz::ConfigV1 QuartzConfigType;
static const unsigned max_row_pixels    = 2048;
static const unsigned max_column_pixels = 2048;

namespace Pds_ConfigDb {
  namespace V1 {
    enum QuartzLUT { None };

    static const char* QuartzLUTNames[] = { "None",
                                            NULL };
    static const char* depth_range[] = { "8 Bit", "10 Bit", NULL };
    static const char* binning_range[] = { "x1", "x2", "x4", NULL };
    static const char* mirroring_range[] = { "None", "HFlip", "VFlip", "HVFlip", NULL };

    class QuartzConfig::Private_Data {
    public:
      Private_Data() :
        _black_level      ("Black Level",   0x20, 0, 0x3ff),
        _gain             ("Gain (x100)"   , 100, 100, 800),
        _depth            ("Depth"      , QuartzConfigType::Ten_bit, depth_range),
        _hbinning         ("Hor. Binning", QuartzConfigType::x1, binning_range),
        _vbinning         ("Vert.Binning", QuartzConfigType::x1, binning_range),
        _mirroring        ("Mirroring", QuartzConfigType::None, mirroring_range),
        _defect_pixel_corr("Defect Pixel Correction", Enums::True, Enums::Bool_Names),
        _output_lut       ("Output Lookup Table", None, QuartzLUTNames)
      {}

      void insert(Pds::LinkedList<Parameter>& pList) {
        pList.insert(&_black_level);
        pList.insert(&_gain);
        pList.insert(&_depth);
        pList.insert(&_hbinning);
        pList.insert(&_vbinning);
        pList.insert(&_mirroring);
        pList.insert(&_defect_pixel_corr);
        pList.insert(&_output_lut);
      }

      int pull(void* from) {
        QuartzConfigType& tc = *reinterpret_cast<QuartzConfigType*>(from);
        _black_level.value = tc.black_level();
        _gain       .value = tc.gain_percent();
        _depth      .value = tc.output_resolution();
        _hbinning   .value = tc.horizontal_binning();
        _vbinning   .value = tc.vertical_binning();
        _mirroring  .value = tc.output_mirroring();
        _defect_pixel_corr.value = tc.defect_pixel_correction_enabled() ? Enums::True : Enums::False;
        _output_lut.value = None;
        return tc._sizeof();
      }

      int push(void* to) {
        QuartzConfigType& tc = *new(to) QuartzConfigType(_black_level.value,
                                                         _gain.value,
                                                         _depth.value,
                                                         _hbinning.value,
                                                         _vbinning.value,
                                                         _mirroring.value,
                                                         _defect_pixel_corr.value==Enums::True,
                                                         false, 0, 0, 0);
        return tc._sizeof();
      }

      int dataSize() const {
        return sizeof(QuartzConfigType);
      }

    public:
      NumericInt<unsigned short>    _black_level;
      NumericInt<unsigned short>    _gain;
      Enumerated<QuartzConfigType::Depth>     _depth;
      Enumerated<QuartzConfigType::Binning>   _hbinning;
      Enumerated<QuartzConfigType::Binning>   _vbinning;
      Enumerated<QuartzConfigType::Mirroring> _mirroring;
      Enumerated<Enums::Bool>            _defect_pixel_corr;
      Enumerated<QuartzLUT>           _output_lut;
    };
  };
};


using Pds_ConfigDb::V1::QuartzConfig;
using namespace Pds_ConfigDb;

QuartzConfig::QuartzConfig() : 
  Serializer("Quartz_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  QuartzConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  QuartzConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  QuartzConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<QuartzConfigType::Depth>;
template class Enumerated<QuartzConfigType::Binning>;
template class Enumerated<QuartzConfigType::Mirroring>;
template class Enumerated<V1::QuartzLUT>;
