#include "FrameFexConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/FrameFexConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

namespace Pds_ConfigDb {

  enum Forwarding_exp { None, FullFrame, RegionOfInterest };
  static const char* Forwarding_range[] = { "None",
					    "FullFrame",
					    "RegionOfInterest",
					    NULL };
  static const char* Processing_range[] = { "None",
					    "GssFullFrame",
					    "GssRegionOfInterest",
					    "GssThreshold",
					    NULL };

  enum MaskedPixels { NoPixels };
  static const char* MaskedPixels_range[] = { "None", NULL };

  class FrameFexConfig::Private_Data {
  public:
    Private_Data() :
      _forwarding       ("Frame Forwarding", FrameFexConfigType::FullFrame, Forwarding_range),
      _fwd_prescale     ("Frame Fwd Prescale", 1, 1, 0x7fffffff),
      _processing       ("Processing", FrameFexConfigType::NoProcessing, Processing_range),
      _roi_begin_col    ("ROI Begin Column", 0, 0, 0x3ff),
      _roi_begin_row    ("ROI Begin Row"   , 0, 0, 0x3ff),
      _roi_end_col      ("ROI   End Column", 0, 0, 0x400),
      _roi_end_row      ("ROI   End Row"   , 0, 0, 0x400),
      _threshold        ("FEX Threshold"   , 0, 0, 0xfff),
      _masked_pixels    ("Masked Pixels"   , NoPixels, MaskedPixels_range)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_forwarding);
      pList.insert(&_fwd_prescale);
      pList.insert(&_processing);
      pList.insert(&_roi_begin_col);
      pList.insert(&_roi_begin_row);
      pList.insert(&_roi_end_col);
      pList.insert(&_roi_end_row);
      pList.insert(&_threshold);
      pList.insert(&_masked_pixels);
    }

    int pull(void* from) {
      FrameFexConfigType& tc = *new(from) FrameFexConfigType;
      _forwarding.value = tc.forwarding();
      _fwd_prescale.value = tc.forward_prescale();
      _processing.value    = tc.processing();
      _roi_begin_col.value = tc.roiBegin().column;
      _roi_begin_row.value = tc.roiBegin().row;
      _roi_end_col.value = tc.roiEnd().column;
      _roi_end_row.value = tc.roiEnd().row;
      _threshold.value   = tc.threshold();
      _masked_pixels.value = NoPixels;
      return tc.size();
    }

    int push(void* to) {
      FrameFexConfigType& tc = 
	*new(to) FrameFexConfigType(_forwarding.value,
				    _fwd_prescale.value,
				    _processing.value,
				    Pds::Camera::FrameCoord(_roi_begin_col.value,
							    _roi_begin_row.value),
				    Pds::Camera::FrameCoord(_roi_end_col.value,
							    _roi_end_row.value),
				    _threshold.value,
				    0, 0);
      return tc.size();
    }

    int dataSize() const {
      return sizeof(FrameFexConfigType);
    }

  public:
    Enumerated<FrameFexConfigType::Forwarding> _forwarding;
    NumericInt<unsigned>          _fwd_prescale;
    Enumerated<FrameFexConfigType::Processing> _processing;
    NumericInt<unsigned short>    _roi_begin_col;
    NumericInt<unsigned short>    _roi_begin_row;
    NumericInt<unsigned short>    _roi_end_col;
    NumericInt<unsigned short>    _roi_end_row;
    NumericInt<unsigned short>    _threshold;
    Enumerated<MaskedPixels>      _masked_pixels;
  };
};


FrameFexConfig::FrameFexConfig() : 
  Serializer("FrameFex_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  FrameFexConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  FrameFexConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  FrameFexConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<Forwarding_exp>;
template class Enumerated<FrameFexConfigType::Processing>;
template class Enumerated<MaskedPixels>;
