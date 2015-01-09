#include "pdsapp/config/QuartzConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pdsapp/config/ROI.hh"
#include "pds/config/QuartzConfigType.hh"

#include <QtGui/QStackedWidget>
#include <QtGui/QCheckBox>

#include <new>

namespace Pds_ConfigDb {

  enum QuartzLUT { None };
  enum TapSetting { two_taps, four_taps, eight_taps };
  
  static const char* QuartzLUTNames[] = { "None",
                                          NULL };
  static const char* depth_range[] = { "8 Bit", "10 Bit", NULL };
  static const char* binning_range[] = { "x1", "x2", "x4", NULL };
  static const char* mirroring_range[] = { "None", "HFlip", "VFlip", "HVFlip", NULL };
  static const char* tap_range[] = {"2 Taps (Base)", "4 Taps (Medium)", "8 Taps (Full)", NULL };

  class QuartzConfig::Private_Data {
  public:
    Private_Data() :
      _black_level      ("Black Level",   0x10, 0, 0x10),
      _gain             ("Gain (x100)"   , 100, 100, 800),
      _depth            ("Depth"      , QuartzConfigType::Eight_bit, depth_range),
      _max_taps         ("Max Taps"   , eight_taps, tap_range),
      _hbinning         ("Hor. Binning", QuartzConfigType::x1, binning_range),
      _vbinning         ("Vert.Binning", QuartzConfigType::x1, binning_range),
      _mirroring        ("Mirroring", QuartzConfigType::None, mirroring_range),
      _use_roi          ("Use Hardware ROI",false),
      _hw_roi_x         ("Hardware","X","N/A"),
      _hw_roi_y         ("Hardware","Y","N/A"),
      _defect_pixel_corr("Defect Pixel Correction", Enums::True, Enums::Bool_Names),
      _output_lut       ("Output Lookup Table", None, QuartzLUTNames),
      _test_pattern     ("Use Test Pattern", false)
    {
      //      QObject::connect(_use_roi._input, SIGNAL(toggled(bool)), &_roi_concealer, SLOT(show(bool)));
      _roi_concealer.show(_use_roi.value=false);
      _hw_roi_x._sw->setCurrentIndex(0);
      _hw_roi_y._sw->setCurrentIndex(0);
    }

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_black_level);
      pList.insert(&_gain);
      pList.insert(&_depth);
      pList.insert(&_max_taps);
      pList.insert(&_hbinning);
      pList.insert(&_vbinning);
      pList.insert(&_mirroring);
      pList.insert(&_use_roi);
      pList.insert(&_hw_roi_x);
      pList.insert(&_hw_roi_y);
      pList.insert(&_defect_pixel_corr);
      pList.insert(&_output_lut);
      pList.insert(&_test_pattern);
    }

    int pull(void* from) {
      QuartzConfigType& tc = *reinterpret_cast<QuartzConfigType*>(from);
      _black_level.value = tc.black_level();
      _gain       .value = tc.gain_percent();
      _depth      .value = tc.output_resolution();
      switch(tc.max_taps()) {
      case 2:  _max_taps.value = two_taps; break;
      case 4:  _max_taps.value = four_taps; break;
      default: _max_taps.value = eight_taps; break;
      }
      _hbinning   .value = tc.horizontal_binning();
      _vbinning   .value = tc.vertical_binning();
      _mirroring  .value = tc.output_mirroring();
      _use_roi    .value = tc.use_hardware_roi();
      _hw_roi_x._lo.value = tc.roi_lo().column();
      _hw_roi_y._lo.value = tc.roi_lo().row();
      _hw_roi_x._hi.value = tc.roi_hi().column();
      _hw_roi_y._hi.value = tc.roi_hi().row();
      _defect_pixel_corr.value = tc.defect_pixel_correction_enabled() ? Enums::True : Enums::False;
      _output_lut.value = None;
      _test_pattern.value = tc.use_test_pattern();
      return tc._sizeof();
    }

    int push(void* to) {
      uint8_t max_taps=8;
      switch(_max_taps.value) {
      case two_taps : max_taps=2; break;
      case four_taps: max_taps=4; break;
      default: break;
      }
      QuartzConfigType& tc = 
        *new(to) QuartzConfigType(_black_level.value,
                                  _gain.value,
                                  _depth.value,
                                  _hbinning.value,
                                  _vbinning.value,
                                  _mirroring.value,
                                  false,
                                  _defect_pixel_corr.value==Enums::True,
                                  _use_roi.value,
                                  _test_pattern.value ? 1:0,
                                  max_taps,
                                  Pds::Camera::FrameCoord(_hw_roi_x._lo.value,
                                                          _hw_roi_y._lo.value),
                                  Pds::Camera::FrameCoord(_hw_roi_x._hi.value,
                                                          _hw_roi_y._hi.value),
                                  0, 0, 0);
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(QuartzConfigType);
    }

  public:
    NumericInt<unsigned short>    _black_level;
    NumericInt<unsigned short>    _gain;
    Enumerated<QuartzConfigType::Depth>     _depth;
    Enumerated<TapSetting>        _max_taps;
    Enumerated<QuartzConfigType::Binning>   _hbinning;
    Enumerated<QuartzConfigType::Binning>   _vbinning;
    Enumerated<QuartzConfigType::Mirroring> _mirroring;
    CheckValue                              _use_roi;
    ROI                                     _hw_roi_x;
    ROI                                     _hw_roi_y;
    Enumerated<Enums::Bool>            _defect_pixel_corr;
    Enumerated<QuartzLUT>           _output_lut;
    CheckValue                              _test_pattern;
    QtConcealer                     _roi_concealer;
  };
};


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
template class Enumerated<QuartzLUT>;
template class Enumerated<TapSetting>;
