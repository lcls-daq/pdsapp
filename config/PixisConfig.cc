#include "pdsapp/config/PixisConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/QtConcealer.hh"
#include "pds/config/PixisConfigType.hh"
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

#include <new>

using namespace Pds_ConfigDb;

class Pds_ConfigDb::PixisConfig::Private_Data : public Parameter {
public:
  Private_Data(bool expert_mode);
  ~Private_Data();

   QLayout* initialize( QWidget* p );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(PixisConfigType); }
   void flush ()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->flush(); }
   void update()
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->update(); }
   void enable(bool l)
      { for(Parameter* p=pList.forward(); p!=pList.empty(); p=p->forward()) p->enable(l); }
   bool   validate();

  Pds::LinkedList<Parameter> pList;
  NumericInt<uint32_t>    _uWidth;
  NumericInt<uint32_t>    _uHeight;
  NumericInt<uint32_t>    _uOrgX;
  NumericInt<uint32_t>    _uOrgY;
  NumericInt<uint32_t>    _uBinX;
  NumericInt<uint32_t>    _uBinY;
  NumericFloat<float>     _f32ExposureTime;
  NumericFloat<float>     _f32CoolingTemp;
  Enumerated<int>         _enumReadoutSpeed;
  Enumerated<PixisConfigType::GainMode>     _gainMode;
  Enumerated<PixisConfigType::AdcMode>      _adcMode;
  Enumerated<PixisConfigType::TriggerMode>  _triggerMode;
  NumericInt<uint32_t>    _uActiveWidth;
  NumericInt<uint32_t>    _uActiveHeight;
  NumericInt<uint32_t>    _uActiveTopMargin;
  NumericInt<uint32_t>    _uActiveBottomMargin;
  NumericInt<uint32_t>    _uActiveLeftMargin;
  NumericInt<uint32_t>    _uActiveRightMargin;
  NumericInt<uint32_t>    _uCleanCycleCount;
  NumericInt<uint32_t>    _uCleanCycleHeight;
  NumericInt<uint32_t>    _uCleanFinalHeight;
  NumericInt<uint32_t>    _uCleanFinalHeightCount;
  Enumerated<int>         _enumVsSpeed;
  NumericInt<uint16_t>    _i16InfoReportInterval;
  NumericInt<uint16_t>    _u16ExposureEventCode;
  NumericInt<uint32_t>    _u32NumIntegrationShots;
  QtConcealer             _concealerExpert;
private:
  bool _expert_mode;
  static const char*    lsReadoutSpeed[];
  static const char*    lsGainMode    [];
  static const char*    lsAdcMode     [];
  static const char*    lsTriggerMode [];
  static const char*    lsVsSpeed     [];
  static const float    lfReadoutSpeed[2];
  static const float    lfVsSpeed     [16];

  static const PixisConfigType::GainMode    luGainMode[3];
  static const PixisConfigType::AdcMode     luAdcMode [2];

  static int readoutSpeedToEnum (float fReadoutSpeed);
  static int vsSpeedToEnum      (float fVsSpeed);
};

const char*   Pds_ConfigDb::PixisConfig::Private_Data::lsReadoutSpeed[] = { "2 MHz", "100 kHz", NULL};
const char*   Pds_ConfigDb::PixisConfig::Private_Data::lsGainMode[]     = { "Low", "Medium", "High", NULL };
const char*   Pds_ConfigDb::PixisConfig::Private_Data::lsAdcMode[]      = { "Low Noise", "High Capacity", NULL };
const char*   Pds_ConfigDb::PixisConfig::Private_Data::lsTriggerMode[]  = { "Software", "External", "External + Clean", NULL };
const char*   Pds_ConfigDb::PixisConfig::Private_Data::lsVsSpeed[]      = {
  "3.2", "6.2", "9.2", "12.2", "15.2", "18.2", "21.2", "24.2", "27.2", "30.2", "33.2", "36.2", "39.2", "42.2", "45.2", "48.2", NULL
};
const float   Pds_ConfigDb::PixisConfig::Private_Data::lfReadoutSpeed[] = { 2.0, 0.1 };
const float   Pds_ConfigDb::PixisConfig::Private_Data::lfVsSpeed[]      = { 
  3.2, 6.2, 9.2, 12.2, 15.2, 18.2, 21.2, 24.2, 27.2, 30.2, 33.2, 36.2, 39.2, 42.2, 45.2, 48.2
};
const PixisConfigType::GainMode  Pds_ConfigDb::PixisConfig::Private_Data::luGainMode[] = {
  PixisConfigType::Low, PixisConfigType::Medium, PixisConfigType::High
};
const PixisConfigType::AdcMode   Pds_ConfigDb::PixisConfig::Private_Data::luAdcMode[]  = {
  PixisConfigType::LowNoise, PixisConfigType::HighCapacity
};

Pds_ConfigDb::PixisConfig::Private_Data::Private_Data(bool expert_mode) :
  _uWidth                   ("Width",                 2048, 1,    2048),
  _uHeight                  ("Height",                2048, 1,    2048),
  _uOrgX                    ("Orgin X",               0,    0,    2048),
  _uOrgY                    ("Orgin Y",               0,    0,    2048),
  _uBinX                    ("Binning X",             1,    1,    2048),
  _uBinY                    ("Binning Y",             1,    1,    2048),
  _f32ExposureTime          ("Exposure time (sec)", 0.1, 1.0e-7,  8355.0),
  _f32CoolingTemp           ("Cooling Temp (C)",      25,   -100,  25),
  _enumReadoutSpeed         ("Readout Speed",         0,    lsReadoutSpeed),
  _gainMode                 ("Gain Mode",             PixisConfigType::Medium,    lsGainMode, luGainMode),
  _adcMode                  ("ADC Mode",              PixisConfigType::LowNoise,  lsAdcMode,  luAdcMode),
  _triggerMode              ("Trigger Mode",          PixisConfigType::Software,  lsTriggerMode),
  _uActiveWidth             ("Active Width",          2048, 1,    4296),
  _uActiveHeight            ("Active Height",         2048, 1,    4106),
  _uActiveTopMargin         ("Top Margin",            2,    0,    4106),
  _uActiveBottomMargin      ("Bottom Margin",         3,    0,    4106),
  _uActiveLeftMargin        ("Left Margin",           50,   0,    4296),
  _uActiveRightMargin       ("Right Margin",          50,   0,    4296),
  _uCleanCycleCount         ("Clean Cycle Count",     1,    0,    65535),
  _uCleanCycleHeight        ("Clean Cycle Height",    2048, 1,    2053),
  _uCleanFinalHeight        ("Clean Final Height",    2,    1,    2053),
  _uCleanFinalHeightCount   ("Clean Final Height Count",  512, 1, 2053),
  // skip _u32MaskedHeight, _u32KineticHeight
  _enumVsSpeed              ("Vertical Shift Speed",  9,    lsVsSpeed),
  _i16InfoReportInterval    ("Info Report Interval",  1,    0,    10000),
  _u16ExposureEventCode     ("Exposure Event Code",   1,    1,    255),
  _u32NumIntegrationShots   ("Num Integration Shots", 1,    0,    0x7FFFFFFF),
  _expert_mode              (expert_mode)
{
  pList.insert( &_uWidth );
  pList.insert( &_uHeight );
  pList.insert( &_uOrgX );
  pList.insert( &_uOrgY );
  pList.insert( &_uBinX );
  pList.insert( &_uBinY );
  pList.insert( &_f32ExposureTime );
  pList.insert( &_f32CoolingTemp );
  pList.insert( &_enumReadoutSpeed );
  pList.insert( &_gainMode );
  pList.insert( &_adcMode );
  pList.insert( &_triggerMode );
  pList.insert( &_uActiveWidth );
  pList.insert( &_uActiveHeight );
  pList.insert( &_uActiveTopMargin );
  pList.insert( &_uActiveBottomMargin );
  pList.insert( &_uActiveLeftMargin );
  pList.insert( &_uActiveRightMargin );
  pList.insert( &_uCleanCycleCount );
  pList.insert( &_uCleanCycleHeight );
  pList.insert( &_uCleanFinalHeight );
  pList.insert( &_uCleanFinalHeightCount );
  pList.insert( &_enumVsSpeed );
  pList.insert( &_i16InfoReportInterval );
  pList.insert( &_u16ExposureEventCode );
  pList.insert( &_u32NumIntegrationShots );
}

Pds_ConfigDb::PixisConfig::Private_Data::~Private_Data()
{}

QLayout* Pds_ConfigDb::PixisConfig::Private_Data::initialize(QWidget* p)
{
  QVBoxLayout* layout = new QVBoxLayout;
  { QVBoxLayout* m = new QVBoxLayout;
    m->addWidget(new QLabel("ROI / Image settings: "));
    m->addLayout(_uWidth .initialize(p));
    m->addLayout(_uHeight.initialize(p));
    m->addLayout(_uOrgX  .initialize(p));
    m->addLayout(_uOrgY  .initialize(p));
    m->addLayout(_uBinX  .initialize(p));
    m->addLayout(_uBinY  .initialize(p));
    m->setSpacing(5);
    layout->addLayout(m); }
  { QVBoxLayout* r = new QVBoxLayout;
    r->addWidget(new QLabel("Readout settings: "));
    r->addLayout(_f32ExposureTime .initialize(p));
    r->addLayout(_enumReadoutSpeed.initialize(p));
    r->addLayout(_gainMode        .initialize(p));
    r->addLayout(_adcMode         .initialize(p));
    r->addLayout(_enumVsSpeed     .initialize(p));
    r->addLayout(_concealerExpert.add(_triggerMode.initialize(p)));
    r->setSpacing(5);
    layout->addLayout(r); }
  { QVBoxLayout* a = new QVBoxLayout;
    a->addWidget(new QLabel("Active Area settings: "));
    a->addLayout(_uActiveWidth        .initialize(p));
    a->addLayout(_uActiveHeight       .initialize(p));
    a->addLayout(_uActiveTopMargin    .initialize(p));
    a->addLayout(_uActiveBottomMargin .initialize(p));
    a->addLayout(_uActiveLeftMargin   .initialize(p));
    a->addLayout(_uActiveRightMargin  .initialize(p));
    a->setSpacing(5);
    layout->addLayout(_concealerExpert.add(a)); }
  { QVBoxLayout* c = new QVBoxLayout;
    c->addWidget(new QLabel("Cleaning settings: "));
    c->addLayout(_uCleanCycleCount      .initialize(p));
    c->addLayout(_uCleanCycleHeight     .initialize(p));
    c->addLayout(_uCleanFinalHeight     .initialize(p));
    c->addLayout(_uCleanFinalHeightCount.initialize(p));
    c->setSpacing(5);
    layout->addLayout(_concealerExpert.add(c)); }
  { QVBoxLayout* n = new QVBoxLayout;
    n->addWidget(new QLabel("Misc settings: "));
    n->addLayout(_f32CoolingTemp        .initialize(p));
    n->addLayout(_i16InfoReportInterval .initialize(p));
    n->addLayout(_u16ExposureEventCode  .initialize(p));
    n->addLayout(_u32NumIntegrationShots.initialize(p));
    n->setSpacing(5);
    layout->addLayout(n); }
  layout->setSpacing(25);

  return layout;
}

int Pds_ConfigDb::PixisConfig::Private_Data::pull(void* from) {
    PixisConfigType& tc = *new(from) PixisConfigType;
    _uWidth                   .value = tc.width   ();
    _uHeight                  .value = tc.height  ();
    _uOrgX                    .value = tc.orgX    ();
    _uOrgY                    .value = tc.orgY    ();
    _uBinX                    .value = tc.binX    ();
    _uBinY                    .value = tc.binY    ();
    _f32ExposureTime          .value = tc.exposureTime();
    _f32CoolingTemp           .value = tc.coolingTemp ();
    _enumReadoutSpeed         .value = readoutSpeedToEnum(tc.readoutSpeed());
    _gainMode                 .value = tc.gainMode();
    _adcMode                  .value = tc.adcMode();
    _triggerMode              .value = tc.triggerMode();
    _uActiveWidth             .value = tc.activeWidth();
    _uActiveHeight            .value = tc.activeHeight();
    _uActiveTopMargin         .value = tc.activeTopMargin();
    _uActiveBottomMargin      .value = tc.activeBottomMargin();
    _uActiveLeftMargin        .value = tc.activeLeftMargin();
    _uActiveRightMargin       .value = tc.activeRightMargin();
    _uCleanCycleCount         .value = tc.cleanCycleCount();
    _uCleanCycleHeight        .value = tc.cleanCycleHeight();
    _uCleanFinalHeight        .value = tc.cleanFinalHeight();
    _uCleanFinalHeightCount   .value = tc.cleanFinalHeightCount();
    _enumVsSpeed              .value = vsSpeedToEnum(tc.vsSpeed());
    _i16InfoReportInterval    .value = tc.infoReportInterval();
    _u16ExposureEventCode     .value = tc.exposureEventCode();
    _u32NumIntegrationShots   .value = tc.numIntegrationShots();

    _concealerExpert.show(_expert_mode);

    return tc._sizeof();
}

int Pds_ConfigDb::PixisConfig::Private_Data::readoutSpeedToEnum(float fReadoutSpeed)
{
  for (unsigned i=0; i< sizeof(lfReadoutSpeed) / sizeof(lfReadoutSpeed[0]); ++i)
    if (fReadoutSpeed == lfReadoutSpeed[i])
      return i;

  return 0;
}

int Pds_ConfigDb::PixisConfig::Private_Data::vsSpeedToEnum(float fVsSpeed)
{
  for (unsigned i=0; i< sizeof(lfVsSpeed) / sizeof(lfVsSpeed[0]); ++i)
    if (fVsSpeed == lfVsSpeed[i])
      return i;

  return 0;
}

int Pds_ConfigDb::PixisConfig::Private_Data::push(void* to) {
  PixisConfigType& tc = *new(to) PixisConfigType(
                          _uWidth                   .value,
                          _uHeight                  .value,
                          _uOrgX                    .value,
                          _uOrgY                    .value,
                          _uBinX                    .value,
                          _uBinY                    .value,
                          _f32ExposureTime          .value,
                          _f32CoolingTemp           .value,
                          lfReadoutSpeed            [_enumReadoutSpeed.value],
                          _gainMode                 .value,
                          _adcMode                  .value,
                          _triggerMode              .value,
                          _uActiveWidth             .value,
                          _uActiveHeight            .value,
                          _uActiveTopMargin         .value,
                          _uActiveBottomMargin      .value,
                          _uActiveLeftMargin        .value,
                          _uActiveRightMargin       .value,
                          _uCleanCycleCount         .value,
                          _uCleanCycleHeight        .value,
                          _uCleanFinalHeight        .value,
                          _uCleanFinalHeightCount   .value,
                          0                               , // maskedHeight is not supported yet
                          0                               , // kineticHeight is not supported yet
                          lfVsSpeed                 [_enumVsSpeed.value],
                          _i16InfoReportInterval    .value,
                          _u16ExposureEventCode     .value,
                          _u32NumIntegrationShots .value
                          );
  return tc._sizeof();
}

bool Pds_ConfigDb::PixisConfig::Private_Data::validate()
{
  return true;
}

Pds_ConfigDb::PixisConfig::PixisConfig(bool expert_mode) :
  Serializer("pixis_Config"),
  _private_data( new Private_Data(expert_mode) )
{
  pList.insert(_private_data);
}

int Pds_ConfigDb::PixisConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int Pds_ConfigDb::PixisConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int Pds_ConfigDb::PixisConfig::dataSize() const {
  return _private_data->dataSize();
}

bool Pds_ConfigDb::PixisConfig::validate()
{
  return _private_data->validate();
}

#include "Parameters.icc"
