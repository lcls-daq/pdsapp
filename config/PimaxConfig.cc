#include "pdsapp/config/PimaxConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/PimaxConfigType.hh"

#include <new>

namespace Pds_ConfigDb {
class PimaxConfig::Private_Data {
public:
  Private_Data() :
    _uWidth               ("Width",                 1024, 1,    1024),
    _uHeight              ("Height",                1024, 1,    1024),
    _uOrgX                ("Orgin X",               0,    0,    1024),
    _uOrgY                ("Orgin Y",               0,    0,    1024),
    _uBinX                ("Binning X",             1,    1,    1024),
    _uBinY                ("Binning Y",             1,    1,    1024),
    // skip exposure time. Not needed by PI-MAX 3
    _f32CoolingTemp       ("Cooling Temp (C)",      25,   -300,  25),
    _enumReadoutSpeed     ("Readout Speed",         0,    lsReadoutSpeed),
    _enumGainIndex        ("Gain Index",            0,    lsGainIndex),
    _u16IntensifierGain   ("Intensifier Gain",      1,    1,    100),
    _f64GateDelay         ("Gate Delay (ns)",       1000, 32,   3e10),
    _f64GateWidth         ("Gate Width (ns)",       1e6,  32,   3e10),
    // skip _u32MaskedHeight, _u32KineticHeight and _f32VsSpeed
    _i16InfoReportInterval("Info Report Interval ", 1,    0,    10000),
    _u16ExposureEventCode ("Exposure Event Code",   1,    1,    255),
    _u32NumIntegrationShots
                          ("Num Integration Shots", 1,    0,    0x7FFFFFFF)
  {}

  //uint32_t width() const { return _uWidth; }
  //uint32_t height() const { return _uHeight; }
  //uint32_t orgX() const { return _uOrgX; }
  //uint32_t orgY() const { return _uOrgY; }
  //uint32_t binX() const { return _uBinX; }
  //uint32_t binY() const { return _uBinY; }
  //float exposureTime() const { return _f32ExposureTime; }
  //float coolingTemp() const { return _f32CoolingTemp; }
  //float readoutSpeed() const { return _f32ReadoutSpeed; }
  //uint16_t gainIndex() const { return _u16GainIndex; }
  //uint16_t intensifierGain() const { return _u16IntensifierGain; }
  //double gateDelay() const { return _f64GateDelay; }
  //double gateWidth() const { return _f64GateWidth; }
  //uint32_t maskedHeight() const { return _u32MaskedHeight; }
  //uint32_t kineticHeight() const { return _u32KineticHeight; }
  //float vsSpeed() const { return _f32VsSpeed; }
  //int16_t infoReportInterval() const { return _i16InfoReportInterval; }
  //uint16_t exposureEventCode() const { return _u16ExposureEventCode; }
  //uint32_t numIntegrationShots() const { return _u32NumIntegrationShots; }

  void insert(Pds::LinkedList<Parameter>& pList) {
    pList.insert(&_uWidth);
    pList.insert(&_uHeight);
    pList.insert(&_uOrgX);
    pList.insert(&_uOrgY);
    pList.insert(&_uBinX);
    pList.insert(&_uBinY);
    pList.insert(&_f32CoolingTemp);
    pList.insert(&_enumReadoutSpeed);
    pList.insert(&_enumGainIndex);
    pList.insert(&_u16IntensifierGain);
    pList.insert(&_f64GateDelay);
    pList.insert(&_f64GateWidth);
    pList.insert(&_i16InfoReportInterval);
    pList.insert(&_u16ExposureEventCode);
    pList.insert(&_u32NumIntegrationShots);
  }

  int pull(void* from) {
    PimaxConfigType& tc = *new(from) PimaxConfigType;
    _uWidth                 .value = tc.width   ();
    _uHeight                .value = tc.height  ();
    _uOrgX                  .value = tc.orgX    ();
    _uOrgY                  .value = tc.orgY    ();
    _uBinX                  .value = tc.binX    ();
    _uBinY                  .value = tc.binY    ();
    _f32CoolingTemp         .value = tc.coolingTemp ();
    _enumReadoutSpeed       .value = readoutSpeedToEnum(tc.readoutSpeed());
    _enumGainIndex          .value = gainIndexToEnum(tc.gainIndex());
    _u16IntensifierGain     .value = tc.intensifierGain();
    _f64GateDelay           .value = tc.gateDelay();
    _f64GateWidth           .value = tc.gateWidth();
    _i16InfoReportInterval  .value = tc.infoReportInterval();
    _u16ExposureEventCode   .value = tc.exposureEventCode();
    _u32NumIntegrationShots .value = tc.numIntegrationShots();
    return tc._sizeof();
  }

  int push(void* to) {
    PimaxConfigType& tc = *new(to) PimaxConfigType(
                            _uWidth                 .value,
                            _uHeight                .value,
                            _uOrgX                  .value,
                            _uOrgY                  .value,
                            _uBinX                  .value,
                            _uBinY                  .value,
                            0.0f                          , // exposure is not needed for PI-MAX3
                            _f32CoolingTemp         .value,
                            lfReadoutSpeed          [_enumReadoutSpeed.value],
                            liGainIndex             [_enumGainIndex   .value],
                            _u16IntensifierGain     .value,
                            _f64GateDelay           .value,
                            _f64GateWidth           .value,
                            0                             , // maskedHeight is not supported yet in PI-MAX3
                            0                             , // kineticHeight is not supported yet in PI-MAX3
                            0.0f                          , // vsSpeed is not supported yet in PI-MAX3
                            _i16InfoReportInterval  .value,
                            _u16ExposureEventCode   .value,
                            _u32NumIntegrationShots .value
                          );
    return tc._sizeof();
  }

  int dataSize() const {
    return sizeof(PimaxConfigType);
  }

public:
  NumericInt<uint32_t>    _uWidth;
  NumericInt<uint32_t>    _uHeight;
  NumericInt<uint32_t>    _uOrgX;
  NumericInt<uint32_t>    _uOrgY;
  NumericInt<uint32_t>    _uBinX;
  NumericInt<uint32_t>    _uBinY;
  NumericFloat<float>     _f32CoolingTemp;
  Enumerated<int>         _enumReadoutSpeed;
  Enumerated<int>         _enumGainIndex;
  NumericInt<uint16_t>    _u16IntensifierGain;
  NumericFloat<double>    _f64GateDelay;
  NumericFloat<double>    _f64GateWidth;
  NumericInt<uint16_t>    _i16InfoReportInterval;
  NumericInt<uint16_t>    _u16ExposureEventCode;
  NumericInt<uint32_t>    _u32NumIntegrationShots;

private:
  static const char*    lsReadoutSpeed[];
  static const char*    lsGainIndex   [];
  static const float    lfReadoutSpeed[3];
  static const int      liGainIndex   [2];

  static int readoutSpeedToEnum(float fReadoutSpeed)
  {
    for (unsigned i=0; i< sizeof(lfReadoutSpeed) / sizeof(lfReadoutSpeed[0]); ++i)
      if (fReadoutSpeed == lfReadoutSpeed[i])
        return i;

    return 0;
  }

  static int gainIndexToEnum(int gainIndex)
  {
    for (unsigned i=0; i< (int) sizeof(liGainIndex) / sizeof(liGainIndex[0]); ++i)
      if (gainIndex == liGainIndex[i])
        return i;

    return 0;
  }
};


const char*   PimaxConfig::Private_Data::lsReadoutSpeed[] = { "16 MHz", "8 MHz", "2 MHz", NULL};
const char*   PimaxConfig::Private_Data::lsGainIndex   [] = { "Low", "High ", NULL};
const float   PimaxConfig::Private_Data::lfReadoutSpeed[] = { 16, 8, 2 };
const int     PimaxConfig::Private_Data::liGainIndex   [] = { 1, 3 };

};

using namespace Pds_ConfigDb;

PimaxConfig::PimaxConfig() :
  Serializer("pimax_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  PimaxConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  PimaxConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  PimaxConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

