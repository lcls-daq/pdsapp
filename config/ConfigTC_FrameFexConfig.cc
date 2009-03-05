#include "ConfigTC_FrameFexConfig.hh"

#include "pdsapp/config/ConfigTC_Parameters.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"

#include <new>

using namespace ConfigGui;

#define FexTC Camera::FrameFexConfigV1

namespace ConfigGui {

  enum Forwarding_exp { None, FullFrame, RegionOfInterest, 
			Reserved,
			Summary, Summary_Full, Summary_ROI };
  static const char* Forwarding_range[] = { "None",
					    "FullFrame",
					    "RegionOfInterest",
					    "--reserved--",
					    "Summary",
					    "Summary+FullFrame",
					    "Summary+RegionOfInterest",
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
      _forwarding       ("Forwarding", FullFrame, Forwarding_range),
      _processing       ("Processing", FexTC::None, Processing_range),
      _roi_begin_col    ("ROI Begin Column", 0, 0, 0x3ff),
      _roi_begin_row    ("ROI Begin Row"   , 0, 0, 0x3ff),
      _roi_end_col      ("ROI   End Column", 0, 0, 0x3ff),
      _roi_end_row      ("ROI   End Row"   , 0, 0, 0x3ff),
      _threshold        ("FEX Threshold"   , 0, 0, 0xfff),
      _masked_pixels    ("Masked Pixels"   , NoPixels, MaskedPixels_range)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_forwarding);
      pList.insert(&_processing);
      pList.insert(&_roi_begin_col);
      pList.insert(&_roi_begin_row);
      pList.insert(&_roi_end_col);
      pList.insert(&_roi_end_row);
      pList.insert(&_threshold);
      pList.insert(&_masked_pixels);
    }

    bool pull(void* from) {
      FexTC& tc = *new(from) FexTC;
      _forwarding.value = (Forwarding_exp)
	((tc.forwarding(FexTC::FullFrame) ? 1<<FexTC::FullFrame : 0) |
	 (tc.forwarding(FexTC::RegionOfInterest) ? 1<<FexTC::RegionOfInterest : 0) |
	 (tc.forwarding(FexTC::Summary) ? 1<<FexTC::Summary : 0));
      _processing.value    = tc.processing();
      _roi_begin_col.value = tc.roiBegin().column;
      _roi_begin_row.value = tc.roiBegin().row;
      _roi_end_col.value = tc.roiEnd().column;
      _roi_end_row.value = tc.roiEnd().row;
      _threshold.value   = tc.threshold();
      _masked_pixels.value = NoPixels;
      return true;
    }

    int push(void* to) {
      FexTC& tc = *new(to) FexTC((unsigned)_forwarding.value,
				 _processing.value,
				 Camera::FrameCoord(_roi_begin_col.value,
						    _roi_begin_row.value),
				 Camera::FrameCoord(_roi_end_col.value,
						    _roi_end_row.value),
				 _threshold.value,
				 0, 0);
      return tc.size();
    }
  public:
    Enumerated<Forwarding_exp>    _forwarding;
    Enumerated<FexTC::Processing> _processing;
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

bool FrameFexConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  FrameFexConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

#include "ConfigTC_Parameters.icc"

template class Enumerated<Forwarding_exp>;
template class Enumerated<FexTC::Processing>;
template class Enumerated<MaskedPixels>;
