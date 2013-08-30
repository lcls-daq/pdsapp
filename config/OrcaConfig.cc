#include "pdsapp/config/OrcaConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/OrcaConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  static const char* readout_range[] = { "Bin x1", "Bin x2", "Bin x4", "Subarray", NULL };

  static const char* cooling_range[] = { "Off", "On", "Max", NULL };

  class OrcaConfig::Private_Data {
  public:
    Private_Data() :
      _mode             ("Readout Mode" , OrcaConfigType::x1, readout_range),
      _sub_rows         ("Subarray Rows", OrcaConfigType::Row_Pixels/8, 8, OrcaConfigType::Row_Pixels/8, Scaled, 8.),
      _cooling          ("Cooling", OrcaConfigType::Off, cooling_range),
      _defect_pixel_corr("Defect Pixel Correction", Enums::True, Enums::Bool_Names)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_mode);
      pList.insert(&_sub_rows);
      pList.insert(&_cooling);
      pList.insert(&_defect_pixel_corr);
    }

    int pull(void* from) {
      OrcaConfigType& tc = *reinterpret_cast<OrcaConfigType*>(from);
      _mode             .value = tc.mode();
      _sub_rows         .value = tc.rows   ()/8;
      _cooling          .value = tc.cooling();
      _defect_pixel_corr.value = tc.defect_pixel_correction_enabled() ? Enums::True : Enums::False;
      return tc._sizeof();
    }

    int push(void* to) {
      OrcaConfigType& tc = *new(to) OrcaConfigType(_mode.value,
						   _cooling.value,
						   _defect_pixel_corr.value==Enums::True,
						   _sub_rows.value*8);
      return tc._sizeof();
    }

    int dataSize() const {
      return sizeof(OrcaConfigType);
    }

  public:
    Enumerated<OrcaConfigType::ReadoutMode> _mode;
    NumericInt<unsigned>                    _sub_rows;
    Enumerated<OrcaConfigType::Cooling>     _cooling;
    Enumerated<Enums::Bool>                 _defect_pixel_corr;
  };
};


using namespace Pds_ConfigDb;

OrcaConfig::OrcaConfig() : 
  Serializer("Orca_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  OrcaConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  OrcaConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  OrcaConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<OrcaConfigType::ReadoutMode>;
template class Enumerated<OrcaConfigType::Cooling>;
